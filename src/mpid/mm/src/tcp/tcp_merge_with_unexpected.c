/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "tcpimpl.h"

int tcp_merge_with_unexpected(MM_Car *car_ptr, MM_Car *unex_car_ptr)
{
    int num_left, num_updated;
    char *unex_data_ptr, *orig_unex_data_ptr;

    MM_ENTER_FUNC(TCP_MERGE_WITH_UNEXPECTED);

    /* copy the unexpected packet into the posted packet */
    car_ptr->msg_header.pkt = unex_car_ptr->msg_header.pkt;

    if (unex_car_ptr->next_ptr == NULL)
    {
	/* unex header packet matched a previously posted recv */

	if (unex_car_ptr->msg_header.pkt.u.hdr.type == MPID_EAGER_PKT)
	{
	    /* mark the head car as completed */
	    /*printf("dec cc: read eager head car\n");fflush(stdout);*/
	    mm_dec_cc(car_ptr->request_ptr);
	    car_ptr = car_ptr->next_ptr;

	    /* start reading the eager data */
	    if (car_ptr)
		tcp_car_head_enqueue(car_ptr->vc_ptr, car_ptr);
	} 
	else if (unex_car_ptr->msg_header.pkt.u.hdr.type == MPID_RNDV_REQUEST_TO_SEND_PKT)
	{
	    /*err_printf("tcp_merge_with_unexpected doesn't handle unexpected rndv yet.\n");*/
	    mm_post_rndv_clear_to_send(car_ptr, unex_car_ptr);
	}
	else
	{
	    err_printf("tcp_merge_with_unexpected cannot process packet of type: %d\n", 
		unex_car_ptr->msg_header.pkt.u.hdr.type);
	}
	
	MM_EXIT_FUNC(TCP_MERGE_WITH_UNEXPECTED);
	return MPI_SUCCESS;
    }

    /* reading of unexpected data in progress */

    /* move to the data car */
    unex_car_ptr = unex_car_ptr->next_ptr;
    /* get the tmp buffer and number of bytes read */
    num_left = unex_car_ptr->buf_ptr->tmp.num_read;
    orig_unex_data_ptr = unex_data_ptr = unex_car_ptr->buf_ptr->tmp.buf;

    /* mark the head car as completed */
    /*printf("dec cc: read eager head car\n");fflush(stdout);*/
    mm_dec_cc(car_ptr->request_ptr);
    car_ptr = car_ptr->next_ptr;
    while (num_left)
    {
	num_updated = car_ptr->vc_ptr->merge_unexpected_data(car_ptr->vc_ptr, car_ptr, unex_data_ptr, num_left);
	num_left -= num_updated;
	unex_data_ptr += num_updated;
	car_ptr = car_ptr->next_ptr;
    }

    /* free the temporary buffer and request */
    MPIU_Free(orig_unex_data_ptr);
    mm_request_free(unex_car_ptr->request_ptr);

    MM_EXIT_FUNC(TCP_MERGE_WITH_UNEXPECTED);
    return MPI_SUCCESS;
}
