/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/*
 * MPIDI_CH3_iWrite()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iWrite
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3_iWrite(MPIDI_VC * vc, MPID_Request * req)
{
    int nb;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_IWRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_IWRITE);
    assert(vc->ib.state = MPIDI_CH3I_VC_STATE_CONNECTED);
    req->ib.iov_offset = 0;

    MPIU_dbg_printf("ch3_iwrite\n");
    /*MPIDI_CH3I_IB_post_write(vc, req);*/
    vc->ib.send_active = req;
    nb = ibu_post_writev(vc->ib.ibu, req->ch3.iov + req->ib.iov_offset, req->ch3.iov_count - req->ib.iov_offset, NULL);

    if (nb > 0)
    {
	if (MPIDI_CH3I_Request_adjust_iov(req, nb))
	{
	    /* Write operation complete */
	    MPIDI_CA_t ca = req->ch3.ca;
	    
	    vc->ib.send_active = NULL;
	    
	    if (ca == MPIDI_CH3_CA_COMPLETE)
	    {
		MPIDI_CH3I_SendQ_dequeue(vc);
		/*post_queued_send(vc); is reduced to: */ vc->ib.send_active = MPIDI_CH3I_SendQ_head(vc);
		/* mark data transfer as complete and decrment CC */
		req->ch3.iov_count = 0;
		MPIDI_CH3U_Request_complete(req);
	    }
	    else if (ca == MPIDI_CH3I_CA_HANDLE_PKT)
	    {
		MPIDI_CH3_Pkt_t * pkt = &req->ib.pkt;
		
		if (pkt->type < MPIDI_CH3_PKT_END_CH3)
		{
		    /*post_queued_send(vc); is reduced to: */ vc->ib.send_active = MPIDI_CH3I_SendQ_head(vc);
		}
		else
		{
		    MPIDI_DBG_PRINTF((71, FCNAME, "unknown packet type %d", pkt->type));
		}
	    }
	    else if (ca < MPIDI_CH3_CA_END_CH3)
	    {
		/*DBGMSG((65, "finished sending iovec, calling CH3U_Handle_send_req()"));*/
		MPIDI_CH3U_Handle_send_req(vc, req);
		if (vc->ib.send_active == NULL)
		{
		/* NOTE: This code assumes that if another write is not posted by the device during the callback, then the
		device has completed the current request.  As a result, the current request is dequeded and next request
		    in the queue is processed. */
		    /*DBGMSG((65, "request (assumed) complete"));
		    DBGMSG((65, "dequeuing req and posting next send"));*/
		    MPIDI_CH3I_SendQ_dequeue(vc);
		    /*post_queued_send(vc); is reduced to: */ vc->ib.send_active = MPIDI_CH3I_SendQ_head(vc);
		}
	    }
	    else
	    {
		assert(ca < MPIDI_CH3I_CA_END_IB);
	    }
	}
	else
	{
	    assert(req->ib.iov_offset < req->ch3.iov_count);
	}
    }
    else if (nb == 0)
    {
	MPIDI_DBG_PRINTF((55, FCNAME, "unable to write, enqueuing"));
	MPIDI_CH3I_SendQ_enqueue(vc, req);
	/*MPIDI_CH3I_IB_post_write(vc, sreq);*/
    }
    else
    {
	/* Connection just failed.  Mark the request complete and return an error. */
	vc->ib.state = MPIDI_CH3I_VC_STATE_FAILED;
	/* TODO: Create an appropriate error message based on the value of errno */
	req->status.MPI_ERROR = MPI_ERR_INTERN;
	/* MT - CH3U_Request_complete performs write barrier */
	MPIDI_CH3U_Request_complete(req);
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_IWRITE);
}
