/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "tcpimpl.h"

#ifdef WITH_METHOD_SHM
int tcp_write_shm(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
#endif
#ifdef WITH_METHOD_VIA
int tcp_write_via(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
#endif
#ifdef WITH_METHOD_VIA_RDMA
int tcp_write_via_rdma(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
#endif
int tcp_write_vec(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);
int tcp_write_tmp(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr);

int tcp_write(MPIDI_VC *vc_ptr)
{
    MM_Car *car_ptr;
    MM_Segment_buffer *buf_ptr;

    if (!vc_ptr->data.tcp.connected)
	return MPI_SUCCESS;

    if (vc_ptr->writeq_head == NULL)
	return MPI_SUCCESS;

    car_ptr = vc_ptr->writeq_head;
    buf_ptr = car_ptr->buf_ptr;

    switch (buf_ptr->type)
    {
#ifdef WITH_METHOD_SHM
    case MM_SHM_BUFFER:
	return tcp_write_shm(vc_ptr, car_ptr, buf_ptr);
	break;
#endif
#ifdef WITH_METHOD_VIA
    case MM_VIA_BUFFER:
	return tcp_write_via(vc_ptr, car_ptr, buf_ptr);
	break;
#endif
#ifdef WITH_METHOD_VIA_RDMA
    case MM_VIA_RDMA_BUFFER:
	return tcp_write_via_rdma(vc_ptr, car_ptr, buf_ptr);
	break;
#endif
    case MM_VEC_BUFFER:
	return (buf_ptr->vec.num_cars_outstanding > 0) ? tcp_write_vec(vc_ptr, car_ptr, buf_ptr) : MPI_SUCCESS;
	break;
    case MM_TMP_BUFFER:
	return tcp_write_tmp(vc_ptr, car_ptr, buf_ptr);
	break;
#ifdef WITH_METHOD_NEW
    case MM_NEW_METHOD_BUFFER:
	return tcp_write_new(vc_ptr, car_ptr, buf_ptr);
	break;
#endif
    case MM_NULL_BUFFER:
	err_printf("Error: tcp_write called on a null buffer\n");
	break;
    default:
	err_printf("Error: tcp_write: unknown or unsupported buffer type: %d\n", car_ptr->buf_ptr->type);
	break;
    }

    return -1;
}

#ifdef WITH_METHOD_SHM
int tcp_write_shm(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_VIA
int tcp_write_via(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_VIA_RDMA
int tcp_write_via_rdma(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    return MPI_SUCCESS;
}
#endif

int tcp_write_vec(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    int num_written;
    int cur_index;
    MPID_VECTOR *car_vec, *buf_vec;
    int num_left, i;
    
    /* this function assumes that buf_ptr->vec.num_cars_outstanding > 0 */

    if (car_ptr->data.tcp.buf.vec_write.num_read_copy != buf_ptr->vec.num_read)
    {
	/* update vector */
	cur_index = car_ptr->data.tcp.buf.vec_write.cur_index;
	car_vec = car_ptr->data.tcp.buf.vec_write.vec;
	buf_vec = buf_ptr->vec.vec;
	
	/* update num_read_copy */
	car_ptr->data.tcp.buf.vec_write.num_read_copy = buf_ptr->vec.num_read;
	
	/* copy the buf vector into the car vector from the current index to the end */
	memcpy(&car_vec[cur_index], &buf_vec[cur_index], 
	    (buf_ptr->vec.vec_size - cur_index) * sizeof(MPID_VECTOR));
	car_vec[cur_index].MPID_VECTOR_BUF = 
	    (char*)car_vec[cur_index].MPID_VECTOR_BUF + car_ptr->data.tcp.buf.vec_write.num_written_at_cur_index;
	car_vec[cur_index].MPID_VECTOR_LEN = car_vec[cur_index].MPID_VECTOR_LEN - car_ptr->data.tcp.buf.vec_write.num_written_at_cur_index;
	
	/* set the size of the car vector to zero */
	car_ptr->data.tcp.buf.vec_write.vec_size = 0;
	
	/* add vector elements to the size until all the read data is accounted for */
	num_left = car_ptr->data.tcp.buf.vec_write.num_read_copy - car_ptr->data.tcp.buf.vec_write.cur_num_written;
	i = cur_index;
	while (num_left > 0)
	{
	    car_ptr->data.tcp.buf.vec_write.vec_size++;
	    num_left -= car_vec[i].MPID_VECTOR_LEN;
	    i++;
	}
	/* if the last vector buffer is larger than the amount of data read into that buffer,
	update the length field in the car's copy of the vector */
	if (num_left < 0)
	{
	    car_vec[i].MPID_VECTOR_LEN += num_left;
	}
	
	/* at this point the vec in the car describes all the currently read data */
    }
    
    if (car_ptr->data.tcp.buf.vec_write.cur_num_written < car_ptr->data.tcp.buf.vec_write.num_read_copy)
    {
	/* write */
	if (car_ptr->data.tcp.buf.vec_write.vec_size == 1) /* optimization for single buffer writes */
	{
	    num_written = bwrite(vc_ptr->data.tcp.bfd, 
		car_ptr->data.tcp.buf.vec_write.vec[car_ptr->data.tcp.buf.vec_write.cur_index].MPID_VECTOR_BUF,
		car_ptr->data.tcp.buf.vec_write.vec[car_ptr->data.tcp.buf.vec_write.cur_index].MPID_VECTOR_LEN);
	    if (num_written == SOCKET_ERROR)
	    {
		TCP_Process.error = beasy_getlasterror();
		beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
		err_printf("tcp_write: bwrite failed, error %d: %s\n", TCP_Process.error, TCP_Process.err_msg);
		return -1;
	    }
	}
	else
	{
	    num_written = bwritev(
		vc_ptr->data.tcp.bfd, 
		&car_ptr->data.tcp.buf.vec_write.vec[car_ptr->data.tcp.buf.vec_write.cur_index], 
		car_ptr->data.tcp.buf.vec_write.vec_size);
	    if (num_written == SOCKET_ERROR)
	    {
		TCP_Process.error = beasy_getlasterror();
		beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
		err_printf("tcp_write: bwritev failed, error %d: %s\n", TCP_Process.error, TCP_Process.err_msg);
		return -1;
	    }
	}
	
	/* update vector */
	car_ptr->data.tcp.buf.vec_write.cur_num_written += num_written;
	car_ptr->data.tcp.buf.vec_write.total_num_written += num_written;
	if (car_ptr->data.tcp.buf.vec_write.cur_num_written == buf_ptr->vec.buf_size)
	{
	    /* reset this car */
	    car_ptr->data.tcp.buf.vec_write.cur_index = 0;
	    car_ptr->data.tcp.buf.vec_write.num_read_copy = 0;
	    car_ptr->data.tcp.buf.vec_write.cur_num_written = 0;
	    car_ptr->data.tcp.buf.vec_write.num_written_at_cur_index = 0;
	    car_ptr->data.tcp.buf.vec_write.vec_size = 0;
	    /* signal that we have finished writing the current vector */
	    mm_dec_atomic(&(buf_ptr->vec.num_cars_outstanding));
	}
	else
	{
	    num_left = num_written;
	    i = car_ptr->data.tcp.buf.vec_write.cur_index;
	    while (num_left > 0)
	    {
		num_left -= car_ptr->data.tcp.buf.vec_write.vec[i].MPID_VECTOR_LEN;
		if (num_left > 0)
		{
		    i++;
		}
		else
		{
		    car_ptr->data.tcp.buf.vec_write.vec[i].MPID_VECTOR_BUF = car_ptr->data.tcp.buf.vec_write.vec[i].MPID_VECTOR_BUF - num_left;
		    car_ptr->data.tcp.buf.vec_write.vec[i].MPID_VECTOR_LEN += num_left;
		}
	    }
	    car_ptr->data.tcp.buf.vec_write.cur_index = i;
	}
    }
    
    /* if the entire mpi segment has been written, enqueue the car in the completion queue */
    if (car_ptr->data.tcp.buf.vec_write.total_num_written == buf_ptr->vec.segment_last)
    {
	tcp_car_dequeue(car_ptr->vc_ptr, car_ptr);
	mm_cq_enqueue(car_ptr);
    }

    return MPI_SUCCESS;
}

int tcp_write_tmp(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    return MPI_SUCCESS;
}
