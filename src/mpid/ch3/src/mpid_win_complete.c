/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int MPID_Win_complete(MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS, comm_size, *nops_to_proc, dest;
    int i, *tags, total_op_count, req_cnt;
    MPIU_RMA_ops *curr_ptr, *next_ptr;
    MPI_Comm comm;
    typedef struct MPIU_RMA_op_info {
        int type;
        MPI_Aint disp;
        int count;
        int datatype;
        MPI_Op op;
    } MPIU_RMA_op_info;
    MPIU_RMA_op_info *rma_op_infos;
    MPI_Request *reqs;
    MPI_Group win_grp, start_grp;
    int start_grp_size, *ranks_in_start_grp, *ranks_in_win_grp;

    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_COMPLETE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_WIN_COMPLETE);

    MPIR_Nest_incr();

    comm = win_ptr->comm;
    NMPI_Comm_size(comm, &comm_size);

    /* First inform every process in start_group how many
       RMA ops from this process is it the target for. */

    nops_to_proc = (int *) MPIU_Calloc(comm_size, sizeof(int));
    if (!nops_to_proc) {
        mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
        return mpi_errno;
    }
    
    total_op_count = 0;
    curr_ptr = MPIU_RMA_ops_list;
    while (curr_ptr != NULL) {
        nops_to_proc[curr_ptr->target_rank]++;
        total_op_count++;
        curr_ptr = curr_ptr->next;
    }

    /* We need to translate the ranks of the processes in
       start_group to ranks in win_ptr->comm, so that we
       can do communication */

    NMPI_Comm_group(win_ptr->comm, &win_grp);
    start_grp_size = win_ptr->start_group_ptr->size;

    ranks_in_start_grp = (int *) MPIU_Malloc(start_grp_size * sizeof(int));
    if (!ranks_in_start_grp) {
        mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
        return mpi_errno;
    }
    ranks_in_win_grp = (int *) MPIU_Malloc(start_grp_size * sizeof(int));
    if (!ranks_in_win_grp) {
        mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
        return mpi_errno;
    }

    for (i=0; i<start_grp_size; i++)
        ranks_in_start_grp[i] = i;

    start_grp = win_ptr->start_group_ptr->handle;
    NMPI_Group_translate_ranks(start_grp, start_grp_size,
                               ranks_in_start_grp, win_grp, ranks_in_win_grp);

    reqs = (MPI_Request *)
        MPIU_Malloc((start_grp_size+2*total_op_count)*sizeof(MPI_Request));
    if (!reqs) {
        mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
        return mpi_errno;
    }
    
    tags = (int *) MPIU_Calloc(comm_size, sizeof(int)); 
    if (!tags) {
        mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
        return mpi_errno;
    }

    for (i=0; i<start_grp_size; i++) {
        dest = ranks_in_win_grp[i];
        mpi_errno = NMPI_Isend(&nops_to_proc[dest], 1, MPI_INT, dest,
                               tags[dest], comm, &reqs[i]);
        if (mpi_errno) return mpi_errno;
        tags[dest]++;
    }

    MPIU_Free(ranks_in_win_grp);
    MPIU_Free(ranks_in_start_grp);

    /* For each RMA op, first send the type (put or get), target
       displ, count, datatype. Then issue an isend for a 
       put or irecv for a get. */
    
    rma_op_infos = (MPIU_RMA_op_info *) 
        MPIU_Malloc(total_op_count * sizeof(MPIU_RMA_op_info));
    if (!rma_op_infos) {
        mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
        return mpi_errno;
    }

    i = 0;
    req_cnt = start_grp_size;
    curr_ptr = MPIU_RMA_ops_list;
    while (curr_ptr != NULL) {
        rma_op_infos[i].type = curr_ptr->type;
        rma_op_infos[i].disp = curr_ptr->target_disp;
        rma_op_infos[i].count = curr_ptr->target_count;
        rma_op_infos[i].datatype = curr_ptr->target_datatype;
        rma_op_infos[i].op = curr_ptr->op;
        
        dest = curr_ptr->target_rank;
        
        /* NEED TO CONVERT THE FOLLOWING TO USE STRUCT DATATYPE */
        mpi_errno = NMPI_Isend(&rma_op_infos[i],
                               sizeof(MPIU_RMA_op_info), MPI_BYTE, 
                               dest, tags[dest], comm,
                               &reqs[req_cnt]); 
        if (mpi_errno) return mpi_errno;
        req_cnt++;
        tags[dest]++;
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
        if (mpi_errno) return mpi_errno;
        req_cnt++;
        tags[dest]++;
        
        curr_ptr = curr_ptr->next;
        i++;
    }        

    mpi_errno = NMPI_Waitall(req_cnt, reqs, MPI_STATUSES_IGNORE);
    if (mpi_errno) return mpi_errno;

    MPIR_Nest_decr();

    MPIU_Free(tags);
    MPIU_Free(reqs);
    MPIU_Free(rma_op_infos);
    MPIU_Free(nops_to_proc);
    NMPI_Group_free(&win_grp);

    /* free MPIU_RMA_ops_list */
    curr_ptr = MPIU_RMA_ops_list;
    while (curr_ptr != NULL) {
        next_ptr = curr_ptr->next;
        MPIU_Free(curr_ptr);
        curr_ptr = next_ptr;
    }
    MPIU_RMA_ops_list = NULL;
    
    MPIU_Object_release_ref(win_ptr->start_group_ptr,&i);
    if (!i) {
        /* Only if refcount is 0 do we actually free. */
        MPIU_Free( win_ptr->start_group_ptr->lrank_to_lpid );
        MPIU_Handle_obj_free( &MPID_Group_mem, win_ptr->start_group_ptr );
    }

    win_ptr->start_group_ptr = NULL; 

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_WIN_COMPLETE);

    return mpi_errno;
}
