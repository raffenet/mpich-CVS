/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "tcpimpl.h"

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifdef WITH_METHOD_SHM
int tcp_merge_shm(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length);
#endif
#ifdef WITH_METHOD_VIA
int tcp_merge_via(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length);
#endif
#ifdef WITH_METHOD_VIA_RDMA
int tcp_merge_via_rdma(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length);
#endif
int tcp_merge_vec(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length);
int tcp_merge_tmp(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length);

int tcp_merge_unexpected_data(MPIDI_VC *vc_ptr, MM_Car *car_ptr, char *buffer, int length)
{
    int ret_val;
    MM_Segment_buffer *buf_ptr;

    MM_ENTER_FUNC(TCP_MERGE_UNEXPECTED_DATA);

    if (vc_ptr->data.tcp.connecting)
    {
	if (tcp_read_connecting(vc_ptr) != MPI_SUCCESS)
	{
	    err_printf("Error:tcp_merge_unexpected_data: tcp_read_connecting failed.\n");
	}
	MM_EXIT_FUNC(TCP_MERGE_UNEXPECTED_DATA);
	return 0;
    }

    assert(car_ptr->type & MM_READ_CAR);

    buf_ptr = car_ptr->buf_ptr;
    switch (buf_ptr->type)
    {
    case MM_VEC_BUFFER:
	ret_val = tcp_merge_vec(vc_ptr, car_ptr, buf_ptr, buffer, length);
	MM_EXIT_FUNC(TCP_MERGE_UNEXPECTED_DATA);
	return ret_val;
	break;
    case MM_TMP_BUFFER:
	ret_val = tcp_merge_tmp(vc_ptr, car_ptr, buf_ptr, buffer, length);
	MM_EXIT_FUNC(TCP_MERGE_UNEXPECTED_DATA);
	return ret_val;
	break;
#ifdef WITH_METHOD_SHM
    case MM_SHM_BUFFER:
	ret_val = tcp_merge_shm(vc_ptr, car_ptr, buf_ptr, buffer, length);
	MM_EXIT_FUNC(TCP_MERGE_UNEXPECTED_DATA);
	return ret_val;
	break;
#endif
#ifdef WITH_METHOD_VIA
    case MM_VIA_BUFFER:
	ret_val = tcp_merge_via(vc_ptr, car_ptr, buf_ptr, buffer, length);
	MM_EXIT_FUNC(TCP_MERGE_UNEXPECTED_DATA);
	return ret_val;
	break;
#endif
#ifdef WITH_METHOD_VIA_RDMA
    case MM_VIA_RDMA_BUFFER:
	ret_val = tcp_merge_via_rdma(vc_ptr, car_ptr, buf_ptr, buffer, length);
	MM_EXIT_FUNC(TCP_MERGE_UNEXPECTED_DATA);
	return ret_val;
	break;
#endif
#ifdef WITH_METHOD_NEW
    case MM_NEW_METHOD_BUFFER:
	ret_val = tcp_merge_new(vc_ptr, car_ptr, buf_ptr, buffer, length);
	MM_EXIT_FUNC(TCP_MERGE_UNEXPECTED_DATA);
	return ret_val;
	break;
#endif
    case MM_NULL_BUFFER:
	err_printf("Error: tcp_merge_unexpected_data called on a null MM_Segment_buffer\n");
	break;
    default:
	err_printf("Error: tcp_merge_unexpected_data: unknown or unsupported buffer type: %d\n", car_ptr->buf_ptr->type);
	break;
    }

    MM_EXIT_FUNC(TCP_MERGE_UNEXPECTED_DATA);
    return length;
}

#ifdef WITH_METHOD_SHM
int tcp_merge_shm(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length)
{
    MM_ENTER_FUNC(TCP_MERGE_SHM);
    MM_EXIT_FUNC(TCP_MERGE_SHM);
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_VIA
int tcp_merge_via(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length)
{
    MM_ENTER_FUNC(TCP_MERGE_VIA);
    MM_EXIT_FUNC(TCP_MERGE_VIA);
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_VIA_RDMA
int tcp_merge_via_rdma(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length)
{
    MM_ENTER_FUNC(TCP_MERGE_VIA_RDMA);
    MM_EXIT_FUNC(TCP_MERGE_VIA_RDMA);
    return MPI_SUCCESS;
}
#endif

int tcp_merge_vec(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length)
{
    int num_read = 0;
    int num_left, i;

    MM_ENTER_FUNC(TCP_MERGE_VEC);

    if (buf_ptr->vec.num_cars_outstanding == 0)
    {
	/* get more buffers */
	car_ptr->request_ptr->mm.get_buffers(car_ptr->request_ptr);
	/* reset the progress structures in the car */
	car_ptr->data.tcp.buf.vec_write.cur_index = 0;
	car_ptr->data.tcp.buf.vec_write.num_read_copy = 0;
	car_ptr->data.tcp.buf.vec_write.cur_num_written = 0;
	car_ptr->data.tcp.buf.vec_write.num_written_at_cur_index = 0;
	car_ptr->data.tcp.buf.vec_write.vec_size = 0;
	/* copy the vector from the buffer to the car */
	memcpy(car_ptr->data.tcp.buf.vec_read.vec,
	    buf_ptr->vec.vec,
	    buf_ptr->vec.vec_size * sizeof(MPID_VECTOR));
	car_ptr->data.tcp.buf.vec_read.vec_size = buf_ptr->vec.vec_size;
	buf_ptr->vec.num_read = 0;
	/* reset the number of outstanding write cars */
	buf_ptr->vec.num_cars_outstanding = buf_ptr->vec.num_cars;
    }
    
    if (car_ptr->data.tcp.buf.vec_read.cur_num_read < buf_ptr->vec.buf_size)
    {
	/* read */
	num_read = min(length, buf_ptr->vec.segment_last); /* This is incorrect because the segment may not fit in the current vector */
	
	/* update vectors */
	buf_ptr->vec.num_read += num_read;
	car_ptr->data.tcp.buf.vec_read.cur_num_read += num_read;
	car_ptr->data.tcp.buf.vec_read.total_num_read += num_read;
	if (car_ptr->data.tcp.buf.vec_read.cur_num_read == buf_ptr->vec.buf_size)
	{
	    /* reset the car */
	    car_ptr->data.tcp.buf.vec_read.cur_index = 0;
	    car_ptr->data.tcp.buf.vec_read.cur_num_read = 0;
	    car_ptr->data.tcp.buf.vec_read.num_read_at_cur_index = 0;
	    car_ptr->data.tcp.buf.vec_read.vec_size = 0;
	}
	else
	{
	    num_left = num_read;
	    i = car_ptr->data.tcp.buf.vec_read.cur_index;
	    while (num_left > 0)
	    {
		num_left -= car_ptr->data.tcp.buf.vec_read.vec[i].MPID_VECTOR_LEN;
		if (num_left > 0)
		{
		    i++;
		}
		else
		{
		    car_ptr->data.tcp.buf.vec_read.vec[i].MPID_VECTOR_BUF = 
			car_ptr->data.tcp.buf.vec_read.vec[i].MPID_VECTOR_BUF +
			car_ptr->data.tcp.buf.vec_read.vec[i].MPID_VECTOR_LEN +
			num_left;
		    car_ptr->data.tcp.buf.vec_read.num_read_at_cur_index = 
			car_ptr->data.tcp.buf.vec_read.vec[i].MPID_VECTOR_LEN + num_left;
		    car_ptr->data.tcp.buf.vec_read.vec[i].MPID_VECTOR_LEN -= num_left;
		}
	    }
	    car_ptr->data.tcp.buf.vec_read.cur_index = i;
	}
    }
    
    if (car_ptr->data.tcp.buf.vec_read.total_num_read == buf_ptr->vec.segment_last)
    {
	tcp_car_dequeue(vc_ptr, car_ptr);
	mm_cq_enqueue(car_ptr);
    }
    else
    {
	msg_printf("total_num_read %d, segment_last %d\n", car_ptr->data.tcp.buf.vec_read.total_num_read, buf_ptr->vec.segment_last);
	/* somehow save the extra data because it must be completely read or it will be lost */
	err_printf("Error: tcp_merge_vec: data lost.\n");
    }

    MM_EXIT_FUNC(TCP_MERGE_VEC);
    return num_read;
}

int tcp_merge_tmp(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr, char *buffer, int length)
{
    int num_read;

    MM_ENTER_FUNC(TCP_MERGE_TMP);

    if (buf_ptr->tmp.buf == NULL)
    {
	/* get the tmp buffer */
	car_ptr->request_ptr->mm.get_buffers(car_ptr->request_ptr);
    }

    /* read as much as possible */
    num_read = min(length, buf_ptr->tmp.len);
    /* update the amount read */
    buf_ptr->tmp.num_read += num_read;

    /* check to see if finished */
    if (buf_ptr->tmp.num_read == buf_ptr->tmp.len)
    {
	dbg_printf("num_read: %d\n", buf_ptr->tmp.num_read);
	/* remove from read queue and insert in completion queue */
	tcp_car_dequeue(vc_ptr, car_ptr);
	mm_cq_enqueue(car_ptr);
    }

    MM_EXIT_FUNC(TCP_MERGE_TMP);
    return MPI_SUCCESS;
}
