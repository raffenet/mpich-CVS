/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int mm_post_recv(MM_Car *car_ptr)
{
    MM_Car *iter_ptr, *trailer_ptr;
    
    /* check if this is a packer car */
    /* packer cars are not matched */
    if (car_ptr->type & MM_PACKER_CAR)
    {
	packer_post_read(MPID_Process.packer_vc_ptr, car_ptr);
	return MPI_SUCCESS;
    }

    /* find in unex_q or enqueue into the posted_q */
    MPID_Thread_lock(MPID_Process.qlock);
    trailer_ptr = iter_ptr = MPID_Process.unex_q_head;
    while (iter_ptr)
    {
	if ((iter_ptr->msg_header.pkt.context == car_ptr->msg_header.pkt.context) &&
	    (iter_ptr->msg_header.pkt.tag == car_ptr->msg_header.pkt.tag) &&
	    (iter_ptr->src == car_ptr->src))
	{
	    if (iter_ptr->msg_header.pkt.size > car_ptr->msg_header.pkt.size)
	    {
		err_printf("Error: unex msg size %d > posted msg size %d\n", iter_ptr->msg_header.pkt.size, car_ptr->msg_header.pkt.size);
		return -1;
	    }
	    /* dequeue the car from the unex_q */
	    if (trailer_ptr == iter_ptr)
	    {
		MPID_Process.unex_q_head = iter_ptr->qnext_ptr;
		if (MPID_Process.unex_q_head == NULL)
		    MPID_Process.unex_q_tail = NULL;
	    }
	    else
	    {
		trailer_ptr->qnext_ptr = iter_ptr->qnext_ptr;
		if (MPID_Process.unex_q_tail == iter_ptr)
		    MPID_Process.unex_q_tail = trailer_ptr;
	    }
	    MPID_Thread_unlock(MPID_Process.qlock);
	    /* merge the unex car with the posted car using the method in the vc */
	    iter_ptr->vc_ptr->merge_with_unexpected(car_ptr, iter_ptr);
	    return MPI_SUCCESS;
	}
	if (trailer_ptr != iter_ptr)
	    trailer_ptr = trailer_ptr->qnext_ptr;
	iter_ptr = iter_ptr->qnext_ptr;
    }

    /* the car was not found in the unexpected queue so put it in the posted queueu */
    if (MPID_Process.posted_q_tail == NULL)
    {
	MPID_Process.posted_q_head = car_ptr;
    }
    else
    {
	MPID_Process.posted_q_tail->qnext_ptr = car_ptr;
    }
    car_ptr->qnext_ptr = NULL;
    MPID_Process.posted_q_tail = car_ptr;

    MPID_Thread_unlock(MPID_Process.qlock);

    return MPI_SUCCESS;
}
