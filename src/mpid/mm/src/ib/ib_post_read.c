/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "ibimpl.h"

#ifdef WITH_METHOD_IB

int ib_post_read(MPIDI_VC *vc_ptr, MM_Car *car_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_POST_READ);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_POST_READ);

    MPIU_dbg_printf("ib_post_read\n");
    ib_car_enqueue_read(vc_ptr, car_ptr);

    MPIDI_FUNC_EXIT(MPID_STATE_IB_POST_READ);
    return MPI_SUCCESS;
}

int ib_post_read_pkt(MPIDI_VC *vc_ptr)
{
    int result;
    MPIDI_STATE_DECL(MPID_STATE_IB_POST_READ_PKT);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_POST_READ_PKT);

    vc_ptr->data.ib.reading_header = TRUE;
    result = ibu_post_receive(vc_ptr);
    if (result != MPI_SUCCESS)
    {
	err_printf("ib_post_read_pkt: unable to post a read packet.\n");
    }

    MPIDI_FUNC_EXIT(MPID_STATE_IB_POST_READ_PKT);
    return MPI_SUCCESS;
}

int ib_handle_read(MPIDI_VC *vc_ptr, void *mem_ptr, int num_bytes)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_HANDLE_READ);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_HANDLE_READ);

    if (vc_ptr == NULL)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_IB_HANDLE_READ);
	return MPI_SUCCESS;
    }

    if (vc_ptr->data.ib.reading_header)
    {
	MPIU_dbg_printf("ib_handle_read() received header - %d bytes\n", num_bytes);
	memcpy(&vc_ptr->pkt_car.msg_header.pkt.u.hdr, mem_ptr, num_bytes);
	BlockFree(vc_ptr->data.ib.info.m_allocator, mem_ptr);
	mm_cq_enqueue(&vc_ptr->pkt_car);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_READ);
	return MPI_SUCCESS;
    }

    MPIU_dbg_printf("ib_handle_read() received data - %d bytes\n", num_bytes);
    ib_handle_read_data(vc_ptr, mem_ptr, num_bytes);
    BlockFree(vc_ptr->data.ib.info.m_allocator, mem_ptr);

    MPIDI_FUNC_EXIT(MPID_STATE_IB_HANDLE_READ);
    return -1;
}

#endif
