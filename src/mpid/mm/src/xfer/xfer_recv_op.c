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
    MM_Car *pCar, *pCarIter;
    MPID_Request *pRequest, *pRequestIter;
    BOOL bNeedHeader = TRUE;

    dbg_printf("xfer_recv_op\n");

    /* Get a pointer to the current unused request, allocating if necessary. */
    if (request_ptr->mm.op_valid == FALSE)
    {
	pRequest = request_ptr;
    }
    else
    {
	pRequest = mm_request_alloc();
	pRequestIter = request_ptr;
	pCarIter = pRequestIter->mm.rcar;
	if (pCarIter->type & MM_HEAD_CAR)
	{
	    if (pCarIter->src == src)
	    {
		while (pCarIter->next_ptr)
		{
		    pCarIter = pCarIter->next_ptr;
		}
		pCarIter->next_ptr = pRequest->mm.rcar;
		bNeedHeader = FALSE;
	    }
	}
	while (pRequestIter->mm.next_ptr)
	{
	    pRequestIter = pRequestIter->mm.next_ptr;
	    if (bNeedHeader)
	    {
		pCarIter = pRequestIter->mm.rcar;
		if (pCarIter->type & MM_HEAD_CAR)
		{
		    if (pCarIter->src == src)
		    {
			while (pCarIter->next_ptr)
			{
			    pCarIter = pCarIter->next_ptr;
			}
			pCarIter->next_ptr = pRequest->mm.rcar;
			bNeedHeader = FALSE;
		    }
		}
	    }
	}
	pRequestIter->mm.next_ptr = pRequest;
    }

    pRequest->mm.op_valid = TRUE;
    pRequest->comm = request_ptr->comm;
    pRequest->cc_ptr = &request_ptr->cc;
    pRequest->mm.next_ptr = NULL;

    /* Save the mpi segment */
    /* These fields may not be necessary since we have MPID_Segment_init */
    pRequest->mm.user_buf.recv = buf;
    pRequest->mm.count = count;
    pRequest->mm.dtype = dtype;
    pRequest->mm.first = first;
    pRequest->mm.size = count * MPID_Datatype_get_size(dtype);
    if (last == MPID_DTYPE_END)
	pRequest->mm.last = pRequest->mm.size;
    else
	pRequest->mm.last = last;

    MPID_Segment_init(buf, count, dtype, &pRequest->mm.segment);

    /* setup the read car */
    if (bNeedHeader)
    {
	pRequest->mm.rcar[0].type = MM_HEAD_CAR | MM_READ_CAR;
	pRequest->mm.rcar[0].src = src;
	pRequest->mm.rcar[0].request_ptr = pRequest;
	pRequest->mm.rcar[0].vc_ptr = mm_vc_from_communicator(request_ptr->comm, src);
	pRequest->mm.rcar[0].buf_ptr = &pRequest->mm.rcar[0].msg_header.buf;
	pRequest->mm.rcar[0].msg_header.pkt.u.type = MPID_EAGER_PKT; /* this should be set by the method */
	pRequest->mm.rcar[0].msg_header.pkt.u.hdr.context = request_ptr->comm->context_id;
	pRequest->mm.rcar[0].msg_header.pkt.u.hdr.tag = request_ptr->mm.tag;
	pRequest->mm.rcar[0].msg_header.pkt.u.hdr.src = src;
	pRequest->mm.rcar[0].msg_header.pkt.u.hdr.size = 0;
	pRequest->mm.rcar[0].opnext_ptr = &pRequest->mm.rcar[1];
	pRequest->mm.rcar[0].next_ptr = &pRequest->mm.rcar[1];
	pRequest->mm.rcar[0].qnext_ptr = NULL;
	mm_inc_cc(pRequest);

	pCar = &pRequest->mm.rcar[1];
	pCar->vc_ptr = pRequest->mm.rcar[0].vc_ptr;
    }
    else
    {
	pCar = pRequest->mm.rcar;
	pCar->vc_ptr = mm_vc_from_communicator(request_ptr->comm, src);
    }

    pCar->type = MM_READ_CAR;
    pCar->request_ptr = pRequest;
    pCar->buf_ptr = &pRequest->mm.buf;
    pCar->src = src;
    pCar->next_ptr = NULL;
    pCar->opnext_ptr = NULL;
    pCar->qnext_ptr = NULL;
    mm_inc_cc(pRequest);

    /* setup a write car for unpacking */
    pRequest->mm.write_list = &pRequest->mm.wcar[0];
    pCar = pRequest->mm.wcar;
    pCar->type = MM_WRITE_CAR | MM_UNPACKER_CAR;
    pCar->request_ptr = pRequest;
    pCar->buf_ptr = &pRequest->mm.buf;
    pCar->vc_ptr = MPID_Process.unpacker_vc_ptr;
    pCar->next_ptr = NULL;
    pCar->opnext_ptr = NULL;
    pCar->qnext_ptr = NULL;
    mm_inc_cc(pRequest);

    return MPI_SUCCESS;
}
