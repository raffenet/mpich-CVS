/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*
 * MPIDI_CH3U_Handle_recv_pkt()
 *
 * NOTE: This routine must be reentrant.  Routines like MPIDI_CH3_iRead() are
 * allowed to perform additional up-calls if they complete the requested work
 * immediately. *** Care must be take to avoid deep recursion.  With some
 * thread packages, exceeding the stack space allocated to a thread can result
 * in overwriting the stack of another thread. ***
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Handle_recv_pkt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Handle_recv_pkt(MPIDI_VC * vc, MPIDI_CH3_Pkt_t * pkt)
{
    int completion = FALSE;

    assert(pkt->type < MPIDI_CH3_PKT_END_CH3);
    
    switch(pkt->type)
    {
	case MPIDI_CH3_PKT_EAGER_SEND:
	{
	    MPIDI_CH3_Pkt_eager_send_t * eager_pkt = &pkt->eager_send;
	    MPID_Request * rreq;
	    int found;

	    MPIDI_dbg_printf(30, FCNAME, "received eager send packet");
	    MPIDI_dbg_printf(10, FCNAME, "rank=%d, tag=%d, context=%d",
			     eager_pkt->match.rank, eager_pkt->match.tag,
			     eager_pkt->match.context_id);
	    
	    rreq = MPIDI_CH3U_Request_FPOAU(&eager_pkt->match, &found);
	    assert(rreq != NULL);
	    
	    rreq->status.MPI_SOURCE = eager_pkt->match.rank;
	    rreq->status.MPI_TAG = eager_pkt->match.tag;
	    rreq->ch3.vc = vc;
	    rreq->ch3.recv_data_sz = eager_pkt->data_sz;
	    MPIDI_Request_set_msg_type(rreq, MPIDI_REQUEST_EAGER_MSG);

	    if (rreq->ch3.recv_data_sz == 0)
	    {
		int cc;
		
		MPIDI_dbg_printf(30, FCNAME, "null message, %s, decrementing "
				 "completion counter",
				 (found ? "found in posted queue" :
				  "allocated in unexpected queue"));
		
		MPIDI_CH3U_Request_decrement_cc(rreq, &cc);
		if (cc == 0)
		{
		    completion = TRUE;
		    MPID_Request_free(rreq);
		}
	    }
	    else if (found)
	    {
		long dt_sz;
		int dt_contig;
		    
		MPIDI_dbg_printf(30, FCNAME, "found match in posted queue");

		if (HANDLE_GET_KIND(rreq->ch3.datatype) == HANDLE_KIND_BUILTIN)
		{
		    dt_sz = MPID_Datatype_get_size(rreq->ch3.datatype);
		    dt_contig = TRUE;
		}
		else
		{
		    MPIDI_err_printf(
			FCNAME, "only basic datatypes are supported"
			"(MPIDI_CH3_PKT_RNDV_REQ_TO_SEND)");
		    abort();
		}
		    
		if (dt_contig) 
		{
		    if (rreq->ch3.recv_data_sz <= dt_sz * rreq->ch3.user_count)
		    {
			rreq->ch3.iov[0].iov_base = rreq->ch3.user_buf;
			rreq->ch3.iov[0].iov_len = rreq->ch3.recv_data_sz;
			rreq->ch3.iov_count = 1;
			rreq->ch3.iov_offset = 0;
			rreq->ch3.ca = MPIDI_CH3_CA_COMPLETE;
			MPIDI_CH3_iRead(vc, rreq);
		    }
		    else
		    {
			MPIDI_err_printf(FCNAME, "receive buffer overflow");
			abort();
			/* TODO: handle buffer overflow properly */
		    }
		}
		else
		{
		    MPIDI_err_printf(
			FCNAME, "only contiguous data is supported "
			"(MPIDI_CH3_PKT_RNDV_REQ_TO_SEND)");
		    abort();
		}
	    }
	    else
	    {
		MPIDI_dbg_printf(30, FCNAME,
				 "allocated request in unexpected queue");
		
		/* TODO: to improve performance, allocate temporary buffer
		   from a specialized buffer pool. */
		    
		rreq->ch3.tmp_buf = MPIU_Malloc(rreq->ch3.recv_data_sz);
		rreq->ch3.tmp_sz = rreq->ch3.recv_data_sz;
		    
		rreq->ch3.iov[0].iov_base = rreq->ch3.tmp_buf;
		rreq->ch3.iov[0].iov_len = rreq->ch3.recv_data_sz;
		rreq->ch3.iov_count = 1;
		rreq->ch3.iov_offset = 0;
		rreq->ch3.ca = MPIDI_CH3_CA_COMPLETE;
		MPIDI_CH3_iRead(vc, rreq);
	    }
	    
	    break;
	}
	
	case MPIDI_CH3_PKT_RNDV_REQ_TO_SEND:
	{
	    MPIDI_CH3_Pkt_rndv_req_to_send_t * rts_pkt =
		&pkt->rndv_req_to_send;
	    MPID_Request * rreq;
	    int found;

	    MPIDI_dbg_printf(30, FCNAME, "received rendezvous RTS packet");
	    MPIDI_dbg_printf(10, FCNAME, "rank=%d, tag=%d, context=%d",
			     rts_pkt->match.rank, rts_pkt->match.tag,
			     rts_pkt->match.context_id);
	    
	    rreq = MPIDI_CH3U_Request_FPOAU(&rts_pkt->match, &found);
	    assert(rreq != NULL);
	    
	    rreq->status.MPI_SOURCE = rts_pkt->match.rank;
	    rreq->status.MPI_TAG = rts_pkt->match.tag;
	    rreq->ch3.vc = vc;
	    rreq->ch3.recv_data_sz = rts_pkt->data_sz;
	    rreq->ch3.rndv_req_id = rts_pkt->req_id_sender;
	    MPIDI_Request_set_msg_type(rreq, MPIDI_REQUEST_RNDV_MSG);
	    
	    if (found)
	    {
		MPID_Request * cts_req;
		MPIDI_CH3_Pkt_t upkt;
		MPIDI_CH3_Pkt_rndv_clr_to_send_t * cts_pkt =
		    &upkt.rndv_clr_to_send;
		
		MPIDI_dbg_printf(30, FCNAME, "found match in posted queue");

		/* XXX: What if the receive user buffer is not big enough to
                   hold the data about to be cleared for sending? */
		
		MPIDI_dbg_printf(30, FCNAME, "sending rendezvous CTS packet");
		cts_pkt->type = MPIDI_CH3_PKT_RNDV_CLR_TO_SEND;
		cts_pkt->req_id_sender = rts_pkt->req_id_sender;
		cts_pkt->req_id_receiver = rreq->handle;
		cts_req = MPIDI_CH3_iStartMsg(vc, cts_pkt, sizeof(*cts_pkt));
		if (cts_req != NULL)
		{
		    MPID_Request_free(cts_req);
		}
	    }
	    else
	    {
		MPIDI_dbg_printf(30, FCNAME,
				 "allocated request in unexpected queue");
	    }

	    break;
	}
	
	case MPIDI_CH3_PKT_RNDV_CLR_TO_SEND:
	{
	    MPIDI_CH3_Pkt_rndv_clr_to_send_t * cts_pkt =
		&pkt->rndv_clr_to_send;
	    MPID_Request * sreq;
	    long dt_sz;
	    int dt_contig;
	    MPIDI_CH3_Pkt_t upkt;
	    MPIDI_CH3_Pkt_rndv_send_t * rs_pkt = &upkt.rndv_send;
	    
	    MPIDI_dbg_printf(30, FCNAME, "received rendezvous CTS packet");

	    MPID_Request_get_ptr(cts_pkt->req_id_sender, sreq);

	    rs_pkt->type = MPIDI_CH3_PKT_RNDV_SEND;
	    rs_pkt->req_id_receiver = cts_pkt->req_id_receiver;

	    if (HANDLE_GET_KIND(sreq->ch3.datatype) == HANDLE_KIND_BUILTIN)
	    {
		dt_sz = MPID_Datatype_get_size(sreq->ch3.datatype);
		dt_contig = TRUE;
	    }
	    else
	    {
		MPIDI_err_printf(FCNAME, "only basic datatypes are supported"
				 "(MPIDI_CH3_PKT_RNDV_CLR_TO_SEND)");
		abort();
	    }
		    
	    if (dt_contig) 
	    {
		struct iovec iov[2];

		MPIDI_dbg_printf(30, FCNAME, "sending rndv pkt + data");
		iov[0].iov_base = rs_pkt;
		iov[0].iov_len = sizeof(*rs_pkt);
		iov[1].iov_base = sreq->ch3.user_buf;
		iov[1].iov_len = sreq->ch3.user_count * dt_sz;
		sreq->ch3.ca = MPIDI_CH3_CA_COMPLETE;
		MPIDI_CH3_iSendv(vc, sreq, iov, 2);
	    }
	    else
	    {
		MPIDI_err_printf(FCNAME, "only contiguous data is supported"
				 "(MPIDI_CH3_PKT_RNDV_CLR_TO_SEND)");
		abort();
	    }
	    
	    break;
	}
	
	case MPIDI_CH3_PKT_RNDV_SEND:
	{
	    MPIDI_CH3_Pkt_rndv_send_t * rs_pkt = &pkt->rndv_send;
	    MPID_Request * rreq;
	    long dt_sz;
	    int dt_contig;
		    
	    MPID_Request_get_ptr(rs_pkt->req_id_receiver, rreq);
	    
	    MPIDI_dbg_printf(30, FCNAME, "received rendezvous send packet");

	    if (HANDLE_GET_KIND(rreq->ch3.datatype) == HANDLE_KIND_BUILTIN)
	    {
		dt_sz = MPID_Datatype_get_size(rreq->ch3.datatype);
		dt_contig = TRUE;
	    }
	    else
	    {
		MPIDI_err_printf(FCNAME, "only basic datatypes are supported"
				 "(MPIDI_CH3_PKT_RNDV_SEND)");
		abort();
	    }
		    
	    if (dt_contig) 
	    {
		if (rreq->ch3.recv_data_sz <= dt_sz * rreq->ch3.user_count)
		{
		    rreq->ch3.iov[0].iov_base = rreq->ch3.user_buf;
		    rreq->ch3.iov[0].iov_len = rreq->ch3.recv_data_sz;
		    rreq->ch3.iov_count = 1;
		    rreq->ch3.iov_offset = 0;
		    rreq->ch3.ca = MPIDI_CH3_CA_COMPLETE;
		    MPIDI_CH3_iRead(vc, rreq);
		}
		else
		{
		    MPIDI_err_printf(FCNAME, "receive buffer overflow "
				     "(MPIDI_CH3_PKT_RNDV_SEND)");
		    abort();
		    /* TODO: handle buffer overflow properly */
		}
	    }
	    else
	    {
		MPIDI_err_printf(FCNAME, "only contiguous data is supported "
				 "(MPIDI_CH3_PKT_RNDV_SEND)");
		abort();
	    }
		
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
	    MPIDI_err_printf(FCNAME, "MPIDI_CH3U_Handle_pkt(): packet type "
			     "%d not implemented.\n", pkt->type);
	    abort();
	}
    }

    return completion;
}
