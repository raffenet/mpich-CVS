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
void MPIDI_CH3U_Handle_recv_pkt(MPIDI_VC * vc, MPIDI_CH3_Pkt_t * pkt)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_HANDLE_RECV_PKT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_HANDLE_RECV_PKT);

    assert(pkt->type < MPIDI_CH3_PKT_END_CH3);
    
    switch(pkt->type)
    {
	case MPIDI_CH3_PKT_READY_SEND:
	    /* XXX - ready send should only use Request_FDP() */
	case MPIDI_CH3_PKT_EAGER_SEND:
	case MPIDI_CH3_PKT_EAGER_SYNC_SEND:
	{
	    MPIDI_CH3_Pkt_eager_send_t * eager_pkt = &pkt->eager_send;
	    MPID_Request * rreq;
	    int found;

	    MPIDI_DBG_PRINTF((30, FCNAME, "received eager send pkt"));
	    MPIDI_DBG_PRINTF((10, FCNAME, "rank=%d, tag=%d, context=%d",
			      eager_pkt->match.rank, eager_pkt->match.tag,
			      eager_pkt->match.context_id));
	    
	    rreq = MPIDI_CH3U_Request_FDP_or_AEU(&eager_pkt->match, &found);
	    assert(rreq != NULL);

	    rreq->status.MPI_SOURCE = eager_pkt->match.rank;
	    rreq->status.MPI_TAG = eager_pkt->match.tag;
	    rreq->status.count = eager_pkt->data_sz;
	    rreq->ch3.vc = vc;
	    rreq->ch3.sender_req_id = eager_pkt->sender_req_id;
	    rreq->ch3.recv_data_sz = eager_pkt->data_sz;
	    MPIDI_Request_set_msg_type(rreq, MPIDI_REQUEST_EAGER_MSG);

	    if (rreq->ch3.recv_data_sz == 0)
	    {
		MPIDI_DBG_PRINTF((30, FCNAME, "null message, %s, decrementing "
				  "completion counter",
				  (found ? "posted request found" :
				   "unexpected request allocated")));
		
		MPIDI_CH3U_Request_complete(rreq);
		
		if (pkt->type == MPIDI_CH3_PKT_EAGER_SYNC_SEND)
		{
		    if (found)
		    {
			
			MPIDI_CH3_Pkt_t upkt;
			MPIDI_CH3_Pkt_eager_sync_ack_t * const esa_pkt =
			    &upkt.eager_sync_ack;
			MPID_Request * esa_req;
		    
			MPIDI_DBG_PRINTF(
			    (30, FCNAME, "sending eager sync ack"));
			
			esa_pkt->type = MPIDI_CH3_PKT_EAGER_SYNC_ACK;
			esa_pkt->sender_req_id = eager_pkt->sender_req_id;
			esa_req = MPIDI_CH3_iStartMsg(
			    vc, esa_pkt, sizeof(*esa_pkt));
			if (esa_req != NULL)
			{
			    MPID_Request_release(esa_req);
			}
		    }
		    else
		    {
			MPIDI_Request_set_sync_send_flag(rreq, TRUE);
		    }
		}
	    }
	    else if (found)
	    {
		int dt_contig;
		MPIDI_msg_sz_t userbuf_sz;
		MPIDI_msg_sz_t data_sz;
		
		MPIDI_DBG_PRINTF((30, FCNAME, "posted request found"));

		if (pkt->type == MPIDI_CH3_PKT_EAGER_SYNC_SEND)
		{
		    MPIDI_CH3_Pkt_t upkt;
		    MPIDI_CH3_Pkt_eager_sync_ack_t * const esa_pkt =
			&upkt.eager_sync_ack;
		    MPID_Request * esa_req;

		    MPIDI_DBG_PRINTF((30, FCNAME, "sending eager sync ack"));
		    esa_pkt->type = MPIDI_CH3_PKT_EAGER_SYNC_ACK;
		    esa_pkt->sender_req_id = eager_pkt->sender_req_id;
		    esa_req = MPIDI_CH3_iStartMsg(vc, esa_pkt,
						  sizeof(*esa_pkt));
		    if (esa_req != NULL)
		    {
			MPID_Request_release(esa_req);
		    }
		}
		
		if (HANDLE_GET_KIND(rreq->ch3.datatype) == HANDLE_KIND_BUILTIN)
		{
		    dt_contig = TRUE;
		    userbuf_sz = rreq->ch3.user_count *
			MPID_Datatype_get_basic_size(rreq->ch3.datatype);
		}
		else
		{
		    MPID_Datatype * dtp;

		    MPID_Datatype_get_ptr(rreq->ch3.datatype, dtp);
		    dt_contig = dtp->is_contig;
		    userbuf_sz = rreq->ch3.user_count * dtp->size;
		}
		
		if (rreq->ch3.recv_data_sz <= userbuf_sz)
		{
		    data_sz = rreq->ch3.recv_data_sz;
		}
		else
		{
		    MPIDI_DBG_PRINTF((35, FCNAME,
				      "receive buffer too small; message "
				      "truncated, msg_sz=" MPIDI_MSG_SZ_FMT
				      ", userbuf_sz=" MPIDI_MSG_SZ_FMT,
				      rreq->ch3.recv_data_sz, userbuf_sz));
		    rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;
		    data_sz = userbuf_sz;
		}

		if (dt_contig && data_sz == rreq->ch3.recv_data_sz)
		{
		    /* user buffer is contiguous and large enough to store the
                       entire message */
		    MPIDI_DBG_PRINTF((35, FCNAME,
				      "IOV loaded for contiguous read"));
		    rreq->ch3.iov[0].MPID_IOV_BUF = rreq->ch3.user_buf;
		    rreq->ch3.iov[0].MPID_IOV_LEN = data_sz;
		    rreq->ch3.iov_count = 1;
		    rreq->ch3.ca = MPIDI_CH3_CA_COMPLETE;
		}
		else
		{
		    /* user buffer is not contiguous or is too small to hold
                       the entire message */

		    int mpi_errno;
		    
		    MPIDI_DBG_PRINTF((35, FCNAME,
				      "IOV loaded for non-contiguous read"));
		    MPID_Segment_init(rreq->ch3.user_buf, rreq->ch3.user_count,
				      rreq->ch3.datatype, &rreq->ch3.segment);
		    rreq->ch3.segment_first = 0;
		    rreq->ch3.segment_size = data_sz;
		    mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
		    if (mpi_errno != MPI_SUCCESS)
		    {
			MPIDI_ERR_PRINTF((FCNAME, "MPIDI_CH3_PKT_EAGER_SEND "
					  "failed to load IOV"));
			abort();
		    }
		}

		MPIDI_DBG_PRINTF((35, FCNAME, "posting iRead"));
		MPIDI_CH3_iRead(vc, rreq);
	    }
	    else /* if (!found) */
	    {
		if (pkt->type != MPIDI_CH3_PKT_READY_SEND)
		{
		    /* TODO: to improve performance, allocate temporary buffer
		       from a specialized buffer pool. */
		    MPIDI_DBG_PRINTF((30, FCNAME,
				      "unexpected request allocated"));
		    rreq->ch3.tmpbuf = MPIU_Malloc(rreq->ch3.recv_data_sz);
		    rreq->ch3.tmpbuf_sz = rreq->ch3.recv_data_sz;
		    
		    rreq->ch3.iov[0].MPID_IOV_BUF = rreq->ch3.tmpbuf;
		    rreq->ch3.iov[0].MPID_IOV_LEN = rreq->ch3.recv_data_sz;
		    rreq->ch3.iov_count = 1;
		    rreq->ch3.ca = MPIDI_CH3_CA_COMPLETE;

		    if (pkt->type == MPIDI_CH3_PKT_EAGER_SYNC_SEND)
		    {
			MPIDI_Request_set_sync_send_flag(rreq, TRUE);
		    }

		    MPIDI_CH3_iRead(vc, rreq);
		}
		else
		{
		    /* If this is a ready-mode send and a matching request has
                       not been posted, then we need to consume the data and
                       mark the request with an error. */
		    
		    int mpi_errno;
		    
		    rreq->status.MPI_ERROR = MPI_ERR_UNKNOWN;
		    rreq->status.count = 0;
		    rreq->ch3.segment_first = 0;
		    rreq->ch3.segment_size = 0;
		    mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
		    assert(mpi_errno != MPI_SUCCESS);
		    MPIDI_CH3_iRead(vc, rreq);
		}
	    }
	    
	    break;
	}

	case MPIDI_CH3_PKT_EAGER_SYNC_ACK:
	{
	    MPIDI_CH3_Pkt_eager_sync_ack_t * esa_pkt = &pkt->eager_sync_ack;
	    MPID_Request * sreq;
	    
	    MPID_Request_get_ptr(esa_pkt->sender_req_id, sreq);
	    MPIDI_CH3U_Request_complete(sreq);
	    break;
	}
	
	case MPIDI_CH3_PKT_RNDV_REQ_TO_SEND:
	{
	    MPIDI_CH3_Pkt_rndv_req_to_send_t * rts_pkt =
		&pkt->rndv_req_to_send;
	    MPID_Request * rreq;
	    int found;

	    MPIDI_DBG_PRINTF((30, FCNAME, "received rndv RTS pkt"));
	    MPIDI_DBG_PRINTF((35, FCNAME, "rank=%d, tag=%d, context=%d",
			      rts_pkt->match.rank, rts_pkt->match.tag,
			      rts_pkt->match.context_id));
	    
	    rreq = MPIDI_CH3U_Request_FDP_or_AEU(&rts_pkt->match, &found);
	    assert(rreq != NULL);
	    
	    rreq->status.MPI_SOURCE = rts_pkt->match.rank;
	    rreq->status.MPI_TAG = rts_pkt->match.tag;
	    rreq->status.count = rts_pkt->data_sz;
	    rreq->ch3.vc = vc;
	    rreq->ch3.sender_req_id = rts_pkt->sender_req_id;
	    rreq->ch3.recv_data_sz = rts_pkt->data_sz;
	    MPIDI_Request_set_msg_type(rreq, MPIDI_REQUEST_RNDV_MSG);
	    
	    if (found)
	    {
		MPID_Request * cts_req;
		MPIDI_CH3_Pkt_t upkt;
		MPIDI_CH3_Pkt_rndv_clr_to_send_t * cts_pkt =
		    &upkt.rndv_clr_to_send;
		
		MPIDI_DBG_PRINTF((30, FCNAME, "posted request found"));

		/* XXX: What if the receive user buffer is not big enough to
                   hold the data about to be cleared for sending? */
		
		MPIDI_DBG_PRINTF((30, FCNAME, "sending rndv CTS packet"));
		cts_pkt->type = MPIDI_CH3_PKT_RNDV_CLR_TO_SEND;
		cts_pkt->sender_req_id = rts_pkt->sender_req_id;
		cts_pkt->receiver_req_id = rreq->handle;
		cts_req = MPIDI_CH3_iStartMsg(vc, cts_pkt, sizeof(*cts_pkt));
		if (cts_req != NULL)
		{
		    MPID_Request_release(cts_req);
		}
	    }
	    else
	    {
		MPIDI_DBG_PRINTF((30, FCNAME, "unexpected request allocated"));
	    }

	    break;
	}
	
	case MPIDI_CH3_PKT_RNDV_CLR_TO_SEND:
	{
	    MPIDI_CH3_Pkt_rndv_clr_to_send_t * cts_pkt =
		&pkt->rndv_clr_to_send;
	    MPID_Request * sreq;
	    MPIDI_CH3_Pkt_t upkt;
	    MPIDI_CH3_Pkt_rndv_send_t * rs_pkt = &upkt.rndv_send;
	    int dt_contig;
	    MPIDI_msg_sz_t data_sz;
	    MPID_IOV iov[MPID_IOV_LIMIT];
	    int iov_n;
	    int mpi_errno = MPI_SUCCESS;
	    
	    MPIDI_DBG_PRINTF((30, FCNAME, "received rndv CTS pkt"));

	    MPID_Request_get_ptr(cts_pkt->sender_req_id, sreq);

	    rs_pkt->type = MPIDI_CH3_PKT_RNDV_SEND;
	    rs_pkt->receiver_req_id = cts_pkt->receiver_req_id;
	    iov[0].MPID_IOV_BUF = rs_pkt;
	    iov[0].MPID_IOV_LEN = sizeof(*rs_pkt);

	    if (HANDLE_GET_KIND(sreq->ch3.datatype) == HANDLE_KIND_BUILTIN)
	    {
		dt_contig = TRUE;
		data_sz = sreq->ch3.user_count *
		    MPID_Datatype_get_basic_size(sreq->ch3.datatype);
	    }
	    else
	    {
		MPID_Datatype * dtp;
		
		MPID_Datatype_get_ptr(sreq->ch3.datatype, dtp);
		dt_contig = dtp->is_contig;
		data_sz = sreq->ch3.user_count * dtp->size;
	    }
	
	    if (dt_contig) 
	    {
		MPIDI_DBG_PRINTF((30, FCNAME, "sending contiguous rndv data, "
				  "data_sz=" MPIDI_MSG_SZ_FMT, data_sz));
		
		sreq->ch3.ca = MPIDI_CH3_CA_COMPLETE;
		
		iov[1].MPID_IOV_BUF = sreq->ch3.user_buf;
		iov[1].MPID_IOV_LEN = data_sz;
		iov_n = 2;
	    }
	    else
	    {
		MPID_Segment_init(sreq->ch3.user_buf, sreq->ch3.user_count,
				  sreq->ch3.datatype, &sreq->ch3.segment);
		iov_n = MPID_IOV_LIMIT - 1;
		sreq->ch3.segment_first = 0;
		sreq->ch3.segment_size = data_sz;
		mpi_errno = MPIDI_CH3U_Request_load_send_iov(
		    sreq, &iov[1], &iov_n);
		if (mpi_errno != MPI_SUCCESS)
		{
		    MPIDI_ERR_PRINTF((FCNAME, "MPIDI_CH3_PKT_RNDV_CLR_TO_SEND "
				      "failed to load IOV"));
		    abort();
		}
		iov_n += 1;
	    }
	    
	    MPIDI_CH3_iSendv(vc, sreq, iov, iov_n);
	    break;
	}
	
	case MPIDI_CH3_PKT_RNDV_SEND:
	{
	    MPIDI_CH3_Pkt_rndv_send_t * rs_pkt = &pkt->rndv_send;
	    MPID_Request * rreq;
	    MPIDI_msg_sz_t dt_sz;
	    int dt_contig;
		    
	    MPID_Request_get_ptr(rs_pkt->receiver_req_id, rreq);
	    
	    MPIDI_DBG_PRINTF((30, FCNAME, "received rndv send (data) pkt"));

	    if (HANDLE_GET_KIND(rreq->ch3.datatype) == HANDLE_KIND_BUILTIN)
	    {
		MPID_Datatype_get_size_macro(rreq->ch3.datatype, dt_sz);
		dt_contig = TRUE;
	    }
	    else
	    {
		MPIDI_ERR_PRINTF((FCNAME, "only basic datatypes are supported"
				  "(MPIDI_CH3_PKT_RNDV_SEND)"));
		abort();
	    }
		    
	    if (dt_contig) 
	    {
		if (rreq->ch3.recv_data_sz <= dt_sz * rreq->ch3.user_count)
		{
		    rreq->ch3.iov[0].MPID_IOV_BUF = rreq->ch3.user_buf;
		    rreq->ch3.iov[0].MPID_IOV_LEN = rreq->ch3.recv_data_sz;
		    rreq->ch3.iov_count = 1;
		    rreq->ch3.ca = MPIDI_CH3_CA_COMPLETE;
		    MPIDI_CH3_iRead(vc, rreq);
		}
		else
		{
		    MPIDI_ERR_PRINTF((FCNAME, "receive buffer overflow "
				      "(MPIDI_CH3_PKT_RNDV_SEND)"));
		    abort();
		    /* TODO: handle buffer overflow properly */
		}
	    }
	    else
	    {
		MPIDI_ERR_PRINTF((FCNAME, "only contiguous data is supported "
				  "(MPIDI_CH3_PKT_RNDV_SEND)"));
		abort();
	    }
		
	    break;
	}
	
	case MPIDI_CH3_PKT_CANCEL_SEND_REQ:
	{
	    MPIDI_CH3_Pkt_cancel_send_req_t * req_pkt = &pkt->cancel_send_req;
	    MPID_Request * rreq;
	    MPIDI_CH3_Pkt_t upkt;
	    MPIDI_CH3_Pkt_cancel_send_resp_t * resp_pkt =
		&upkt.cancel_send_resp;
	    MPID_Request * resp_req;

	    MPIDI_DBG_PRINTF((30, FCNAME, "received cancel send req pkt"));
	    
	    rreq = MPIDI_CH3U_Request_FDU(req_pkt->sender_req_id,
					  &req_pkt->match);
	    if (rreq != NULL)
	    {
		MPIDI_DBG_PRINTF((35, FCNAME, "message cancelled"));
		MPID_Request_release(rreq);
		resp_pkt->ack = TRUE;
	    }
	    else
	    {
		MPIDI_DBG_PRINTF((35, FCNAME, "unable to cancel message"));
		resp_pkt->ack = FALSE;
	    }
	    
	    resp_pkt->type = MPIDI_CH3_PKT_CANCEL_SEND_RESP;
	    resp_pkt->sender_req_id = req_pkt->sender_req_id;
	    resp_req = MPIDI_CH3_iStartMsg(vc, resp_pkt, sizeof(*resp_pkt));
	    if (resp_req != NULL)
	    {
		MPID_Request_release(resp_req);
	    }
	    
	    break;
	}
	
	case MPIDI_CH3_PKT_CANCEL_SEND_RESP:
	{
	    MPIDI_CH3_Pkt_cancel_send_resp_t * resp_pkt =
		&pkt->cancel_send_resp;
	    MPID_Request * sreq;
	    int cc;

	    MPIDI_DBG_PRINTF((30, FCNAME, "received cancel send resp pkt"));
	    
	    MPID_Request_get_ptr(resp_pkt->sender_req_id, sreq);
	    
	    if (resp_pkt->ack)
	    {
		sreq->status.cancelled = TRUE;
		MPIDI_DBG_PRINTF((35, FCNAME, "message cancelled"));
	    }
	    else
	    {
		MPIDI_DBG_PRINTF((35, FCNAME, "unable to cancel message"));
	    }

	    MPIDI_CH3U_Request_decrement_cc(sreq, &cc);
	    MPID_Request_release(sreq);
	    
	    break;
	}
	
	case MPIDI_CH3_PKT_PUT:
	{
	    MPIDI_DBG_PRINTF((30, FCNAME, "received put pkt"));
	    MPIDI_ERR_PRINTF((FCNAME, "MPIDI_CH3_PKT_PUT UMIMPLEMENTED"));
	    abort();
	    break;
	}
	
	case MPIDI_CH3_PKT_FLOW_CNTL_UPDATE:
	{
	    MPIDI_DBG_PRINTF((30, FCNAME, "received flow control update pkt"));
	    MPIDI_ERR_PRINTF((FCNAME,
			      "MPIDI_CH3_PKT_FLOW_CNTL_UPDATE UMIMPLEMENTED"));
	    abort();
	    break;
	}
	
	default:
	{
	    MPIDI_ERR_PRINTF((FCNAME, "packet type %d not implemented.\n",
			      pkt->type));
	    abort();
	}
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_HANDLE_RECV_PKT);
}
