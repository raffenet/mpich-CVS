/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int mm_enqueue_request_to_send(MM_Car *unex_head_car_ptr)
{
    MM_Car *car_ptr;

    MM_ENTER_FUNC(MM_ENQUEUE_REQUEST_TO_SEND);
    dbg_printf("mm_enqueue_request_to_send\n");

    printf("mm_enqueue_request_to_send\n");fflush(stdout);
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

    MM_EXIT_FUNC(MM_ENQUEUE_REQUEST_TO_SEND);
    return MPI_SUCCESS;
}
