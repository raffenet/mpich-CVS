/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   mm_choose_buffer - choose the best buffer for the operation

   Parameters:
+  MPID_Request *request_ptr - request pointer

   Notes:
@*/
int mm_choose_buffer(MPID_Request *request_ptr)
{
    /* look at the read car and all of the write cars */
    /* pick the best buffer type that everyone can handle */
    /* if there are incompatible cars, allocate other requests and 
       connect them together with copier methods */

    /* but for now always choose the vector buffer type */
    if (request_ptr->mm.rcar[0].type != MM_NULL_CAR)
    {
	request_ptr->mm.buf.type = MM_VEC_BUFFER;
	request_ptr->mm.buf.vec.vec_size = MPID_VECTOR_LIMIT;
	request_ptr->mm.buf.vec.msg_size = 0;
	request_ptr->mm.buf.vec.num_read = 0;
	request_ptr->mm.buf.vec.min_num_written = 0;
	request_ptr->mm.buf.vec.local_last = 0;
	request_ptr->mm.get_buffers = mm_get_buffers_vec;
    }
    else
    {
	request_ptr->mm.buf.type = MM_NULL_BUFFER;
	request_ptr->mm.get_buffers = NULL;
    }

    return MPI_SUCCESS;
}
