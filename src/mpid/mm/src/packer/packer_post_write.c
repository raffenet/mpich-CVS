/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpidimpl.h"

int packer_post_write(MPIDI_VC *vc_ptr, MM_Car *car_ptr)
{
    MPID_STATE_DECL(MPID_STATE_PACKER_POST_WRITE);
    MPID_FUNC_ENTER(MPID_STATE_PACKER_POST_WRITE);

    packer_car_enqueue(vc_ptr, car_ptr);

    MPID_FUNC_EXIT(MPID_STATE_PACKER_POST_WRITE);
    return MPI_SUCCESS;
}
