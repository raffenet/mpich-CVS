/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "tcpimpl.h"

int tcp_post_read(MPIDI_VC *vc_ptr, MM_Car *car_ptr)
{
    MM_ENTER_FUNC(TCP_POST_READ);
    tcp_car_enqueue(vc_ptr, car_ptr);
    MM_EXIT_FUNC(TCP_POST_READ);
    return MPI_SUCCESS;
}

int tcp_post_read_pkt(MPIDI_VC *vc_ptr)
{
    MM_ENTER_FUNC(TCP_POST_READ_PKT);

    tcp_setup_packet_car(vc_ptr, MM_READ_CAR, vc_ptr->rank, &vc_ptr->pkt_car);
    tcp_post_read(vc_ptr, &vc_ptr->pkt_car);

    MM_EXIT_FUNC(TCP_POST_READ_PKT);
    return MPI_SUCCESS;
}
