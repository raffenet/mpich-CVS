/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   xfer_scatter_send_op - xfer_scatter_send_op

   Parameters:
+  MPID_Request *request_ptr - request
.  const void *buf - buffer
.  int count - count
.  MPI_Datatype dtype - datatype
.  int first - first
.  int last - last
-  int dest - destination

   Notes:
@*/
int xfer_scatter_send_op(MPID_Request *request_ptr, const void *buf, int count, MPI_Datatype dtype, int first, int last, int dest)
{
    MM_Car *pCar;
    MPID_Request *pRequest;

    if (!request_ptr->op_valid)
    {
	pRequest = request_ptr;
    }
    else
    {
	pRequest = request_ptr;
	while (pRequest->next_ptr)
	    pRequest = pRequest->next_ptr;
	pRequest->next_ptr = mm_request_alloc();
	pRequest = pRequest->next_ptr;
    }

    pRequest->op_valid = 1;
    pRequest->next_ptr = NULL;

    /* Save the mpi buffer */
    pRequest->sbuf = buf;
    pRequest->count = count;
    pRequest->dtype = dtype;
    pRequest->first = first;
    pRequest->last = last;

    /* allocate a write car */
    if (request_ptr->write_list == NULL)
    {
	request_ptr->write_list = &request_ptr->wcar;
	pCar = &request_ptr->wcar;
    }
    else
    {
	pCar = request_ptr->write_list;
	while (pCar->next_ptr)
	    pCar = pCar->next_ptr;
	pCar->next_ptr = mm_car_alloc();
	pCar = pCar->next_ptr;
    }
    
    pCar->request_ptr = request_ptr;
    pCar->dest = dest;
    pCar->next_ptr = NULL;

    return MPI_SUCCESS;
}
