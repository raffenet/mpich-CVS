/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   xfer_scatter_start - xfer_scatter_start

   Parameters:
+  MPID_Request *request_ptr - request

   Notes:
@*/
int xfer_scatter_start(MPID_Request *request_ptr)
{
    MPID_Request *pRequest;

    pRequest = request_ptr;
    while (pRequest)
    {
	/*mm_choose_buffers(pRequest);*/
	pRequest = pRequest->mm.next_ptr;
    }

    return MPI_SUCCESS;
}
