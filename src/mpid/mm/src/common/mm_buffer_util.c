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
    request_ptr->mm.buf.vec.first = request_ptr->mm.buf.vec.last;
    request_ptr->mm.buf.vec.last = request_ptr->mm.last;

    MPID_Segment_pack_vector(
	&request_ptr->mm.segment,
	request_ptr->mm.buf.vec.first,
	&request_ptr->mm.buf.vec.last,
	request_ptr->mm.buf.vec.vec,
	&request_ptr->mm.buf.vec.vec_size);

    request_ptr->mm.buf.vec.buf_size = request_ptr->mm.buf.vec.last - request_ptr->mm.buf.vec.first;

    return MPI_SUCCESS;
}
