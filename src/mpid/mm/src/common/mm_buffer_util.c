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

int tmp_buffer_init(MPID_Request *request_ptr)
{
    request_ptr->mm.buf.type = MM_TMP_BUFFER;
    request_ptr->mm.buf.tmp.buf[0] = NULL;
    request_ptr->mm.buf.tmp.buf[1] = NULL;
    request_ptr->mm.buf.tmp.len[0] = 0;
    request_ptr->mm.buf.tmp.len[1] = 0;
    request_ptr->mm.buf.tmp.cur_buf = 0;
    request_ptr->mm.buf.tmp.min_num_written = 0;
    request_ptr->mm.buf.tmp.num_read = 0;
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

int vec_buffer_init(MPID_Request *request_ptr)
{
    request_ptr->mm.buf.type = MM_VEC_BUFFER;
    request_ptr->mm.buf.vec.vec_size = MPID_VECTOR_LIMIT;
    request_ptr->mm.buf.vec.num_read = 0;
    request_ptr->mm.buf.vec.first = 0;
    request_ptr->mm.buf.vec.last = 0;
    request_ptr->mm.buf.vec.segment_last = request_ptr->mm.last;
    request_ptr->mm.buf.vec.buf_size = 0;
    request_ptr->mm.buf.vec.num_cars_outstanding = 0;
    request_ptr->mm.buf.vec.num_cars = 0;
    return MPI_SUCCESS;
}
