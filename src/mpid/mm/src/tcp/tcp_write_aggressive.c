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
int tcp_stuff_vector_shm(MPID_VECTOR *vec, int *cur_pos, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MM_ENTER_FUNC(TCP_STUFF_VECTOR_SHM);
    MM_EXIT_FUNC(TCP_STUFF_VECTOR_SHM);
    return FALSE;
}
#endif

#ifdef WITH_METHOD_VIA
int tcp_stuff_vector_via(MPID_VECTOR *vec, int *cur_pos, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MM_ENTER_FUNC(TCP_STUFF_VECTOR_VIA);
    MM_EXIT_FUNC(TCP_STUFF_VECTOR_VIA);
    return FALSE;
}
#endif

#ifdef WITH_METHOD_VIA_RDMA
int tcp_stuff_vector_via_rdma(MPID_VECTOR *vec, int *cur_pos, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MM_ENTER_FUNC(TCP_STUFF_VECTOR_VIA_RDMA);
    MM_EXIT_FUNC(TCP_STUFF_VECTOR_VIA_RDMA);
    return FALSE;
}
#endif

int tcp_stuff_vector_vec(MPID_VECTOR *vec, int *cur_pos_ptr, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    int cur_pos, cur_index, num_avail, final_segment;
    MM_ENTER_FUNC(TCP_STUFF_VECTOR_VEC);

    /* check to see that there is data available and space in the vector to put it */
    if ((*cur_pos_ptr == MPID_VECTOR_LIMIT) ||
        (car_ptr->data.tcp.buf.vec_write.num_read_copy == buf_ptr->vec.num_read))
    {
	MM_EXIT_FUNC(TCP_STUFF_VECTOR_TMP);
	return FALSE;
    }
    
    cur_pos = *cur_pos_ptr;
    cur_index = car_ptr->data.tcp.buf.vec_write.cur_index;
    num_avail = buf_ptr->vec.num_read - car_ptr->data.tcp.buf.vec_write.cur_num_written;
    final_segment = (num_avail + car_ptr->data.tcp.buf.vec_write.total_num_written) == buf_ptr->vec.segment_last;

    vec[cur_pos].MPID_VECTOR_BUF = 
	(char*)buf_ptr->vec.vec[cur_index].MPID_VECTOR_BUF + car_ptr->data.tcp.buf.vec_write.num_written_at_cur_index;
    vec[cur_pos].MPID_VECTOR_LEN = buf_ptr->vec.vec[cur_index].MPID_VECTOR_LEN - car_ptr->data.tcp.buf.vec_write.num_written_at_cur_index;
    num_avail -= vec[cur_pos].MPID_VECTOR_LEN;
    cur_pos++;
    cur_index++;

    while ((cur_pos < MPID_VECTOR_LIMIT) && num_avail)
    {
	vec[cur_pos].MPID_VECTOR_BUF = buf_ptr->vec.vec[cur_index].MPID_VECTOR_BUF;
	vec[cur_pos].MPID_VECTOR_LEN = buf_ptr->vec.vec[cur_index].MPID_VECTOR_LEN;
	num_avail -= buf_ptr->vec.vec[cur_index].MPID_VECTOR_LEN;
	cur_index++;
	cur_pos++;
    }
    *cur_pos_ptr = cur_pos;

    MM_EXIT_FUNC(TCP_STUFF_VECTOR_TMP);
    return (num_avail == 0 && final_segment)
    /*
    if (buf_ptr->vec.vec_size == 1)
    {
	car_ptr->data.tcp.buf.vec_write.num_read_copy = buf_ptr->vec.num_read;
	vec[*cur_pos].MPID_VECTOR_BUF = buf_ptr->vec.vec[0].MPID_VECTOR_BUF;
	vec[*cur_pos].MPID_VECTOR_LEN = buf_ptr->vec.vec[0].MPID_VECTOR_LEN;
	(*cur_pos)++;
	MM_EXIT_FUNC(TCP_STUFF_VECTOR_TMP);
	return TRUE;
    }
    */
#ifdef FOO
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
#endif
    MM_EXIT_FUNC(TCP_STUFF_VECTOR_VEC);
    return FALSE;
}

int tcp_stuff_vector_tmp(MPID_VECTOR *vec, int *cur_pos, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MM_ENTER_FUNC(TCP_STUFF_VECTOR_TMP);

    /* check to see that there is data available and space in the vector to put it */
    if ((*cur_pos == MPID_VECTOR_LIMIT) ||
        (car_ptr->data.tcp.buf.tmp.num_written == buf_ptr->tmp.num_read) ||
	(buf_ptr->tmp.num_read == 0))
    {
	MM_EXIT_FUNC(TCP_STUFF_VECTOR_TMP);
	return FALSE;
    }

    /* add the tmp buffer to the vector */
    vec[*cur_pos].MPID_VECTOR_BUF = (char*)(buf_ptr->tmp.buf) + car_ptr->data.tcp.buf.tmp.num_written;
    vec[*cur_pos].MPID_VECTOR_LEN = buf_ptr->tmp.num_read - car_ptr->data.tcp.buf.tmp.num_written;
    (*cur_pos)++;

    /* if the entire segment is in the vector then return true else false */
    if (buf_ptr->tmp.num_read == buf_ptr->tmp.len)
    {
	MM_EXIT_FUNC(TCP_STUFF_VECTOR_TMP);
	return TRUE;
    }

    MM_EXIT_FUNC(TCP_STUFF_VECTOR_TMP);
    return FALSE;
}

#ifdef WITH_METHOD_NEW
int tcp_stuff_vector_new(MPID_VECTOR *vec, int *cur_pos, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MM_ENTER_FUNC(TCP_STUFF_VECTOR_NEW);
    MM_EXIT_FUNC(TCP_STUFF_VECTOR_NEW);
    return FALSE;
}
#endif

int tcp_update_car_num_written(MM_Car *car_ptr, int *num_written_ptr)
{
    MM_Segment_buffer *buf_ptr;
    int num_written;
    int num_left, i;

    MM_ENTER_FUNC(TCP_UPDATE_CAR_NUM_WRITTEN);

    buf_ptr = car_ptr->buf_ptr;
    num_written = *num_written_ptr;

    switch (buf_ptr->type)
    {
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
    case MM_VEC_BUFFER:
	num_written = min(
	    /* the amout of buffer space available to have been written */
	    buf_ptr->vec.segment_last - car_ptr->data.tcp.buf.vec_write.cur_num_written,
	    /* the actual amount written */
	    num_written);
	
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
	
	/* if the entire mpi segment has been written, enqueue the car in the completion queue */
	if (car_ptr->data.tcp.buf.vec_write.total_num_written == buf_ptr->vec.segment_last)
	{
	    tcp_car_dequeue(car_ptr->vc_ptr, car_ptr);
	    //mm_cq_enqueue(car_ptr);
	    if (car_ptr->next_ptr && (*num_written_ptr == num_written))
	    {
		/* if this is the last car written and it has a next car pointer,
		 * enqueue the next car for writing */
		car_ptr->vc_ptr->post_write(car_ptr->vc_ptr, car_ptr->next_ptr);
	    }
	    mm_dec_cc(car_ptr->request_ptr);
	    mm_car_free(car_ptr);
	}
	break;
    case MM_TMP_BUFFER:
	num_written = min(
	    /* the amout of buffer space available to have been written */
	    buf_ptr->tmp.len - car_ptr->data.tcp.buf.tmp.num_written,
	    /* the actual amount written */
	    num_written);
	
	/* update the amount written */
	car_ptr->data.tcp.buf.tmp.num_written += num_written;
	
	/* check to see if finished */
	if (car_ptr->data.tcp.buf.tmp.num_written == buf_ptr->tmp.len)
	{
	    dbg_printf("num_written: %d\n", car_ptr->data.tcp.buf.tmp.num_written);
	    /* remove from write queue and insert in completion queue */
	    tcp_car_dequeue(car_ptr->vc_ptr, car_ptr);
	    //mm_cq_enqueue(car_ptr);
	    if (car_ptr->next_ptr && (*num_written_ptr == num_written))
	    {
		/* if this is the last car written and it has a next car pointer,
		 * enqueue the next car for writing */
		car_ptr->vc_ptr->post_write(car_ptr->vc_ptr, car_ptr->next_ptr);
	    }
	    mm_dec_cc(car_ptr->request_ptr);
	    mm_car_free(car_ptr);
	}
	break;
#ifdef WITH_METHOD_NEW
    case MM_NEW_METHOD_BUFFER:
	break;
#endif
    case MM_NULL_BUFFER:
	err_printf("Error: tcp_update_car_num_written called on a null buffer\n");
	break;
    default:
	err_printf("Error: tcp_update_car_num_written: unknown or unsupported buffer type: %d\n", buf_ptr->type);
	break;
    }

    /* update num_written */
    (*num_written_ptr) -= num_written;

    MM_EXIT_FUNC(TCP_UPDATE_CAR_NUM_WRITTEN);
    return MPI_SUCCESS;
}

int tcp_write_aggressive(MPIDI_VC *vc_ptr)
{
    MM_Car *car_ptr, *next_car_ptr;
    MM_Segment_buffer *buf_ptr;
    MPID_VECTOR vec[MPID_VECTOR_LIMIT];
    int cur_pos = 0;
    BOOL stop = FALSE;
    int num_written;

    MM_ENTER_FUNC(TCP_WRITE_AGGRESSIVE);

    if (!vc_ptr->data.tcp.connected)
    {
	MM_EXIT_FUNC(TCP_WRITE_AGGRESSIVE);
	return MPI_SUCCESS;
    }

    if (vc_ptr->writeq_head == NULL)
    {
	MM_EXIT_FUNC(TCP_WRITE_AGGRESSIVE);
	return MPI_SUCCESS;
    }

    car_ptr = vc_ptr->writeq_head;

    /* pack as many cars into a vector as possible */
    while (car_ptr)
    {
	buf_ptr = car_ptr->buf_ptr;
	switch (buf_ptr->type)
	{
#ifdef WITH_METHOD_SHM
	case MM_SHM_BUFFER:
	    stop = !tcp_stuff_vector_shm(vec, &cur_pos, car_ptr, buf_ptr);
	    break;
#endif
#ifdef WITH_METHOD_VIA
	case MM_VIA_BUFFER:
	    stop = !tcp_stuff_vector_via(vec, &cur_pos, car_ptr, buf_ptr);
	    break;
#endif
#ifdef WITH_METHOD_VIA_RDMA
	case MM_VIA_RDMA_BUFFER:
	    stop = !tcp_stuff_vector_via_rdma(vec, &cur_pos, car_ptr, buf_ptr);
	    break;
#endif
	case MM_VEC_BUFFER:
	    if (buf_ptr->vec.num_cars_outstanding > 0)
	    {
		stop = !tcp_stuff_vector_vec(vec, &cur_pos, car_ptr, buf_ptr);
	    }
	    else
		stop = TRUE;
	    break;
	case MM_TMP_BUFFER:
	    stop = !tcp_stuff_vector_tmp(vec, &cur_pos, car_ptr, buf_ptr);
	    break;
#ifdef WITH_METHOD_NEW
	case MM_NEW_METHOD_BUFFER:
	    stop = !tcp_stuff_vector_new(vec, &cur_pos, car_ptr, buf_ptr);
	    break;
#endif
	case MM_NULL_BUFFER:
	    err_printf("Error: tcp_write_aggressive called on a null buffer\n");
	    break;
	default:
	    err_printf("Error: tcp_write_aggressive: unknown or unsupported buffer type: %d\n", buf_ptr->type);
	    break;
	}
	if (stop)
	    break;
	car_ptr = car_ptr->next_ptr;
    }

    if (cur_pos > 0)
    {
	/* write the data */
	if (cur_pos == 1)
	{
	    num_written = bwrite(vc_ptr->data.tcp.bfd, vec[0].MPID_VECTOR_BUF, vec[0].MPID_VECTOR_LEN);
	    if (num_written == SOCKET_ERROR)
	    {
		TCP_Process.error = beasy_getlasterror();
		beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
		err_printf("tcp_write_aggressive: bwrite failed, error %d: %s\n", TCP_Process.error, TCP_Process.err_msg);
		MM_EXIT_FUNC(TCP_WRITE_AGGRESSIVE);
		return -1;
	    }
	}
	else
	{
	    num_written = bwritev(vc_ptr->data.tcp.bfd, vec, cur_pos);
	    if (num_written == SOCKET_ERROR)
	    {
		TCP_Process.error = beasy_getlasterror();
		beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
		err_printf("tcp_write_aggressive: bwritev failed, error %d: %s\n", TCP_Process.error, TCP_Process.err_msg);
		MM_EXIT_FUNC(TCP_WRITE_AGGRESSIVE);
		return -1;
	    }
	}

	/* update all the cars and buffers affected */
	car_ptr = vc_ptr->writeq_head;
	while (num_written)
	{
	    next_car_ptr = car_ptr->next_ptr;
	    if (tcp_update_car_num_written(car_ptr, &num_written) != MPI_SUCCESS)
	    {
		err_printf("tcp_write_aggressive:tcp_update_car_num_written failed.\n");
		return -1;
	    }
	    car_ptr = next_car_ptr;
	}
    }

    MM_EXIT_FUNC(TCP_WRITE_AGGRESSIVE);
    return MPI_SUCCESS;
}
