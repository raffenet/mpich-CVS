/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "tcpimpl.h"

int tcp_write(MPIDI_VC *vc_ptr)
{
    MM_Car *car_ptr;
    int num_read;

    if (!vc_ptr->data.tcp.connected)
	return MPI_SUCCESS;

    car_ptr = vc_ptr->writeq_head;

    switch (car_ptr->buf_ptr->type)
    {
    case MM_NULL_BUFFER:
	err_printf("Error: tcp_write called on a null buffer\n");
	break;
    case MM_TMP_BUFFER:
	break;
    case MM_VEC_BUFFER:
	/* write as much of the vector as possible */
	if (car_ptr->data.tcp.buf.vec.len > 1)
	{
	    num_read = bwritev(vc_ptr->data.tcp.bfd, car_ptr->data.tcp.buf.vec.vec, car_ptr->data.tcp.buf.vec.len);
	    if (num_read == SOCKET_ERROR)
	    {
		TCP_Process.error = beasy_getlasterror();
		beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
		err_printf("tcp_write: bwritev failed, error %d: %s\n", TCP_Process.error, TCP_Process.err_msg);
		return -1;
	    }
	}
	else
	{
	    num_read = bwrite(vc_ptr->data.tcp.bfd, 
		car_ptr->data.tcp.buf.vec.vec[0].MPID_VECTOR_BUF,
		car_ptr->data.tcp.buf.vec.vec[0].MPID_VECTOR_LEN);
	    if (num_read == SOCKET_ERROR)
	    {
		TCP_Process.error = beasy_getlasterror();
		beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
		err_printf("tcp_write: bwrite failed, error %d: %s\n", TCP_Process.error, TCP_Process.err_msg);
		return -1;
	    }
	}
	/* update the current position */
	/* update the min_num_written */
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
	err_printf("Error: tcp_write: unknown or unsupported buffer type: %d\n", car_ptr->buf_ptr->type);
	break;
    }

    return MPI_SUCCESS;
}
