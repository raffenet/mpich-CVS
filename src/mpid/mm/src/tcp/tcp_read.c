/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "tcpimpl.h"

int tcp_read(MPIDI_VC *vc_ptr)
{
    int num_read;
    MM_Car *car_ptr;
    MM_Segment_buffer *buf_ptr;
    char ack;
    int num_left, i;

    if (vc_ptr->data.tcp.connecting)
    {
	if (beasy_receive(vc_ptr->data.tcp.bfd, &ack, 1) == SOCKET_ERROR)
	{
	    TCP_Process.error = beasy_getlasterror();
	    beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
	    err_printf("tcp_read: beasy_receive(ack) failed, error %d: %s\n", TCP_Process.error, TCP_Process.err_msg);
	    return -1;
	}
	if (ack == TCP_ACCEPT_CONNECTION)
	{
	    vc_ptr->data.tcp.connected = TRUE;
	    vc_ptr->data.tcp.connecting = FALSE;
	}
	else if (ack == TCP_REJECT_CONNECTION)
	{
	    vc_ptr->data.tcp.reject_received = TRUE;
	}
	else
	{
	    err_printf("tcp_read: unknown ack char #%d received in read function.\n", (int)ack);
	}
	return MPI_SUCCESS;
    }

    car_ptr = vc_ptr->readq_head;
    buf_ptr = car_ptr->buf_ptr;

    switch (buf_ptr->type)
    {
    case MM_NULL_BUFFER:
	err_printf("Error: tcp_read called on a null buffer\n");
	break;
    case MM_TMP_BUFFER:
	num_read = bread(vc_ptr->data.tcp.bfd, 
	    (char*)(buf_ptr->tmp.buf_ptr[buf_ptr->tmp.cur_buf]) + car_ptr->data.tcp.buf.tmp.num_read, 
	    buf_ptr->tmp.num_read - car_ptr->data.tcp.buf.tmp.num_read);
	if (num_read == SOCKET_ERROR)
	{
	    err_printf("tcp_read:bread failed, error %d\n", beasy_getlasterror());
	}
	car_ptr->data.tcp.buf.tmp.num_read += num_read;
	break;
    case MM_VEC_BUFFER:
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
	    buf_ptr->vec.num_cars_outstanding = buf_ptr->vec.num_cars;
	}
	
	if (car_ptr->data.tcp.buf.vec_read.cur_num_read < buf_ptr->vec.buf_size)
	{
	    /* read */
	    if (car_ptr->data.tcp.buf.vec_read.vec_size == 1) /* optimization for single buffer reads */
	    {
		num_read = bread(vc_ptr->data.tcp.bfd,
		    car_ptr->data.tcp.buf.vec_read.vec[car_ptr->data.tcp.buf.vec_read.cur_index].MPID_VECTOR_BUF,
		    car_ptr->data.tcp.buf.vec_read.vec[car_ptr->data.tcp.buf.vec_read.cur_index].MPID_VECTOR_LEN);
		if (num_read == SOCKET_ERROR)
		{
		    TCP_Process.error = beasy_getlasterror();
		    beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
		    err_printf("tcp_read: bread failed, error %d: %s\n", TCP_Process.error, TCP_Process.err_msg);
		    return -1;
		}
	    }
	    else
	    {
		num_read = breadv(vc_ptr->data.tcp.bfd,
		    &car_ptr->data.tcp.buf.vec_read.vec[car_ptr->data.tcp.buf.vec_read.cur_index],
		    car_ptr->data.tcp.buf.vec_read.vec_size);
		if (num_read == SOCKET_ERROR)
		{
		    TCP_Process.error = beasy_getlasterror();
		    beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
		    err_printf("tcp_read: breadv failed, error %d: %s\n", TCP_Process.error, TCP_Process.err_msg);
		    return -1;
		}
	    }
	    
	    /* update vector */
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
			car_ptr->data.tcp.buf.vec_read.vec[i].MPID_VECTOR_BUF = car_ptr->data.tcp.buf.vec_read.vec[i].MPID_VECTOR_BUF - num_left;
			car_ptr->data.tcp.buf.vec_read.vec[i].MPID_VECTOR_LEN += num_left;
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
	err_printf("Error: tcp_read: unknown or unsupported buffer type: %d\n", buf_ptr->type);
	break;
    }

    return MPI_SUCCESS;
}
