/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int mm_cq_test()
{
    MM_Car *car_ptr, *old_car_ptr;

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
	/* handle completed car */
	if (car_ptr->type & MM_UNEX_HEAD_CAR)
	{
	    /* find in posted_q */
	    /* else allocate a temp buffer, place in the unex_q, and post a read */
	}
	mm_dec_cc(car_ptr->request_ptr);

	/* free car */
	old_car_ptr = car_ptr;
	car_ptr = car_ptr->qnext_ptr;
	mm_car_free(old_car_ptr);
    }

    return MPI_SUCCESS;
}
