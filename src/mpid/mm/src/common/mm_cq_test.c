/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

MM_Car *find_in_queue(MM_Car **find_q_head_ptr, MM_Car **find_q_tail_ptr, MM_Car *car_ptr)
{
    MM_Car *iter_ptr, *trailer_ptr;

    trailer_ptr = iter_ptr = *find_q_head_ptr;
    while (iter_ptr)
    {
	if ((iter_ptr->msg_header.pkt.u.hdr.context == car_ptr->msg_header.pkt.u.hdr.context) &&
	    (iter_ptr->msg_header.pkt.u.hdr.tag == car_ptr->msg_header.pkt.u.hdr.tag) &&
	    (iter_ptr->src == car_ptr->src))
	{
	    if (iter_ptr->msg_header.pkt.u.hdr.size > car_ptr->msg_header.pkt.u.hdr.size)
	    {
		err_printf("Error: unex msg size %d > posted msg size %d\n", iter_ptr->msg_header.pkt.u.hdr.size, car_ptr->msg_header.pkt.u.hdr.size);
		return NULL;
	    }
	    /* dequeue the car from the posted_q */
	    if (trailer_ptr == iter_ptr)
	    {
		if ((*find_q_head_ptr = iter_ptr->qnext_ptr) == NULL)
		    *find_q_tail_ptr = NULL;
	    }
	    else
	    {
		trailer_ptr->qnext_ptr = iter_ptr->qnext_ptr;
		if (*find_q_tail_ptr == iter_ptr)
		    *find_q_tail_ptr = trailer_ptr;
	    }
	    return iter_ptr;
	}
	if (trailer_ptr != iter_ptr)
	    trailer_ptr = trailer_ptr->qnext_ptr;
	iter_ptr = iter_ptr->qnext_ptr;
    }
    
    return NULL;
}

int cq_handle_read_head_car(MM_Car *car_ptr)
{
    MM_Car *qcar_ptr;

    switch (car_ptr->msg_header.pkt.u.type)
    {
    case MPID_EAGER_PKT:
	MPID_Thread_lock(MPID_Process.qlock);
	qcar_ptr = find_in_queue(&MPID_Process.posted_q_head, &MPID_Process.posted_q_tail, car_ptr);
	if (qcar_ptr)
	{
	    /* merge the unex car with the posted car using the method in the vc */
	    qcar_ptr->vc_ptr->merge_with_unexpected(qcar_ptr, car_ptr);
	}
	else
	{
	    /* else allocate a temp buffer, place in the unex_q, and post a read */
	    mm_create_post_unex(car_ptr);
	}
	MPID_Thread_unlock(MPID_Process.qlock);
	break;
    case MPID_RNDV_REQUEST_TO_SEND_PKT:
	MPID_Thread_lock(MPID_Process.qlock);
	qcar_ptr = find_in_queue(&MPID_Process.posted_q_head, &MPID_Process.posted_q_tail, car_ptr);
	if (qcar_ptr)
	{
	    /* send cts header packet */
	    mm_post_rndv_clear_to_send(qcar_ptr, car_ptr);
	}
	else
	{
	    /* post car in unex queue */
	    mm_post_unex_rndv(car_ptr);
	}
	MPID_Thread_unlock(MPID_Process.qlock);
	break;
    case MPID_RNDV_CLEAR_TO_SEND_PKT:
	/* post the rndv_data head packet for writing */
	mm_post_rndv_data_send(car_ptr);
	break;
    case MPID_RNDV_DATA_PKT:
	err_printf("Help me, I'm melting.\n");
	break;
    case MPID_RDMA_ACK_PKT:
	break;
    case MPID_RDMA_DATA_ACK_PKT:
	break;
    case MPID_RDMA_REQUEST_DATA_ACK_PKT:
	break;
    default:
	break;
    }

    return MPI_SUCCESS;
}

int cq_handle_read_data_car(MM_Car *car_ptr)
{
    if (car_ptr->next_ptr)
    {
	/* enqueue next car to be read before any other pending cars */
	car_ptr->vc_ptr->enqueue_read_at_head(car_ptr->vc_ptr, car_ptr->next_ptr);
    }
    else
    {
	if (car_ptr->vc_ptr->post_read_pkt)
	    car_ptr->vc_ptr->post_read_pkt(car_ptr->vc_ptr);
    }
    mm_dec_cc(car_ptr->request_ptr);
    mm_car_free(car_ptr);
    return MPI_SUCCESS;
}

int cq_handle_read_car(MM_Car *car_ptr)
{
    if (car_ptr->type & MM_HEAD_CAR)
    {
	return cq_handle_read_head_car(car_ptr);
    }

    return cq_handle_read_data_car(car_ptr);
}

int cq_handle_write_head_car(MM_Car *car_ptr)
{
    /* rndv */
    if (car_ptr->msg_header.pkt.u.hdr.type == MPID_RNDV_REQUEST_TO_SEND_PKT)
	return MPI_SUCCESS;

    /* eager */
    if (car_ptr->next_ptr)
    {
	car_ptr->vc_ptr->enqueue_write_at_head(car_ptr->vc_ptr, car_ptr->next_ptr);
    }
    mm_dec_cc(car_ptr->request_ptr);
    mm_car_free(car_ptr);
    return MPI_SUCCESS;
}

int cq_handle_write_data_car(MM_Car *car_ptr)
{
    if (car_ptr->next_ptr)
    {
	/* enqueue next car to be written before any other pending cars */
	car_ptr->vc_ptr->enqueue_write_at_head(car_ptr->vc_ptr, car_ptr->next_ptr);
    }
    mm_dec_cc(car_ptr->request_ptr);
    mm_car_free(car_ptr);
    return MPI_SUCCESS;
}

int cq_handle_write_car(MM_Car *car_ptr)
{
    if (car_ptr->type & MM_HEAD_CAR)
    {
	return cq_handle_write_head_car(car_ptr);
    }

    return cq_handle_write_data_car(car_ptr);
}

int mm_cq_test()
{
    MM_Car *car_ptr, *next_car_ptr;

    MM_ENTER_FUNC(MM_CQ_TEST);

    dbg_printf(".");

    /* Should we call make_progress on all the methods?
     * before checking the cq?
     * after checking the cq?
     * only if the cq is empty?
     */
    if (MPID_Process.cq_head == NULL)
    {
	packer_make_progress();
#ifdef WITH_METHOD_TCP
	tcp_make_progress();
#endif
#ifdef WITH_METHOD_SHM
	shm_make_progress();
#endif
#ifdef WITH_METHOD_VIA
	via_make_progress();
#endif
#ifdef WITH_METHOD_VIA_RDMA
	via_rdma_make_progress();
#endif
#ifdef WITH_METHOD_NEW
	new_make_progress();
#endif
	unpacker_make_progress();
    }

    if (MPID_Process.cq_head == NULL)
    {
	MM_EXIT_FUNC(MM_CQ_TEST);
	return MPI_SUCCESS;
    }

    MPID_Thread_lock(MPID_Process.cqlock);
    car_ptr = MPID_Process.cq_head;
    MPID_Process.cq_head = NULL;
    MPID_Process.cq_tail = NULL;
    MPID_Thread_unlock(MPID_Process.cqlock);

    while (car_ptr)
    {
	next_car_ptr = car_ptr->qnext_ptr;

	if (car_ptr->type & MM_READ_CAR)
	{
	    cq_handle_read_car(car_ptr);
	}

	if (car_ptr->type & MM_WRITE_CAR)
	{
	    cq_handle_write_car(car_ptr);
	}

	car_ptr = next_car_ptr;
    }

    MM_EXIT_FUNC(MM_CQ_TEST);
    return MPI_SUCCESS;
}

int mm_post_rndv_data_send(MM_Car *rndv_cts_car_ptr)
{
    MM_Car *rndv_car_ptr;
    MPID_Rndv_data_pkt *rndv_data_ptr;
    MM_Car *sender_car_ptr;

    MM_ENTER_FUNC(MM_POST_RNDV_DATA_SEND);

    /* get a rndv car and the sender car */
    rndv_car_ptr = mm_car_alloc();
    sender_car_ptr = rndv_cts_car_ptr->msg_header.pkt.u.cts.sender_car_ptr;

    /* initialize the rndv car for sending a rndv data header packet */
    sender_car_ptr->vc_ptr->setup_packet_car(
	sender_car_ptr->vc_ptr, 
	MM_WRITE_CAR,
	sender_car_ptr->src, /* this could be an error because src could be MPI_ANY_SRC */
	rndv_car_ptr);
    
    /* set up the rndv data header packet */
    rndv_data_ptr = &rndv_car_ptr->msg_header.pkt.u.rdata;
    rndv_data_ptr->receiver_car_ptr = rndv_cts_car_ptr->msg_header.pkt.u.cts.receiver_car_ptr;
    rndv_data_ptr->size = 0; /* is this necessary? */
    rndv_data_ptr->type = MPID_RNDV_DATA_PKT;

    rndv_car_ptr->next_ptr = sender_car_ptr->next_ptr;

    sender_car_ptr->vc_ptr->post_write(sender_car_ptr->vc_ptr, rndv_car_ptr);

    MM_EXIT_FUNC(MM_POST_RNDV_DATA_SEND);
    return MPI_SUCCESS;
}

int mm_post_rndv_clear_to_send(MM_Car *posted_car_ptr, MM_Car *rndv_rts_car_ptr)
{
    MM_Car *rndv_car_ptr;
    MPID_Rndv_clear_to_send_pkt *rndv_cts_ptr;

    MM_ENTER_FUNC(MM_POST_RNDV_CLEAR_TO_SEND);

    rndv_car_ptr = mm_car_alloc();
    
    tcp_setup_packet_car(posted_car_ptr->vc_ptr, MM_WRITE_CAR, 
	posted_car_ptr->src, /* this could be an error because src could be MPI_ANY_SRC */
	rndv_car_ptr);
    
    /* set up the cts header packet */
    rndv_cts_ptr = &rndv_car_ptr->msg_header.pkt.u.cts;
    rndv_cts_ptr->receiver_car_ptr = posted_car_ptr;
    rndv_cts_ptr->sender_car_ptr = rndv_rts_car_ptr->msg_header.pkt.u.hdr.sender_car_ptr;
    rndv_cts_ptr->type = MPID_RNDV_CLEAR_TO_SEND_PKT;

    posted_car_ptr->vc_ptr->post_write(posted_car_ptr->vc_ptr, rndv_car_ptr);

    MM_EXIT_FUNC(MM_POST_RNDV_CLEAR_TO_SEND);
    return MPI_SUCCESS;
}

int mm_post_unex_rndv(MM_Car *unex_head_car_ptr)
{
    MM_Car *car_ptr;

    MM_ENTER_FUNC(MM_POST_UNEX_RNDV);
    dbg_printf("mm_post_unex_rndv\n");

    car_ptr = mm_car_alloc();

    car_ptr->msg_header = unex_head_car_ptr->msg_header;
    car_ptr->buf_ptr = &car_ptr->msg_header.buf;
    car_ptr->qnext_ptr = NULL;

    /* enqueue the car in the unexpected queue */
    if (MPID_Process.unex_q_tail == NULL)
	MPID_Process.unex_q_head = car_ptr;
    else
	MPID_Process.unex_q_tail->qnext_ptr = car_ptr;
    MPID_Process.unex_q_tail = car_ptr;

    MM_EXIT_FUNC(MM_POST_UNEX_RNDV);
    return MPI_SUCCESS;
}

int mm_create_post_unex(MM_Car *unex_head_car_ptr)
{
    MPID_Request *request_ptr;
    MM_Car *car_ptr;
    MM_Segment_buffer *buf_ptr;

    MM_ENTER_FUNC(MM_CREATE_POST_UNEX);

    err_printf("mm_creat_post_unex not implemented yet\n");

    request_ptr = mm_request_alloc();
    if (request_ptr == NULL)
    {
	MM_EXIT_FUNC(MM_CREATE_POST_UNEX);
	return -1;
    }

    /* save the packet header */
    car_ptr = &request_ptr->mm.rcar[0];
    car_ptr->msg_header.pkt = unex_head_car_ptr->msg_header.pkt;

    car_ptr->type = MM_HEAD_CAR | MM_READ_CAR;
    car_ptr->src = unex_head_car_ptr->msg_header.pkt.u.hdr.src;
    car_ptr->request_ptr = request_ptr;
    car_ptr->vc_ptr = unex_head_car_ptr->vc_ptr;
    car_ptr->buf_ptr = &car_ptr->msg_header.buf;
    car_ptr->opnext_ptr = &request_ptr->mm.rcar[1];
    car_ptr->next_ptr = &request_ptr->mm.rcar[1];
    car_ptr->qnext_ptr = NULL;

    /* use rcar[1] as the data car */
    car_ptr->next_ptr = &request_ptr->mm.rcar[1];
    car_ptr = car_ptr->next_ptr;
    
    /* allocate a temporary buffer to hold the unexpected data */
    car_ptr->type = MM_READ_CAR;
    car_ptr->src = unex_head_car_ptr->src;
    car_ptr->vc_ptr = unex_head_car_ptr->vc_ptr;
    car_ptr->request_ptr = request_ptr;
    car_ptr->freeme = FALSE;
    car_ptr->next_ptr = NULL;
    car_ptr->opnext_ptr = NULL;
    car_ptr->qnext_ptr = NULL;
    car_ptr->request_ptr->mm.size = unex_head_car_ptr->msg_header.pkt.u.hdr.size;
    mm_inc_cc(request_ptr);

    buf_ptr = car_ptr->buf_ptr = &request_ptr->mm.buf;
    buf_ptr->type = MM_TMP_BUFFER;
    buf_ptr->tmp.buf = MPIU_Malloc(unex_head_car_ptr->msg_header.pkt.u.hdr.size);
    buf_ptr->tmp.len = unex_head_car_ptr->msg_header.pkt.u.hdr.size;
    buf_ptr->tmp.num_read = 0;
    
    /* enqueue the head car in the unexpected queue */
    if (MPID_Process.unex_q_tail == NULL)
    {
	MPID_Process.unex_q_head = &request_ptr->mm.rcar[0];
    }
    else
    {
	MPID_Process.unex_q_tail->qnext_ptr = &request_ptr->mm.rcar[0];
    }
    MPID_Process.unex_q_tail = &request_ptr->mm.rcar[0];
    
    /* post a read of the unexpected data */
    car_ptr->vc_ptr->enqueue_read_at_head(car_ptr->vc_ptr, car_ptr);

    MM_EXIT_FUNC(MM_CREATE_POST_UNEX);
    return MPI_SUCCESS;
}
