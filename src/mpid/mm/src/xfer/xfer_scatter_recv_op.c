/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   xfer_scatter_recv_op - xfer_scatter_recv_op

   Parameters:
+  MPID_Request *request_ptr - request
.  void *buf - buffer
.  int count - count
.  MPI_Datatype dtype - datatype
.  int first - first
-  int last - last

   Notes:
@*/
int xfer_scatter_recv_op(MPID_Request *request_ptr, void *buf, int count, MPI_Datatype dtype, int first, int last)
{
    MPID_Request *pRequest;

    /* Get a pointer to the current request.
       Either the first request, allocated by scatter_init,
       will be unused or we will allocate a new request at
       the end of the list */
    if (!request_ptr->mm.op_valid)
    {
	pRequest = request_ptr;
    }
    else
    {
	pRequest = request_ptr;
	while (pRequest->mm.next_ptr)
	    pRequest = pRequest->mm.next_ptr;
	pRequest->mm.next_ptr = mm_request_alloc();
	pRequest = pRequest->mm.next_ptr;
    }

    pRequest->mm.op_valid = 1;
    pRequest->mm.next_ptr = NULL;

    /* Save the mpi buffer */
    pRequest->mm.rbuf = buf;
    pRequest->mm.count = count;
    pRequest->mm.dtype = dtype;
    pRequest->mm.first = first;
    pRequest->mm.last = last;

    pRequest->mm.rcar.request_ptr = request_ptr;
    pRequest->mm.rcar.next_ptr = NULL;

    return MPI_SUCCESS;
}
