/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int mm_car_enqueue(MM_Car *car_ptr)
{
    MPIDI_VC *vc_ptr;

    vc_ptr = car_ptr->vc_ptr;

    switch (car_ptr->type)
    {
    case MM_WRITE_CAR:
	if (vc_ptr->writeq_tail != NULL)
	{
	    vc_ptr->writeq_tail->qnext_ptr = car_ptr;
	    car_ptr->qnext_ptr = NULL;
	    vc_ptr->writeq_tail = car_ptr;
	}
	else
	{
	    vc_ptr->writeq_head = vc_ptr->writeq_tail = car_ptr;
	    car_ptr->qnext_ptr = NULL;
	}
	break;
    case MM_READ_CAR:
	break;
    }

    return MPI_SUCCESS;
}

int mm_car_dequeue(MM_Car *car_ptr)
{
    return MPI_SUCCESS;
}
