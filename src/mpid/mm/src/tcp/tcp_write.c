/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "tcpimpl.h"

int tcp_write(MPIDI_VC *vc_ptr)
{
    MM_Car *car_ptr;

    car_ptr = vc_ptr->readq_head;

    switch (car_ptr->buf_ptr->type)
    {
    case MM_NULL_BUFFER:
	err_printf("Error: tcp_write called on a null buffer\n");
	break;
    case MM_TMP_BUFFER:
	break;
    case MM_VEC_BUFFER:
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
