/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME xfer_scatter_recv_op

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
    static const char FCNAME[] = "xfer_scatter_recv_op";
    MM_Car *pCar;
    MPID_Request *pRequest;

    MPID_MPI_FUNC_ENTER(MPID_STATE_XFER_SCATTER_RECV_OP);

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
    pRequest->rbuf = buf;
    pRequest->count = count;
    pRequest->dtype = dtype;
    pRequest->first = first;
    pRequest->last = last;

    /* allocate a read car */
    if (request_ptr->read_list == NULL)
    {
	request_ptr->read_list = &request_ptr->rcar;
	pCar = &request_ptr->rcar;
    }
    else
    {
	pCar = request_ptr->read_list;
	while (pCar->next_ptr)
	    pCar = pCar->next_ptr;
	pCar->next_ptr = mm_car_alloc();
	pCar = pCar->next_ptr;
    }
    
    pCar->request_ptr = request_ptr;
    pCar->src = request_ptr->src;
    pCar->next_ptr = NULL;

    MPID_MPI_FUNC_EXIT(MPID_STATE_XFER_SCATTER_RECV_OP);
    return MPI_SUCCESS;
}
