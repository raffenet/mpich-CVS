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
    int ret_val;

    MM_ENTER_FUNC(TCP_WRITE);

    if (!vc_ptr->data.tcp.connected)
    {
	MM_EXIT_FUNC(TCP_WRITE);
	return MPI_SUCCESS;
    }

    if (vc_ptr->writeq_head == NULL)
    {
	msg_printf("tcp_write: write called with no vc's in the write queue.\n");
	MM_EXIT_FUNC(TCP_WRITE);
	return MPI_SUCCESS;
    }

    car_ptr = vc_ptr->writeq_head;
    buf_ptr = car_ptr->buf_ptr;

    switch (buf_ptr->type)
    {
#ifdef WITH_METHOD_SHM
    case MM_SHM_BUFFER:
	ret_val = tcp_write_shm(vc_ptr, car_ptr, buf_ptr);
	MM_EXIT_FUNC(TCP_WRITE);
	return ret_val;
	break;
#endif
#ifdef WITH_METHOD_VIA
    case MM_VIA_BUFFER:
	ret_val = tcp_write_via(vc_ptr, car_ptr, buf_ptr);
	MM_EXIT_FUNC(TCP_WRITE);
	return ret_val;
	break;
#endif
#ifdef WITH_METHOD_VIA_RDMA
    case MM_VIA_RDMA_BUFFER:
	ret_val = tcp_write_via_rdma(vc_ptr, car_ptr, buf_ptr);
	MM_EXIT_FUNC(TCP_WRITE);
	return ret_val;
	break;
#endif
    case MM_VEC_BUFFER:
	ret_val = (buf_ptr->vec.num_cars_outstanding > 0) ? tcp_write_vec(vc_ptr, car_ptr, buf_ptr) : MPI_SUCCESS;
	MM_EXIT_FUNC(TCP_WRITE);
	return ret_val;
	break;
    case MM_TMP_BUFFER:
	ret_val = tcp_write_tmp(vc_ptr, car_ptr, buf_ptr);
	MM_EXIT_FUNC(TCP_WRITE);
	return ret_val;
	break;
#ifdef WITH_METHOD_NEW
    case MM_NEW_METHOD_BUFFER:
	ret_val = tcp_write_new(vc_ptr, car_ptr, buf_ptr);
	MM_EXIT_FUNC(TCP_WRITE);
	return ret_val;
	break;
#endif
    case MM_NULL_BUFFER:
	err_printf("Error: tcp_write called on a null buffer\n");
	break;
    default:
	err_printf("Error: tcp_write: unknown or unsupported buffer type: %d\n", buf_ptr->type);
	break;
    }

    MM_EXIT_FUNC(TCP_WRITE);
    return -1;
}

#ifdef WITH_METHOD_SHM
int tcp_write_shm(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MM_ENTER_FUNC(TCP_WRITE_SHM);
    MM_EXIT_FUNC(TCP_WRITE_SHM);
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_VIA
int tcp_write_via(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MM_ENTER_FUNC(TCP_WRITE_VIA);
    MM_EXIT_FUNC(TCP_WRITE_VIA);
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_VIA_RDMA
int tcp_write_via_rdma(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MM_ENTER_FUNC(TCP_WRITE_VIA_RDMA);
    MM_EXIT_FUNC(TCP_WRITE_VIA_RDMA);
    return MPI_SUCCESS;
}
#endif

int tcp_write_vec(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    int num_written;
    int cur_index;
    MPID_VECTOR *car_vec, *buf_vec;
    int num_left, i;
    
    MM_ENTER_FUNC(TCP_WRITE_VEC);

#ifdef MPICH_DEV_BUILD
    /* this function assumes that buf_ptr->vec.num_cars_outstanding > 0 */
    if (buf_ptr->vec.num_cars_outstanding == 0)
    {
	err_printf("Error: tcp_write_vec called when num_cars_outstanding == 0.\n");
	return -1;
    }
#endif

    /* num_read_copy is the amount of data described by the vector in car_ptr->data.tcp.buf.vec_write */
    /* num_read is the amount of data described by the vector in buf_ptr->vec */
    /* If they are not equal then this car needs to be updated */
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

	/* modify the vector copied from buf_ptr->vec to represent only the data that has been read 
	 * This is done by traversing the vector, subtracting the lengths of each buffer until all the read
	 * data is accounted for.
	 */

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
	update the length field in the car's copy of the vector to represent only the read data */
	if (num_left < 0)
	{
	    car_vec[i].MPID_VECTOR_LEN += num_left;
	}
	
	/* at this point the vec in the car describes all the currently read data */
    }

    /* num_read_copy is the amount of data described by the vector in car_ptr->data.tcp.buf.vec_write */
    /* cur_num_written is the amount of data in this car that has already been written */
    /* If they are not equal then there is data available to be written */
    if (car_ptr->data.tcp.buf.vec_write.cur_num_written < car_ptr->data.tcp.buf.vec_write.num_read_copy)
    {
	/* write */
	if (car_ptr->data.tcp.buf.vec_write.vec_size == 1) /* optimization for single buffer writes */
	{
	    MM_ENTER_FUNC(BWRITE);
	    num_written = bwrite(vc_ptr->data.tcp.bfd, 
		car_ptr->data.tcp.buf.vec_write.vec[car_ptr->data.tcp.buf.vec_write.cur_index].MPID_VECTOR_BUF,
		car_ptr->data.tcp.buf.vec_write.vec[car_ptr->data.tcp.buf.vec_write.cur_index].MPID_VECTOR_LEN);
	    MM_EXIT_FUNC(BWRITE);
	    if (num_written == SOCKET_ERROR)
	    {
		TCP_Process.error = beasy_getlasterror();
		beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
		err_printf("tcp_write: bwrite failed, error %d: %s\n", TCP_Process.error, TCP_Process.err_msg);
		MM_EXIT_FUNC(TCP_WRITE_VEC);
		return -1;
	    }
	}
	else
	{
	    MM_ENTER_FUNC(BWRITEV);
	    num_written = bwritev(
		vc_ptr->data.tcp.bfd, 
		&car_ptr->data.tcp.buf.vec_write.vec[car_ptr->data.tcp.buf.vec_write.cur_index], 
		car_ptr->data.tcp.buf.vec_write.vec_size);
	    MM_EXIT_FUNC(BWRITEV);
	    if (num_written == SOCKET_ERROR)
	    {
		TCP_Process.error = beasy_getlasterror();
		beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
		err_printf("tcp_write: bwritev failed, error %d: %s\n", TCP_Process.error, TCP_Process.err_msg);
		MM_EXIT_FUNC(TCP_WRITE_VEC);
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
		    car_ptr->data.tcp.buf.vec_write.vec[i].MPID_VECTOR_BUF = 
			car_ptr->data.tcp.buf.vec_write.vec[i].MPID_VECTOR_BUF +
			car_ptr->data.tcp.buf.vec_write.vec[i].MPID_VECTOR_LEN +
			num_left;
		    car_ptr->data.tcp.buf.vec_write.vec[i].MPID_VECTOR_LEN = -num_left;
		}
	    }
	    car_ptr->data.tcp.buf.vec_write.cur_index = i;
	}
    }
    
    /* if the entire mpi segment has been written, enqueue the car in the completion queue */
    if (car_ptr->data.tcp.buf.vec_write.total_num_written == buf_ptr->vec.segment_last)
    {
#ifdef MPICH_DEV_BUILD
	if (car_ptr != car_ptr->vc_ptr->writeq_head)
	{
	    err_printf("Error: tcp_write_vec not dequeueing the head write car.\n");
	}
#endif
	tcp_car_dequeue_write(car_ptr->vc_ptr);
	car_ptr->next_ptr = NULL; /* prevent the next car from being enqueued by cq_handle_write_car() */
	mm_cq_enqueue(car_ptr);
    }

    MM_EXIT_FUNC(TCP_WRITE_VEC);
    return MPI_SUCCESS;
}

int tcp_write_tmp(MPIDI_VC *vc_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    int num_written;

    MM_ENTER_FUNC(TCP_WRITE_TMP);

    if ((car_ptr->data.tcp.buf.tmp.num_written == buf_ptr->tmp.num_read) || (buf_ptr->tmp.num_read == 0))
    {
	return MPI_SUCCESS;
    }

    /* write as much as possible */
    MM_ENTER_FUNC(BWRITE);
    num_written = bwrite(vc_ptr->data.tcp.bfd, 
	(char*)(buf_ptr->tmp.buf) + car_ptr->data.tcp.buf.tmp.num_written,
	buf_ptr->tmp.num_read - car_ptr->data.tcp.buf.tmp.num_written);
    MM_EXIT_FUNC(BWRITE);
    if (num_written == SOCKET_ERROR)
    {
	err_printf("tcp_write_tmp:bread failed, error %d\n", beasy_getlasterror());
    }
    /* update the amount written */
    car_ptr->data.tcp.buf.tmp.num_written += num_written;

    /* check to see if finished */
    if (car_ptr->data.tcp.buf.tmp.num_written == buf_ptr->tmp.len)
    {
	dbg_printf("num_written: %d\n", car_ptr->data.tcp.buf.tmp.num_written);
	/* remove from write queue and insert in completion queue */
#ifdef MPICH_DEV_BUILD
	if (car_ptr != vc_ptr->writeq_head)
	{
	    err_printf("Error: tcp_write_tmp not dequeueing the head write car.\n");
	}
#endif
	tcp_car_dequeue_write(vc_ptr);
	car_ptr->next_ptr = NULL; /* prevent the next car from being enqueued by cq_handle_write_car() */
	mm_cq_enqueue(car_ptr);
    }

    MM_EXIT_FUNC(TCP_WRITE_TMP);
    return MPI_SUCCESS;
}
