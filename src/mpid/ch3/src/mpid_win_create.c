/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/* THE CALL TO MPI_IPROBE BELOW HAS BEEN COMMENTED OUT OTHERWISE EVEN THE
   ACTIVE-TARGET RMA WILL NOT WORK BECAUSE OF THREAD-SAFETY
   ISSUES. UNCOMMENT THE LINE BELOW FOR PASSIVE TARGET TO WORK */

void *MPIDI_Win_passive_target_thread(void *arg);

volatile int MPIDI_Passive_target_thread_exit_flag=0;

int MPID_Win_create(void *base, MPI_Aint size, int disp_unit, MPI_Info info, 
                    MPID_Comm *comm_ptr, MPID_Win **win_ptr)
{
    int mpi_errno;

    MPIDI_STATE_DECL(MPID_STATE_MPI_WIN_CREATE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPI_WIN_CREATE);

    *win_ptr = (MPID_Win *)MPIU_Handle_obj_alloc( &MPID_Win_mem );
    if (!(*win_ptr)) {
        mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
        MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_CREATE);
        return mpi_errno;
    }

    (*win_ptr)->fence_cnt = 0;
    (*win_ptr)->base = base;
    (*win_ptr)->size = size;
    (*win_ptr)->disp_unit = disp_unit;
    (*win_ptr)->start_group_ptr = NULL; 
    (*win_ptr)->post_group_ptr = NULL; 

    mpi_errno = NMPI_Comm_dup(comm_ptr->handle, &((*win_ptr)->comm));

    pthread_create(&((*win_ptr)->passive_target_thread_id), NULL,
                   MPIDI_Win_passive_target_thread, (void *) (*win_ptr));  

    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_CREATE);

    return mpi_errno;
}


void *MPIDI_Win_passive_target_thread(void *arg)
{
    int comm_size, src, nops_from_proc, rank, i, j, tag;
    MPI_Comm comm;
    typedef struct MPIU_RMA_op_info {
        int type;
        MPI_Aint disp;
        int count;
        int datatype;
        int datatype_kind;  /* basic or derived */
        MPI_Op op;
        int lock_type;
    } MPIU_RMA_op_info;
    MPIU_RMA_op_info rma_op_info;
    typedef struct MPIU_RMA_dtype_info { /* for derived datatypes */
        int           is_contig; 
        int           size;     
        MPI_Aint      extent;   
        int           loopsize; 
        void          *loopinfo;  /* pointer needed to update pointers
                                     within dataloop on remote side */
        int           loopinfo_depth; 
        MPI_Aint ub, lb, true_ub, true_lb;
        int has_sticky_ub, has_sticky_lb;
    } MPIU_RMA_dtype_info;
    MPIU_RMA_dtype_info dtype_info;
    void *dataloop=NULL;    /* to store dataloops for each datatype */
    MPI_User_function *uop;
    MPI_Op op;
    void *tmp_buf;
    MPI_Aint extent, ptrdiff;
    MPID_Win *win_ptr;
    int *mpi_errno, flag=0;
    MPI_Status status;
    MPID_Datatype *new_dtp=NULL;

    mpi_errno = (int *) MPIU_Malloc(sizeof(int));
    if (!mpi_errno) {
        *mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
        return mpi_errno;
    }
    *mpi_errno = MPI_SUCCESS;

    win_ptr = (MPID_Win *) arg;

    MPIR_Nest_incr();
    
    comm = win_ptr->comm;
    NMPI_Comm_size(comm, &comm_size);
    NMPI_Comm_rank(comm, &rank);

    /* for each process in comm, do an iprobe to see if there is a
       passive target RMA request. If there is, go ahead and perform
       the RMA operations. Repeat until 
       MPIDI_Passive_target_thread_exit_flag is set. The flag gets set in
       MPI_Win_free after all processes have called MPI_Barrier. Then
       this thread can safely exit. */

    while (!MPIDI_Passive_target_thread_exit_flag) {
/* THE IPROBE BELOW HAS BEEN COMMENTED OUT OTHERWISE EVEN THE
   ACTIVE-TARGET RMA WILL NOT WORK BECAUSE OF THREAD-SAFETY
   ISSUES. UNCOMMENT THE LINE BELOW FOR PASSIVE TARGET TO WORK */

/*        *mpi_errno = NMPI_Iprobe(MPI_ANY_SOURCE,
                                 MPIDI_PASSIVE_TARGET_RMA_TAG, comm,
                                 &flag, &status);
        if (*mpi_errno) return mpi_errno;
*/        
        if (flag) {
            src = status.MPI_SOURCE;
            *mpi_errno = NMPI_Recv(&nops_from_proc, 1, MPI_INT, src,
                                    MPIDI_PASSIVE_TARGET_RMA_TAG, comm,
                                    MPI_STATUS_IGNORE); 
            if (*mpi_errno) return mpi_errno;

            /* Now for each op from the source, first
               get the info regarding that op and then post an isend or
               irecv to perform the operation. */

            tag = 234;
            for (j=0; j<nops_from_proc; j++) {
                *mpi_errno = NMPI_Recv(&rma_op_info,
                                       sizeof(MPIU_RMA_op_info), MPI_BYTE, 
                                       src, tag, comm,
                                       MPI_STATUS_IGNORE);
                if (*mpi_errno) return mpi_errno;
                tag++;
                
                if (rma_op_info.datatype_kind == MPID_RMA_DATATYPE_DERIVED) {
                    /* recv the derived datatype info and create
                       derived datatype */
                    *mpi_errno = NMPI_Recv(&dtype_info,
                                           sizeof(MPIU_RMA_dtype_info),
                                           MPI_BYTE, src, tag, comm,
                                           MPI_STATUS_IGNORE);
                    if (*mpi_errno) return mpi_errno;
                    tag++;

                    /* recv dataloop */
                    dataloop = (void *) MPIU_Malloc(dtype_info.loopsize);
                    if (!dataloop) {
                        *mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
                        return mpi_errno;
                    }
                    
                    *mpi_errno = NMPI_Recv(dataloop, dtype_info.loopsize,
                                           MPI_BYTE, src, tag, comm,
                                           MPI_STATUS_IGNORE);
                    if (*mpi_errno) return mpi_errno;
                    tag++;
                    
                    /* create derived datatype */
                    
                    /* allocate new datatype object and handle */
                    new_dtp = (MPID_Datatype *) MPIU_Handle_obj_alloc(&MPID_Datatype_mem);
                    if (!new_dtp) {
                        *mpi_errno = MPIR_Err_create_code(MPI_ERR_OTHER, "**nomem", 0);
                        return mpi_errno;
                    }
                    
                    /* Note: handle is filled in by MPIU_Handle_obj_alloc() */
                    MPIU_Object_set_ref(new_dtp, 1);
                    new_dtp->is_permanent = 0;
                    new_dtp->is_committed = 1;
                    new_dtp->attributes   = 0;
                    new_dtp->cache_id     = 0;
                    new_dtp->name[0]      = 0;
                    new_dtp->is_contig = dtype_info.is_contig;
                    new_dtp->size = dtype_info.size;
                    new_dtp->extent = dtype_info.extent;
                    new_dtp->loopsize = dtype_info.loopsize;
                    new_dtp->loopinfo_depth = dtype_info.loopinfo_depth; 
                    /* set dataloop pointer */
                    new_dtp->loopinfo = dataloop;
                    /* set datatype handle to be used in send/recv
                       below */
                    rma_op_info.datatype = new_dtp->handle;
                    
                    new_dtp->ub = dtype_info.ub;
                    new_dtp->lb = dtype_info.lb;
                    new_dtp->true_ub = dtype_info.true_ub;
                    new_dtp->true_lb = dtype_info.true_lb;
                    new_dtp->has_sticky_ub = dtype_info.has_sticky_ub;
                    new_dtp->has_sticky_lb = dtype_info.has_sticky_lb;
                    /* update pointers in dataloop */
                    ptrdiff = (char *) (new_dtp->loopinfo) - (char *)
                        (dtype_info.loopinfo); 
                    
                    MPID_Dataloop_update(new_dtp->loopinfo, ptrdiff);
                }

                switch (rma_op_info.type) {
                case MPID_REQUEST_LOCK:
                    /* We don't need to do anything for a lock request
                       because all RMA requests from src 
                       will be performed below before we perform RMA ops
                       from any other process. */
                    break;
                case MPID_REQUEST_PUT:
                    /* recv the put */
                    *mpi_errno = NMPI_Recv((char *) win_ptr->base +
                                           win_ptr->disp_unit *
                                           rma_op_info.disp,
                                           rma_op_info.count,
                                           rma_op_info.datatype,
                                           src, tag, comm,
                                           MPI_STATUS_IGNORE);
                    if (*mpi_errno) return mpi_errno;
                    tag++;
                    break;
                case MPID_REQUEST_GET:
                    /* send the get */
                    *mpi_errno = NMPI_Send((char *) win_ptr->base +
                                           win_ptr->disp_unit *
                                           rma_op_info.disp,
                                           rma_op_info.count,
                                           rma_op_info.datatype,
                                           src, tag, comm);
                    if (*mpi_errno) return mpi_errno;
                    tag++;
                    break;
                case MPID_REQUEST_ACCUMULATE:
                    /* recv the data into a temp buffer and perform
                       the reduction operation */
                    NMPI_Type_extent(rma_op_info.datatype, 
                                     &extent); 
                    tmp_buf = MPIU_Malloc(extent * 
                                          rma_op_info.count);
                    if (!tmp_buf) {
                        *mpi_errno = MPIR_Err_create_code(
                            MPI_ERR_OTHER, "**nomem", 0 ); 
                        return mpi_errno;
                    }
                    *mpi_errno = NMPI_Recv(tmp_buf,
                                           rma_op_info.count,
                                           rma_op_info.datatype,
                                           src, tag, comm,
                                           MPI_STATUS_IGNORE);
                    if (*mpi_errno) return mpi_errno;
                    tag++;
                    
                    op = rma_op_info.op;
                    if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
                        /* get the function by indexing into the op table */
                        uop = MPIR_Op_table[op%16 - 1];
                    }
                    else {
                        *mpi_errno = MPIR_Err_create_code( MPI_ERR_OP,
                                                           "**opundefined","**opundefined %s", "only predefined ops valid for MPI_Accumulate" );
                        return mpi_errno;
                    }
                    (*uop)(tmp_buf, (char *) win_ptr->base +
                           win_ptr->disp_unit *
                           rma_op_info.disp,
                           &(rma_op_info.count),
                           &(rma_op_info.datatype));
                    MPIU_Free(tmp_buf);
                    break;
                default:
                    *mpi_errno = MPIR_Err_create_code( MPI_ERR_OP,
                                                       "****intern","**opundefined %s", "RMA target received unknown RMA operation" );
                    return mpi_errno;
                }

                if (rma_op_info.datatype_kind == MPID_RMA_DATATYPE_DERIVED) {
                    MPIU_Handle_obj_free(&MPID_Datatype_mem, new_dtp);
                    MPIU_Free(dataloop);
                }
            }
            /* We need to acknowledge that all the operations are complete at
               the target because the origin needs to know that before
               it can return from win_unlock. */

            *mpi_errno = NMPI_Send(&i, 0, MPI_INT, src,
                                   MPIDI_PASSIVE_TARGET_DONE_TAG, comm); 
            if (*mpi_errno) return mpi_errno;
        }
    }

    MPIR_Nest_decr();
    
    return mpi_errno;
}
