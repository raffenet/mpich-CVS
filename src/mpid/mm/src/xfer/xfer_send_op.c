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
    MPID_Request *pRequest, *pRequestIter;
    BOOL bNeedHeader = TRUE;
    MM_Car *pCarIter;

    /* Get a pointer to the current unused request, allocating if necessary. */
    if (request_ptr->mm.op_valid == FALSE)
    {
	pRequest = request_ptr;
    }
    else
    {
	pRequest = mm_request_alloc();
	pRequestIter = request_ptr;
	pCarIter = pRequestIter->mm.write_list;
	while (pCarIter)
	{
	    if (pCarIter->dest == dest)
	    {
		while (pCarIter->next_ptr)
		{
		    pCarIter = pCarIter->next_ptr;
		}
		pCarIter->next_ptr = pRequest->mm.wcar;
		bNeedHeader = FALSE;
		break;
	    }
	    pCarIter = pCarIter->opnext_ptr;
	}
	while (pRequestIter->mm.next_ptr)
	{
	    pRequestIter = pRequestIter->mm.next_ptr;
	    if (bNeedHeader)
	    {
		pCarIter = pRequestIter->mm.write_list;
		while (pCarIter)
		{
		    if (pCarIter->dest == dest)
		    {
			while (pCarIter->next_ptr)
			{
			    pCarIter = pCarIter->next_ptr;
			}
			pCarIter->next_ptr = pRequest->mm.wcar;
			bNeedHeader = FALSE;
			break;
		    }
		    pCarIter = pCarIter->opnext_ptr;
		}
	    }
	}
	pRequestIter->mm.next_ptr = pRequest;
    }

    pRequest->mm.op_valid = TRUE;
    pRequest->comm = request_ptr->comm;
    /* point the completion counter to the primary request */
    pRequest->cc_ptr = &request_ptr->cc;
    pRequest->mm.next_ptr = NULL;

    /* Save the mpi segment */
    /* These fields may not be necessary since we have MPID_Segment_init */
    pRequest->mm.user_buf.send = buf;
    pRequest->mm.count = count;
    pRequest->mm.dtype = dtype;
    pRequest->mm.first = first;
    pRequest->mm.last = last;
    pRequest->mm.size = count * MPID_Datatype_get_size(dtype);

    MPID_Segment_init(buf, count, dtype, &pRequest->mm.segment);

    /* setup the read car for packing the mpi buffer to be sent */
    pRequest->mm.rcar[0].request_ptr = pRequest;
    pRequest->mm.rcar[0].buf_ptr = &pRequest->mm.buf;
    pRequest->mm.rcar[0].type = MM_HEAD_CAR | MM_READ_CAR | MM_PACKER_CAR;
    pRequest->mm.rcar[0].vc_ptr = NULL;
    pRequest->mm.rcar[0].next_ptr = NULL;
    pRequest->mm.rcar[0].opnext_ptr = NULL;
    pRequest->mm.rcar[0].qnext_ptr = NULL;
    pRequest->mm.rcar[0].src = request_ptr->comm->rank; /* packer source is myself */
    mm_inc_cc(pRequest);

    /* setup the write car, adding a header car if this is the first send op to this destination */
    pRequest->mm.write_list = &pRequest->mm.wcar[0];
    if (bNeedHeader)
    {
	pRequest->mm.wcar[0].request_ptr = pRequest;
	pRequest->mm.wcar[0].buf_ptr = &pRequest->mm.buf;
	pRequest->mm.wcar[0].type = MM_HEAD_CAR | MM_WRITE_CAR;
	pRequest->mm.wcar[0].vc_ptr = mm_get_vc(request_ptr->comm, dest);
	pRequest->mm.wcar[0].dest = dest;
	pRequest->mm.wcar[0].data.pkt.context = request_ptr->comm->context_id;
	pRequest->mm.wcar[0].data.pkt.tag = request_ptr->mm.tag;
	pRequest->mm.wcar[0].data.pkt.size = 0;
	pRequest->mm.wcar[0].next_ptr = &pRequest->mm.wcar[1];
	pRequest->mm.wcar[0].opnext_ptr = NULL;
	pRequest->mm.wcar[0].qnext_ptr = NULL;
	mm_inc_cc(pRequest);

	pCar = &pRequest->mm.wcar[1];
	pCar->vc_ptr = pRequest->mm.wcar[0].vc_ptr;
    }
    else
    {
	pCar = &pRequest->mm.wcar[0];
	pCar->vc_ptr = mm_get_vc(request_ptr->comm, dest);
    }

    pCar->request_ptr = pRequest;
    pCar->buf_ptr = &pRequest->mm.buf;
    pCar->type = MM_WRITE_CAR;
    pCar->dest = dest;
    pCar->next_ptr = NULL;
    pCar->opnext_ptr = NULL;
    pCar->qnext_ptr = NULL;
    mm_inc_cc(pRequest);

    return MPI_SUCCESS;
}
