/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpidimpl.h"

#ifdef WITH_METHOD_SHM
int unpacker_write_shm(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
#endif
#ifdef WITH_METHOD_VIA
int unpacker_write_via(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
#endif
#ifdef WITH_METHOD_VIA_RDMA
int unpacker_write_via_rdma(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
#endif
int unpacker_write_vec(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
int unpacker_write_tmp(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);

/*@
   unpacker_make_progress - make progress

   Notes:
@*/
int unpacker_make_progress()
{
    MM_Car *car_ptr, *car_next_ptr;
    MM_Segment_buffer *buf_ptr;
    MPIDI_VC *vc_ptr;

    if (MPID_Process.unpacker_vc_ptr->writeq_head == NULL)
    {
	/* shortcut out if the queue is empty */
	return MPI_SUCCESS;
    }

    vc_ptr = MPID_Process.unpacker_vc_ptr;
    car_ptr = MPID_Process.unpacker_vc_ptr->writeq_head;
    
    while (car_ptr)
    {
	car_next_ptr = car_ptr->qnext_ptr;
	buf_ptr = car_ptr->buf_ptr;
	switch (buf_ptr->type)
	{
#ifdef WITH_METHOD_SHM
	case MM_SHM_BUFFER:
	    unpacker_write_shm(vc_ptr, car_ptr, buf_ptr);
	    break;
#endif
#ifdef WITH_METHOD_VIA
	case MM_VIA_BUFFER:
	    unpacker_write_via(vc_ptr, car_ptr, buf_ptr);
	    break;
#endif
#ifdef WITH_METHOD_VIA_RDMA
	case MM_VIA_RDMA_BUFFER:
	    unpacker_write_via_rdma(vc_ptr, car_ptr, buf_ptr);
	    break;
#endif
	case MM_VEC_BUFFER:
	    unpacker_write_vec(vc_ptr, car_ptr, buf_ptr);
	    break;
	case MM_TMP_BUFFER:
	    unpacker_write_tmp(vc_ptr, car_ptr, buf_ptr);
	    break;
#ifdef WITH_METHOD_NEW
	case MM_NEW_METHOD_BUFFER:
	    unpacker_write_new(vc_ptr, car_ptr, buf_ptr);
	    break;
#endif
	case MM_NULL_BUFFER:
	    err_printf("error, cannot unpack from a null buffer\n");
	    break;
	default:
	    err_printf("illegal buffer type: %d\n", car_ptr->request_ptr->mm.buf.type);
	    break;
	}
	car_ptr = car_next_ptr;
    }
    
    return MPI_SUCCESS;
}

#ifdef WITH_METHOD_SHM
int unpacker_write_shm(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_VIA
int unpacker_write_via(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_VIA_RDMA
int unpacker_write_via_rdma(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    return MPI_SUCCESS;
}
#endif

int unpacker_write_vec(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    int num_written;
    
    /* this function assumes that buf_ptr->vec.num_cars_outstanding > 0 */

    if (car_ptr->data.unpacker.buf.vec_write.num_read_copy != buf_ptr->vec.num_read)
    {
	/* update num_read_copy and num_written */
	num_written = buf_ptr->vec.num_read - car_ptr->data.unpacker.buf.vec_write.num_read_copy;
	car_ptr->data.unpacker.buf.vec_write.num_read_copy = buf_ptr->vec.num_read;

	/* update vector */
	car_ptr->data.unpacker.buf.vec_write.cur_num_written += num_written;
	car_ptr->data.unpacker.buf.vec_write.total_num_written += num_written;
	if (car_ptr->data.unpacker.buf.vec_write.cur_num_written == buf_ptr->vec.buf_size)
	{
	    /* reset this car */
	    car_ptr->data.unpacker.buf.vec_write.num_read_copy = 0;
	    car_ptr->data.unpacker.buf.vec_write.cur_num_written = 0;
	    /* signal that we have finished writing the current vector */
	    mm_dec_atomic(&(buf_ptr->vec.num_cars_outstanding));
	}
    }
    
    /* if the entire mpi segment has been written, enqueue the car in the completion queue */
    if (car_ptr->data.unpacker.buf.vec_write.total_num_written == buf_ptr->vec.segment_last)
    {
	unpacker_car_dequeue(car_ptr->vc_ptr, car_ptr);
	mm_cq_enqueue(car_ptr);
    }

    return MPI_SUCCESS;
}

int unpacker_write_tmp(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    if ((car_ptr->data.unpacker.buf.tmp.last == buf_ptr->tmp.num_read) || (buf_ptr->tmp.buf == NULL))
    {
	/* no new data available or
	 * no buffer provided by the reader */
	return MPI_SUCCESS;
    }
    /* set the last variable to the number of bytes read */
    car_ptr->data.unpacker.buf.tmp.last = buf_ptr->tmp.num_read;
    /* unpack the buffer */
    MPID_Segment_unpack(
	&car_ptr->request_ptr->mm.segment,
	car_ptr->data.unpacker.buf.tmp.first,
	&car_ptr->data.unpacker.buf.tmp.last,
	buf_ptr->tmp.buf
	);
    
    if (car_ptr->data.packer.last == car_ptr->request_ptr->mm.last)
    {
	/* the entire buffer is unpacked */
	unpacker_car_dequeue(car_ptr->vc_ptr, car_ptr);
	mm_cq_enqueue(car_ptr);
	return MPI_SUCCESS;
    }

    /* There is more unpacking needed so update the first variable */
    /* The last variable will be updated the next time through this function */
    car_ptr->data.unpacker.buf.tmp.first = car_ptr->data.unpacker.buf.tmp.last;

    return MPI_SUCCESS;
}
