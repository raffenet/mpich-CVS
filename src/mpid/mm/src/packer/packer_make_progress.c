/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpidimpl.h"

/*@
   packer_make_progress - make progress

   Notes:
@*/
int packer_make_progress()
{
    int i;
    MM_Car *car_ptr, *car_tmp_ptr;
    MM_Segment_buffer *buf_ptr;
    BOOL finished;

    if (MPID_Process.packer_vc_ptr->readq_head == NULL &&
	MPID_Process.packer_vc_ptr->writeq_head == NULL)
    {
	/* shortcut out if the queues are empty */
	return MPI_SUCCESS;
    }

    for (i=0; i<2; i++)
    {
	if (i==0)
	    car_ptr = MPID_Process.packer_vc_ptr->readq_head;
	else
	    car_ptr = MPID_Process.packer_vc_ptr->writeq_head;
	
	while (car_ptr)
	{
	    finished = FALSE;
	    buf_ptr = car_ptr->buf_ptr;
	    switch (buf_ptr->type)
	    {
	    case MM_NULL_BUFFER:
		err_printf("error, cannot pack from a null buffer\n");
		break;
	    case MM_TMP_BUFFER:
		MPID_Segment_pack(
		    &car_ptr->request_ptr->mm.segment,       /* pack the segment in the request */
		    car_ptr->data.unpacker.first,            /* first and last are kept in the car */
		    &car_ptr->data.unpacker.last,
		    car_ptr->request_ptr->mm.buf.tmp.buf[car_ptr->request_ptr->mm.buf.tmp.cur_buf] /* pack into the current buffer */
		    );
		break;
	    case MM_VEC_BUFFER:
		if (car_ptr->buf_ptr->vec.num_cars_outstanding == 0)
		{
		    car_ptr->request_ptr->mm.get_buffers(car_ptr->request_ptr);
		    car_ptr->buf_ptr->vec.num_read = car_ptr->buf_ptr->vec.last - car_ptr->buf_ptr->vec.first;
		    car_ptr->buf_ptr->vec.num_cars_outstanding = car_ptr->buf_ptr->vec.num_cars;
		}
		if (car_ptr->buf_ptr->vec.last == car_ptr->request_ptr->mm.last)
		    finished = TRUE;
		break;
#ifdef WITH_METHOD_SHM
	    case MM_SHM_BUFFER:
		break;
#endif
#ifdef WITH_METHOD_VIA
	    case MM_VIA_BUFFER:
		break;
#endif
#ifdef WITH_METHOD_VIA_RDMA
	    case MM_VIA_RDMA_BUFFER:
		break;
#endif
#ifdef WITH_METHOD_NEW
	    case MM_NEW_METHOD_BUFFER:
		break;
#endif
	    default:
		err_printf("illegal buffer type: %d\n", car_ptr->request_ptr->mm.buf.type);
		break;
	    }
	    if (finished)
	    {
		car_tmp_ptr = car_ptr;
		car_ptr = car_ptr->qnext_ptr;
		packer_car_dequeue(MPID_Process.packer_vc_ptr, car_tmp_ptr);
		mm_cq_enqueue(car_tmp_ptr);
	    }
	    else
	    {
		car_ptr = car_ptr->qnext_ptr;
	    }
	}
    }

    return MPI_SUCCESS;
}
