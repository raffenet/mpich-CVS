/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "viaimpl.h"

int via_post_read(MPIDI_VC *vc_ptr, MM_Car *car_ptr)
{
    return MPI_SUCCESS;
}

int via_merge_unexpected(MM_Car *car_ptr, MM_Car *unex_car_ptr)
{
    return MPI_SUCCESS;
}
