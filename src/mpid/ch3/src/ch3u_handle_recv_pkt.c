/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*
 * MPIDI_CH3U_Handle_recv_pkt()
 *
 * NOTE: This routine must be reentrant safe.  Routines like MPIDI_CH3_iRead()
 * are allowed to perform up-calls if they complete the requested work
 * immediately. *** Care must be take to avoid deep recursion which with some
 * thread packages can result in overwriting the stack of another thread. ***
 */
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
		    int cc;

		    MPIDI_CH3U_Request_decrement_cc(req, &cc);
		    if (cc == 0)
		    {
			completion = TRUE;
			MPID_Request_free(req);
		    }
		}
		else
		{
		    long dt_sz;
		    int dt_contig;
		    void * const buf = req->ch3.user_buf;
		    const int count = req->ch3.user_count;
		    const MPI_Datatype datatype = req->ch3.datatype;
		    
		    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN)
		    {
			dt_sz = MPID_Datatype_get_size(datatype);
			dt_contig = TRUE;
		    }
		    else
		    {
			assert(HANDLE_GET_KIND(datatype) ==
			       HANDLE_KIND_BUILTIN);
			abort();
		    }
		    
		    if (dt_contig) 
		    {
			if (pkt->data_sz <= dt_sz * count)
			{
			    req->ch3.iov[0].iov_base = buf;
			    req->ch3.iov[0].iov_len = pkt->data_sz;
			    req->ch3.iov_count = 1;
			    req->ch3.iov_offset = 0;
			    req->ch3.ca = MPIDI_CH3_CA_COMPLETE;
			    MPIDI_CH3_iRead(vc, req);
			}
			else
			{
			    assert(pkt->data_sz < dt_sz * count);
			    /* TODO: handle buffer overflow */
			}
		    }
		    else
		    {
			assert(dt_contig);
			abort();
		    }
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
	    MPIDI_CH3_Pkt_rndv_req_to_send_t * pkt = &upkt->rndv_req_to_send;
	    MPID_Request * req;
	    int found;

	    MPIDI_dbg_printf(30, FCNAME, "received rendezvous RTS packet");
	    MPIDI_dbg_printf(10, FCNAME, "rank=%d, tag=%d, context=%d",
			     pkt->match.rank, pkt->match.tag,
			     pkt->match.context_id);
	    
	    req = MPIDI_CH3U_Request_FPOAU(&pkt->match, &found);

	    if (found)
	    {
		MPIDI_dbg_printf(30, FCNAME, "found match in posted queue");
		/* TODO: send CTS including new req */
	    }
	    else
	    {
		MPIDI_dbg_printf(30, FCNAME,
				 "allocated request in unexpected queue");
		/* TODO: mark new request as RNDV_RTS so MPID_Recv() knows to
                   send CTS */
	    }
	    
	    MPIDI_dbg_printf(30, FCNAME, "UMIMPLEMENTED");
	    abort();
	    break;
	}
	
	case MPIDI_CH3_PKT_RNDV_CLR_TO_SEND:
	{
	    MPIDI_CH3_Pkt_rndv_clr_to_send_t * pkt = &upkt->rndv_clr_to_send;
	    MPID_Request * sreq;
	    MPID_Request * rreq;
	    
	    MPIDI_dbg_printf(30, FCNAME, "received rendezvous CTS packet");

	    MPID_Request_get_ptr(pkt->req_id_sender, sreq);
	    MPID_Request_get_ptr(pkt->req_id_receiver,rreq);

	    /* TODO: construct RNDV_SEND pkt and send */
	    
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


