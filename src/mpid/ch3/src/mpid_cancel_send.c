/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Cancel_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPID_Cancel_send(MPID_Request * sreq)
{
    MPIDI_VC * vc;
    int proto;
    int flag;
    MPIDI_STATE_DECL(MPID_STATE_MPID_CANCEL_SEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_CANCEL_SEND);
    MPIDI_DBG_PRINTF((10, FCNAME, "entering"));
    
    assert(sreq->kind == MPID_REQUEST_SEND);

    MPIDI_Request_cancel_pending(sreq, &flag);
    if (flag)
    {
	goto fn_exit;
    }
    
    vc = sreq->comm->vcr[sreq->ch3.match.rank];

    proto = MPIDI_Request_get_msg_type(sreq);

    if (proto == MPIDI_REQUEST_SELF_MSG)
    {
	MPID_Request * rreq;
	
	MPIDI_DBG_PRINTF((15, FCNAME, "attempting to cancel message sent to self"));
	
	rreq = MPIDI_CH3U_Request_FDU(sreq->handle, &sreq->ch3.match);
	if (rreq)
	{
	    assert(rreq->partner_request == sreq);
	    
	    MPIDI_DBG_PRINTF((15, FCNAME, "send-to-self cancellation successful, sreq=0x%08x, rreq=0x%08x",
			      sreq->handle, rreq->handle));
	    
	    MPIU_Object_set_ref(rreq, 0);
	    MPIDI_CH3_Request_destroy(rreq);
	    
	    sreq->status.cancelled = TRUE;
	    /* no other thread should be waiting on sreq, so it is safe to reset ref_count and cc */
	    sreq->cc = 0;
	    MPIU_Object_set_ref(sreq, 1);
	    /* MPID_Request_set_complete(sreq);
	       MPID_Request_release(sreq); */
	}
	else
	{
	    MPIDI_DBG_PRINTF((15, FCNAME, "send-to-self cancellation failed, sreq=0x%08x, rreq=0x%08x",
			      sreq->handle, rreq->handle));
	}
	
	goto fn_exit;
    }

    /* Check to see if the send is still in the send queue.  If so, remove it, mark the request and cancelled and complete, and
       release the device's reference to the request object.  QUESTION: what is the right interface for MPIDI_CH3_Send_cancel()?
       It needs to be able to cancel requests to send a RTS packet for this request.  Perhaps we can use the partner request
       field to track RTS requests. */
#   if 0
    {
	MPID_Request * sreq2c;
	
	sreq2c = sreq;
	if (proto == MPIDI_REQUEST_RNDV_MSG)
	{
	    if (MPIDI_Request_get_rndv_state(sreq) == MPIDI_REQUEST_RNDV_SENDING_RTS)
	    {
		sreq2c = sreq->partner_request;
	    }
	    else
	    {
		/* Optimization: if request-to-send is know to have already been sent (aggressive write finished or clear-to-send
		   has been received), then we already know that we cannot cancel the message. */
		goto fn_exit;
	    }
	}
	
	if (MPIDI_CH3_Cancel_send(vc, sreq2c))
	{
	    sreq->status.cancelled = TRUE;
	    /* no other thread should be waiting on sreq, so it is safe to reset ref_count and cc */
	    sreq->cc = 0;
	    MPIU_Object_set_ref(sreq, 1);
	    /* MPID_Request_set_complete(sreq);
	       MPID_Request_release(sreq); */
	    goto fn_exit;
	}
    }
#   endif    

    /* Part or all of the message has already been sent, so we need to send a cancellation request to the receiver in an attempt
       to catch the message before it is matched. */
    {
	int old_cc;
	MPIDI_CH3_Pkt_t upkt;
	MPIDI_CH3_Pkt_cancel_send_req_t * const csr_pkt = &upkt.cancel_send_req;
	MPID_Request * csr_sreq;
	
	MPIDI_DBG_PRINTF((15, FCNAME, "sending cancel request to %d for 0x%08x", sreq->ch3.match.rank, sreq->handle));
	
	/* The completion counter and reference count are incremented to keep the request around long enough to receive a
	   response regardless of what the user does (free the request before waiting, etc.). */
	old_cc = sreq->cc;
	MPIDI_CH3U_Request_increment_cc(sreq);
	/* FIXME - MT: the reference count should only be incremented if the completion was zero before the increment.  This
           requires an atomic test for zero and increment. */
	if (old_cc == 0)
	{
	    MPIDI_CH3_Request_add_ref(sreq);
	}

	csr_pkt->type = MPIDI_CH3_PKT_CANCEL_SEND_REQ;
	csr_pkt->match.rank = sreq->comm->rank;
	csr_pkt->match.tag = sreq->ch3.match.tag;
	csr_pkt->match.context_id = sreq->ch3.match.context_id;
	csr_pkt->sender_req_id = sreq->handle;
	
	csr_sreq = MPIDI_CH3_iStartMsg(vc, csr_pkt, sizeof(*csr_pkt));
	if (csr_sreq != NULL)
	{
	    MPID_Request_release(csr_sreq);
	}
    }
    
    /* FIXME: if send cancellation packets are allowed to arrive out-of-order with respect to send packets, then we need to
       timestamp send and cancel packets to insure that a cancellation request does not bypass the send packet to be cancelled
       and erroneously cancel a previously sent message with the same request handle. */

  fn_exit:
    MPIDI_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_CANCEL_SEND);
}
