/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "tcpimpl.h"

int tcp_post_write(MPIDI_VC *vc_ptr, MM_Car *car_ptr)
{
    /*MM_Car *iter_ptr;*/
    MM_Car *rndv_car_ptr;
    MPID_Header_pkt *rndv_rts_ptr;

    MM_ENTER_FUNC(TCP_POST_WRITE);

#ifdef MPICH_DEV_BUILD
    if (!(car_ptr->type & MM_HEAD_CAR))
    {
	err_printf("tcp_post_write: only head cars can be posted.\n");
	MM_EXIT_FUNC(TCP_POST_WRITE);
	return -1;
    }
#endif

    if (car_ptr->msg_header.pkt.u.hdr.size < TCP_EAGER_LIMIT)
    {
	/* enqueue the head packet car */
	tcp_car_enqueue(vc_ptr, car_ptr);
	
	/* point the qnext pointers in all the data cars
	 * to the same qnext pointer in the head car 
	 */
	/* this is done by tcp_car_enqueue now.
	iter_ptr = car_ptr->next_ptr;
	while (iter_ptr)
	{
	    iter_ptr->qnext_ptr = car_ptr->qnext_ptr;
	    iter_ptr = iter_ptr->next_ptr;
	}
	*/
    }
    else
    {
	rndv_car_ptr = mm_car_alloc();

	tcp_setup_packet_car(rndv_car_ptr, MM_WRITE_CAR, car_ptr->dest, car_ptr->vc_ptr);

	/* set up the rts header packet */
	rndv_rts_ptr = &rndv_car_ptr->msg_header.pkt.u.hdr;
	rndv_rts_ptr->context = car_ptr->msg_header.pkt.u.hdr.context;
	rndv_rts_ptr->size = car_ptr->msg_header.pkt.u.hdr.size;
	rndv_rts_ptr->src = car_ptr->msg_header.pkt.u.hdr.src;
	rndv_rts_ptr->tag = car_ptr->msg_header.pkt.u.hdr.tag;
	rndv_rts_ptr->type = MPID_RNDV_REQUEST_TO_SEND_PKT;
	rndv_rts_ptr->sender_car_ptr = car_ptr;

	tcp_car_enqueue(vc_ptr, rndv_car_ptr);
    }

    MM_EXIT_FUNC(TCP_POST_WRITE);
    return MPI_SUCCESS;
}
