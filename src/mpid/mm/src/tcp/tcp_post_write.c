/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "tcpimpl.h"

int tcp_post_write(MPIDI_VC *vc_ptr, MM_Car *car_ptr)
{
    MM_ENTER_FUNC(TCP_POST_WRITE);
    tcp_car_enqueue(vc_ptr, car_ptr);
    MM_EXIT_FUNC(TCP_POST_WRITE);
    return MPI_SUCCESS;
}
