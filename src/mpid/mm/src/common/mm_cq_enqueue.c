/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int mm_cq_enqueue(MM_Car *car_ptr)
{
    MPID_Thread_lock(MPID_Process.cqlock);
    if (MPID_Process.cq_tail)
	MPID_Process.cq_tail->qnext_ptr = car_ptr;
    else
	MPID_Process.cq_head = car_ptr;
    car_ptr->qnext_ptr = NULL;
    MPID_Process.cq_tail = car_ptr;
    MPID_Thread_unlock(MPID_Process.cqlock);
    return MPI_SUCCESS;
}
