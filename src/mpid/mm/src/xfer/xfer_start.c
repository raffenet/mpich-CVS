/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   xfer_start - xfer_start

   Parameters:
+  MPID_Request *request_ptr - request

   Notes:
@*/
int xfer_start(MPID_Request *request_ptr)
{
    int mpi_errno;
    MPID_Request *pRequest;
    MM_Car *pCar, *pCarIter;

    /* choose the buffers scheme to satisfy each segment */
    pRequest = request_ptr;
    while (pRequest)
    {
	mpi_errno = mm_choose_buffer(pRequest);
	if (mpi_errno != MPI_SUCCESS)
	{
	    return mpi_errno;
	}
	pRequest = pRequest->mm.next_ptr;
    }

    /* enqueue the cars */
    pRequest = request_ptr;
    while (pRequest)
    {
	if (pRequest->mm.rcar[0].type & MM_HEAD_CAR)
	{
	    /* add up the size of the message and put it in the packet */
	    pRequest->mm.rcar[0].data.pkt.size = 0;
	    pCarIter = pRequest->mm.rcar->next_ptr;
	    while (pCarIter)
	    {
		pRequest->mm.rcar[0].data.pkt.size += pCarIter->request_ptr->mm.size;
		pCarIter = pCarIter->next_ptr;
	    }
	    /* post the recv */
	    mm_post_recv(pRequest->mm.rcar);
	}
	pCar = pRequest->mm.write_list;
	while (pCar)
	{
	    if (pCar->type & MM_HEAD_CAR)
	    {
		/* add up the size of the message and put it in the packet */
		pCar->data.pkt.size = 0;
		/* figure out the total size by adding up the size fields of the data cars */
		pCarIter = pCar->next_ptr; /* skip the header car */
		while (pCarIter)
		{
		    pCar->data.pkt.size += pCarIter->request_ptr->mm.size;
		    pCarIter = pCarIter->next_ptr;
		}
		/* enqueue the send */
		mm_post_send(pCar);
	    }
	    pCar = pCar->opnext_ptr;
	}
	pRequest = pRequest->mm.next_ptr;
    }

    return MPI_SUCCESS;
}
