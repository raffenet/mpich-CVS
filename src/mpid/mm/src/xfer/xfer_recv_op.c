/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   xfer_recv_op - xfer_recv_op

   Parameters:
+  MPID_Request *request_ptr - request
.  void *buf - buffer
.  int count - count
.  MPI_Datatype dtype - datatype
.  int first - first
.  int last - last
-  int src - source

   Notes:
@*/
int xfer_recv_op(MPID_Request *request_ptr, void *buf, int count, MPI_Datatype dtype, int first, int last, int src)
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
    pRequest->cc_ptr = &request_ptr->cc;
    (*(pRequest->cc_ptr))++;
    pRequest->mm.next_ptr = NULL;

    /* Save the mpi segment */
    pRequest->mm.buf.recv = buf;
    pRequest->mm.count = count;
    pRequest->mm.dtype = dtype;
    pRequest->mm.first = first;
    pRequest->mm.last = last;

    /* setup the read car */
    pRequest->mm.rcar.request_ptr = request_ptr;
    pRequest->mm.rcar.src = src;
    pRequest->mm.rcar.type = MM_READ_CAR;
    pRequest->mm.rcar.vc_ptr = mm_get_vc(request_ptr->comm, src);
    pRequest->mm.rcar.next_ptr = NULL;

    /* allocate a write car for unpacking */
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
    pCar->vc_ptr = mm_get_unpacker_vc();
    pCar->next_ptr = NULL;

    MPID_Segment_init(buf, count, dtype, &pRequest->mm.segment);

    return MPI_SUCCESS;
}
