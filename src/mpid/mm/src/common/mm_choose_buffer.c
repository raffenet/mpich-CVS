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
    if (request_ptr->mm.rcar.type != MM_NULL_CAR)
    {
	request_ptr->mm.read_buf_type = MM_MPI_BUFFER;
	request_ptr->mm.read_buf.mpi.size = MPID_VECTOR_LIMIT;
	request_ptr->mm.read_buf.mpi.num_read = 0;
	request_ptr->mm.read_buf.mpi.min_num_written = 0;
    }
    else
    {
	request_ptr->mm.read_buf_type = MM_NULL_BUFFER;
    }

    return MPI_SUCCESS;
}
