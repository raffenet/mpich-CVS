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
	else
	{
	    err_printf("tcp_read: TCP_REJECT_CONNECTION ack received in read function.\n");
	}
	return MPI_SUCCESS;
    }

    car_ptr = vc_ptr->readq_head;
    buf_ptr = car_ptr->buf_ptr;

    switch (car_ptr->buf_ptr->type)
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
	if (car_ptr->data.tcp.buf.vec.len > 1)
	{
	    num_read = breadv(vc_ptr->data.tcp.bfd,
		car_ptr->data.tcp.buf.vec.vec,
		car_ptr->data.tcp.buf.vec.len);
	    if (num_read == SOCKET_ERROR)
	    {
		TCP_Process.error = beasy_getlasterror();
		beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
		err_printf("tcp_read: breadv failed, error %d: %s\n", TCP_Process.error, TCP_Process.err_msg);
		return -1;
	    }
	}
	else
	{
	    num_read = bread(vc_ptr->data.tcp.bfd,
		car_ptr->data.tcp.buf.vec.vec[0].MPID_VECTOR_BUF,
		car_ptr->data.tcp.buf.vec.vec[0].MPID_VECTOR_LEN);
	    if (num_read == SOCKET_ERROR)
	    {
		TCP_Process.error = beasy_getlasterror();
		beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
		err_printf("tcp_read: bread failed, error %d: %s\n", TCP_Process.error, TCP_Process.err_msg);
		return -1;
	    }
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
	err_printf("Error: tcp_read: unknown or unsupported buffer type: %d\n", car_ptr->buf_ptr->type);
	break;
    }

    return MPI_SUCCESS;
}
