/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "tcpimpl.h"

int tcp_post_read(MPIDI_VC *vc_ptr, MM_Car *car_ptr)
{
    tcp_car_enqueue(vc_ptr, car_ptr);
    return MPI_SUCCESS;
}

int tcp_merge_with_unexpected(MM_Car *car_ptr, MM_Car *unex_car_ptr)
{
    return MPI_SUCCESS;
}
