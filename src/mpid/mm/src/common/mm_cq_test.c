/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int mm_cq_test()
{
    MM_Car *car_ptr, *old_car_ptr;
    MM_Car *iter_ptr, *trailer_ptr;
    int found;
    MM_Segment_buffer *buf_ptr;
    int complete;

    /* Should we call cq_test on all the methods?
     * before checking the cq?
     * after checking the cq?
     * only if the cq is empty?
     */
    if (MPID_Process.cq_head == NULL)
    {
#ifdef WITH_METHOD_TCP
	tcp_cq_test();
#endif
#ifdef WITH_METHOD_SHM
	shm_cq_test();
#endif
#ifdef WITH_METHOD_VIA
	via_cq_test();
#endif
#ifdef WITH_METHOD_VIA_RDMA
	via_rdma_cq_test();
#endif
#ifdef WITH_METHOD_NEW
	new_cq_test();
#endif
    }

    /* lock */
    car_ptr = MPID_Process.cq_head;
    MPID_Process.cq_head = NULL;
    MPID_Process.cq_tail = NULL;
    /* unlock */

    while (car_ptr)
    {
	complete = TRUE;
	/* handle completed car */
	if (car_ptr->type & MM_UNEX_HEAD_CAR)
	{
	    /* find in posted_q */
	    found = FALSE;
	    MPID_Thread_lock(MPID_Process.qlock);
	    trailer_ptr = iter_ptr = MPID_Process.posted_q_head;
	    while (iter_ptr)
	    {
		if ((iter_ptr->data.pkt.context == car_ptr->data.pkt.context) &&
		    (iter_ptr->data.pkt.tag == car_ptr->data.pkt.tag) &&
		    (iter_ptr->src == car_ptr->src))
		{
		    if (iter_ptr->data.pkt.size > car_ptr->data.pkt.size)
		    {
			err_printf("Error: unex msg size %d > posted msg size %d\n", iter_ptr->data.pkt.size, car_ptr->data.pkt.size);
			return -1;
		    }
		    /* dequeue the car from the posted_q */
		    if (trailer_ptr == iter_ptr)
		    {
			MPID_Process.posted_q_head = iter_ptr->qnext_ptr;
			if (MPID_Process.posted_q_head == NULL)
			    MPID_Process.posted_q_tail = NULL;
		    }
		    else
		    {
			trailer_ptr->qnext_ptr = iter_ptr->qnext_ptr;
			if (MPID_Process.posted_q_tail == iter_ptr)
			    MPID_Process.posted_q_tail = trailer_ptr;
		    }
		    MPID_Thread_unlock(MPID_Process.qlock);
		    /* merge the unex car with the posted car using the method in the vc */
		    iter_ptr->vc_ptr->merge_with_unexpected(car_ptr, iter_ptr);
		    found = TRUE;
		    break;
		}
		if (trailer_ptr != iter_ptr)
		    trailer_ptr = trailer_ptr->qnext_ptr;
		iter_ptr = iter_ptr->qnext_ptr;
	    }

	    /* else allocate a temp buffer, place in the unex_q, and post a read */
	    if (!found)
	    {
		/* save the head car */
		old_car_ptr = car_ptr;
		/* use rcar[1] as the data car */
		car_ptr->next_ptr = &car_ptr->request_ptr->mm.rcar[1];
		car_ptr = car_ptr->next_ptr;

		/* allocate a temporary buffer to hold the unexpected data */
		car_ptr->src = old_car_ptr->src;
		car_ptr->vc_ptr = old_car_ptr->vc_ptr;
		car_ptr->request_ptr = old_car_ptr->request_ptr;
		car_ptr->type = MM_READ_CAR | MM_UNEX_CAR;
		car_ptr->freeme = FALSE;
		car_ptr->next_ptr = NULL;
		car_ptr->opnext_ptr = NULL;
		car_ptr->qnext_ptr = NULL;
		car_ptr->request_ptr->mm.size = old_car_ptr->data.pkt.size;
		buf_ptr = car_ptr->buf_ptr = &old_car_ptr->request_ptr->mm.buf;
		buf_ptr->type = MM_TMP_BUFFER;
		buf_ptr->tmp.buf_ptr[0] = MPIU_Malloc(old_car_ptr->data.pkt.size);
		buf_ptr->tmp.buf_ptr[1] = NULL;
		buf_ptr->tmp.cur_buf = 0;
		buf_ptr->tmp.min_num_written = 0;
		buf_ptr->tmp.num_read = 0;

		/* enqueue the head car in the unexpected queue */
		if (MPID_Process.unex_q_tail == NULL)
		{
		    MPID_Process.unex_q_head = old_car_ptr;
		}
		else
		{
		    MPID_Process.unex_q_tail->qnext_ptr = old_car_ptr;
		}
		MPID_Process.unex_q_tail = old_car_ptr;

		/* post a read of the unexpected data */
		car_ptr->vc_ptr->post_read(car_ptr->vc_ptr, car_ptr);
		complete = FALSE;
	    }
	}

	if (complete)
	{
	    mm_dec_cc(car_ptr->request_ptr);
	    
	    /* free car */
	    old_car_ptr = car_ptr;
	    car_ptr = car_ptr->qnext_ptr;
	    mm_car_free(old_car_ptr);
	}
	else
	{
	    car_ptr = car_ptr->qnext_ptr;
	}
    }

    return MPI_SUCCESS;
}
