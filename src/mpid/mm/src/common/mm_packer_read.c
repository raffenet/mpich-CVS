/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

static void remove_car(MM_Car **list_pptr, MM_Car *car_ptr)
{
    MM_Car *iter_ptr, *trailer_ptr, *list_ptr;

    list_ptr = *list_pptr;
    if (car_ptr == list_ptr)
    {
	*list_pptr = list_ptr->qnext_ptr;
    }
    else
    {
	trailer_ptr = list_ptr;
	iter_ptr = list_ptr->qnext_ptr;
	while (iter_ptr != car_ptr)
	{
	    trailer_ptr = trailer_ptr->qnext_ptr;
	    iter_ptr = iter_ptr->qnext_ptr;
	}
	trailer_ptr->qnext_ptr = iter_ptr->qnext_ptr;
    }
}

/*@
   mm_packer_read - read

   Notes:
@*/
int mm_packer_read()
{
    MM_Car *car_ptr, *car_tmp_ptr;
    MM_Segment_buffer *buf_ptr;
    BOOL finished;

    car_ptr = MPID_Process.pkr_read_list;
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
		&car_ptr->request_ptr->mm.segment, /* unpack the segment in the request */
		car_ptr->request_ptr->mm.buf.tmp.cur_buf, /* unpack from the read buffer */
		&car_ptr->data.unpacker.first, /* first and last are kept in the car */
		&car_ptr->data.unpacker.last);
	    /* update first and last */
	    /* update min_num_written */
	    break;
	case MM_VEC_BUFFER:
	    /*
	    MPID_Segment_pack_vector(
		&car_ptr->request_ptr->mm.segment,
		car_ptr->data.unpacker.first,
		&car_ptr->data.unpacker.last,
		car_ptr->request_ptr->mm.buf.vec.vec,
		&car_ptr->request_ptr->mm.buf.vec.size);
		*/
	    /* update first and last */
	    /* update min_num_written */
	    buf_ptr->vec.vec[0].MPID_VECTOR_BUF = (void*)car_ptr->request_ptr->mm.user_buf.send;
	    buf_ptr->vec.vec[0].MPID_VECTOR_LEN = car_ptr->request_ptr->mm.size;
	    buf_ptr->vec.vec_size = 1;
	    buf_ptr->vec.num_read = 0;
	    buf_ptr->vec.min_num_written = 0;
	    buf_ptr->vec.local_last = 0;
	    buf_ptr->vec.msg_size = 0;
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
	    remove_car(&MPID_Process.pkr_read_list, car_tmp_ptr);
	    mm_cq_enqueue(car_tmp_ptr);
	}
	else
	{
	    car_ptr = car_ptr->qnext_ptr;
	}
    }

    return 0;
}
