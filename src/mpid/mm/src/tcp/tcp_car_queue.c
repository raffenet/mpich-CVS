/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "tcpimpl.h"

/*@
   tcp_car_queue_replace_head - replace the head car in the queue

   Parameters:
+  MPIDI_VC *vc_ptr - vc
-  MM_Car *car_ptr - car

   Notes:
@*/
int tcp_car_queue_replace_head(MPIDI_VC *vc_ptr, MM_Car *car_ptr)
{
    MM_ENTER_FUNC(TCP_CAR_HEAD_ENQUEUE);

    if (car_ptr->type & MM_WRITE_CAR)
    {
	if (vc_ptr->writeq_head == NULL)
	{
	    err_printf("Error: tcp_car_queue_replace_head called with no write head.\n");
	    MM_EXIT_FUNC(TCP_CAR_HEAD_ENQUEUE);
	    return -1;
	}

	/* enqueue at the head */
	car_ptr->vcqnext_ptr = vc_ptr->writeq_head->vcqnext_ptr;
	if (vc_ptr->writeq_tail == vc_ptr->writeq_head)
	    vc_ptr->writeq_tail = car_ptr;
	vc_ptr->writeq_head = car_ptr;
    }
    if (car_ptr->type & MM_READ_CAR)
    {
	if (vc_ptr->readq_head == NULL)
	{
	    err_printf("Error: tcp_car_queue_replace_head called with no read head.\n");
	    MM_EXIT_FUNC(TCP_CAR_HEAD_ENQUEUE);
	    return -1;
	}

	/* enqueue at the head */
	car_ptr->vcqnext_ptr = vc_ptr->readq_head->vcqnext_ptr;
	if (vc_ptr->readq_tail == vc_ptr->readq_head)
	    vc_ptr->readq_tail = car_ptr;
	vc_ptr->readq_head = car_ptr;
    }

    MM_EXIT_FUNC(TCP_CAR_HEAD_ENQUEUE);
    return MPI_SUCCESS;
}

/*@
   tcp_car_head_enqueue - enqueue a car in a vc at the head

   Parameters:
+  MPIDI_VC *vc_ptr - vc
-  MM_Car *car_ptr - car

   Notes:
@*/
int tcp_car_head_enqueue(MPIDI_VC *vc_ptr, MM_Car *car_ptr)
{
    MM_Car *iter_ptr;
    MM_ENTER_FUNC(TCP_CAR_HEAD_ENQUEUE);

    if (car_ptr->type & MM_WRITE_CAR)
    {
	/* If the write queue for this vc is empty then enqueue this vc in the process active write list */
	if (vc_ptr->writeq_head == NULL)
	{
	    TCP_Process.max_bfd = BFD_MAX(vc_ptr->data.tcp.bfd, TCP_Process.max_bfd);
	    if (!BFD_ISSET(vc_ptr->data.tcp.bfd, &TCP_Process.writeset))
	    {
		BFD_SET(vc_ptr->data.tcp.bfd, &TCP_Process.writeset);
		TCP_Process.num_writers++;
	    }
	    vc_ptr->write_next_ptr = TCP_Process.write_list;
	    TCP_Process.write_list = vc_ptr;
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
    }

    MM_EXIT_FUNC(TCP_CAR_HEAD_ENQUEUE);
    return MPI_SUCCESS;
}

/*@
   tcp_car_enqueue - enqueue a car in a vc

   Parameters:
+  MPIDI_VC *vc_ptr - vc
-  MM_Car *car_ptr - car

   Notes:
@*/
int tcp_car_enqueue(MPIDI_VC *vc_ptr, MM_Car *car_ptr)
{
    MM_Car *iter_ptr;
    MM_ENTER_FUNC(TCP_CAR_ENQUEUE);

    if (car_ptr->type & MM_WRITE_CAR)
    {
	/* If the write queue for this vc is empty then enqueue this vc in the process active write list */
	if (vc_ptr->writeq_head == NULL)
	{
	    TCP_Process.max_bfd = BFD_MAX(vc_ptr->data.tcp.bfd, TCP_Process.max_bfd);
	    if (!BFD_ISSET(vc_ptr->data.tcp.bfd, &TCP_Process.writeset))
	    {
		BFD_SET(vc_ptr->data.tcp.bfd, &TCP_Process.writeset);
		TCP_Process.num_writers++;
	    }
	    vc_ptr->write_next_ptr = TCP_Process.write_list;
	    TCP_Process.write_list = vc_ptr;
	}
	/* enqueue the write car in the vc_ptr write queue */
	if (vc_ptr->writeq_tail != NULL)
	{
	    /* enqueue the list of cars as a single entity
	     * by setting their vcqnext_ptrs to the same next car
	     */
	    iter_ptr = vc_ptr->writeq_tail;
	    do
	    {
		iter_ptr->vcqnext_ptr = car_ptr;
		iter_ptr = iter_ptr->next_ptr;
	    } while (iter_ptr);
	    /* enqueue only the head car */
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
	}
	vc_ptr->writeq_tail = car_ptr;
    }
    if (car_ptr->type & MM_READ_CAR)
    {
	/* enqueue the read car in the vc_ptr read queue */
	if (vc_ptr->readq_tail != NULL)
	{
	    /* enqueue the list of cars as a single entity
	     * by setting their vcqnext_ptrs to the same next car
	     */
	    iter_ptr = vc_ptr->readq_tail;
	    do
	    {
		iter_ptr->vcqnext_ptr = car_ptr;
		iter_ptr = iter_ptr->next_ptr;
	    } while (iter_ptr);
	    /* enqueue only the head car */
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

    car_ptr->vcqnext_ptr = NULL;

    MM_EXIT_FUNC(TCP_CAR_ENQUEUE);
    return MPI_SUCCESS;
}

static int tcp_vc_dequeue_write(MPIDI_VC *vc_ptr)
{
    MPIDI_VC *iter_ptr;
    BFD_CLR(vc_ptr->data.tcp.bfd, &TCP_Process.writeset);
    TCP_Process.num_writers--;
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

/*@
   tcp_car_dequeue - dequeue a car from a vc

   Parameters:
+  MPIDI_VC *vc_ptr - vc
-  MM_Car *car_ptr - car

   Notes:
@*/
#ifdef FOO
int tcp_car_dequeue(MPIDI_VC *vc_ptr, MM_Car *car_ptr)
{
    MM_Car *iter_ptr, *next_ptr;

    MM_ENTER_FUNC(TCP_CAR_DEQUEUE);

    if (car_ptr->type & MM_WRITE_CAR)
    {
	/* dequeue the car from the vc_ptr write queue */
	if (vc_ptr->writeq_head == NULL)
	{
	    MM_EXIT_FUNC(TCP_CAR_DEQUEUE);
	    return MPI_SUCCESS;
	}
	if (vc_ptr->writeq_head == car_ptr)
	{
	    vc_ptr->writeq_head = car_ptr->next_ptr ? car_ptr->next_ptr : vc_ptr->writeq_head->vcqnext_ptr;
	    if (vc_ptr->writeq_head == NULL)
		vc_ptr->writeq_tail = NULL;
	}
	else
	{
	    iter_ptr = vc_ptr->writeq_head;
	    while (iter_ptr->vcqnext_ptr)
	    {
		if (iter_ptr->vcqnext_ptr == car_ptr)
		{
		    if (car_ptr->next_ptr)
		    {
			do
			{
			    iter_ptr->vcqnext_ptr = car_ptr->next_ptr;
			    iter_ptr = iter_ptr->next_ptr;
			} while (iter_ptr);
		    }
		    else
		    {
			if (iter_ptr->vcqnext_ptr == vc_ptr->writeq_tail)
			    vc_ptr->writeq_tail = iter_ptr;
			/* make the entire list of cars point to the new vcqnext car */
			next_ptr = iter_ptr->vcqnext_ptr->vcqnext_ptr;
			do
			{
			    iter_ptr->vcqnext_ptr = next_ptr;
			    iter_ptr = iter_ptr->next_ptr;
			} while (iter_ptr);
		    }
		    /* make only the head car point to the new vcqnext car */
		    /*iter_ptr->vcqnext_ptr = iter_ptr->vcqnext_ptr->vcqnext_ptr;*/
		    break;
		}
		iter_ptr = iter_ptr->vcqnext_ptr;
	    }
	}
	/* If the write queue becomes empty, remove the vc from the process active vc write list */
	if (vc_ptr->writeq_head == NULL)
	    tcp_vc_dequeue_write(car_ptr->vc_ptr);
    }
    if (car_ptr->type & MM_READ_CAR)
    {
	/* dequeue the car from the vc_ptr read queue */
	if (vc_ptr->readq_head == NULL)
	{
	    MM_EXIT_FUNC(TCP_CAR_DEQUEUE);
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

#ifdef MPICH_DEV_BUILD
    iter_ptr = car_ptr;
    while (iter_ptr)
    {
	iter_ptr->vcqnext_ptr = NULL;
	iter_ptr = iter_ptr->next_ptr;
    }
#else
    car_ptr->vcqnext_ptr = NULL;
#endif

    MM_EXIT_FUNC(TCP_CAR_DEQUEUE);
    return MPI_SUCCESS;
}
#endif

int tcp_car_dequeue(MPIDI_VC *vc_ptr, MM_Car *car_ptr)
{
    MM_Car *iter_ptr, *next_ptr;

    MM_ENTER_FUNC(TCP_CAR_DEQUEUE);

    if (car_ptr->type & MM_WRITE_CAR)
    {
	/* dequeue the car from the vc_ptr write queue */
	if (vc_ptr->writeq_head == NULL)
	{
	    MM_EXIT_FUNC(TCP_CAR_DEQUEUE);
	    return MPI_SUCCESS;
	}
	if (vc_ptr->writeq_head == car_ptr)
	{
	    vc_ptr->writeq_head = vc_ptr->writeq_head->vcqnext_ptr;
	    if (vc_ptr->writeq_head == NULL)
		vc_ptr->writeq_tail = NULL;
	}
	else
	{
	    iter_ptr = vc_ptr->writeq_head;
	    while (iter_ptr->vcqnext_ptr)
	    {
		if (iter_ptr->vcqnext_ptr == car_ptr)
		{
		    if (iter_ptr->vcqnext_ptr == vc_ptr->writeq_tail)
			vc_ptr->writeq_tail = iter_ptr;
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
	/* If the write queue becomes empty, remove the vc from the process active vc write list */
	if (vc_ptr->writeq_head == NULL)
	    tcp_vc_dequeue_write(car_ptr->vc_ptr);
    }
    if (car_ptr->type & MM_READ_CAR)
    {
	/* dequeue the car from the vc_ptr read queue */
	if (vc_ptr->readq_head == NULL)
	{
	    MM_EXIT_FUNC(TCP_CAR_DEQUEUE);
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

#ifdef MPICH_DEV_BUILD
    iter_ptr = car_ptr;
    while (iter_ptr)
    {
	iter_ptr->vcqnext_ptr = NULL;
	iter_ptr = iter_ptr->next_ptr;
    }
#else
    car_ptr->vcqnext_ptr = NULL;
#endif

    MM_EXIT_FUNC(TCP_CAR_DEQUEUE);
    return MPI_SUCCESS;
}
