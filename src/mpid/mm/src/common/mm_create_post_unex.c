/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int mm_create_post_unex(MM_Car *unex_head_car_ptr)
{
    MPID_Request *request_ptr;
    MM_Car *car_ptr;
    MM_Segment_buffer *buf_ptr;
    MPID_Header_pkt *hdr_ptr;

    MM_ENTER_FUNC(MM_CREATE_POST_UNEX);

    hdr_ptr = &unex_head_car_ptr->msg_header.pkt.u.hdr;

    /* XFER_INIT */
    request_ptr = mm_request_alloc();
    if (request_ptr == NULL)
    {
	err_printf("mm_create_post_unex failed to allocate a request.\n");
	MM_EXIT_FUNC(MM_CREATE_POST_UNEX);
	return -1;
    }

    request_ptr->comm = MPIR_Process.comm_world; /* ??? We don't know the comm yet. */
    request_ptr->mm.tag = hdr_ptr->tag;
    request_ptr->mm.op_valid = TRUE;
    request_ptr->cc = 0;
    request_ptr->cc_ptr = &request_ptr->cc;
    /* END XFER_INIT */

    /* XFER_RECV_OP */
    request_ptr->mm.next_ptr = NULL;
    request_ptr->mm.user_buf.recv = NULL;
    request_ptr->mm.count = hdr_ptr->size;
    request_ptr->mm.dtype = MPI_BYTE;
    request_ptr->mm.first = 0;
    request_ptr->mm.size = hdr_ptr->size;
    request_ptr->mm.last = hdr_ptr->size;

    /* save the packet header */
    car_ptr = &request_ptr->mm.rcar[0];
    car_ptr->msg_header.pkt = unex_head_car_ptr->msg_header.pkt;

    /* set up the read car */
    car_ptr->type = MM_HEAD_CAR | MM_READ_CAR;
    car_ptr->src = hdr_ptr->src;
    car_ptr->request_ptr = request_ptr;
    car_ptr->vc_ptr = unex_head_car_ptr->vc_ptr;
    car_ptr->buf_ptr = &car_ptr->msg_header.buf;
    car_ptr->opnext_ptr = &request_ptr->mm.rcar[1];
    car_ptr->qnext_ptr = NULL;
    /*mm_inc_cc(request_ptr);*/ /* the head car has already been received */

    /* use rcar[1] as the data car */
    car_ptr->next_ptr = &request_ptr->mm.rcar[1];
    car_ptr = car_ptr->next_ptr;
    
    /* allocate a temporary buffer to hold the unexpected data */
    car_ptr->type = MM_READ_CAR;
    car_ptr->src = /*unex_head_car_ptr->src;*/ hdr_ptr->src;
    car_ptr->vc_ptr = unex_head_car_ptr->vc_ptr;
    car_ptr->request_ptr = request_ptr;
    car_ptr->freeme = FALSE;
    car_ptr->next_ptr = NULL;
    car_ptr->opnext_ptr = NULL;
    car_ptr->qnext_ptr = NULL;
    /*printf("inc cc: read unex data\n");fflush(stdout);*/
    mm_inc_cc(request_ptr);

    buf_ptr = car_ptr->buf_ptr = &request_ptr->mm.buf;
    buf_ptr->type = MM_TMP_BUFFER;
    buf_ptr->tmp.buf = MPIU_Malloc(unex_head_car_ptr->msg_header.pkt.u.hdr.size);
    buf_ptr->tmp.len = unex_head_car_ptr->msg_header.pkt.u.hdr.size;
    buf_ptr->tmp.num_read = 0;
    /* END XFER_RECV_OP */
    
    /* XFER_START */
    /* enqueue the head car in the unexpected queue so it can be matched */
    if (MPID_Process.unex_q_tail == NULL)
    {
	MPID_Process.unex_q_head = &request_ptr->mm.rcar[0];
    }
    else
    {
	MPID_Process.unex_q_tail->qnext_ptr = &request_ptr->mm.rcar[0];
    }
    MPID_Process.unex_q_tail = &request_ptr->mm.rcar[0];
    
    if (hdr_ptr->type == MPID_EAGER_PKT)
    {
	/* post a read of the unexpected data */
	car_ptr->vc_ptr->enqueue_read_at_head(car_ptr->vc_ptr, car_ptr);
    }
    /* END XFER_START */

    MM_EXIT_FUNC(MM_CREATE_POST_UNEX);
    return MPI_SUCCESS;
}
