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
    MM_Car *pCar;

    /* choose the buffers scheme to complete this operation */
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
	mm_car_enqueue(&pRequest->mm.rcar);
	pCar = pRequest->mm.write_list;
	while (pCar)
	{
	    mm_car_enqueue(pCar);
	    pCar = pCar->next_ptr;
	}
	pRequest = pRequest->mm.next_ptr;
    }

    return MPI_SUCCESS;
}
