/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "tcpimpl.h"

/*@
   tcp_car_enqueue - enqueue a car in a vc

   Parameters:
+  MPIDI_VC *vc_ptr - vc
-  MM_Car *car_ptr - car

   Notes:
@*/
int tcp_car_enqueue(MPIDI_VC *vc_ptr, MM_Car *car_ptr)
{
    if (car_ptr->type & MM_WRITE_CAR)
    {
	/* If the write queue for this vc is empty then enqueue this vc in the process active write list */
	if (vc_ptr->writeq_head == NULL)
	{
	    vc_ptr->write_next_ptr = TCP_Process.write_list;
	    TCP_Process.write_list = vc_ptr;
	}
	/* enqueue the write car in the vc_ptr write queue */
	if (vc_ptr->writeq_tail != NULL)
	    vc_ptr->writeq_tail->mnext_ptr = car_ptr;
	else
	    vc_ptr->writeq_head = car_ptr;
	vc_ptr->writeq_tail = car_ptr;
    }
    if (car_ptr->type & MM_READ_CAR)
    {
	/* If the read queue for this vc is empty then enqueue this vc in the process active read list */
	if (vc_ptr->readq_head == NULL)
	{
	    vc_ptr->read_next_ptr = TCP_Process.read_list;
	    TCP_Process.read_list = vc_ptr;
	}
	/* enqueue the read car in the vc_ptr read queue */
	if (vc_ptr->readq_tail != NULL)
	    vc_ptr->readq_tail->mnext_ptr = car_ptr;
	else
	    vc_ptr->readq_head = car_ptr;
	vc_ptr->readq_tail = car_ptr;
    }

    car_ptr->mnext_ptr = NULL;

    return MPI_SUCCESS;
}

static int tcp_vc_dequeue_write(MPIDI_VC *vc_ptr)
{
    MPIDI_VC *iter_ptr;
    if (vc_ptr == TCP_Process.write_list)
    {
	TCP_Process.write_list = vc_ptr->write_next_ptr;
	return MPI_SUCCESS;
    }
    iter_ptr = TCP_Process.write_list;
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

static int tcp_vc_dequeue_read(MPIDI_VC *vc_ptr)
{
    MPIDI_VC *iter_ptr;
    if (vc_ptr == TCP_Process.read_list)
    {
	TCP_Process.read_list = vc_ptr->read_next_ptr;
	return MPI_SUCCESS;
    }
    iter_ptr = TCP_Process.read_list;
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
   tcp_car_dequeue - dequeue a car from a vc

   Parameters:
+  MPIDI_VC *vc_ptr - vc
-  MM_Car *car_ptr - car

   Notes:
@*/
int tcp_car_dequeue(MPIDI_VC *vc_ptr, MM_Car *car_ptr)
{
    MM_Car *iter_ptr;

    if (car_ptr->type & MM_WRITE_CAR)
    {
	/* dequeue the car from the vc_ptr write queue */
	if (vc_ptr->writeq_head == NULL)
	    return MPI_SUCCESS;
	if (vc_ptr->writeq_head == car_ptr)
	{
	    vc_ptr->writeq_head = vc_ptr->writeq_head->mnext_ptr;
	    if (vc_ptr->writeq_head == NULL)
		vc_ptr->writeq_tail = NULL;
	}
	iter_ptr = vc_ptr->writeq_head;
	while (iter_ptr->mnext_ptr)
	{
	    if (iter_ptr->mnext_ptr == car_ptr)
	    {
		if (iter_ptr->mnext_ptr == vc_ptr->writeq_tail)
		    vc_ptr->writeq_tail = iter_ptr;
		iter_ptr->mnext_ptr = iter_ptr->mnext_ptr->mnext_ptr;
		break;
	    }
	    iter_ptr = iter_ptr->mnext_ptr;
	}
	/* If the write queue becomes empty, remove the vc from the process active vc write list */
	if (vc_ptr->writeq_head == NULL)
	    tcp_vc_dequeue_write(car_ptr->vc_ptr);
    }
    if (car_ptr->type & MM_READ_CAR)
    {
	/* dequeue the car from the vc_ptr read queue */
	if (vc_ptr->readq_head == NULL)
	    return MPI_SUCCESS;
	if (vc_ptr->readq_head == car_ptr)
	{
	    vc_ptr->readq_head = vc_ptr->readq_head->mnext_ptr;
	    if (vc_ptr->readq_head == NULL)
		vc_ptr->readq_tail = NULL;
	}
	iter_ptr = vc_ptr->readq_head;
	while (iter_ptr->mnext_ptr)
	{
	    if (iter_ptr->mnext_ptr == car_ptr)
	    {
		if (iter_ptr->mnext_ptr == vc_ptr->readq_tail)
		    vc_ptr->readq_tail = iter_ptr;
		iter_ptr->mnext_ptr = iter_ptr->mnext_ptr->mnext_ptr;
		break;
	    }
	    iter_ptr = iter_ptr->mnext_ptr;
	}
	/* If the read queue becomes empty, remove the vc from the process active vc read list */
	if (vc_ptr->readq_head == NULL)
	    tcp_vc_dequeue_read(car_ptr->vc_ptr);
    }

    car_ptr->mnext_ptr = NULL;

    return MPI_SUCCESS;
}
