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
	if ((iter_ptr->msg_header.pkt.context == car_ptr->msg_header.pkt.context) &&
	    (iter_ptr->msg_header.pkt.tag == car_ptr->msg_header.pkt.tag) &&
	    (iter_ptr->src == car_ptr->src))
	{
	    if (iter_ptr->msg_header.pkt.size > car_ptr->msg_header.pkt.size)
	    {
		err_printf("Error: unex msg size %d > posted msg size %d\n", iter_ptr->msg_header.pkt.size, car_ptr->msg_header.pkt.size);
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

    switch (car_ptr->msg_header.pkt.type)
    {
    case MPID_EAGER_PKT:
    case MPID_RNDV_REQUEST_TO_SEND_PKT:
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
	    if (car_ptr->msg_header.pkt.type == MPID_RNDV_REQUEST_TO_SEND_PKT)
		mm_post_unex_rndv(car_ptr);
	    else
		mm_create_post_unex(car_ptr);
	}
	MPID_Thread_unlock(MPID_Process.qlock);
	break;
    case MPID_RNDV_OK_TO_SEND_PKT:
	break;
    case MPID_RNDV_DATA_PKT:
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
	car_ptr->vc_ptr->post_read(car_ptr->vc_ptr, car_ptr->next_ptr);
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
    /* for now, all writes are eager - no rndv */
    /*
    if (car_ptr->next_ptr)
    {
	car_ptr->vc_ptr->post_write(car_ptr->vc_ptr, car_ptr->next_ptr);
    }
    */
    mm_dec_cc(car_ptr->request_ptr);
    mm_car_free(car_ptr);
    return MPI_SUCCESS;
}

int cq_handle_write_data_car(MM_Car *car_ptr)
{
    /* for now, all writes are eager - no rndv */
    /*
    if (car_ptr->next_ptr)
    {
	car_ptr->vc_ptr->post_write(car_ptr->vc_ptr, car_ptr->next_ptr);
    }
    */
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
	return MPI_SUCCESS;

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

    return MPI_SUCCESS;
}

int mm_post_unex_rndv(MM_Car *unex_head_car_ptr)
{
    MM_Car *car_ptr;

    dbg_printf("mm_post_unex_rndv\n");

    car_ptr = mm_car_alloc();

    car_ptr->msg_header = unex_head_car_ptr->msg_header;
    car_ptr->qnext_ptr = NULL;

    /* enqueue the car in the unexpected queue */
    if (MPID_Process.unex_q_tail == NULL)
	MPID_Process.unex_q_head = car_ptr;
    else
	MPID_Process.unex_q_tail->qnext_ptr = car_ptr;
    MPID_Process.unex_q_tail = car_ptr;

    return MPI_SUCCESS;
}

int mm_create_post_unex(MM_Car *unex_head_car_ptr)
{
    MPID_Request *request_ptr;
    MM_Car *car_ptr;
    MM_Segment_buffer *buf_ptr;

    err_printf("mm_creat_post_unex not implemented yet\n");

    request_ptr = mm_request_alloc();
    if (request_ptr == NULL)
	return -1;

    /* save the packet header */
    car_ptr = &request_ptr->mm.rcar[0];
    car_ptr->msg_header.pkt = unex_head_car_ptr->msg_header.pkt;

    car_ptr->type = MM_HEAD_CAR | MM_READ_CAR;
    car_ptr->src = unex_head_car_ptr->msg_header.pkt.src;
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
    car_ptr->request_ptr->mm.size = unex_head_car_ptr->msg_header.pkt.size;
    mm_inc_cc(request_ptr);

    buf_ptr = car_ptr->buf_ptr = &request_ptr->mm.buf;
    buf_ptr->type = MM_TMP_BUFFER;
    buf_ptr->tmp.buf[0] = MPIU_Malloc(unex_head_car_ptr->msg_header.pkt.size);
    buf_ptr->tmp.len[0] = unex_head_car_ptr->msg_header.pkt.size;
    buf_ptr->tmp.buf[1] = NULL;
    buf_ptr->tmp.len[1] = 0;
    buf_ptr->tmp.cur_buf = 0;
    buf_ptr->tmp.min_num_written = 0;
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
    car_ptr->vc_ptr->post_read(car_ptr->vc_ptr, car_ptr);

    return MPI_SUCCESS;
}
