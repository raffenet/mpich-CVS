/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "ibimpl.h"

/*@
   ib_enqueue_read_at_head - enqueue a car in a vc at the head

   Parameters:
+  MPIDI_VC *vc_ptr - vc
-  MM_Car *car_ptr - car

   Notes:
@*/
int ib_enqueue_read_at_head(MPIDI_VC *vc_ptr, MM_Car *car_ptr)
{
    MM_Car *iter_ptr;
    MPIDI_STATE_DECL(MPID_STATE_IB_CAR_HEAD_ENQUEUE);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_CAR_HEAD_ENQUEUE);

#ifdef MPICH_DEV_BUILD
    if (car_ptr->type & MM_WRITE_CAR)
    {
	err_printf("MM_WRITE_CAR sent to ib_enqueue_read_at_head\n");
    }
#endif
    if (car_ptr->type & MM_READ_CAR)
    {
	/* enqueue at the head */
	iter_ptr = car_ptr;
	do
	{
	    iter_ptr->vcqnext_ptr = vc_ptr->readq_head;
	    iter_ptr = iter_ptr->next_ptr;
	} while (iter_ptr);
	vc_ptr->readq_head = car_ptr;
	if (vc_ptr->readq_tail == NULL)
	    vc_ptr->readq_tail = car_ptr;

	/* change the state from reading_header to reading_data */
	/*vc_ptr->data.ib.read = ib_read_data;*/
	/*ib_read_data(vc_ptr);*/
    }

    MPIDI_FUNC_EXIT(MPID_STATE_IB_CAR_HEAD_ENQUEUE);
    return MPI_SUCCESS;
}

/*@
   ib_enqueue_write_at_head - enqueue a car in a vc at the head

   Parameters:
+  MPIDI_VC *vc_ptr - vc
-  MM_Car *car_ptr - car

   Notes:
@*/
int ib_enqueue_write_at_head(MPIDI_VC *vc_ptr, MM_Car *car_ptr)
{
    MM_Car *iter_ptr;
    MPIDI_STATE_DECL(MPID_STATE_IB_CAR_HEAD_ENQUEUE);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_CAR_HEAD_ENQUEUE);

    if (car_ptr->type & MM_WRITE_CAR)
    {
	/*msg_printf("ib_car_head_enqueue: enqueueing write head packet\n");*/
	/* If the write queue for this vc is empty then enqueue this vc in the process active write list */
	if (vc_ptr->writeq_head == NULL)
	{
	    vc_ptr->write_next_ptr = IB_Process.write_list;
	    IB_Process.write_list = vc_ptr;
	}

	/* enqueue at the head */
	iter_ptr = car_ptr;
	do
	{
	    iter_ptr->vcqnext_ptr = vc_ptr->writeq_head;
	    iter_ptr = iter_ptr->next_ptr;
	} while (iter_ptr);
	vc_ptr->writeq_head = car_ptr;
	if (vc_ptr->writeq_tail == NULL)
	    vc_ptr->writeq_tail = car_ptr;
    }
#ifdef MPICH_DEV_BUILD
    if (car_ptr->type & MM_READ_CAR)
    {
	err_printf("MM_READ_CAR sent to ib_enqueue_write_at_head\n");
    }
#endif

    MPIDI_FUNC_EXIT(MPID_STATE_IB_CAR_HEAD_ENQUEUE);
    return MPI_SUCCESS;
}

#if 0
/*@
   ib_car_enqueue - enqueue a car in a vc

   Parameters:
+  MPIDI_VC *vc_ptr - vc
-  MM_Car *car_ptr - car

   Notes:
@*/
int ib_car_enqueue(MPIDI_VC *vc_ptr, MM_Car *car_ptr)
{
    MM_Car *iter_ptr;
    MPIDI_STATE_DECL(MPID_STATE_IB_CAR_ENQUEUE);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_CAR_ENQUEUE);

    if (car_ptr->type & MM_WRITE_CAR)
    {
	/* enqueue the write car in the vc_ptr write queue */
	if (vc_ptr->writeq_tail != NULL)
	{
	    /* set all the vcqnext_ptrs in the tail chain to this newly enqueued car */
	    iter_ptr = vc_ptr->writeq_tail;
	    do
	    {
		iter_ptr->vcqnext_ptr = car_ptr;
		iter_ptr = iter_ptr->next_ptr;
	    } while (iter_ptr);
	    /* OR enqueue only the head car */
	    /*vc_ptr->writeq_tail->vcqnext_ptr = car_ptr;*/
	}
	else
	{
	    vc_ptr->writeq_head = car_ptr;
	    iter_ptr = car_ptr;
	    do
	    {
		iter_ptr->vcqnext_ptr = NULL;
		iter_ptr = iter_ptr->next_ptr;
	    } while (iter_ptr);
	    vc_ptr->writeq_tail = car_ptr;

	    if (vc_ptr->data.ib.state & IB_CONNECTED)
		ib_write_aggressive(vc_ptr);

	    MPIDI_FUNC_EXIT(MPID_STATE_IB_CAR_ENQUEUE);
	    return MPI_SUCCESS;
	}
	vc_ptr->writeq_tail = car_ptr;
    }
    if (car_ptr->type & MM_READ_CAR)
    {
	/* enqueue the read car in the vc_ptr read queue */
	if (vc_ptr->readq_tail != NULL)
	{
	    /* set all the vcqnext_ptrs in the tail chain to this newly enqueued car */
	    iter_ptr = vc_ptr->readq_tail;
	    do
	    {
		iter_ptr->vcqnext_ptr = car_ptr;
		iter_ptr = iter_ptr->next_ptr;
	    } while (iter_ptr);
	    /* OR enqueue only the head car */
	    /*vc_ptr->readq_tail->vcqnext_ptr = car_ptr;*/
	}
	else
	{
	    vc_ptr->readq_head = car_ptr;
	    iter_ptr = car_ptr;
	    do
	    {
		iter_ptr->vcqnext_ptr = NULL;
		iter_ptr = iter_ptr->next_ptr;
	    } while (iter_ptr);
	}
	vc_ptr->readq_tail = car_ptr;
    }

    iter_ptr = car_ptr;
    do
    {
	iter_ptr->vcqnext_ptr = NULL;
	iter_ptr = iter_ptr->next_ptr;
    } while (iter_ptr);

    MPIDI_FUNC_EXIT(MPID_STATE_IB_CAR_ENQUEUE);
    return MPI_SUCCESS;
}

static int ib_vc_dequeue_write(MPIDI_VC *vc_ptr)
{
    MPIDI_VC *iter_ptr;
    /* check the head of the list */
    if (vc_ptr == IB_Process.write_list)
    {
	IB_Process.write_list = vc_ptr->write_next_ptr;
	return MPI_SUCCESS;
    }
    /* iterate through the rest of the list */
    iter_ptr = IB_Process.write_list;
    while (iter_ptr->write_next_ptr)
    {
	if (iter_ptr->write_next_ptr == vc_ptr)
	{
	    /* set the write_next_ptr to jump over the vc being removed */
	    iter_ptr->write_next_ptr = vc_ptr->write_next_ptr;
	    return MPI_SUCCESS;
	}
	iter_ptr = iter_ptr->write_next_ptr;
    }
    return MPI_ERR_ARG;
}

#endif /* #if 0 */

/*@
   ib_car_dequeue_write - dequeue the head write car from a vc

   Parameters:
+  MPIDI_VC *vc_ptr - vc

   Notes:
@*/
int ib_car_dequeue_write(MPIDI_VC *vc_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_CAR_DEQUEUE_WRITE);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_CAR_DEQUEUE_WRITE);

#ifdef MPICH_DEV_BUILD
    if (vc_ptr->writeq_head == NULL)
    {
	err_printf("ib_car_dequeue_write called on empty queue\n");
    }
#endif
    vc_ptr->writeq_head = (vc_ptr->writeq_head->next_ptr != NULL) ? vc_ptr->writeq_head->next_ptr : vc_ptr->writeq_head->vcqnext_ptr;
    if (vc_ptr->writeq_head == NULL)
    {
	vc_ptr->writeq_tail = NULL;
	/* If the write queue becomes empty, remove the vc from the process active vc write list */
	/*ib_vc_dequeue_write(vc_ptr);*/
    }

    MPIDI_FUNC_EXIT(MPID_STATE_IB_CAR_DEQUEUE_WRITE);
    return MPI_SUCCESS;
}

/*@
   ib_car_dequeue_read - dequeue a car from a vc

   Parameters:
+  MPIDI_VC *vc_ptr - vc
-  MM_Car *car_ptr - car

   Notes:
@*/
int ib_car_dequeue_read(MPIDI_VC *vc_ptr, MM_Car *car_ptr)
{
    MM_Car *iter_ptr, *next_ptr;
    MPIDI_STATE_DECL(MPID_STATE_IB_CAR_DEQUEUE);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_CAR_DEQUEUE);

#ifdef MPICH_DEV_BUILD
    if (car_ptr->type & MM_WRITE_CAR)
    {
	err_printf("you should call ib_car_dequeue_write.\n");
    }
#endif
    if (car_ptr->type & MM_READ_CAR)
    {
	/* dequeue the car from the vc_ptr read queue */
	if (vc_ptr->readq_head == NULL)
	{
	    err_printf("ib_car_dequeue called on an empty read queue.\n");
	    MPIDI_FUNC_EXIT(MPID_STATE_IB_CAR_DEQUEUE);
	    return MPI_SUCCESS;
	}
	if (vc_ptr->readq_head == car_ptr)
	{
	    vc_ptr->readq_head = vc_ptr->readq_head->vcqnext_ptr;
	    if (vc_ptr->readq_head == NULL)
		vc_ptr->readq_tail = NULL;
	}
	else
	{
	    iter_ptr = vc_ptr->readq_head;
	    while (iter_ptr->vcqnext_ptr)
	    {
		if (iter_ptr->vcqnext_ptr == car_ptr)
		{
		    if (iter_ptr->vcqnext_ptr == vc_ptr->readq_tail)
			vc_ptr->readq_tail = iter_ptr;
		    /* make the entire list of cars point to the new vcqnext car */
		    next_ptr = iter_ptr->vcqnext_ptr->vcqnext_ptr;
		    do
		    {
			iter_ptr->vcqnext_ptr = next_ptr;
			iter_ptr = iter_ptr->next_ptr;
		    } while (iter_ptr);
		    /* make only the head car point to the new vcqnext car */
		    /*iter_ptr->vcqnext_ptr = iter_ptr->vcqnext_ptr->vcqnext_ptr;*/
		    break;
		}
		iter_ptr = iter_ptr->vcqnext_ptr;
	    }
	}
    }
    else
    {
	err_printf("ib_car_dequeue called with unknown car type: %d\n", car_ptr->type);
    }

#ifdef MPICH_DEV_BUILD
    iter_ptr = car_ptr;
    while (iter_ptr)
    {
	iter_ptr->vcqnext_ptr = INVALID_POINTER;
	iter_ptr = iter_ptr->next_ptr;
    }
#endif

    MPIDI_FUNC_EXIT(MPID_STATE_IB_CAR_DEQUEUE);
    return MPI_SUCCESS;
}
