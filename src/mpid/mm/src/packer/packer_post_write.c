/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpidimpl.h"

int packer_post_write(MPIDI_VC *vc_ptr, MM_Car *car_ptr)
{
    packer_car_enqueue(vc_ptr, car_ptr);
    return MPI_SUCCESS;
}
