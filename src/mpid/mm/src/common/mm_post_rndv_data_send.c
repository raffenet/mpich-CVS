/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int mm_post_rndv_data_send(MM_Car *rndv_cts_car_ptr)
{
    MPID_Rndv_data_pkt *rndv_data_ptr;
    MM_Car *sender_car_ptr;

    MM_ENTER_FUNC(MM_POST_RNDV_DATA_SEND);

    /* Convert the sender car into a rndv data car */
    sender_car_ptr = rndv_cts_car_ptr->msg_header.pkt.u.cts.sender_car_ptr;

    /* set up the rndv data header packet */
    rndv_data_ptr = &sender_car_ptr->msg_header.pkt.u.rdata;
    rndv_data_ptr->receiver_car_ptr = rndv_cts_car_ptr->msg_header.pkt.u.cts.receiver_car_ptr;
    rndv_data_ptr->size = 0; /* How do I figure out this value? */
    rndv_data_ptr->type = MPID_RNDV_DATA_PKT;

    sender_car_ptr->vc_ptr->post_write(sender_car_ptr->vc_ptr, sender_car_ptr);
    /*mm_dec_cc(sender_car_ptr->request_ptr);*/ /* decrement once for the unsent msg header car */

    MM_EXIT_FUNC(MM_POST_RNDV_DATA_SEND);
    return MPI_SUCCESS;
}

#ifdef FOO
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
    rndv_car_ptr->request_ptr = sender_car_ptr->request_ptr;
    /*mm_inc_cc(sender_car_ptr->request_ptr);*/ /* increment once for this rndv data header car */
    
    /* set up the rndv data header packet */
    rndv_data_ptr = &rndv_car_ptr->msg_header.pkt.u.rdata;
    rndv_data_ptr->receiver_car_ptr = rndv_cts_car_ptr->msg_header.pkt.u.cts.receiver_car_ptr;
    rndv_data_ptr->size = 0; /* How do I figure out this value? */
    rndv_data_ptr->type = MPID_RNDV_DATA_PKT;

    rndv_car_ptr->next_ptr = sender_car_ptr->next_ptr;

    sender_car_ptr->vc_ptr->post_write(sender_car_ptr->vc_ptr, rndv_car_ptr);
    /*mm_dec_cc(sender_car_ptr->request_ptr);*/ /* decrement once for the unsent msg header car */

    MM_EXIT_FUNC(MM_POST_RNDV_DATA_SEND);
    return MPI_SUCCESS;
}
#endif
