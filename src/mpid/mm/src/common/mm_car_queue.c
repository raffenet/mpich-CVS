/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   mm_car_enqueue - enqueue a car into the vc in the car

   Parameters:
+  MM_Car *car_ptr - car

   Notes:
@*/
int mm_car_enqueue(MM_Car *car_ptr)
{
    MPIDI_VC *vc_ptr;

    vc_ptr = car_ptr->vc_ptr;

    switch (car_ptr->type)
    {
    case MM_WRITE_CAR:
	if (vc_ptr->writeq_tail != NULL)
	    vc_ptr->writeq_tail->qnext_ptr = car_ptr;
	else
	    vc_ptr->writeq_head = car_ptr;
	vc_ptr->writeq_tail = car_ptr;
	break;
    case MM_READ_CAR:
	if (vc_ptr->readq_tail != NULL)
	    vc_ptr->readq_tail->qnext_ptr = car_ptr;
	else
	    vc_ptr->readq_head = car_ptr;
	vc_ptr->readq_tail = car_ptr;
	break;
    default:
	err_printf("illegal car type: %d\n", car_ptr->type);
	break;
    }

    car_ptr->qnext_ptr = NULL;

    return MPI_SUCCESS;
}

/*@
   mm_car_dequeue - dequeue the car from the vc in the car

   Parameters:
+  MM_Car *car_ptr = car

   Notes:
@*/
int mm_car_dequeue(MM_Car *car_ptr)
{
    MPIDI_VC *vc_ptr;
    MM_Car *iter_ptr;

    vc_ptr = car_ptr->vc_ptr;

    switch (car_ptr->type)
    {
    case MM_WRITE_CAR:
	if (vc_ptr->writeq_head == NULL)
	    return MPI_SUCCESS;
	if (vc_ptr->writeq_head == car_ptr)
	{
	    vc_ptr->writeq_head = vc_ptr->writeq_head->qnext_ptr;
	    if (vc_ptr->writeq_head == NULL)
		vc_ptr->writeq_tail = NULL;
	}
	iter_ptr = vc_ptr->writeq_head;
	while (iter_ptr->qnext_ptr)
	{
	    if (iter_ptr->qnext_ptr == car_ptr)
	    {
		if (iter_ptr->qnext_ptr == vc_ptr->writeq_tail)
		    vc_ptr->writeq_tail = iter_ptr;
		iter_ptr->qnext_ptr = iter_ptr->qnext_ptr->qnext_ptr;
		break;
	    }
	    iter_ptr = iter_ptr->qnext_ptr;
	}
	break;
    case MM_READ_CAR:
	if (vc_ptr->readq_head == NULL)
	    return MPI_SUCCESS;
	if (vc_ptr->readq_head == car_ptr)
	{
	    vc_ptr->readq_head = vc_ptr->readq_head->qnext_ptr;
	    if (vc_ptr->readq_head == NULL)
		vc_ptr->readq_tail = NULL;
	}
	iter_ptr = vc_ptr->readq_head;
	while (iter_ptr->qnext_ptr)
	{
	    if (iter_ptr->qnext_ptr == car_ptr)
	    {
		if (iter_ptr->qnext_ptr == vc_ptr->readq_tail)
		    vc_ptr->readq_tail = iter_ptr;
		iter_ptr->qnext_ptr = iter_ptr->qnext_ptr->qnext_ptr;
		break;
	    }
	    iter_ptr = iter_ptr->qnext_ptr;
	}
	break;
    default:
	err_printf("illegal car type: %d\n", car_ptr->type);
	break;
    }

    car_ptr->qnext_ptr = NULL;

    return MPI_SUCCESS;
}
