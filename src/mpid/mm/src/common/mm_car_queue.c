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
	/* If the write queue for this vc is empty then enqueue this vc in the process active write list */
	if (vc_ptr->writeq_head == NULL)
	{
	    vc_ptr->write_next_ptr = MPID_Process.active_write_vc_list;
	    MPID_Process.active_write_vc_list = vc_ptr;
	}
	/* enqueue the write car in the vc_ptr write queue */
	if (vc_ptr->writeq_tail != NULL)
	    vc_ptr->writeq_tail->qnext_ptr = car_ptr;
	else
	    vc_ptr->writeq_head = car_ptr;
	vc_ptr->writeq_tail = car_ptr;
	break;
    case MM_READ_CAR:
	/* If the read queue for this vc is empty then enqueue this vc in the process active read list */
	if (vc_ptr->readq_head == NULL)
	{
	    vc_ptr->read_next_ptr = MPID_Process.active_read_vc_list;
	    MPID_Process.active_read_vc_list = vc_ptr;
	}
	/* enqueue the read car in the vc_ptr read queue */
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

static int mm_vc_dequeue_write(MPIDI_VC *vc_ptr)
{
    MPIDI_VC *iter_ptr;
    if (vc_ptr == MPID_Process.active_write_vc_list)
    {
	MPID_Process.active_write_vc_list = vc_ptr->write_next_ptr;
	return MPI_SUCCESS;
    }
    iter_ptr = MPID_Process.active_write_vc_list;
    while (iter_ptr->write_next_ptr)
    {
	if (iter_ptr->write_next_ptr == vc_ptr)
	{
	    iter_ptr->write_next_ptr = vc_ptr->write_next_ptr;
	    return MPI_SUCCESS;
	}
	iter_ptr = iter_ptr->write_next_ptr;
    }
    return MPI_ERR_ARG;
}

static int mm_vc_dequeue_read(MPIDI_VC *vc_ptr)
{
    MPIDI_VC *iter_ptr;
    if (vc_ptr == MPID_Process.active_read_vc_list)
    {
	MPID_Process.active_read_vc_list = vc_ptr->read_next_ptr;
	return MPI_SUCCESS;
    }
    iter_ptr = MPID_Process.active_read_vc_list;
    while (iter_ptr->read_next_ptr)
    {
	if (iter_ptr->read_next_ptr == vc_ptr)
	{
	    iter_ptr->read_next_ptr = vc_ptr->read_next_ptr;
	    return MPI_SUCCESS;
	}
	iter_ptr = iter_ptr->read_next_ptr;
    }
    return MPI_ERR_ARG;
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
	/* dequeue the car from the vc_ptr write queue */
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
	/* If the write queue becomes empty, remove the vc from the process active vc write list */
	if (vc_ptr->writeq_head == NULL)
	    mm_vc_dequeue_write(car_ptr->vc_ptr);
	break;
    case MM_READ_CAR:
	/* dequeue the car from the vc_ptr read queue */
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
	/* If the read queue becomes empty, remove the vc from the process active vc read list */
	if (vc_ptr->readq_head == NULL)
	    mm_vc_dequeue_read(car_ptr->vc_ptr);
	break;
    default:
	err_printf("illegal car type: %d\n", car_ptr->type);
	break;
    }

    car_ptr->qnext_ptr = NULL;

    return MPI_SUCCESS;
}
