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

int tcp_merge_with_unexpected(MM_Car *car_ptr, MM_Car *unex_car_ptr)
{
    MM_ENTER_FUNC(TCP_MERGE_WITH_UNEXPECTED);

    /* copy the unexpected packet into the posted packet */
    car_ptr->msg_header.pkt = unex_car_ptr->msg_header.pkt;

    if (unex_car_ptr->next_ptr)
    {
	/* reading of unexpected data in progress */

	err_printf("Help the ship is sinking.\n");

	/* copy/unpack the read data into the posted cars */
	/* mark the completed cars complete */
	/* post reads for the partially filled or incomplete cars */
    }
    else
    {
	/* unex header matched a previously posted recv */

	/* mark the head car as completed */
	mm_dec_cc(car_ptr->request_ptr);
	car_ptr = car_ptr->next_ptr;
	
	/* read the data eagerly for now */
	/*
	while (car_ptr)
	{
	    tcp_post_read(car_ptr->vc_ptr, car_ptr);
	    car_ptr = car_ptr->next_ptr;
	}
	*/
	if (car_ptr)
	    tcp_car_head_enqueue(car_ptr->vc_ptr, car_ptr);
    }

    MM_EXIT_FUNC(TCP_MERGE_WITH_UNEXPECTED);
    return MPI_SUCCESS;
}

int tcp_post_read_pkt(MPIDI_VC *vc_ptr)
{
    /*
    MM_Car *car_ptr;
    MM_Segment_buffer *buf_ptr;
    */

    MM_ENTER_FUNC(TCP_POST_READ_PKT);

    tcp_setup_packet_car(vc_ptr, MM_READ_CAR, vc_ptr->rank, &vc_ptr->pkt_car);
    tcp_post_read(vc_ptr, &vc_ptr->pkt_car);

    /*
    car_ptr = &vc_ptr->pkt_car;
    buf_ptr = &vc_ptr->pkt_car.msg_header.buf;
    
    car_ptr->type = MM_HEAD_CAR | MM_READ_CAR;
    car_ptr->src = vc_ptr->rank;
    car_ptr->dest = -1;
    car_ptr->vc_ptr = vc_ptr;
    car_ptr->next_ptr = NULL;
    car_ptr->opnext_ptr = NULL;
    car_ptr->qnext_ptr = NULL;
    car_ptr->request_ptr = NULL;
    car_ptr->buf_ptr = buf_ptr;
    
    car_ptr->data.tcp.buf.vec_read.cur_index = 0;
    car_ptr->data.tcp.buf.vec_read.cur_num_read = 0;
    car_ptr->data.tcp.buf.vec_read.num_read_at_cur_index = 0;
    car_ptr->data.tcp.buf.vec_read.total_num_read = 0;
    car_ptr->data.tcp.buf.vec_read.vec[0].MPID_VECTOR_BUF = (void*)&car_ptr->msg_header.pkt;
    car_ptr->data.tcp.buf.vec_read.vec[0].MPID_VECTOR_LEN = sizeof(MPID_Packet);
    car_ptr->data.tcp.buf.vec_read.vec_size = 1;

    buf_ptr->type = MM_VEC_BUFFER;
    buf_ptr->vec.vec[0].MPID_VECTOR_BUF = (void*)&car_ptr->msg_header.pkt;
    buf_ptr->vec.vec[0].MPID_VECTOR_LEN = sizeof(MPID_Packet);
    buf_ptr->vec.vec_size = 1;
    buf_ptr->vec.num_read = 0;
    buf_ptr->vec.first = 0;
    buf_ptr->vec.last = sizeof(MPID_Packet);
    buf_ptr->vec.segment_last = sizeof(MPID_Packet);
    buf_ptr->vec.buf_size = sizeof(MPID_Packet);
    buf_ptr->vec.num_cars = 1;
    buf_ptr->vec.num_cars_outstanding = 1;

    tcp_post_read(vc_ptr, car_ptr);
    */

    MM_EXIT_FUNC(TCP_POST_READ_PKT);
    return MPI_SUCCESS;
}
