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
    MPIDI_STATE_DECL(MPID_STATE_MPID_CANCEL_SEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_CANCEL_SEND);
    MPIDI_DBG_PRINTF((10, FCNAME, "entering"));
    
    assert(sreq->kind == MPID_REQUEST_SEND);

    vc = sreq->comm->vcr[sreq->ch3.match.rank];

    proto = MPIDI_Request_get_msg_type(sreq);

    if (proto == MPIDI_REQUEST_SELF_MSG)
    {
/* FIXME FIXME FIXME */
	abort();
	goto fn_exit;
    }

#if 0    
    /* Check to see if the send is still in the send queue.  If so, remove it, mark the request and cancelled and complete, and
       release the device's reference to the request object.  QUESTION: what is the right interface for MPIDI_CH3_Send_cancel()?
       It needs to be able to cancel requests to send a RTS packet for this request.  Perhaps we can use the partner request
       field to track RTS requests. */
    if (MPIDI_CH3_Send_cancel(vc, sreq->ch3.match, sreq->handle))
    {
	sreq->status.cancelled = TRUE;
	MPID_Request_set_complete(sreq);
	MPID_Request_release(sreq);
	goto fn_exit;
    }
#endif    

    /* Part or all of the message has already been sent, so we need to send a cancellation request to the receiver in an attempt
       to catch the message before it is matched.  OPTIMIZATION: if this is a rendezvous message and the clear to send has
       already been received, then we already know that we cannot cancel the message. */
    {
	/* The completion counter _and_ reference count are incremented to keep the request around long enough to receive a
	   response regardless of what the user does (free the request before waiting, etc.). */
	MPIDI_CH3U_Request_increment_cc(sreq);
	MPIDI_CH3_Request_add_ref(sreq);
    }
    
    /* FIXME: if send cancellation packets are allowed to arrive out-of-order with respect to send packets, then we need to
       timestamp send and cancel packets to insure that a cancellation request does not bypass the send packet to be cancelled
       and erroneously cancel a previously sent message with the same request handle. */

  fn_exit:
    MPIDI_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_CANCEL_SEND);
}
