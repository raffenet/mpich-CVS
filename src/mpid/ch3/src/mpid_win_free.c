/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int MPID_Win_free(MPID_Win **win_ptr)
{
    int mpi_errno=MPI_SUCCESS, *err;

    MPIDI_STATE_DECL(MPID_STATE_MPI_WIN_FREE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPI_WIN_FREE);

    MPIR_Nest_incr();

    NMPI_Barrier((*win_ptr)->comm);

    MPIDI_Passive_target_thread_exit_flag = 1;

    pthread_join((*win_ptr)->passive_target_thread_id, (void **) &err);
    mpi_errno = *err;
    MPIU_Free(err);

    NMPI_Comm_free(&((*win_ptr)->comm));

    MPIR_Nest_decr();
 
    /* check if refcount needs to be decremented here as in group_free */
    MPIU_Handle_obj_free( &MPID_Win_mem, *win_ptr );
 
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_FREE);

    return mpi_errno;
}
