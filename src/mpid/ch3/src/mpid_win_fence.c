/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

MPIU_RMA_ops *MPIU_RMA_ops_list=NULL;

#undef FUNCNAME
#define FUNCNAME MPID_Win_fence
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_fence(int assert, MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS, comm_size, done;
    int *rma_target_proc, *nops_to_proc, i, total_op_count, *curr_ops_cnt;
    MPIU_RMA_ops *curr_ptr, *next_ptr;
    MPID_Comm *comm_ptr;
    MPID_Request **requests; /* array of requests */
    int *decr_addr;

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
    MPIU_RMA_dtype_info *dtype_infos;
    void **dataloops;    /* to store dataloops for each datatype */
    MPID_Datatype *dtp, *new_dtp=NULL;

    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_FENCE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_FENCE);

    if (assert & MPI_MODE_NOPRECEDE) {
        win_ptr->fence_cnt = (assert & MPI_MODE_NOSUCCEED) ? 0 : 1;

        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
        return MPI_SUCCESS;
    }

    if ((win_ptr->fence_cnt == 0) && ((assert & MPI_MODE_NOSUCCEED) != 1)) {
        /* win_ptr->fence_cnt == 0 means either this is the very first
           call to fence or the preceding fence had the
           MPI_MODE_NOSUCCEED assert. 
           Do nothing except increment the count. */
        win_ptr->fence_cnt = 1;
    }
    else {
        /* This is the second or later fence. Do all the preceding RMA ops. */

        MPID_Comm_get_ptr( win_ptr->comm, comm_ptr );

        /* First inform every process whether it is a target of RMA
           ops from this process */
        comm_size = comm_ptr->local_size;
        rma_target_proc = (int *) MPIU_Calloc(comm_size, sizeof(int));
        if (!rma_target_proc) {
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
            MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
            return mpi_errno;
        }

        /* keep track of no. of ops to each proc. Needed for knowing
           whether or not to decrement the completion counter. The
           completion counter is decremented only on the last
           operation. */
        nops_to_proc = (int *) MPIU_Calloc(comm_size, sizeof(int));
        if (!nops_to_proc) {
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
            MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
            return mpi_errno;
        }

        /* set rma_target_proc[i] to 1 if rank i is a target of RMA
           ops from this process */
        total_op_count = 0;
        curr_ptr = MPIU_RMA_ops_list;
        while (curr_ptr != NULL) {
            total_op_count++;
            rma_target_proc[curr_ptr->target_rank] = 1;
            nops_to_proc[curr_ptr->target_rank]++;
            curr_ptr = curr_ptr->next;
        }

        /* do a global sum on rma_target_proc. As a result,
           rma_target_proc[i] indicates how many procs. will be
           doing RMA ops on rank i's window */
        
        /* first initialize the completion counter. needed for the MT case. */
        win_ptr->my_counter = comm_size;

        MPIR_Nest_incr();
        mpi_errno = NMPI_Allreduce(MPI_IN_PLACE, rma_target_proc, comm_size,
                                   MPI_INT, MPI_SUM, win_ptr->comm);
        MPIR_Nest_decr();
        if (mpi_errno != MPI_SUCCESS) return mpi_errno;

        /* Set the completion counter */
        /* FIXME: MT: this needs to be done atomically because other
           procs have the address and could decrement it. */
        win_ptr->my_counter = win_ptr->my_counter - comm_size +
            rma_target_proc[comm_ptr->rank];  

        MPIU_Free(rma_target_proc);

        requests = (MPID_Request **) MPIU_Malloc(total_op_count *
                                                 sizeof(MPID_Request*));
        if (!requests) {
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
            MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
            return mpi_errno;
        }

        curr_ops_cnt = (int *) MPIU_Calloc(comm_size, sizeof(int));
        if (!curr_ops_cnt) {
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
            MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
            return mpi_errno;
        }

        i = 0;
        curr_ptr = MPIU_RMA_ops_list;
        while (curr_ptr != NULL) {
            /* The completion counter at the target is decremented
               only on the last operation on the target. Otherwise, we
               pass NULL */
            if (curr_ops_cnt[curr_ptr->target_rank] ==
                nops_to_proc[curr_ptr->target_rank] - 1) 
                decr_addr = win_ptr->all_counters[curr_ptr->target_rank];
            else 
                decr_addr = NULL;

            switch (curr_ptr->type) {
            case (MPID_REQUEST_PUT):
            case (MPID_REQUEST_ACCUMULATE):
                mpi_errno = MPIDI_CH3I_Send_rma_msg(curr_ptr, win_ptr,
                                                    decr_addr, &requests[i]); 
                if (mpi_errno != MPI_SUCCESS) return mpi_errno;
                break;
            case (MPID_REQUEST_GET):
                mpi_errno = MPIDI_CH3I_Recv_rma_msg(curr_ptr, win_ptr,
                                                    decr_addr, &requests[i]);
                if (mpi_errno != MPI_SUCCESS) return mpi_errno;
                break;
            default:
                /* FIXME - return some error code here */
                break;
            }
            i++;
            curr_ops_cnt[curr_ptr->target_rank]++;
            curr_ptr = curr_ptr->next;
        }

        MPIU_Free(nops_to_proc);
        MPIU_Free(curr_ops_cnt);

        done = 1;
        while (total_op_count) {
            MPID_Progress_start();
            for (i=0; i<total_op_count; i++) {
                if (requests[i] != NULL) {
                    if (*(requests[i]->cc_ptr) != 0)
                        done = 0;
                    else {
                        mpi_errno = requests[i]->status.MPI_ERROR;
                        if (mpi_errno != MPI_SUCCESS) {
                            MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
                            return mpi_errno;
                        }
                        MPID_Request_release(requests[i]);
                        requests[i] = NULL;
                    }
                }
            }
            if (!done) {
                mpi_errno = MPID_Progress_wait();
                done = 1;
            }
            else {
                MPID_Progress_end();
                break;
            }
        } 

        MPIU_Free(requests);

        /* free MPIU_RMA_ops_list */
        curr_ptr = MPIU_RMA_ops_list;
        while (curr_ptr != NULL) {
            next_ptr = curr_ptr->next;
            MPIU_Free(curr_ptr);
            curr_ptr = next_ptr;
        }
        MPIU_RMA_ops_list = NULL;

        /* wait for all operations from other processes to finish */
        while (win_ptr->my_counter) {
            MPID_Progress_start();
            if (win_ptr->my_counter) {
                mpi_errno = MPID_Progress_wait();
                if (mpi_errno != MPI_SUCCESS) {
                    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
                    return mpi_errno;
                }
            }
            else 
                MPID_Progress_end();
        } 

        if (assert & MPI_MODE_NOSUCCEED) 
            win_ptr->fence_cnt = 0;
    }

    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
    return mpi_errno;
}


int MPIDI_CH3I_Send_rma_msg(MPIU_RMA_ops *rma_op, MPID_Win *win_ptr,
                            int *decr_addr, MPID_Request **request) 
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_put_t *put_pkt = &upkt.put;
    MPIDI_CH3_Pkt_accum_t *accum_pkt = &upkt.accum;
    MPID_IOV iov[MPID_IOV_LIMIT];
    int type_size, mpi_errno=MPI_SUCCESS;
    MPIDI_VC *vc;
    MPID_Comm *comm_ptr;

    if (rma_op->type == MPID_REQUEST_PUT) {
        put_pkt->type = MPIDI_CH3_PKT_PUT;
        put_pkt->addr = (char *) win_ptr->base_addrs[rma_op->target_rank] +
            win_ptr->disp_units[rma_op->target_rank] * rma_op->target_disp;
        put_pkt->count = rma_op->target_count;
        put_pkt->datatype = rma_op->target_datatype;
        put_pkt->decr_ctr = decr_addr;

        iov[0].MPID_IOV_BUF = (void*) put_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*put_pkt);
    }
    else {
        accum_pkt->type = MPIDI_CH3_PKT_ACCUMULATE;
        accum_pkt->addr = (char *) win_ptr->base_addrs[rma_op->target_rank] +
            win_ptr->disp_units[rma_op->target_rank] * rma_op->target_disp;
        accum_pkt->count = rma_op->target_count;
        accum_pkt->datatype = rma_op->target_datatype;
        accum_pkt->op = rma_op->op;
        accum_pkt->decr_ctr = decr_addr;

        iov[0].MPID_IOV_BUF = (void*) accum_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*accum_pkt);
    }

/*    printf("send pkt: type %d, addr %d, count %d, base %d\n", rma_pkt->type,
           rma_pkt->addr, rma_pkt->count, win_ptr->base_addrs[rma_op->target_rank]);
    fflush(stdout);
*/

    if (HANDLE_GET_KIND(rma_op->origin_datatype) == HANDLE_KIND_BUILTIN) {
        iov[1].MPID_IOV_BUF = rma_op->origin_addr;
        MPID_Datatype_get_size_macro(rma_op->origin_datatype, type_size);
        iov[1].MPID_IOV_LEN = rma_op->origin_count * type_size;
	    
        MPID_Comm_get_ptr(win_ptr->comm, comm_ptr);
        vc = comm_ptr->vcr[rma_op->target_rank];

        mpi_errno = MPIDI_CH3_iStartMsgv(vc, iov, 2, request);
        if (mpi_errno != MPI_SUCCESS)
        {
            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|rmamsg", 0);
            return mpi_errno;
        }
    }
    else
    {
        printf("derived datatype\n");
        exit(1);
    }

    return mpi_errno;
}



int MPIDI_CH3I_Recv_rma_msg(MPIU_RMA_ops *rma_op, MPID_Win *win_ptr,
                            int *decr_addr, MPID_Request **request) 
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_get_t *get_pkt = &upkt.get;
    int mpi_errno;
    MPIDI_VC *vc;
    MPID_Comm *comm_ptr;
    MPID_Request *req = NULL;

    req = MPID_Request_create();
    *request = req;

    MPIU_Object_set_ref(req, 2);

    req->ch3.user_buf = rma_op->origin_addr;
    req->ch3.user_count = rma_op->origin_count;
    req->ch3.datatype = rma_op->origin_datatype;
    req->ch3.decr_ctr = NULL;

    get_pkt->type = MPIDI_CH3_PKT_GET;
    get_pkt->addr = (char *) win_ptr->base_addrs[rma_op->target_rank] +
        win_ptr->disp_units[rma_op->target_rank] * rma_op->target_disp;
    get_pkt->count = rma_op->target_count;
    get_pkt->datatype = rma_op->target_datatype;
    get_pkt->request = req;
    get_pkt->decr_ctr = decr_addr;

/*    printf("send pkt: type %d, addr %d, count %d, base %d\n", rma_pkt->type,
           rma_pkt->addr, rma_pkt->count, win_ptr->base_addrs[rma_op->target_rank]);
    fflush(stdout);
*/
	    
    MPID_Comm_get_ptr(win_ptr->comm, comm_ptr);
    vc = comm_ptr->vcr[rma_op->target_rank];

    mpi_errno = MPIDI_CH3_iStartMsg(vc, get_pkt, sizeof(*get_pkt), &req);

    if (mpi_errno != MPI_SUCCESS)
    {
        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|rmamsg", 0);
        return mpi_errno;
    }
    if (req != NULL)
        MPID_Request_release(req);

    return mpi_errno;
}




#ifdef OLDSTUFF
/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

MPIU_RMA_ops *MPIU_RMA_ops_list=NULL;

#undef FUNCNAME
#define FUNCNAME MPID_Win_fence
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_fence(int assert, MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS, comm_size, *nops_to_proc, src, dest;
    int *nops_from_proc, rank, i, j, *tags, total_op_count, req_cnt;
    MPIU_RMA_ops *curr_ptr, *next_ptr;
    MPI_Comm comm;
    typedef struct MPIU_RMA_op_info {
        int type;
        MPI_Aint disp;
        int count;
        int datatype;
        int datatype_kind;  /* basic or derived */
        MPI_Op op;
    } MPIU_RMA_op_info;
    MPIU_RMA_op_info *rma_op_infos;
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
    MPIU_RMA_dtype_info *dtype_infos;
    void **dataloops;    /* to store dataloops for each datatype */
    MPI_Request *reqs;
    MPI_User_function *uop;
    MPI_Op op;
    void *tmp_buf;
    MPI_Aint extent, ptrdiff, true_lb, true_extent;
    MPID_Datatype *dtp, *new_dtp=NULL;
    void *win_buf_addr;

    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_FENCE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_FENCE);

    if (assert & MPI_MODE_NOPRECEDE) {
        win_ptr->fence_cnt = (assert & MPI_MODE_NOSUCCEED) ? 0 : 1;

        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
        return MPI_SUCCESS;
    }

    if ((win_ptr->fence_cnt == 0) && ((assert & MPI_MODE_NOSUCCEED) != 1)) {
        /* win_ptr->fence_cnt == 0 means either this is the very first
           call to fence or the preceding fence had the
           MPI_MODE_NOSUCCEED assert. 
           Do nothing except increment the count. */
        win_ptr->fence_cnt = 1;
    }
    else {
        /* This is the second or later fence. Do all the preceding RMA ops. */

	MPIR_Nest_incr();

        comm = win_ptr->comm;
        NMPI_Comm_size(comm, &comm_size);
        NMPI_Comm_rank(comm, &rank);

        /* First, each process informs every other process how many
           RMA ops from this process is it the target for. */

        nops_to_proc = (int *) MPIU_Calloc(comm_size, sizeof(int));
        if (!nops_to_proc) {
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
            MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
            return mpi_errno;
        }
        nops_from_proc = (int *) MPIU_Calloc(comm_size, sizeof(int));
        if (!nops_from_proc) {
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
            MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
            return mpi_errno;
        }

        total_op_count = 0;
        curr_ptr = MPIU_RMA_ops_list;
        while (curr_ptr != NULL) {
            nops_to_proc[curr_ptr->target_rank]++;
            total_op_count++;
            curr_ptr = curr_ptr->next;
        }
        
        NMPI_Alltoall(nops_to_proc, 1, MPI_INT, nops_from_proc, 1,
                     MPI_INT, comm);

        /* For each RMA op, first send the type (put or get), target
           displ, count, datatype. Then issue an isend for a 
           put or irecv for a get. */

        rma_op_infos = (MPIU_RMA_op_info *) 
            MPIU_Malloc((total_op_count+1) * sizeof(MPIU_RMA_op_info));
        /* allocate one extra for use in receiving info below */ 

        if (!rma_op_infos) {
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
            MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
            return mpi_errno;
        }

        reqs = (MPI_Request *)
            MPIU_Malloc(total_op_count*4*sizeof(MPI_Request)); 
        if (!reqs) {
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
            MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
            return mpi_errno;
        }

        tags = (int *) MPIU_Calloc(comm_size, sizeof(int)); 
        if (!tags) {
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
            MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
            return mpi_errno;
        }

        dtype_infos = (MPIU_RMA_dtype_info *)
            MPIU_Malloc((total_op_count+1)*sizeof(MPIU_RMA_dtype_info));
        /* allocate one extra for use when receiving data. see below */
        if (!dtype_infos) {
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
            MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
            return mpi_errno;
        }

        dataloops = (void **)
            MPIU_Malloc((total_op_count+1)*sizeof(void*));
        /* allocate one extra for use when receiving data. see below */
        if (!dataloops) {
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
            MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
            return mpi_errno;
        }
        for (i=0; i<=total_op_count; i++)
            dataloops[i] = NULL;

        i = 0;
        req_cnt = 0;
        curr_ptr = MPIU_RMA_ops_list;
        while (curr_ptr != NULL) {
            rma_op_infos[i].type = curr_ptr->type;
            rma_op_infos[i].disp = curr_ptr->target_disp;
            rma_op_infos[i].count = curr_ptr->target_count;
            rma_op_infos[i].datatype = curr_ptr->target_datatype;
            rma_op_infos[i].op = curr_ptr->op; 
            dest = curr_ptr->target_rank;

            if (HANDLE_GET_KIND(curr_ptr->target_datatype) ==
                HANDLE_KIND_BUILTIN) {
                /* basic datatype. send only the rma_op_info struct */
                rma_op_infos[i].datatype_kind = MPID_RMA_DATATYPE_BASIC;
                /* NEED TO CONVERT THE FOLLOWING TO USE STRUCT DATATYPE */
                mpi_errno = NMPI_Isend(&rma_op_infos[i],
                                       sizeof(MPIU_RMA_op_info), MPI_BYTE, 
                                       dest, tags[dest], comm,
                                       &reqs[req_cnt]); 
                if (mpi_errno) {
                    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
                    return mpi_errno;
                }
                req_cnt++;
                tags[dest]++;
            }
            else {
                /* derived datatype. send rma_op_info_struct as well
                   as derived datatype information */

                rma_op_infos[i].datatype_kind = MPID_RMA_DATATYPE_DERIVED; 
                /* fill derived datatype info */
                MPID_Datatype_get_ptr(curr_ptr->target_datatype, dtp);
                dtype_infos[i].is_contig = dtp->is_contig;
                dtype_infos[i].n_contig_blocks = dtp->n_contig_blocks;
                dtype_infos[i].size = dtp->size;
                dtype_infos[i].extent = dtp->extent;
                dtype_infos[i].loopsize = dtp->loopsize;
                dtype_infos[i].loopinfo_depth = dtp->loopinfo_depth;
                dtype_infos[i].eltype = dtp->eltype;
                dtype_infos[i].loopinfo = dtp->loopinfo;
                dtype_infos[i].ub = dtp->ub;
                dtype_infos[i].lb = dtp->lb;
                dtype_infos[i].true_ub = dtp->true_ub;
                dtype_infos[i].true_lb = dtp->true_lb;
                dtype_infos[i].has_sticky_ub = dtp->has_sticky_ub;
                dtype_infos[i].has_sticky_lb = dtp->has_sticky_lb;

                dataloops[i] = MPIU_Malloc(dtp->loopsize);
                if (!dataloops[i]) {
                    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
                    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
                    return mpi_errno;
                }
                memcpy(dataloops[i], dtp->loopinfo, dtp->loopsize);

                /* NEED TO CONVERT THE FOLLOWING TO USE STRUCT DATATYPE */
                mpi_errno = NMPI_Isend(&rma_op_infos[i],
                                       sizeof(MPIU_RMA_op_info), MPI_BYTE, 
                                       dest, tags[dest], comm,
                                       &reqs[req_cnt]); 
                if (mpi_errno) {
                    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
                    return mpi_errno;
                }
                req_cnt++;
                tags[dest]++;

                /* send the datatype info */
                /* NEED TO CONVERT THE FOLLOWING TO USE STRUCT DATATYPE */
                mpi_errno = NMPI_Isend(&dtype_infos[i],
                                       sizeof(MPIU_RMA_dtype_info), MPI_BYTE, 
                                       dest, tags[dest], comm,
                                       &reqs[req_cnt]); 
                if (mpi_errno) {
                    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
                    return mpi_errno;
                }
                req_cnt++;
                tags[dest]++;

                mpi_errno = NMPI_Isend(dataloops[i],
                                       dtp->loopsize, MPI_BYTE, 
                                       dest, tags[dest], comm,
                                       &reqs[req_cnt]); 
                if (mpi_errno) {
                    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
                    return mpi_errno;
                }
                req_cnt++;
                tags[dest]++;

                /* release the target dataype */
                MPID_Datatype_release(dtp);
            }

            /* now send or recv the data */

            if ((curr_ptr->type == MPID_REQUEST_PUT) ||
                (curr_ptr->type == MPID_REQUEST_ACCUMULATE))
                mpi_errno = NMPI_Isend(curr_ptr->origin_addr,
                                       curr_ptr->origin_count,
                                       curr_ptr->origin_datatype,
                                       dest, tags[dest], comm,
                                       &reqs[req_cnt]); 
            else
                mpi_errno = NMPI_Irecv(curr_ptr->origin_addr,
                                       curr_ptr->origin_count,
                                       curr_ptr->origin_datatype,
                                       dest, tags[dest], comm,
                                       &reqs[req_cnt]); 
            if (mpi_errno) {
                MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
                return mpi_errno;
            }
            if (HANDLE_GET_KIND(curr_ptr->origin_datatype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype_get_ptr(curr_ptr->origin_datatype, dtp);
                MPID_Datatype_release(dtp);
            }

            req_cnt++;
            tags[dest]++;

            curr_ptr = curr_ptr->next;
            i++;
        }        


        /* Now for each op for which this process is a target, first
           get the info regarding that op and then post an isend or
           irecv to perform the operation. */

        for (i=0; i<comm_size; i++) {
            /* instead of having everyone start receiving from 0,
               stagger the recvs a bit */ 
            src = (rank + i) % comm_size;
            tags[src] = 0;  /* start from tag 0 */

            for (j=0; j<nops_from_proc[src]; j++) {
                /* recv the info about the RMA op into the extra
                   MPIU_RMA_op_info allocated at the end of the
                   rma_op_infos array. */
                mpi_errno = NMPI_Recv(&rma_op_infos[total_op_count],
                                      sizeof(MPIU_RMA_op_info), MPI_BYTE, 
                                      src, tags[src], comm,
                                      MPI_STATUS_IGNORE);
                if (mpi_errno) {
                    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
                    return mpi_errno;
                }
                tags[src]++;

                if (rma_op_infos[total_op_count].datatype_kind ==
                    MPID_RMA_DATATYPE_DERIVED) {
                    /* recv the derived datatype info and create
                       derived datatype */
                    mpi_errno =
                        NMPI_Recv(&dtype_infos[total_op_count],
                                  sizeof(MPIU_RMA_dtype_info),
                                  MPI_BYTE, src, tags[src], comm,
                                  MPI_STATUS_IGNORE);
                    if (mpi_errno) {
                        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
                        return mpi_errno;
                    }
                    tags[src]++;

                    /* recv dataloop */
                    dataloops[total_op_count] = (void *)
                        MPIU_Malloc(dtype_infos[total_op_count].loopsize);
                    if (!dataloops[total_op_count]) {
                        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
                        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
                        return mpi_errno;
                    }

                    mpi_errno = NMPI_Recv(dataloops[total_op_count],
                                          dtype_infos[total_op_count].loopsize,
                                          MPI_BYTE, src, tags[src], comm,
                                          MPI_STATUS_IGNORE);
                    if (mpi_errno) {
                        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
                        return mpi_errno;
                    }
                    tags[src]++;

                    /* create derived datatype */

                    /* allocate new datatype object and handle */
                    new_dtp = (MPID_Datatype *) MPIU_Handle_obj_alloc(&MPID_Datatype_mem);
                    if (!new_dtp) {
                        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
                        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
                        return mpi_errno;
                    }
                    
                    /* Note: handle is filled in by MPIU_Handle_obj_alloc() */
                    MPIU_Object_set_ref(new_dtp, 1);
                    new_dtp->is_permanent = 0;
                    new_dtp->is_committed = 1;
                    new_dtp->attributes   = 0;
                    new_dtp->cache_id     = 0;
                    new_dtp->name[0]      = 0;
                    new_dtp->is_contig = dtype_infos[total_op_count].is_contig;
                    new_dtp->n_contig_blocks =
                        dtype_infos[total_op_count].n_contig_blocks; 
                    new_dtp->size = dtype_infos[total_op_count].size;
                    new_dtp->extent = dtype_infos[total_op_count].extent;
                    new_dtp->loopsize = dtype_infos[total_op_count].loopsize;
                    new_dtp->loopinfo_depth =
                        dtype_infos[total_op_count].loopinfo_depth; 
                    new_dtp->eltype = dtype_infos[total_op_count].eltype;
                    /* set dataloop pointer */
                    new_dtp->loopinfo = dataloops[total_op_count];
                    /* set datatype handle to be used in send/recv
                       below */
                    rma_op_infos[total_op_count].datatype = new_dtp->handle;

                    new_dtp->ub = dtype_infos[total_op_count].ub;
                    new_dtp->lb = dtype_infos[total_op_count].lb;
                    new_dtp->true_ub = dtype_infos[total_op_count].true_ub;
                    new_dtp->true_lb = dtype_infos[total_op_count].true_lb;
                    new_dtp->has_sticky_ub = dtype_infos[total_op_count].has_sticky_ub;
                    new_dtp->has_sticky_lb = dtype_infos[total_op_count].has_sticky_lb;
                    /* update pointers in dataloop */
                    ptrdiff = (MPI_Aint)((char *) (new_dtp->loopinfo) - (char *)
                        (dtype_infos[total_op_count].loopinfo));

                    MPID_Dataloop_update(new_dtp->loopinfo, ptrdiff);
                }

                switch (rma_op_infos[total_op_count].type) {
                case MPID_REQUEST_PUT:
                    /* recv the put */
                    mpi_errno = NMPI_Recv((char *) win_ptr->base +
                                          win_ptr->disp_unit *
                                          rma_op_infos[total_op_count].disp,
                                          rma_op_infos[total_op_count].count,
                                          rma_op_infos[total_op_count].datatype,
                                          src, tags[src], comm,
                                          MPI_STATUS_IGNORE);
                    if (mpi_errno) {
                        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
                        return mpi_errno;
                    }
                    break;
                case MPID_REQUEST_GET:
                    /* send the get */
                    mpi_errno = NMPI_Send((char *) win_ptr->base +
                                          win_ptr->disp_unit *
                                          rma_op_infos[total_op_count].disp,
                                          rma_op_infos[total_op_count].count,
                                          rma_op_infos[total_op_count].datatype,
                                          src, tags[src], comm);
                    if (mpi_errno) {
                        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
                        return mpi_errno;
                    }
                    break;
                case MPID_REQUEST_ACCUMULATE:
                    /* recv the data into a temp buffer and perform
                       the reduction operation */
                    mpi_errno =
                        NMPI_Type_get_true_extent(rma_op_infos[total_op_count].datatype, 
                                                  &true_lb, &true_extent);  
                    if (mpi_errno) return mpi_errno;

                    MPID_Datatype_get_extent_macro(rma_op_infos[total_op_count].datatype, 
                                                   extent); 
                    tmp_buf = MPIU_Malloc(rma_op_infos[total_op_count].count * 
                                          (MPIR_MAX(extent,true_extent)));  
                    if (!tmp_buf) {
                        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
                        return mpi_errno;
                    }
                    /* adjust for potential negative lower bound in datatype */
                    tmp_buf = (void *)((char*)tmp_buf - true_lb);
                    
                    mpi_errno = NMPI_Recv(tmp_buf,
                                          rma_op_infos[total_op_count].count,
                                          rma_op_infos[total_op_count].datatype,
                                          src, tags[src], comm,
                                          MPI_STATUS_IGNORE);
                    if (mpi_errno) {
                        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
                        return mpi_errno;
                    }

                    op = rma_op_infos[total_op_count].op;
                    if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
                        /* get the function by indexing into the op table */
                        uop = MPIR_Op_table[op%16 - 1];
                    }
                    else {
                        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OP,
                                                          "**opnotpredefined", "**opnotpredefined %d", op );
                        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
                        return mpi_errno;
                    }

                    win_buf_addr = (char *) win_ptr->base +
                        win_ptr->disp_unit * rma_op_infos[total_op_count].disp;

                    if (rma_op_infos[total_op_count].datatype_kind ==
                        MPID_RMA_DATATYPE_BASIC) {
                        (*uop)(tmp_buf, win_buf_addr,
                               &(rma_op_infos[total_op_count].count),
                               &(rma_op_infos[total_op_count].datatype));
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
                                          rma_op_infos[total_op_count].count, 
                                 rma_op_infos[total_op_count].datatype, segp);
                        first = 0;
                        last  = SEGMENT_IGNORE_LAST;

                        vec_len = new_dtp->n_contig_blocks *
                            rma_op_infos[total_op_count].count + 1; 
                        /* +1 needed because Rob says  so */
                        dloop_vec = (DLOOP_VECTOR *)
                            MPIU_Malloc(vec_len * sizeof(DLOOP_VECTOR));
                        if (!dloop_vec) {
                            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, 
                                MPI_ERR_OTHER, "**nomem", 0 ); 
                            MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
                            return mpi_errno;
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
                                                      "**opundefined_rma","**opundefined_rma %d",
						      rma_op_infos[total_op_count].type );
                    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
                    return mpi_errno;
                }
                tags[src]++;

                if (rma_op_infos[total_op_count].datatype_kind ==
                    MPID_RMA_DATATYPE_DERIVED) {
                    MPIU_Handle_obj_free(&MPID_Datatype_mem, new_dtp);
                    MPIU_Free(dataloops[total_op_count]);
                }
            }
        }

        mpi_errno = NMPI_Waitall(req_cnt, reqs, MPI_STATUSES_IGNORE);
        if (mpi_errno) {
            MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
            return mpi_errno;
        }

	MPIR_Nest_decr();

        MPIU_Free(tags);
        MPIU_Free(reqs);
        MPIU_Free(rma_op_infos);
        MPIU_Free(nops_to_proc);
        MPIU_Free(nops_from_proc);
        MPIU_Free(dtype_infos);
        for (i=0; i<total_op_count; i++)
            if (dataloops[i] != NULL) 
                MPIU_Free(dataloops[i]);
        MPIU_Free(dataloops);

        /* free MPIU_RMA_ops_list */
        curr_ptr = MPIU_RMA_ops_list;
        while (curr_ptr != NULL) {
            next_ptr = curr_ptr->next;
            MPIU_Free(curr_ptr);
            curr_ptr = next_ptr;
        }
        MPIU_RMA_ops_list = NULL;

        if (assert & MPI_MODE_NOSUCCEED) 
            win_ptr->fence_cnt = 0;
    }

    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);

    return mpi_errno;
}
#endif
