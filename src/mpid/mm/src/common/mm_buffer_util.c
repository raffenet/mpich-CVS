/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int mm_get_buffers_tmp(MPID_Request *request_ptr)
{
    return MPI_SUCCESS;
}

int mm_get_buffers_vec(MPID_Request *request_ptr)
{
    return MPI_SUCCESS;
}

#ifdef WITH_METHOD_SHM
int mm_get_buffers_shm(MPID_Request *request_ptr)
{
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_VIA
int mm_get_buffers_via(MPID_Request *request_ptr)
{
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_VIA_RDMA
int mm_get_buffers_viardma(MPID_Request *request_ptr)
{
    return MPI_SUCCESS;
}
#endif
