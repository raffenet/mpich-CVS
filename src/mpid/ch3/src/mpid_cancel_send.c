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
#if 0    
    assert(sreq->kind == MPID_REQUEST_SEND);
    /* XXX - need to handle persistent requests */

    /* Check to see if the send is still in the send queue.  If so, remove it,
       mark cancelled, decrement completion counter, and release reference to
       object. */

    /* MT - need to lock the request queue */
    {
	/* message can only be removed from the send queue if the first packet
           has not been sent */
    }
    /* MT - need to unlock the request queue */
    

    if (MPIDI_Request_get_msg_type(sreq) == MPIDI_REQUEST_EAGER_MSG ||
	MPIDI_Request_get_rndv_state(sreq) != MPIDI_REQUEST_RNDV_SENDING)
    {
	/* If this is a rendezvous message and the clear to send has already
	   been received or a ready send, then we cannot cancel the message.
	   In all other cases, a cancellation request be sent to the receiver
	   in an attempt to catch the message before it is matched.  The
	   completion counter _and_ reference count are incremented to keep the
	   request around long enough to receive a response regardless of what
	   the user does (free the request before waiting, etc.). */
    }
    
    /* NOTE: if we decide to send cancellation requests out-of-band, then we
       need to timestamp messages and cancellation request to insure that a
       cancellation request does not bypass the message to be cancelled and
       erroneous cancel a previously sent message that is buffered at the
       receiver. */
#endif
}
