/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

static void post_data_receive(MPIDI_VC * vc, MPID_Request * rreq, int found);

#define set_request_info(_rreq, _pkt, _msg_type)		\
{								\
    (_rreq)->status.MPI_SOURCE = (_pkt)->match.rank;		\
    (_rreq)->status.MPI_TAG = (_pkt)->match.tag;		\
    (_rreq)->status.count = (_pkt)->data_sz;			\
    (_rreq)->ch3.sender_req_id = (_pkt)->sender_req_id;		\
    (_rreq)->ch3.recv_data_sz = (_pkt)->data_sz;		\
    MPIDI_CH3U_Request_set_seqnum((_rreq), (_pkt)->seqnum);	\
    MPIDI_Request_set_msg_type((_rreq), (_msg_type));		\
}

/*
 * MPIDI_CH3U_Handle_recv_pkt()
 *
 * NOTE: This routine must be reentrant.  Routines like MPIDI_CH3_iRead() are allowed to perform additional up-calls if they
 * complete the requested work immediately. *** Care must be take to avoid deep recursion.  With some thread packages, exceeding
 * the stack space allocated to a thread can result in overwriting the stack of another thread. ***
 */
void MPIDI_CH3U_Handle_unordered_recv_pkt(MPIDI_VC * vc, MPIDI_CH3_Pkt_t * pkt);
void MPIDI_CH3U_Handle_ordered_recv_pkt(MPIDI_VC * vc, MPIDI_CH3_Pkt_t * pkt);
#if defined(MPIDI_CH3_MSGS_UNORDERED)
#define MPIDI_CH3U_Handle_unordered_recv_pkt MPIDI_CH3U_Handle_recv_pkt
#else
#define MPIDI_CH3U_Handle_ordered_recv_pkt MPIDI_CH3U_Handle_recv_pkt 
#endif

#if defined(MPIDI_CH3_MSGS_UNORDERED)

#define MPIDI_CH3U_Pkt_send_container_alloc() (MPIU_Malloc(sizeof(MPIDI_CH3_Pkt_send_container_t)))
#define MPIDI_CH3U_Pkt_send_container_free(pc) MPIU_Free(pc)

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Handle_unordered_recv_pkt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3U_Handle_unordered_recv_pkt(MPIDI_VC * vc, MPIDI_CH3_Pkt_t * pkt)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_HANDLE_UNORDERED_RECV_PKT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_HANDLE_UNORDERED_RECV_PKT);
    MPIDI_DBG_PRINTF((10, FCNAME, "entering"));
    
    switch(pkt->type)
    {
	case MPIDI_CH3_PKT_EAGER_SEND:
	case MPIDI_CH3_PKT_EAGER_SYNC_SEND:
	case MPIDI_CH3_PKT_READY_SEND:
	case MPIDI_CH3_PKT_RNDV_REQ_TO_SEND:
	{
	    MPIDI_CH3_Pkt_send_t * send_pkt = (MPIDI_CH3_Pkt_send_t *) pkt;
	    MPIDI_CH3_Pkt_send_container_t * pc_cur;
	    MPIDI_CH3_Pkt_send_container_t * pc_last;
	    
	    MPIDI_DBG_PRINTF((30, FCNAME, "received (potentially) out-of-order send pkt"));
	    MPIDI_DBG_PRINTF((30, FCNAME, "rank=%d, tag=%d, context=%d seqnum=%d",
			      send_pkt->match.rank, send_pkt->match.tag, send_pkt->match.context_id, send_pkt->seqnum));
	    
	    /* MT - Need to lock VC !!! */
	    
	    MPIDI_DBG_PRINTF((30, FCNAME, "vc - seqnum_send=%d seqnum_recv=%d reorder_msg_queue=0x%08lx",
			      vc->seqnum_send, vc->seqnum_recv, (unsigned long) vc->msg_reorder_queue));
	    
	    if (send_pkt->seqnum == vc->seqnum_recv)
	    {
		MPIDI_CH3U_Handle_ordered_recv_pkt(vc, pkt);
		vc->seqnum_recv++;
		pc_cur = vc->msg_reorder_queue;
		while(pc_cur != NULL && vc->seqnum_recv == pc_cur->pkt.seqnum)
		{
		    pkt = (MPIDI_CH3_Pkt_t *) &pc_cur->pkt;
		    MPIDI_CH3U_Handle_ordered_recv_pkt(vc, pkt);
		    vc->seqnum_recv++;
		    pc_last = pc_cur;
		    pc_cur = pc_cur->next;
		    MPIDI_CH3U_Pkt_send_container_free(pc_last);
		}
		vc->msg_reorder_queue = pc_cur;
	    }
	    else
	    {
		MPIDI_CH3_Pkt_send_container_t * pc_new;
	
		/* allocate container and copy packet */
		pc_new = MPIDI_CH3U_Pkt_send_container_alloc();
		pc_new->pkt = *send_pkt;

		/* insert packet into reorder queue */
		pc_last = NULL;
		pc_cur = vc->msg_reorder_queue;
		while (pc_cur != NULL)
		{
		    /* the current recv seqnum is subtracted from both the seqnums prior to comparision so as to remove any wrap
		       around effects. */
		    if (pc_new->pkt.seqnum - vc->seqnum_recv < pc_cur->pkt.seqnum - vc->seqnum_recv)
		    {
			break;
		    }

		    pc_last = pc_cur;
		    pc_cur = pc_cur->next;
		}

		if (pc_last == NULL)
		{
		    pc_new->next = pc_cur;
		    vc->msg_reorder_queue = pc_new;
		}
		else
		{
		    pc_new->next = pc_cur;
		    pc_last->next = pc_new;
		}
	    }

	    break;
	}

	case MPIDI_CH3_PKT_CANCEL_SEND_REQ:
	{
	    /* FIXME: processing send cancel requests requires that we be aware
               of pkts in the reorder queue */
	    abort();
	    break;
	}
	
	default:
	{
	    MPIDI_CH3U_Handle_ordered_recv_pkt(vc, pkt);
	    break;
	}
    }
    
    MPIDI_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_HANDLE_UNORDERED_RECV_PKT);
}
#endif

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Handle_ordered_recv_pkt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3U_Handle_ordered_recv_pkt(MPIDI_VC * vc, MPIDI_CH3_Pkt_t * pkt)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_HANDLE_ORDERED_RECV_PKT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_HANDLE_ORDERED_RECV_PKT);
    MPIDI_DBG_PRINTF((10, FCNAME, "entering"));

    assert(pkt->type < MPIDI_CH3_PKT_END_CH3);
    
    switch(pkt->type)
    {
	case MPIDI_CH3_PKT_EAGER_SEND:
	{
	    MPIDI_CH3_Pkt_eager_send_t * eager_pkt = &pkt->eager_send;
	    MPID_Request * rreq;
	    int found;

	    MPIDI_DBG_PRINTF((30, FCNAME, "received eager send pkt, sreq=0x%08x, rank=%d, tag=%d, context=%d",
			      eager_pkt->sender_req_id, eager_pkt->match.rank, eager_pkt->match.tag, eager_pkt->match.context_id));
	    
	    rreq = MPIDI_CH3U_Request_FDP_or_AEU(&eager_pkt->match, &found);
	    if (rreq == NULL)
	    {
		/* FIXME - need to handle memory allocation problems */
		assert(rreq != NULL);
		abort();
	    }
	    
	    set_request_info(rreq, eager_pkt, MPIDI_REQUEST_EAGER_MSG);
	    post_data_receive(vc, rreq, found);

	    break;
	}
	
	case MPIDI_CH3_PKT_READY_SEND:
	{
	    MPIDI_CH3_Pkt_ready_send_t * ready_pkt = &pkt->ready_send;
	    MPID_Request * rreq;
	    int found;
	    
	    MPIDI_DBG_PRINTF((30, FCNAME, "received ready send pkt, sreq=0x%08x, rank=%d, tag=%d, context=%d",
			      ready_pkt->sender_req_id, ready_pkt->match.rank, ready_pkt->match.tag, ready_pkt->match.context_id));
	    
	    rreq = MPIDI_CH3U_Request_FDP_or_AEU(&ready_pkt->match, &found);
	    if (rreq == NULL)
	    {
		/* FIXME - need to handle memory allocation problems */
		assert(rreq != NULL);
		abort();
	    }
	    
	    set_request_info(rreq, ready_pkt, MPIDI_REQUEST_EAGER_MSG);
	    if (found)
	    {
		post_data_receive(vc, rreq, TRUE);
	    }
	    else
	    {
		/* FIXME: an error packet should be sent back to the sender indicating that the ready-send failed.  On the send
                   side, the error handler for the communicator can be invoked even if the ready-send request has already
                   completed. */

		/* We need to consume any outstanding associated data and mark the request with an error. */
		int rc;

		rreq->status.MPI_ERROR = MPI_ERR_UNKNOWN;
		rreq->status.count = 0;
		if (rreq->ch3.recv_data_sz > 0)
		{
		     /* force read of extra data */
		    rreq->ch3.segment_first = 0;
		    rreq->ch3.segment_size = 0;
		    rc = MPIDI_CH3U_Request_load_recv_iov(rreq);
		    if (rc != MPI_SUCCESS)
		    {
			/* FIXME - need to handle buffer allocation problems */
			assert(rc == MPI_SUCCESS);
			abort();
		    }
		    MPIDI_CH3_iRead(vc, rreq);
		}
		else
		{
		    /* mark data transfer as complete adn decrment CC */
		    rreq->ch3.iov_count = 0;
		    MPIDI_CH3U_Request_complete(rreq);
		}
	    }

	    break;
	}
	
	case MPIDI_CH3_PKT_EAGER_SYNC_SEND:
	{
	    MPIDI_CH3_Pkt_eager_send_t * es_pkt = &pkt->eager_send;
	    MPID_Request * rreq;
	    int found;

	    MPIDI_DBG_PRINTF((30, FCNAME, "received eager sync send pkt, sreq=0x%08x, rank=%d, tag=%d, context=%d",
			      es_pkt->sender_req_id, es_pkt->match.rank, es_pkt->match.tag, es_pkt->match.context_id));
	    
	    rreq = MPIDI_CH3U_Request_FDP_or_AEU(&es_pkt->match, &found);
	    if (rreq == NULL)
	    {
		/* FIXME - need to handle memory allocation problems */
		assert(rreq != NULL);
		abort();
	    }
	    
	    set_request_info(rreq, es_pkt, MPIDI_REQUEST_EAGER_MSG);
	    post_data_receive(vc, rreq, found);
	    
	    if (found)
	    {
		MPIDI_CH3_Pkt_t upkt;
		MPIDI_CH3_Pkt_eager_sync_ack_t * const esa_pkt = &upkt.eager_sync_ack;
		MPID_Request * esa_req;
		    
		MPIDI_DBG_PRINTF((30, FCNAME, "sending eager sync ack"));
			
		esa_pkt->type = MPIDI_CH3_PKT_EAGER_SYNC_ACK;
		esa_pkt->sender_req_id = es_pkt->sender_req_id;
		esa_req = MPIDI_CH3_iStartMsg(vc, esa_pkt, sizeof(*esa_pkt));
		if (esa_req != NULL)
		{
		    MPID_Request_release(esa_req);
		}
	    }
	    else
	    {
		MPIDI_Request_set_sync_send_flag(rreq, TRUE);
	    }
	    
	    break;
	}

	case MPIDI_CH3_PKT_EAGER_SYNC_ACK:
	{
	    MPIDI_CH3_Pkt_eager_sync_ack_t * esa_pkt = &pkt->eager_sync_ack;
	    MPID_Request * sreq;
	    
	    MPIDI_DBG_PRINTF((30, FCNAME, "received eager sync ack pkt, sreq=0x%08x", esa_pkt->sender_req_id));
	    
	    MPID_Request_get_ptr(esa_pkt->sender_req_id, sreq);
	    /* decrement CC (but don't mark data transfer as complete since the transfer could still be in progress) */
	    MPIDI_CH3U_Request_complete(sreq);
	    break;
	}
	
	case MPIDI_CH3_PKT_RNDV_REQ_TO_SEND:
	{
	    MPIDI_CH3_Pkt_rndv_req_to_send_t * rts_pkt = &pkt->rndv_req_to_send;
	    MPID_Request * rreq;
	    int found;

	    MPIDI_DBG_PRINTF((30, FCNAME, "received rndv RTS pkt, sreq=0x%08x, rank=%d, tag=%d, context=%d",
			      rts_pkt->sender_req_id, rts_pkt->match.rank, rts_pkt->match.tag, rts_pkt->match.context_id));
	    
	    rreq = MPIDI_CH3U_Request_FDP_or_AEU(&rts_pkt->match, &found);
	    assert(rreq != NULL);

	    set_request_info(rreq, rts_pkt, MPIDI_REQUEST_RNDV_MSG);
	    
	    if (found)
	    {
		MPID_Request * cts_req;
		MPIDI_CH3_Pkt_t upkt;
		MPIDI_CH3_Pkt_rndv_clr_to_send_t * cts_pkt =
		    &upkt.rndv_clr_to_send;
		
		MPIDI_DBG_PRINTF((30, FCNAME, "posted request found"));

		/* FIXME: What if the receive user buffer is not big enough to hold the data about to be cleared for sending? */
		
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
	    MPIDI_CH3_Pkt_rndv_clr_to_send_t * cts_pkt = &pkt->rndv_clr_to_send;
	    MPID_Request * sreq;
	    MPIDI_CH3_Pkt_t upkt;
	    MPIDI_CH3_Pkt_rndv_send_t * rs_pkt = &upkt.rndv_send;
	    int dt_contig;
	    MPIDI_msg_sz_t data_sz;
	    MPID_Datatype * dt_ptr;
	    MPID_IOV iov[MPID_IOV_LIMIT];
	    int iov_n;
	    int mpi_errno = MPI_SUCCESS;
	    
	    MPIDI_DBG_PRINTF((30, FCNAME, "received rndv CTS pkt"));

	    MPID_Request_get_ptr(cts_pkt->sender_req_id, sreq);

	    /* release the RTS request if one exists */
	    /* FIXME - MT: this needs to be atomic to prevent cancel send from cancelling the wrong request */
	    if (sreq->partner_request)
	    {
		MPID_Request_release(sreq->partner_request);
		sreq->partner_request = NULL;
	    }
	    
	    rs_pkt->type = MPIDI_CH3_PKT_RNDV_SEND;
	    rs_pkt->receiver_req_id = cts_pkt->receiver_req_id;
	    iov[0].MPID_IOV_BUF = (void*)rs_pkt;
	    iov[0].MPID_IOV_LEN = sizeof(*rs_pkt);

	    MPIDI_CH3U_Datatype_get_info(sreq->ch3.user_count, sreq->ch3.datatype, dt_contig, data_sz, dt_ptr);
	
	    if (dt_contig) 
	    {
		MPIDI_DBG_PRINTF((30, FCNAME, "sending contiguous rndv data, data_sz=" MPIDI_MSG_SZ_FMT, data_sz));
		
		sreq->ch3.ca = MPIDI_CH3_CA_COMPLETE;
		
		iov[1].MPID_IOV_BUF = sreq->ch3.user_buf;
		iov[1].MPID_IOV_LEN = data_sz;
		iov_n = 2;
	    }
	    else
	    {
		MPID_Segment_init(sreq->ch3.user_buf, sreq->ch3.user_count, sreq->ch3.datatype, &sreq->ch3.segment);
		iov_n = MPID_IOV_LIMIT - 1;
		sreq->ch3.segment_first = 0;
		sreq->ch3.segment_size = data_sz;
		mpi_errno = MPIDI_CH3U_Request_load_send_iov(sreq, &iov[1], &iov_n);
		if (mpi_errno != MPI_SUCCESS)
		{
		    MPIDI_ERR_PRINTF((FCNAME, "MPIDI_CH3_PKT_RNDV_CLR_TO_SEND failed to load IOV"));
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
		    
	    MPIDI_DBG_PRINTF((30, FCNAME, "received rndv send (data) pkt"));
	    MPID_Request_get_ptr(rs_pkt->receiver_req_id, rreq);
	    post_data_receive(vc, rreq, TRUE);
		
	    break;
	}
	
	case MPIDI_CH3_PKT_CANCEL_SEND_REQ:
	{
	    MPIDI_CH3_Pkt_cancel_send_req_t * req_pkt = &pkt->cancel_send_req;
	    MPID_Request * rreq;
	    MPIDI_CH3_Pkt_t upkt;
	    MPIDI_CH3_Pkt_cancel_send_resp_t * resp_pkt = &upkt.cancel_send_resp;
	    MPID_Request * resp_sreq;

	    MPIDI_DBG_PRINTF((30, FCNAME, "received cancel send req pkt, sreq=0x%08x, rank=%d, tag=%d, context=%d",
			      req_pkt->sender_req_id, req_pkt->match.rank, req_pkt->match.tag, req_pkt->match.context_id));
	    
	    rreq = MPIDI_CH3U_Request_FDU(req_pkt->sender_req_id, &req_pkt->match);
	    if (rreq != NULL)
	    {
		MPIDI_DBG_PRINTF((35, FCNAME, "message cancelled"));
		if (MPIDI_Request_get_msg_type(rreq) == MPIDI_REQUEST_EAGER_MSG && rreq->ch3.recv_data_sz > 0)
		{
		    MPIU_Free(rreq->ch3.tmpbuf);
		}
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
	    resp_sreq = MPIDI_CH3_iStartMsg(vc, resp_pkt, sizeof(*resp_pkt));
	    if (resp_sreq != NULL)
	    {
		MPID_Request_release(resp_sreq);
	    }
	    
	    break;
	}
	
	case MPIDI_CH3_PKT_CANCEL_SEND_RESP:
	{
	    MPIDI_CH3_Pkt_cancel_send_resp_t * resp_pkt = &pkt->cancel_send_resp;
	    MPID_Request * sreq;

	    MPIDI_DBG_PRINTF((30, FCNAME, "received cancel send resp pkt, sreq=0x%08x, ack=%d",
			      resp_pkt->sender_req_id, resp_pkt->ack));
	    
	    MPID_Request_get_ptr(resp_pkt->sender_req_id, sreq);
	    
	    if (resp_pkt->ack)
	    {
		sreq->status.cancelled = TRUE;
		
		if (MPIDI_Request_get_msg_type(sreq) == MPIDI_REQUEST_RNDV_MSG ||
		    MPIDI_Request_get_type(sreq) == MPIDI_REQUEST_TYPE_SSEND)
		{
		    int cc;
		    
		    /* decrement the CC one additional time for the CTS/sync ack that is never going to arrive */
		    MPIDI_CH3U_Request_decrement_cc(sreq, &cc);
		}
		
		MPIDI_DBG_PRINTF((35, FCNAME, "message cancelled"));
	    }
	    else
	    {
		MPIDI_DBG_PRINTF((35, FCNAME, "unable to cancel message"));
	    }

	    MPIDI_CH3U_Request_complete(sreq);
	    
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

    MPIDI_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_HANDLE_ORDERED_RECV_PKT);
}



#undef FUNCNAME
#define FUNCNAME post_data_receive
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static void post_data_receive(MPIDI_VC * vc, MPID_Request * rreq, int found)
{
    int dt_contig;
    MPIDI_msg_sz_t userbuf_sz;
    MPID_Datatype * dt_ptr;
    MPIDI_msg_sz_t data_sz;

    MPIDI_DBG_PRINTF((30, FCNAME, "entering"));

    if (rreq->ch3.recv_data_sz == 0)
    {
	MPIDI_DBG_PRINTF((30, FCNAME, "null message, %s, decrementing completion counter",
			  (found ? "posted request found" : "unexpected request allocated")));
	/* mark data transfer as complete adn decrment CC */
	rreq->ch3.iov_count = 0;
	MPIDI_CH3U_Request_complete(rreq);
	goto fn_exit;
    }
	
    if (found)
    {
	MPIDI_DBG_PRINTF((30, FCNAME, "posted request found"));
	
	MPIDI_CH3U_Datatype_get_info(rreq->ch3.user_count, rreq->ch3.datatype, dt_contig, userbuf_sz, dt_ptr);
		
	if (rreq->ch3.recv_data_sz <= userbuf_sz)
	{
	    data_sz = rreq->ch3.recv_data_sz;
	}
	else
	{
	    MPIDI_DBG_PRINTF((35, FCNAME, "receive buffer too small; message truncated, msg_sz=" MPIDI_MSG_SZ_FMT ", userbuf_sz="
			      MPIDI_MSG_SZ_FMT, rreq->ch3.recv_data_sz, userbuf_sz));
	    rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;
	    rreq->status.count = userbuf_sz;
	    data_sz = userbuf_sz;
	}

	if (dt_contig && data_sz == rreq->ch3.recv_data_sz)
	{
	    /* user buffer is contiguous and large enough to store the
	       entire message */
	    MPIDI_DBG_PRINTF((35, FCNAME, "IOV loaded for contiguous read"));
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
		    
	    MPIDI_DBG_PRINTF((35, FCNAME, "IOV loaded for non-contiguous read"));
	    MPID_Segment_init(rreq->ch3.user_buf, rreq->ch3.user_count, rreq->ch3.datatype, &rreq->ch3.segment);
	    rreq->ch3.segment_first = 0;
	    rreq->ch3.segment_size = data_sz;
	    mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		MPIDI_ERR_PRINTF((FCNAME, "failed to load IOV"));
		abort();
	    }
	}
    }
    else /* if (!found) */
    {
	/* TODO: to improve performance, allocate temporary buffer from a specialized buffer pool. */
	MPIDI_DBG_PRINTF((30, FCNAME, "unexpected request allocated"));
		
	rreq->ch3.tmpbuf = MPIU_Malloc(rreq->ch3.recv_data_sz);
	rreq->ch3.tmpbuf_sz = rreq->ch3.recv_data_sz;
		
	rreq->ch3.iov[0].MPID_IOV_BUF = rreq->ch3.tmpbuf;
	rreq->ch3.iov[0].MPID_IOV_LEN = rreq->ch3.recv_data_sz;
	rreq->ch3.iov_count = 1;
	rreq->ch3.ca = MPIDI_CH3_CA_COMPLETE;
    }

    MPIDI_DBG_PRINTF((35, FCNAME, "posting iRead"));
    MPIDI_CH3_iRead(vc, rreq);
    
fn_exit:
    MPIDI_DBG_PRINTF((30, FCNAME, "exiting"));
}
