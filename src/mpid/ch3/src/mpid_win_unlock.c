/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int MPID_Win_unlock(int dest, MPID_Win *win_ptr)
{
    int mpi_errno=MPI_SUCCESS, nops_to_proc;
    int i, tag, req_cnt;
    MPIU_RMA_ops *curr_ptr, *next_ptr;
    MPI_Comm comm;
    typedef struct MPIU_RMA_op_info {
        int type;
        MPI_Aint disp;
        int count;
        int datatype;
        MPI_Op op;
        int lock_type;
    } MPIU_RMA_op_info;
    MPIU_RMA_op_info *rma_op_infos;
    MPI_Request *reqs;

    MPIDI_STATE_DECL(MPID_STATE_MPI_WIN_UNLOCK);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPI_WIN_UNLOCK);

    MPIR_Nest_incr();

    comm = win_ptr->comm;

    /* First inform target process how many
       RMA ops from this process is it the target for.
       There could be ops destined for other processes in
       MPIU_RMA_ops_list because there could be multiple MPI_Win_locks
       outstanding */

    nops_to_proc = 0;
    curr_ptr = MPIU_RMA_ops_list;
    while (curr_ptr != NULL) {
        if (curr_ptr->target_rank == dest) nops_to_proc++;
        curr_ptr = curr_ptr->next;
    }

    reqs = (MPI_Request *) MPIU_Malloc((2*nops_to_proc+1)*sizeof(MPI_Request));
    if (!reqs) {
        mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_UNLOCK);
        return mpi_errno;
    }
    
    tag = MPIDI_PASSIVE_TARGET_RMA_TAG;
    mpi_errno = NMPI_Isend(&nops_to_proc, 1, MPI_INT, dest,
                           tag, comm, reqs);
    if (mpi_errno) {
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_UNLOCK);
        return mpi_errno;
    }

    /* For each RMA op, first send the type (put or get), target
       displ, count, datatype. Then issue an isend for a 
       put or irecv for a get. */
    
    rma_op_infos = (MPIU_RMA_op_info *) 
        MPIU_Malloc(nops_to_proc * sizeof(MPIU_RMA_op_info));
    if (!rma_op_infos) {
        mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_UNLOCK);
        return mpi_errno;
    }

    i = 0;
    tag = 234;
    req_cnt = 1;
    curr_ptr = MPIU_RMA_ops_list;
    while (curr_ptr != NULL) {
        rma_op_infos[i].type = curr_ptr->type;
        rma_op_infos[i].disp = curr_ptr->target_disp;
        rma_op_infos[i].count = curr_ptr->target_count;
        rma_op_infos[i].datatype = curr_ptr->target_datatype;
        rma_op_infos[i].op = curr_ptr->op;
        rma_op_infos[i].lock_type = curr_ptr->lock_type;
        
        /* NEED TO CONVERT THE FOLLOWING TO USE STRUCT DATATYPE */
        mpi_errno = NMPI_Isend(&rma_op_infos[i],
                               sizeof(MPIU_RMA_op_info), MPI_BYTE, 
                               dest, tag, comm, &reqs[req_cnt]); 
        if (mpi_errno) {
            MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_UNLOCK);
            return mpi_errno;
        }
        req_cnt++;
        tag++;
        if ((curr_ptr->type == MPID_REQUEST_PUT) ||
            (curr_ptr->type == MPID_REQUEST_ACCUMULATE)) {
            mpi_errno = NMPI_Isend(curr_ptr->origin_addr,
                                   curr_ptr->origin_count,
                                   curr_ptr->origin_datatype,
                                   dest, tag, comm,
                                   &reqs[req_cnt]); 
            req_cnt++;
            tag++;
        }
        else if (curr_ptr->type == MPID_REQUEST_GET) {
            mpi_errno = NMPI_Irecv(curr_ptr->origin_addr,
                                   curr_ptr->origin_count,
                                   curr_ptr->origin_datatype,
                                   dest, tag, comm,
                                   &reqs[req_cnt]); 
            req_cnt++;
            tag++;
        }
        if (mpi_errno) {
            MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_UNLOCK);
            return mpi_errno;
        }
        
        curr_ptr = curr_ptr->next;
        i++;
    }        

    mpi_errno = NMPI_Waitall(req_cnt, reqs, MPI_STATUSES_IGNORE);
    if (mpi_errno) {
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_UNLOCK);
        return mpi_errno;
    }

    /* Passive target RMA must be complete at target when unlock
       returns. Therefore we need ack from target that it is done. */
    mpi_errno = NMPI_Recv(&i, 0, MPI_INT, dest,
                          MPIDI_PASSIVE_TARGET_DONE_TAG, comm,
                          MPI_STATUS_IGNORE); 
    if (mpi_errno) {
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_UNLOCK);
        return mpi_errno;
    }

    MPIR_Nest_decr();

    MPIU_Free(reqs);
    MPIU_Free(rma_op_infos);

    /* free MPIU_RMA_ops_list */
    curr_ptr = MPIU_RMA_ops_list;
    while (curr_ptr != NULL) {
        next_ptr = curr_ptr->next;
        MPIU_Free(curr_ptr);
        curr_ptr = next_ptr;
    }
    MPIU_RMA_ops_list = NULL;
    
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_UNLOCK);

    return mpi_errno;
}
