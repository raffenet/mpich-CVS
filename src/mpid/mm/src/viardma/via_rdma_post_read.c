/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "via_rdmaimpl.h"

int via_rdma_post_read(MPIDI_VC *vc_ptr, MM_Car *car_ptr)
{
    return MPI_SUCCESS;
}

int via_rdma_merge_post_read(MM_Car *car_ptr, MM_Car *unex_car_ptr)
{
    return MPI_SUCCESS;
}
