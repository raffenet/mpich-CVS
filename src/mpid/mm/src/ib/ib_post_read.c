/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "ibimpl.h"

int ib_post_read(MPIDI_VC *vc_ptr, MM_Car *car_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_POST_READ);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_POST_READ);

    MPIDI_FUNC_EXIT(MPID_STATE_IB_POST_READ);
    return MPI_SUCCESS;
}

int ib_post_read_pkt(MPIDI_VC *vc_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_POST_READ_PKT);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_POST_READ_PKT);

    MPIDI_FUNC_EXIT(MPID_STATE_IB_POST_READ_PKT);
    return MPI_SUCCESS;
}

int ib_handle_read_ack(MPIDI_VC *vc_ptr, int num_read)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_HANDLE_READ_ACK);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_HANDLE_READ_ACK);

    MPIDI_FUNC_EXIT(MPID_STATE_IB_HANDLE_READ_ACK);
    return MPI_SUCCESS;
}

int ib_handle_read_connect_pkt(MPIDI_VC *temp_vc_ptr, int num_read)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_HANDLE_READ_CONNECT_PKT);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_HANDLE_READ_CONNECT_PKT);
    MPIDI_FUNC_EXIT(MPID_STATE_IB_HANDLE_READ_CONNECT_PKT);
    return MPI_SUCCESS;
}

int ib_handle_read(MPIDI_VC *vc_ptr, int num_bytes)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_HANDLE_READ);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_HANDLE_READ);

    MPIDI_FUNC_EXIT(MPID_STATE_IB_HANDLE_READ);
    return -1;
}
