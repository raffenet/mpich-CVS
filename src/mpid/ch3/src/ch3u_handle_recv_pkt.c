/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Handle_recv_pkt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Handle_recv_pkt(MPIDI_VC * vc, MPIDI_CH3_Pkt_t * upkt)
{
    int completion = FALSE;

    assert(upkt->type < MPIDI_CH3_PKT_END_CH3);
    
    switch(upkt->type)
    {
	case MPIDI_CH3_PKT_EAGER_SEND:
	{
	    MPIDI_CH3_Pkt_eager_send_t * pkt = &upkt->eager_send;
	    MPID_Request * req;
	    int found;

	    MPIDI_dbg_printf(30, FCNAME, "received eager send packet");
	    MPIDI_dbg_printf(10, FCNAME, "rank=%d, tag=%d, context=%d",
			     pkt->match.rank, pkt->match.tag,
			     pkt->match.context_id);
	    
	    req = MPIDI_CH3U_Request_FPOAU(&pkt->match, &found);

	    if (found)
	    {
		MPIDI_dbg_printf(30, FCNAME, "found match in posted queue");
		if (pkt->data_sz == 0)
		{
		    if (--(*req->cc_ptr) == 0)
		    {
			completion = TRUE;
			MPID_Request_free(req);
		    }
		}
		else
		{
		    assert(pkt->data_sz == 0);
		}
	    }
	    else
	    {
		MPIDI_dbg_printf(30, FCNAME,
				 "allocated request in unexpected queue");
		req->ch3.match = pkt->match;
		req->cc_ptr = &req->cc;
		if (pkt->data_sz == 0)
		{
		    req->cc = 0;
		}
		else
		{
		    assert(pkt->data_sz == 0);
		    
		    /* TODO: allocate temporary buffer, set up request, and
                       call CH3_iRead */
		    
		    req->cc = 1;
		    req->ch3.iov[0].iov_base = NULL; /* XXX */
		    req->ch3.iov[0].iov_len = pkt->data_sz;
		    req->ch3.iov_count = 1;
		    req->ch3.iov_offset = 0;
		    req->ch3.ca = MPIDI_CH3_CA_COMPLETE;
		}
	    }
	    
	    break;
	}
	
	case MPIDI_CH3_PKT_RNDV_REQ_TO_SEND:
	{
	    MPIDI_dbg_printf(30, FCNAME, "received rendezvous RTS packet");
	    MPIDI_dbg_printf(30, FCNAME, "UMIMPLEMENTED");
	    abort();
	    break;
	}
	
	case MPIDI_CH3_PKT_RNDV_CLR_TO_SEND:
	{
	    MPIDI_dbg_printf(30, FCNAME, "received rendezvous CTS packet");
	    MPIDI_dbg_printf(30, FCNAME, "UMIMPLEMENTED");
	    abort();
	    break;
	}
	
	case MPIDI_CH3_PKT_RNDV_SEND:
	{
	    MPIDI_dbg_printf(30, FCNAME, "received rendezvous send packet");
	    MPIDI_dbg_printf(30, FCNAME, "UMIMPLEMENTED");
	    abort();
	    break;
	}
	
	case MPIDI_CH3_PKT_CANCEL_SEND_REQ:
	{
	    MPIDI_dbg_printf(30, FCNAME,
			     "received cancel send request packet");
	    MPIDI_dbg_printf(30, FCNAME, "UMIMPLEMENTED");
	    abort();
	    break;
	}
	
	case MPIDI_CH3_PKT_CANCEL_SEND_RESP:
	{
	    MPIDI_dbg_printf(30, FCNAME,
			     "received cancel send response packet");
	    MPIDI_dbg_printf(30, FCNAME, "UMIMPLEMENTED");
	    abort();
	    break;
	}
	
	case MPIDI_CH3_PKT_PUT:
	{
	    MPIDI_dbg_printf(30, FCNAME, "received put packet");
	    MPIDI_dbg_printf(30, FCNAME, "UMIMPLEMENTED");
	    abort();
	    break;
	}
	
	case MPIDI_CH3_PKT_FLOW_CNTL_UPDATE:
	{
	    MPIDI_dbg_printf(30, FCNAME,
			     "received flow control update packet");
	    MPIDI_dbg_printf(30, FCNAME, "UMIMPLEMENTED");
	    abort();
	    break;
	}
	
	default:
	{
	    dbg_printf(
		"MPIDI_CH3U_Handle_pkt(): packet type %d not implemented.\n",
		upkt->type);
	    abort();
	}
    }

    return completion;
}


