/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "shmimpl.h"

int shm_post_read(MPIDI_VC *vc_ptr, MM_Car *car_ptr)
{
    MPID_STATE_DECLS;
    MPID_FUNC_ENTER(MPID_STATE_SHM_POST_READ);
    MPID_FUNC_EXIT(MPID_STATE_SHM_POST_READ);
    return MPI_SUCCESS;
}

int shm_merge_with_unexpected(MM_Car *car_ptr, MM_Car *unex_car_ptr)
{
    MPID_STATE_DECLS;
    MPID_FUNC_ENTER(MPID_STATE_SHM_MERGE_WITH_UNEXPECTED);
    MPID_FUNC_EXIT(MPID_STATE_SHM_MERGE_WITH_UNEXPECTED);
    return MPI_SUCCESS;
}
