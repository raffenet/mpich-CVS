/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
*/

#include "mpidimpl.h"

/*@
   mm_unpacker_write - unpack

   Notes:
@*/
int mm_unpacker_write()
{
    MM_Car *car_ptr;
    car_ptr = MPID_Process.unpkr_write_list;
    while (car_ptr)
    {
	switch (car_ptr->request_ptr->mm.buf.type)
	{
	case MM_NULL_BUFFER:
	    err_printf("error, cannot unpack from a null buffer\n");
	    break;
	case MM_TMP_BUFFER:
	    MPID_Segment_unpack(
		&car_ptr->request_ptr->mm.segment, /* unpack the segment in the request */
		car_ptr->request_ptr->mm.buf.tmp.cur_buf, /* unpack from the read buffer */
		&car_ptr->data.unpacker.first, /* first and last are kept in the car */
		&car_ptr->data.unpacker.last);
	    /* update first and last */
	    /* update min_num_written */
	    break;
	case MM_VEC_BUFFER:
	    MPID_Segment_unpack_vector(
		&car_ptr->request_ptr->mm.segment,
		car_ptr->data.unpacker.first,
		&car_ptr->data.unpacker.last,
		car_ptr->request_ptr->mm.buf.vec.vec,
		&car_ptr->request_ptr->mm.buf.vec.size);
	    /* update first and last */
	    /* update min_num_written */
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
	car_ptr = car_ptr->qnext_ptr;
    }

    return 0;
}
