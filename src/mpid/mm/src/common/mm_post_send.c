/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int mm_post_send(MM_Car *car_ptr)
{
    dbg_printf("mm_post_send\n");
    car_ptr->vc_ptr->post_write(car_ptr->vc_ptr, car_ptr);
    if (car_ptr->msg_header.pkt.u.hdr.size == 0)
    {
	/* enqueue zero byte messages directly into the completion queue */
	car_ptr = car_ptr->next_ptr;
	while (car_ptr)
	{
	    mm_cq_enqueue(car_ptr);
	    /* this is not thread safe because the progress engine could free the car before the next line of code executes */
	    car_ptr = car_ptr->next_ptr;
	}
    }
    else
    {
	/* for now enqueue writes eagerly */
	car_ptr = car_ptr->next_ptr;
	while (car_ptr)
	{
	    car_ptr->vc_ptr->post_write(car_ptr->vc_ptr, car_ptr);
	    car_ptr = car_ptr->next_ptr;
	}
    }
    return MPI_SUCCESS;
}
