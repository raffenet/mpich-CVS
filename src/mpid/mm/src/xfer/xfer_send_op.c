/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   xfer_send_op - xfer_send_op

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
int xfer_send_op(MPID_Request *request_ptr, const void *buf, int count, MPI_Datatype dtype, int first, int last, int dest)
{
    MM_Car *pCar;
    MPID_Request *pRequest;

    /* Get a pointer to the current unused request, allocating if necessary. */
    if (request_ptr->mm.op_valid == FALSE)
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

    pRequest->mm.op_valid = TRUE;
    /* point the completion counter to the primary request */
    pRequest->cc_ptr = &request_ptr->cc;
    (*(pRequest->cc_ptr))++;
    pRequest->mm.next_ptr = NULL;

    /* Save the mpi segment */
    pRequest->mm.buf.send = buf;
    pRequest->mm.count = count;
    pRequest->mm.dtype = dtype;
    pRequest->mm.first = first;
    pRequest->mm.last = last;

    /* setup the read car for packing the mpi buffer to be sent */
    pRequest->mm.rcar.request_ptr = request_ptr;
    pRequest->mm.rcar.type = MM_READ_CAR;
    pRequest->mm.rcar.vc_ptr = mm_get_packer_vc();
    pRequest->mm.rcar.next_ptr = NULL;

    /* allocate a write car */
    if (request_ptr->mm.write_list == NULL)
    {
	request_ptr->mm.write_list = &request_ptr->mm.wcar;
	pCar = &request_ptr->mm.wcar;
    }
    else
    {
	pCar = request_ptr->mm.write_list;
	while (pCar->next_ptr)
	    pCar = pCar->next_ptr;
	pCar->next_ptr = mm_car_alloc();
	pCar = pCar->next_ptr;
    }
    
    pCar->request_ptr = request_ptr;
    pCar->type = MM_WRITE_CAR;
    pCar->vc_ptr = mm_get_vc(request_ptr->comm, dest);
    pCar->dest = dest;
    pCar->next_ptr = NULL;

    return MPI_SUCCESS;
}
