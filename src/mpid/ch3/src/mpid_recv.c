/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Recv(void * buf, int count, MPI_Datatype datatype, int rank, int tag, MPID_Comm * comm, int context_offset,
	      MPI_Status * status, MPID_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request * rreq;
    int found;
    MPIDI_STATE_DECL(MPID_STATE_MPID_RECV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_RECV);

    MPIDI_DBG_PRINTF((10, FCNAME, "entering"));
    MPIDI_DBG_PRINTF((15, FCNAME, "rank=%d, tag=%d, context=%d", rank, tag,
		      comm->context_id + context_offset));
    
    if (rank == MPI_PROC_NULL)
    {
	MPIR_Status_set_procnull(status);
	rreq = NULL;
	goto fn_exit;
    }

    rreq = MPIDI_CH3U_Recvq_FDU_or_AEP(rank, tag, comm->context_id + context_offset, &found);
    if (rreq == NULL)
    {
	/*mpi_errno = MPIR_ERR_MEMALLOCFAILED;*/
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, MPI_ERR_NO_MEM, "**nomem", 0);
	goto fn_exit;
    }

    rreq->comm = comm;
    MPIR_Comm_add_ref(comm);
    rreq->ch3.user_buf = buf;
    rreq->ch3.user_count = count;
    rreq->ch3.datatype = datatype;

    if (found)
    {
	MPIDI_VC * vc;
	
	vc = comm->vcr[rreq->ch3.match.rank];

	/* Message was found in the unexepected queue */
	MPIDI_DBG_PRINTF((15, FCNAME, "request found in unexpected queue"));

	if (MPIDI_Request_get_msg_type(rreq) == MPIDI_REQUEST_EAGER_MSG)
	{
	    int recv_pending;
	    
	    /* This is an eager message. */
	    MPIDI_DBG_PRINTF((15, FCNAME, "eager message in the request"));

	    /* If this is a eager synchronous message, then we need to send an acknowledgement back to the sender. */
	    if (MPIDI_Request_get_sync_send_flag(rreq))
	    {
		MPIDI_CH3_Pkt_t upkt;
		MPIDI_CH3_Pkt_eager_sync_ack_t * const esa_pkt = &upkt.eager_sync_ack;
		MPID_Request * esa_req;
		    
		MPIDI_DBG_PRINTF((30, FCNAME, "sending eager sync ack"));
		esa_pkt->type = MPIDI_CH3_PKT_EAGER_SYNC_ACK;
		esa_pkt->sender_req_id = rreq->ch3.sender_req_id;
		esa_req = MPIDI_CH3_iStartMsg(vc, esa_pkt, sizeof(*esa_pkt));
		if (esa_req != NULL)
		{
		    MPID_Request_release(esa_req);
		}
	    }
	    
            MPIDI_Request_recv_pending(rreq, &recv_pending);
	    if (!recv_pending)
	    {
		/* All of the data has arrived, we need to unpack the data and then free the buffer and the request. */
		if (rreq->ch3.recv_data_sz > 0)
		{
		    MPIDI_CH3U_Request_unpack_uebuf(rreq);
		    MPIU_Free(rreq->ch3.tmpbuf);
		}
		
		mpi_errno = rreq->status.MPI_ERROR;
		if (status != MPI_STATUS_IGNORE)
		{
		    *status = rreq->status;
		}
		
		MPID_Request_release(rreq);
		rreq = NULL;
		
		goto fn_exit;
	    }
	    else
	    {
		/* The data is still being transfered across the net.  We'll leave it to the progress engine to handle once the
		   entire message has arrived. */
		if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
		{
		    MPID_Datatype_get_ptr(datatype, rreq->ch3.datatype_ptr);
		    MPID_Datatype_add_ref(rreq->ch3.datatype_ptr);
		}
	    }
	}
	else if (MPIDI_Request_get_msg_type(rreq) == MPIDI_REQUEST_RNDV_MSG)
	{
	    /* A rendezvous request-to-send (RTS) message has arrived.  We need to send a CTS message to the remote process. */
	    MPID_Request * cts_req;
	    MPIDI_CH3_Pkt_t upkt;
	    MPIDI_CH3_Pkt_rndv_clr_to_send_t * cts_pkt = &upkt.rndv_clr_to_send;
		
	    MPIDI_DBG_PRINTF((15, FCNAME, "rndv RTS in the request, sending rndv CTS"));
	    
	    cts_pkt->type = MPIDI_CH3_PKT_RNDV_CLR_TO_SEND;
	    cts_pkt->sender_req_id = rreq->ch3.sender_req_id;
	    cts_pkt->receiver_req_id = rreq->handle;
	    cts_req = MPIDI_CH3_iStartMsg(vc, cts_pkt, sizeof(*cts_pkt));
	    if (cts_req != NULL)
	    {
		/* FIXME: Ideally we could specify that a req not be returned.  This would avoid our having to decrement the
		   reference count on a req we don't want/need. */
		MPID_Request_release(cts_req);
	    }

	    if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
	    {
		MPID_Datatype_get_ptr(datatype, rreq->ch3.datatype_ptr);
		MPID_Datatype_add_ref(rreq->ch3.datatype_ptr);
	    }
	}
	else if (MPIDI_Request_get_msg_type(rreq) == MPIDI_REQUEST_SELF_MSG)
	{
	    MPID_Request * const sreq = rreq->partner_request;

	    if (sreq != NULL)
	    {
		MPIDI_msg_sz_t data_sz;
		
		MPIDI_CH3U_Buffer_copy(sreq->ch3.user_buf, sreq->ch3.user_count, sreq->ch3.datatype, &sreq->status.MPI_ERROR,
				       buf, count, datatype, &data_sz, &rreq->status.MPI_ERROR);
		rreq->status.count = data_sz;
		MPID_Request_set_completed(sreq);
		MPID_Request_release(sreq);
	    }
	    else
	    {
		/* The sreq is missing which means an error occurred.  rreq->status.MPI_ERROR should have been set when the
		   error was detected. */
	    }

	    if (status != MPI_STATUS_IGNORE)
	    {
		*status = rreq->status;
	    }

	    /* no other thread can possibly be waiting on rreq, so it is safe to reset ref_count and cc */
	    rreq->cc = 0;
	    MPIU_Object_set_ref(rreq, 1);
	}
	else
	{
	    MPIDI_ERR_PRINTF((FCNAME, "requst contains unexpected message type %d", MPIDI_Request_get_msg_type(rreq)));
	    abort();
	}
    }
    else
    {
	/* Message has yet to arrived.  The request has been placed on the list of posted receive requests and populated with
           information supplied in the arguments. */
	MPIDI_DBG_PRINTF((15, FCNAME, "request allocated in posted queue"));
	
	if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
	{
	    MPID_Datatype_get_ptr(datatype, rreq->ch3.datatype_ptr);
	    MPID_Datatype_add_ref(rreq->ch3.datatype_ptr);
	}

	rreq->ch3.recv_pending_count = 1;
        MPID_Request_initialized_set(rreq);
    }

  fn_exit:
    *request = rreq;
    if (rreq)
    {
	MPIDI_DBG_PRINTF((15, FCNAME, "request allocated, handle=0x%08x", rreq->handle));
    }
    else
    {
	MPIDI_DBG_PRINTF((15, FCNAME, "operation complete, no requests allocated"));
    }
    MPIDI_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_RECV);
    return mpi_errno;
}
