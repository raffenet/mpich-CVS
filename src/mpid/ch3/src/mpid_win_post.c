/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#ifdef HAVE_PTHREAD_H
#define THREAD_RETURN_TYPE void *
#elif defined(HAVE_WINTHREADS)
#define THREAD_RETURN_TYPE DWORD
#else
#define THREAD_RETURN_TYPE int
#endif

THREAD_RETURN_TYPE MPIDI_Win_wait_thread(void *arg);

#undef FUNCNAME
#define FUNCNAME MPID_Win_post
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_post(MPID_Group *group_ptr, int assert, MPID_Win *win_ptr)
{
#if defined(HAVE_WINTHREADS) && !defined(MPICH_SINGLE_THREADED)
    DWORD dwThreadID;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_POST);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_POST);

#   ifdef MPICH_SINGLE_THREADED
    {
	int mpi_errno;
	mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**needthreads", 0 );
	MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_POST);
	return mpi_errno;
    }
#   endif

    win_ptr->post_group_ptr = group_ptr;
    MPIU_Object_add_ref( group_ptr );

#   if !defined(MPICH_SINGLE_THREADED)
    {
#       ifdef HAVE_PTHREAD_H
	{
	    pthread_create(&(win_ptr->wait_thread_id), NULL, MPIDI_Win_wait_thread, (void *) win_ptr);
	}
#       elif defined(HAVE_WINTHREADS)
	{
	    win_ptr->wait_thread_id = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MPIDI_Win_wait_thread, win_ptr, 0,
						   &dwThreadID);
	}
#       else
#           error Error: No thread package specified.
#       endif
    }
#   endif
    
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_POST);
    return MPI_SUCCESS;
}

THREAD_RETURN_TYPE MPIDI_Win_wait_thread(void *arg)
{
    int comm_size, src, *nops_from_proc, rank, i, j, *tags;
    MPI_Comm comm;
    typedef struct MPIU_RMA_op_info {
        int type;
        MPI_Aint disp;
        int count;
        int datatype;
        int datatype_kind;  /* basic or derived */
        MPI_Op op;
    } MPIU_RMA_op_info;
    MPIU_RMA_op_info rma_op_info;
    typedef struct MPIU_RMA_dtype_info { /* for derived datatypes */
        int           is_contig; 
        int           n_contig_blocks; 
        int           size;     
        MPI_Aint      extent;   
        int           loopsize; 
        void          *loopinfo;  /* pointer needed to update pointers
                                     within dataloop on remote side */
        int           loopinfo_depth; 
        int           eltype;
        MPI_Aint ub, lb, true_ub, true_lb;
        int has_sticky_ub, has_sticky_lb;
    } MPIU_RMA_dtype_info;
    MPIU_RMA_dtype_info dtype_info;
    void *dataloop=NULL;    /* to store dataloops for each datatype */
    MPI_Request *reqs;
    MPI_User_function *uop;
    MPI_Op op;
    void *tmp_buf;
    MPI_Aint extent, ptrdiff, true_extent, true_lb;
    MPI_Group win_grp, post_grp;
    int post_grp_size, *ranks_in_post_grp, *ranks_in_win_grp;
    MPID_Win *win_ptr;
    int mpi_errno;
    MPID_Datatype *new_dtp=NULL;
    void *win_buf_addr;

    mpi_errno = MPI_SUCCESS;

    win_ptr = (MPID_Win *) arg;

    MPIR_Nest_incr();
    
    comm = win_ptr->comm;
    NMPI_Comm_size(comm, &comm_size);
    NMPI_Comm_rank(comm, &rank);

    /* For each process in post_group, find out how many RMA ops from
       that process is this process a target for */

    nops_from_proc = (int *) MPIU_Calloc(comm_size, sizeof(int));
    if (!nops_from_proc) {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        return (THREAD_RETURN_TYPE)mpi_errno;
    }

    /* We need to translate the ranks of the processes in
       post_group to ranks in win_ptr->comm, so that we
       can do communication */

    NMPI_Comm_group(win_ptr->comm, &win_grp);
    post_grp_size = win_ptr->post_group_ptr->size;

    ranks_in_post_grp = (int *) MPIU_Malloc(post_grp_size * sizeof(int));
    if (!ranks_in_post_grp) {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        return (THREAD_RETURN_TYPE)mpi_errno;
    }
    ranks_in_win_grp = (int *) MPIU_Malloc(post_grp_size * sizeof(int));
    if (!ranks_in_win_grp) {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        return (THREAD_RETURN_TYPE)mpi_errno;
    }

    for (i=0; i<post_grp_size; i++)
        ranks_in_post_grp[i] = i;

    post_grp = win_ptr->post_group_ptr->handle;
    NMPI_Group_translate_ranks(post_grp, post_grp_size,
                               ranks_in_post_grp, win_grp, ranks_in_win_grp);

    reqs = (MPI_Request *)  MPIU_Malloc(post_grp_size*sizeof(MPI_Request));
    if (!reqs) {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        return (THREAD_RETURN_TYPE)mpi_errno;
    }
    
    tags = (int *) MPIU_Calloc(comm_size, sizeof(int)); 
    if (!tags) {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        return (THREAD_RETURN_TYPE)mpi_errno;
    }

    for (i=0; i<post_grp_size; i++) {
        src = ranks_in_win_grp[i];
        mpi_errno = NMPI_Irecv(&nops_from_proc[src], 1, MPI_INT, src,
                               tags[src], comm, &reqs[i]);
        if (mpi_errno) return (THREAD_RETURN_TYPE)mpi_errno;
        tags[src]++;
    }

    MPIU_Free(ranks_in_win_grp);
    MPIU_Free(ranks_in_post_grp);

    mpi_errno = NMPI_Waitall(post_grp_size, reqs, MPI_STATUSES_IGNORE);
    if (mpi_errno) return (THREAD_RETURN_TYPE)mpi_errno;

    /* Now for each op for which this process is a target, first
       get the info regarding that op and then post an isend or
       irecv to perform the operation. */

    for (i=0; i<comm_size; i++) {
        /* instead of having everyone start receiving from 0,
           stagger the recvs a bit */ 
        src = (rank + i) % comm_size;
        
        for (j=0; j<nops_from_proc[src]; j++) {
            mpi_errno = NMPI_Recv(&rma_op_info,
                                  sizeof(MPIU_RMA_op_info), MPI_BYTE, 
                                  src, tags[src], comm,
                                  MPI_STATUS_IGNORE);
            if (mpi_errno) return (THREAD_RETURN_TYPE)mpi_errno;
            tags[src]++;
            
            if (rma_op_info.datatype_kind == MPID_RMA_DATATYPE_DERIVED) {
                /* recv the derived datatype info and create
                   derived datatype */
                mpi_errno = NMPI_Recv(&dtype_info,
                                      sizeof(MPIU_RMA_dtype_info),
                                      MPI_BYTE, src, tags[src], comm,
                                      MPI_STATUS_IGNORE);
                if (mpi_errno) return (THREAD_RETURN_TYPE)mpi_errno;
                tags[src]++;

                /* recv dataloop */
                dataloop = (void *) MPIU_Malloc(dtype_info.loopsize);
                if (!dataloop) {
                    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
                    return (THREAD_RETURN_TYPE)mpi_errno;
                }

                mpi_errno = NMPI_Recv(dataloop, dtype_info.loopsize,
                                       MPI_BYTE, src, tags[src], comm,
                                       MPI_STATUS_IGNORE);
                if (mpi_errno) return (THREAD_RETURN_TYPE)mpi_errno;
                tags[src]++;

                /* create derived datatype */

                /* allocate new datatype object and handle */
                new_dtp = (MPID_Datatype *) MPIU_Handle_obj_alloc(&MPID_Datatype_mem);
                if (!new_dtp) {
                    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
                    return (THREAD_RETURN_TYPE)mpi_errno;
                }
                    
                /* Note: handle is filled in by MPIU_Handle_obj_alloc() */
                MPIU_Object_set_ref(new_dtp, 1);
                new_dtp->is_permanent = 0;
                new_dtp->is_committed = 1;
                new_dtp->attributes   = 0;
                new_dtp->cache_id     = 0;
                new_dtp->name[0]      = 0;
                new_dtp->is_contig = dtype_info.is_contig;
                new_dtp->n_contig_blocks = dtype_info.n_contig_blocks;
                new_dtp->size = dtype_info.size;
                new_dtp->extent = dtype_info.extent;
                new_dtp->loopsize = dtype_info.loopsize;
                new_dtp->loopinfo_depth = dtype_info.loopinfo_depth; 
                new_dtp->eltype = dtype_info.eltype;

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
                ptrdiff = (MPI_Aint)((char *) (new_dtp->loopinfo) - (char *)
                    (dtype_info.loopinfo));

                MPID_Dataloop_update(new_dtp->loopinfo, ptrdiff);
            }

            switch (rma_op_info.type) {
            case MPID_REQUEST_PUT:
                /* recv the put */
                mpi_errno = NMPI_Recv((char *) win_ptr->base +
                                      win_ptr->disp_unit *
                                      rma_op_info.disp,
                                      rma_op_info.count,
                                      rma_op_info.datatype,
                                      src, tags[src], comm,
                                      MPI_STATUS_IGNORE);
                if (mpi_errno) return (THREAD_RETURN_TYPE)mpi_errno;
                break;
            case MPID_REQUEST_GET:
                /* send the get */
                mpi_errno = NMPI_Send((char *) win_ptr->base +
                                      win_ptr->disp_unit *
                                      rma_op_info.disp,
                                      rma_op_info.count,
                                      rma_op_info.datatype,
                                      src, tags[src], comm);
                if (mpi_errno) return (THREAD_RETURN_TYPE)mpi_errno;
                break;
            case MPID_REQUEST_ACCUMULATE:
                /* recv the data into a temp buffer and perform
                   the reduction operation */
                mpi_errno =
                    NMPI_Type_get_true_extent(rma_op_info.datatype, 
                                              &true_lb, &true_extent);  
                if (mpi_errno) return (THREAD_RETURN_TYPE)mpi_errno;

                MPID_Datatype_get_extent_macro(rma_op_info.datatype, 
                                               extent); 
                tmp_buf = MPIU_Malloc(rma_op_info.count * 
                                      (MPIR_MAX(extent,true_extent)));  
                if (!tmp_buf) {
                    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
                    return (THREAD_RETURN_TYPE)mpi_errno;
                }
                /* adjust for potential negative lower bound in datatype */
                tmp_buf = (void *)((char*)tmp_buf - true_lb);

                mpi_errno = NMPI_Recv(tmp_buf,
                                      rma_op_info.count,
                                      rma_op_info.datatype,
                                      src, tags[src], comm,
                                      MPI_STATUS_IGNORE);
                if (mpi_errno) return (THREAD_RETURN_TYPE)mpi_errno;
                
                op = rma_op_info.op;
                if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
                    /* get the function by indexing into the op table */
                    uop = MPIR_Op_table[op%16 - 1];
                }
                else {
                    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OP,
                                                      "**opnotpredefined", "**opnotpredefined %d", op );
                    return (THREAD_RETURN_TYPE)mpi_errno;
                }

                win_buf_addr = (char *) win_ptr->base +
                    win_ptr->disp_unit * rma_op_info.disp;

                if (rma_op_info.datatype_kind == MPID_RMA_DATATYPE_BASIC) {
                    (*uop)(tmp_buf, win_buf_addr,
                           &(rma_op_info.count),
                           &(rma_op_info.datatype));
                }
                else { /* derived datatype */
                    MPID_Segment *segp;
                    DLOOP_VECTOR *dloop_vec;
                    MPI_Aint first, last;
		    int vec_len;
                    MPI_Datatype type;
                    int type_size, count;
                    
                    segp = MPID_Segment_alloc();
                    MPID_Segment_init(NULL,
                                      rma_op_info.count, 
                                      rma_op_info.datatype, segp);
                    first = 0;
                    last  = SEGMENT_IGNORE_LAST;
                    
                    vec_len = new_dtp->n_contig_blocks *
                        rma_op_info.count + 1; /* +1 needed
                                                  because Rob says
                                                  so */
                     dloop_vec = (DLOOP_VECTOR *)
                        MPIU_Malloc(vec_len * sizeof(DLOOP_VECTOR));
                    if (!dloop_vec) {
                        mpi_errno = MPIR_Err_create_code(
                            MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 ); 
                        return (THREAD_RETURN_TYPE)mpi_errno;
                    }
                    
                    MPID_Segment_pack_vector(segp, first, &last,
                                             dloop_vec, &vec_len);
                    
                    type = new_dtp->eltype;
                    type_size = MPID_Datatype_get_basic_size(type);
                    for (i=0; i<vec_len; i++) {
                        count = (dloop_vec[i].DLOOP_VECTOR_LEN)/type_size;
                        (*uop)((char *)tmp_buf + POINTER_TO_AINT( dloop_vec[i].DLOOP_VECTOR_BUF ),
                            (char *)win_buf_addr + POINTER_TO_AINT( dloop_vec[i].DLOOP_VECTOR_BUF ),
                               &count, &type);
                    }
                    
                    MPID_Segment_free(segp);
                    MPIU_Free(dloop_vec);
                }

                MPIU_Free((char*)tmp_buf + true_lb);
                break;
            default:
                mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OP,
                                                  "**opundefined_rma","**opundefined_rma %d", rma_op_info.type );
                return (THREAD_RETURN_TYPE)mpi_errno;
            }
            tags[src]++;

            if (rma_op_info.datatype_kind == MPID_RMA_DATATYPE_DERIVED) {
                MPIU_Handle_obj_free(&MPID_Datatype_mem, new_dtp);
                MPIU_Free(dataloop);
            }
        }
    }
    
    MPIR_Nest_decr();
    
    MPIU_Free(tags);
    MPIU_Free(reqs);
    MPIU_Free(nops_from_proc);
    NMPI_Group_free(&win_grp);

    MPIR_Group_release(win_ptr->post_group_ptr);
    win_ptr->post_group_ptr = NULL; 

    return (THREAD_RETURN_TYPE)mpi_errno;
}
