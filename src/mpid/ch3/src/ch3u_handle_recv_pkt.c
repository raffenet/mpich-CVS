/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#define set_request_info(_rreq, _pkt, _msg_type)		\
{								\
    (_rreq)->status.MPI_SOURCE = (_pkt)->match.rank;		\
    (_rreq)->status.MPI_TAG = (_pkt)->match.tag;		\
    (_rreq)->status.count = (_pkt)->data_sz;			\
    (_rreq)->dev.sender_req_id = (_pkt)->sender_req_id;		\
    (_rreq)->dev.recv_data_sz = (_pkt)->data_sz;		\
    MPIDI_CH3U_Request_set_seqnum((_rreq), (_pkt)->seqnum);	\
    MPIDI_Request_set_msg_type((_rreq), (_msg_type));		\
}

/*
 * MPIDI_CH3U_Handle_recv_pkt()
 *
 * NOTE: Multiple threads may NOT simultaneously call this routine with the same VC.  This constraint eliminates the need to
 * lock the VC.  If simultaneous upcalls are a possible, the calling routine for serializing the calls.
 */
int MPIDI_CH3U_Handle_unordered_recv_pkt(MPIDI_VC * vc, MPIDI_CH3_Pkt_t * pkt);
int MPIDI_CH3U_Handle_ordered_recv_pkt(MPIDI_VC * vc, MPIDI_CH3_Pkt_t * pkt);
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
int MPIDI_CH3U_Handle_unordered_recv_pkt(MPIDI_VC * vc, MPIDI_CH3_Pkt_t * pkt, MPID_Request ** rreqp)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_HANDLE_UNORDERED_RECV_PKT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_HANDLE_UNORDERED_RECV_PKT);
    MPIDI_DBG_PRINTF((10, FCNAME, "entering"));

    rreqp = NULL;
    
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
	    MPIDI_DBG_PRINTF((30, FCNAME, "vc - seqnum_send=%d seqnum_recv=%d reorder_msg_queue=0x%08lx",
			      vc->seqnum_send, vc->seqnum_recv, (unsigned long) vc->msg_reorder_queue));
	    
	    if (send_pkt->seqnum == vc->seqnum_recv)
	    {
		mpi_errno = MPIDI_CH3U_Handle_ordered_recv_pkt(vc, pkt, rreqp);
		/* --BEGIN ERROR HANDLING-- */
		if (mpi_errno != MPI_SUCCESS)
		{
		    goto fn_exit;
		}
		/* --END ERROR HANDLING-- */
		vc->seqnum_recv++;
		pc_cur = vc->msg_reorder_queue;
		while(pc_cur != NULL && vc->seqnum_recv == pc_cur->pkt.seqnum)
		{
		    pkt = (MPIDI_CH3_Pkt_t *) &pc_cur->pkt;
		    mpi_errno = MPIDI_CH3U_Handle_ordered_recv_pkt(vc, pkt, rreqp);
		    /* --BEGIN ERROR HANDLING-- */
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
							 "**ch3|pktordered", 0);
			goto fn_exit;
		    }
		    /* --END ERROR HANDLING-- */
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
		/* --BEGIN ERROR HANDLING-- */
		if (pc_new == NULL)
		{
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
						     "**ch3|nopktcontainermem", 0);
		    goto fn_exit;
		}
		/* --END ERROR HANDLING-- */
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
	    /* --BEGIN ERROR HANDLING-- */
	    /* FIXME: processing send cancel requests requires that we be aware of pkts in the reorder queue */
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					     "**ch3|ooocancelreq", 0);
	    goto fn_exit;
	    break;
	    /* --END ERROR HANDLING-- */
	}
	
	default:
	{
	    mpi_errno = MPIDI_CH3U_Handle_ordered_recv_pkt(vc, pkt, rreqp);
	    break;
	}
    }

  fn_exit:
    MPIDI_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_HANDLE_UNORDERED_RECV_PKT);
    return mpi_errno;
}
#endif

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Handle_ordered_recv_pkt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Handle_ordered_recv_pkt(MPIDI_VC * vc, MPIDI_CH3_Pkt_t * pkt, MPID_Request ** rreqp)
{
    int type_size;
    static int in_routine = FALSE;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_HANDLE_ORDERED_RECV_PKT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_HANDLE_ORDERED_RECV_PKT);
    MPIDI_DBG_PRINTF((10, FCNAME, "entering"));
    MPIDI_DBG_Print_packet(pkt);

    MPIU_Assert(in_routine == FALSE);
    in_routine = TRUE;
    
    switch(pkt->type)
    {
	case MPIDI_CH3_PKT_EAGER_SEND:
	{
	    MPIDI_CH3_Pkt_eager_send_t * eager_pkt = &pkt->eager_send;
	    MPID_Request * rreq;
	    int found;

	    MPIDI_DBG_PRINTF((30, FCNAME, "received eager send pkt, sreq=0x%08x, rank=%d, tag=%d, context=%d",
			      eager_pkt->sender_req_id, eager_pkt->match.rank, eager_pkt->match.tag, eager_pkt->match.context_id));
	    
	    rreq = MPIDI_CH3U_Recvq_FDP_or_AEU(&eager_pkt->match, &found);
	    /* --BEGIN ERROR HANDLING-- */
	    if (rreq == NULL)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomemreq", 0);
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */
	    
	    set_request_info(rreq, eager_pkt, MPIDI_REQUEST_EAGER_MSG);
	    *rreqp = rreq;
	    mpi_errno = MPIDI_CH3U_Post_data_receive(vc, found, rreqp);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|postrecv",
						 "**ch3|postrecv %s", "MPIDI_CH3_PKT_EAGER_SEND");
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */
	    
	    break;
	}
	
	case MPIDI_CH3_PKT_READY_SEND:
	{
	    MPIDI_CH3_Pkt_ready_send_t * ready_pkt = &pkt->ready_send;
	    MPID_Request * rreq;
	    int found;
	    
	    MPIDI_DBG_PRINTF((30, FCNAME, "received ready send pkt, sreq=0x%08x, rank=%d, tag=%d, context=%d",
			      ready_pkt->sender_req_id, ready_pkt->match.rank, ready_pkt->match.tag, ready_pkt->match.context_id));
	    
	    rreq = MPIDI_CH3U_Recvq_FDP_or_AEU(&ready_pkt->match, &found);
	    /* --BEGIN ERROR HANDLING-- */
	    if (rreq == NULL)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomemreq", 0);
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */
	    
	    set_request_info(rreq, ready_pkt, MPIDI_REQUEST_EAGER_MSG);
	    *rreqp = rreq;
	    if (found)
	    {
		mpi_errno = MPIDI_CH3U_Post_data_receive(vc, TRUE, rreqp);
		/* --BEGIN ERROR HANDLING-- */
		if (mpi_errno != MPI_SUCCESS)
		{
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|postrecv",
						     "**ch3|postrecv %s", "MPIDI_CH3_PKT_READY_SEND");
		    goto fn_exit;
		}
		/* --END ERROR HANDLING-- */
	    }
	    else
	    {
		/* FIXME: an error packet should be sent back to the sender indicating that the ready-send failed.  On the send
                   side, the error handler for the communicator can be invoked even if the ready-send request has already
                   completed. */

		/* We need to consume any outstanding associated data and mark the request with an error. */

		MPID_Request_initialized_set(rreq);
		rreq->status.MPI_ERROR = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
							      "**rsendnomatch", "**rsendnomatch %d %d", ready_pkt->match.rank,
							      ready_pkt->match.tag);
		rreq->status.count = 0;
		if (rreq->dev.recv_data_sz > 0)
		{
		     /* force read of extra data */
		    rreq->dev.segment_first = 0;
		    rreq->dev.segment_size = 0;
		    mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
							 "**ch3|loadrecviov", 0);
			goto fn_exit;
		    }
		}
		else
		{
		    /* mark data transfer as complete and decrement CC */
		    MPIDI_CH3U_Request_complete(rreq);
		    *rreqp = NULL;
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
	    
	    rreq = MPIDI_CH3U_Recvq_FDP_or_AEU(&es_pkt->match, &found);
	    /* --BEGIN ERROR HANDLING-- */
	    if (rreq == NULL)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomemreq", 0);
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */
	    
	    set_request_info(rreq, es_pkt, MPIDI_REQUEST_EAGER_MSG);
	    *rreqp = rreq;
	    mpi_errno = MPIDI_CH3U_Post_data_receive(vc, found, rreqp);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|postrecv",
						 "**ch3|postrecv %s", "MPIDI_CH3_PKT_EAGER_SYNC_SEND");
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */
	    
	    if (found)
	    {
		MPIDI_CH3_Pkt_t upkt;
		MPIDI_CH3_Pkt_eager_sync_ack_t * const esa_pkt = &upkt.eager_sync_ack;
		MPID_Request * esa_req;
		    
		MPIDI_DBG_PRINTF((30, FCNAME, "sending eager sync ack"));
			
		esa_pkt->type = MPIDI_CH3_PKT_EAGER_SYNC_ACK;
		esa_pkt->sender_req_id = rreq->dev.sender_req_id;
		mpi_errno = MPIDI_CH3_iStartMsg(vc, esa_pkt, sizeof(*esa_pkt), &esa_req);
		/* --BEGIN ERROR HANDLING-- */
		if (mpi_errno != MPI_SUCCESS)
		{
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
						     "**ch3|syncack", 0);
		    goto fn_exit;
		}
		/* --END ERROR HANDLING-- */
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

	    *rreqp = NULL;
	    break;
	}
	
	case MPIDI_CH3_PKT_RNDV_REQ_TO_SEND:
	{
	    MPIDI_CH3_Pkt_rndv_req_to_send_t * rts_pkt = &pkt->rndv_req_to_send;
	    MPID_Request * rreq;
	    int found;

	    MPIDI_DBG_PRINTF((30, FCNAME, "received rndv RTS pkt, sreq=0x%08x, rank=%d, tag=%d, context=%d, data_sz=%d",
			      rts_pkt->sender_req_id, rts_pkt->match.rank, rts_pkt->match.tag, rts_pkt->match.context_id,
			      rts_pkt->data_sz));
	    
	    rreq = MPIDI_CH3U_Recvq_FDP_or_AEU(&rts_pkt->match, &found);
	    /* --BEGIN ERROR HANDLING-- */
	    if (rreq == NULL)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomemreq", 0);
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */

	    set_request_info(rreq, rts_pkt, MPIDI_REQUEST_RNDV_MSG);
	    
	    if (found)
	    {
#ifdef MPIDI_CH3_CHANNEL_RNDV
		/* The channel will be performing the rendezvous */

		mpi_errno = MPIDI_CH3U_Post_data_receive(vc, found, &rreq);
		/* --BEGIN ERROR HANDLING-- */
		if (mpi_errno != MPI_SUCCESS)
		{
		    mpi_errno = MPIR_Err_create_code (mpi_errno, MPIR_ERR_FATAL,
						      FCNAME, __LINE__,
						      MPI_ERR_OTHER,
						      "**ch3|postrecv",
						      "**ch3|postrecv %s",
						      "MPIDI_CH3_PKT_RNDV_REQ_TO_SEND");
		    goto fn_exit;
		}
		/* --END ERROR HANDLING-- */
		mpi_errno = MPIDI_CH3_do_cts (vc, rreq, rreq->dev.sender_req_id,
					      rreq->dev.iov, rreq->dev.iov_count);
		/* --BEGIN ERROR HANDLING-- */
		if (mpi_errno != MPI_SUCCESS)
		{
		    mpi_errno = MPIR_Err_create_code (mpi_errno, MPIR_ERR_FATAL,
						      FCNAME, __LINE__,
						      MPI_ERR_OTHER,
						      "**ch3|ctspkt", 0);
		    goto fn_exit;
		}
		/* --END ERROR HANDLING-- */

#else
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
		mpi_errno = MPIDI_CH3_iStartMsg(vc, cts_pkt, sizeof(*cts_pkt), &cts_req);
		/* --BEGIN ERROR HANDLING-- */
		if (mpi_errno != MPI_SUCCESS)
		{
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
						     "**ch3|ctspkt", 0);
		    goto fn_exit;
		}
		/* --END ERROR HANDLING-- */
		if (cts_req != NULL)
		{
		    MPID_Request_release(cts_req);
		}
#endif
	    }
	    else
	    {
		MPIDI_DBG_PRINTF((30, FCNAME, "unexpected request allocated"));
		MPID_Request_initialized_set(rreq);
	    }

	    *rreqp = NULL;
	    break;
	}
	
	case MPIDI_CH3_PKT_RNDV_CLR_TO_SEND:
	{
	    MPIDI_CH3_Pkt_rndv_clr_to_send_t * cts_pkt = &pkt->rndv_clr_to_send;
	    MPID_Request * sreq;
	    MPID_Request * rts_sreq;
	    MPIDI_CH3_Pkt_t upkt;
	    MPIDI_CH3_Pkt_rndv_send_t * rs_pkt = &upkt.rndv_send;
	    int dt_contig;
	    MPI_Aint dt_true_lb;
	    MPIDI_msg_sz_t data_sz;
	    MPID_Datatype * dt_ptr;
	    MPID_IOV iov[MPID_IOV_LIMIT];
	    int iov_n;
	    int mpi_errno = MPI_SUCCESS;

	    MPIDI_DBG_PRINTF((30, FCNAME, "received rndv CTS pkt"));

	    MPID_Request_get_ptr(cts_pkt->sender_req_id, sreq);
	    MPIU_DBG_PRINTF(("received cts, count=%d\n", sreq->dev.user_count));

	    /* Release the RTS request if one exists.  MPID_Request_fetch_and_clear_rts_sreq() needs to be atomic to prevent
               cancel send from cancelling the wrong (future) request.  If MPID_Request_fetch_and_clear_rts_sreq() returns a NULL
               rts_sreq, then MPID_Cancel_send() is responsible for releasing the RTS request object. */
	    MPIDI_Request_fetch_and_clear_rts_sreq(sreq, &rts_sreq);
	    if (rts_sreq != NULL)
	    {
		MPID_Request_release(rts_sreq);
	    }
	    
	    rs_pkt->type = MPIDI_CH3_PKT_RNDV_SEND;
	    rs_pkt->receiver_req_id = cts_pkt->receiver_req_id;
	    iov[0].MPID_IOV_BUF = (void*)rs_pkt;
	    iov[0].MPID_IOV_LEN = sizeof(*rs_pkt);

	    MPIDI_CH3U_Datatype_get_info(sreq->dev.user_count, sreq->dev.datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
	
	    if (dt_contig) 
	    {
		MPIDI_DBG_PRINTF((30, FCNAME, "sending contiguous rndv data, data_sz=" MPIDI_MSG_SZ_FMT, data_sz));
		
		sreq->dev.ca = MPIDI_CH3_CA_COMPLETE;
		
		iov[1].MPID_IOV_BUF = (char *)sreq->dev.user_buf + dt_true_lb;
		iov[1].MPID_IOV_LEN = data_sz;
		iov_n = 2;
	    }
	    else
	    {
		MPID_Segment_init(sreq->dev.user_buf, sreq->dev.user_count, sreq->dev.datatype, &sreq->dev.segment, 0);
		iov_n = MPID_IOV_LIMIT - 1;
		sreq->dev.segment_first = 0;
		sreq->dev.segment_size = data_sz;
		mpi_errno = MPIDI_CH3U_Request_load_send_iov(sreq, &iov[1], &iov_n);
		/* --BEGIN ERROR HANDLING-- */
		if (mpi_errno != MPI_SUCCESS)
		{
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
						     "**ch3|loadsendiov", 0);
		    goto fn_exit;
		}
		/* --END ERROR HANDLING-- */
		iov_n += 1;
	    }
	    
	    mpi_errno = MPIDI_CH3_iSendv(vc, sreq, iov, iov_n);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|senddata", 0);
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */
	    
	    *rreqp = NULL;
	    break;
	}
	
	case MPIDI_CH3_PKT_RNDV_SEND:
	{
	    MPIDI_CH3_Pkt_rndv_send_t * rs_pkt = &pkt->rndv_send;
		    
	    MPIDI_DBG_PRINTF((30, FCNAME, "received rndv send (data) pkt"));
	    MPID_Request_get_ptr(rs_pkt->receiver_req_id, *rreqp);
	    mpi_errno = MPIDI_CH3U_Post_data_receive(vc, TRUE, rreqp);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|postrecv",
						 "**ch3|postrecv %s", "MPIDI_CH3_PKT_RNDV_SEND");
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */
		
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
	    
	    rreq = MPIDI_CH3U_Recvq_FDU(req_pkt->sender_req_id, &req_pkt->match);
	    if (rreq != NULL)
	    {
		MPIDI_DBG_PRINTF((35, FCNAME, "message cancelled"));
		if (MPIDI_Request_get_msg_type(rreq) == MPIDI_REQUEST_EAGER_MSG && rreq->dev.recv_data_sz > 0)
		{
		    MPIU_Free(rreq->dev.tmpbuf);
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
	    mpi_errno = MPIDI_CH3_iStartMsg(vc, resp_pkt, sizeof(*resp_pkt), &resp_sreq);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|cancelresp", 0);
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */
	    if (resp_sreq != NULL)
	    {
		MPID_Request_release(resp_sreq);
	    }

	    *rreqp = NULL;
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

	    *rreqp = NULL;
	    break;
	}
	
	case MPIDI_CH3_PKT_PUT:
	{
	    MPIDI_CH3_Pkt_put_t * put_pkt = &pkt->put;
            MPID_Request *req;

	    MPIDI_DBG_PRINTF((30, FCNAME, "received put pkt"));

            if (put_pkt->count == 0)
	    {
                /* it's a 0-byte message sent just to decrement the
                   completion counter. Happens only in
                   post/start/complete/wait sync model; therefore, no need
                   to check lock queue. */
                /* FIXME: MT: this has to be done atomically */
                if (put_pkt->win_ptr != NULL)
                    put_pkt->win_ptr->my_counter -= 1;
                MPIDI_CH3_Progress_signal_completion();	
		*rreqp = NULL;
            }
            else
	    {
                req = MPID_Request_create();
                MPIU_Object_set_ref(req, 1);
                
                req->dev.user_buf = put_pkt->addr;
                req->dev.user_count = put_pkt->count;
                req->dev.win_ptr = put_pkt->win_ptr;

                if (HANDLE_GET_KIND(put_pkt->datatype) == HANDLE_KIND_BUILTIN)
		{
                    MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_PUT_RESP);
                    req->dev.datatype = put_pkt->datatype;
                
                    MPID_Datatype_get_size_macro(put_pkt->datatype,
                                                 type_size);
                    req->dev.recv_data_sz = type_size * put_pkt->count;

		    *rreqp = req;
                    mpi_errno = MPIDI_CH3U_Post_data_receive(vc, TRUE, rreqp);
                }
                else
		{
		    /* derived datatype */
                    MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_PUT_RESP_DERIVED_DT);
                    req->dev.datatype = MPI_DATATYPE_NULL;

                    req->dev.dtype_info = (MPIDI_RMA_dtype_info *) 
                        MPIU_Malloc(sizeof(MPIDI_RMA_dtype_info));
		    /* --BEGIN ERROR HANDLING-- */
                    if (! req->dev.dtype_info)
		    {
                        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
							  "**nomem", 0 );
                        goto fn_exit;
                    }
		    /* --END ERROR HANDLING-- */

                    req->dev.dataloop = MPIU_Malloc(put_pkt->dataloop_size);
		    /* --BEGIN ERROR HANDLING-- */
                    if (! req->dev.dataloop)
		    {
                        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
							  "**nomem", 0 );
                        goto fn_exit;
                    }
		    /* --END ERROR HANDLING-- */

                    req->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)req->dev.dtype_info;
                    req->dev.iov[0].MPID_IOV_LEN = sizeof(MPIDI_RMA_dtype_info);
                    req->dev.iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)req->dev.dataloop;
                    req->dev.iov[1].MPID_IOV_LEN = put_pkt->dataloop_size;
                    req->dev.iov_count = 2;
                    req->dev.ca = MPIDI_CH3_CA_COMPLETE;

		    *rreqp = req;
                }

		/* --BEGIN ERROR HANDLING-- */
                if (mpi_errno != MPI_SUCCESS)
		{
                    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
						     "**ch3|postrecv", "**ch3|postrecv %s", "MPIDI_CH3_PKT_PUT");
		}
		/* --END ERROR HANDLING-- */

            }
            break;
        }
	
	case MPIDI_CH3_PKT_ACCUMULATE:
	{
	    MPIDI_CH3_Pkt_accum_t * accum_pkt = &pkt->accum;
            MPID_Request *req;
            MPI_Aint true_lb, true_extent, extent;
            void *tmp_buf;

	    MPIDI_DBG_PRINTF((30, FCNAME, "received accumulate pkt"));

            req = MPID_Request_create();
            MPIU_Object_set_ref(req, 1);
	    *rreqp = req;

            req->dev.user_count = accum_pkt->count;
            req->dev.op = accum_pkt->op;
            req->dev.win_ptr = accum_pkt->win_ptr;
            req->dev.real_user_buf = accum_pkt->addr;

            if (HANDLE_GET_KIND(accum_pkt->datatype) == HANDLE_KIND_BUILTIN)
	    {
                MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_ACCUM_RESP);
                req->dev.datatype = accum_pkt->datatype;

                mpi_errno = NMPI_Type_get_true_extent(accum_pkt->datatype, 
                                                      &true_lb, &true_extent);
		/* --BEGIN ERROR HANDLING-- */
                if (mpi_errno)
		{
		    MPIR_Err_create_code(mpi_errno , MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
		    return mpi_errno;
		}
		/* --END ERROR HANDLING-- */

                MPID_Datatype_get_extent_macro(accum_pkt->datatype, extent); 
                tmp_buf = MPIU_Malloc(accum_pkt->count * 
                                      (MPIR_MAX(extent,true_extent)));
		/* --BEGIN ERROR HANDLING-- */
                if (!tmp_buf)
		{
                    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
						      "**nomem", 0 );
                    return mpi_errno;
                }
		/* --END ERROR HANDLING-- */

                /* adjust for potential negative lower bound in datatype */
                tmp_buf = (void *)((char*)tmp_buf - true_lb);

                req->dev.user_buf = tmp_buf;

                MPID_Datatype_get_size_macro(accum_pkt->datatype, type_size);
                req->dev.recv_data_sz = type_size * accum_pkt->count;
                
                mpi_errno = MPIDI_CH3U_Post_data_receive(vc, TRUE, rreqp);
            }
            else
	    {
                MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_ACCUM_RESP_DERIVED_DT);
                req->dev.datatype = MPI_DATATYPE_NULL;
                
                req->dev.dtype_info = (MPIDI_RMA_dtype_info *) 
                    MPIU_Malloc(sizeof(MPIDI_RMA_dtype_info));
		/* --BEGIN ERROR HANDLING-- */
                if (! req->dev.dtype_info)
		{
                    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
						      "**nomem", 0 );
                    goto fn_exit;
                }
		/* --END ERROR HANDLING-- */
                
                req->dev.dataloop = MPIU_Malloc(accum_pkt->dataloop_size);
		/* --BEGIN ERROR HANDLING-- */
                if (! req->dev.dataloop)
		{
                    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
						      "**nomem", 0 );
                    goto fn_exit;
                }
		/* --END ERROR HANDLING-- */
                
                req->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)req->dev.dtype_info;
                req->dev.iov[0].MPID_IOV_LEN = sizeof(MPIDI_RMA_dtype_info);
                req->dev.iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)req->dev.dataloop;
                req->dev.iov[1].MPID_IOV_LEN = accum_pkt->dataloop_size;
                req->dev.iov_count = 2;
                req->dev.ca = MPIDI_CH3_CA_COMPLETE;
            }

	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
						 "**ch3|postrecv", "**ch3|postrecv %s", "MPIDI_CH3_PKT_ACCUMULATE");
	    }
	    /* --END ERROR HANDLING-- */
            break;
        }

	case MPIDI_CH3_PKT_GET:
	{
	    MPIDI_CH3_Pkt_get_t * get_pkt = &pkt->get;
            MPID_Request *req;
            MPID_IOV iov[MPID_IOV_LIMIT];

	    MPIDI_DBG_PRINTF((30, FCNAME, "received get pkt"));

            req = MPID_Request_create();
            req->dev.win_ptr = get_pkt->win_ptr;
            req->dev.ca = MPIDI_CH3_CA_COMPLETE;

            if (HANDLE_GET_KIND(get_pkt->datatype) == HANDLE_KIND_BUILTIN)
	    {
                /* basic datatype. send the data. */
                MPIDI_CH3_Pkt_t upkt;
                MPIDI_CH3_Pkt_get_resp_t * get_resp_pkt = &upkt.get_resp;

                MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_GET_RESP); 
                req->kind = MPID_REQUEST_SEND;

                get_resp_pkt->type = MPIDI_CH3_PKT_GET_RESP;
                get_resp_pkt->request = get_pkt->request;
                
                iov[0].MPID_IOV_BUF = (void*) get_resp_pkt;
                iov[0].MPID_IOV_LEN = sizeof(*get_resp_pkt);

                iov[1].MPID_IOV_BUF = get_pkt->addr;
                MPID_Datatype_get_size_macro(get_pkt->datatype, type_size);
                iov[1].MPID_IOV_LEN = get_pkt->count * type_size;
	    
                mpi_errno = MPIDI_CH3_iSendv(vc, req, iov, 2);
		/* --BEGIN ERROR HANDLING-- */
                if (mpi_errno != MPI_SUCCESS)
                {
                    MPIU_Object_set_ref(req, 0);
                    MPIDI_CH3_Request_destroy(req);
                    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
						     "**ch3|rmamsg", 0);
                    return mpi_errno;
                }
		/* --END ERROR HANDLING-- */

		*rreqp = NULL;
            }
            else
	    {
                /* derived datatype. first get the dtype_info and dataloop. */

                MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_GET_RESP_DERIVED_DT);

                req->dev.user_buf = get_pkt->addr;
                req->dev.user_count = get_pkt->count;
                req->dev.datatype = MPI_DATATYPE_NULL;
                req->dev.request = get_pkt->request;

                req->dev.dtype_info = (MPIDI_RMA_dtype_info *) 
                    MPIU_Malloc(sizeof(MPIDI_RMA_dtype_info));
		/* --BEGIN ERROR HANDLING-- */
                if (! req->dev.dtype_info)
		{
                    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
						      "**nomem", 0 );
                    goto fn_exit;
                }
		/* --END ERROR HANDLING-- */
                
                req->dev.dataloop = MPIU_Malloc(get_pkt->dataloop_size);
		/* --BEGIN ERROR HANDLING-- */
                if (! req->dev.dataloop)
		{
                    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
						      "**nomem", 0 );
                    goto fn_exit;
                }
		/* --END ERROR HANDLING-- */

                req->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)req->dev.dtype_info;
                req->dev.iov[0].MPID_IOV_LEN = sizeof(MPIDI_RMA_dtype_info);
                req->dev.iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)req->dev.dataloop;
                req->dev.iov[1].MPID_IOV_LEN = get_pkt->dataloop_size;
                req->dev.iov_count = 2;

                *rreqp = req;
            }

            break;
        }

	case MPIDI_CH3_PKT_GET_RESP:
	{
	    MPIDI_CH3_Pkt_get_resp_t * get_resp_pkt = &pkt->get_resp;
            MPID_Request *req;

	    MPIDI_DBG_PRINTF((30, FCNAME, "received get response pkt"));

            req = get_resp_pkt->request;  /* FIXME: use handle */

            MPID_Datatype_get_size_macro(req->dev.datatype, type_size);
            req->dev.recv_data_sz = type_size * req->dev.user_count;

	    *rreqp = req;
            mpi_errno = MPIDI_CH3U_Post_data_receive(vc, TRUE, rreqp);
	    /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno != MPI_SUCCESS)
	    {
                mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
						 "**ch3|postrecv", "**ch3|postrecv %s", "MPIDI_CH3_PKT_GET_RESP");
	    }
	    /* --END ERROR HANDLING-- */

            break;
        }

	case MPIDI_CH3_PKT_LOCK:
	{
	    MPIDI_CH3_Pkt_lock_t * lock_pkt = &pkt->lock;
            MPID_Request *req;
            int requested_lock, existing_lock, grant_lock;

	    MPIDI_DBG_PRINTF((30, FCNAME, "received lock pkt"));

            /* Check if lock can be granted. If not, queue it. If yes,
             * send back a lock-granted packet. */

            existing_lock = lock_pkt->win_ptr->current_lock_type;
            requested_lock = lock_pkt->lock_type;

            /* Locking Rules:

               Requested          Existing             Action
               --------           --------             ------
               Shared             Exclusive            Queue it
               Shared             NoLock/Shared        Grant and incr. counter
               Exclusive          NoLock               Grant and incr. counter 
               Exclusive          Exclusive/Shared     Queue it
            */

            if ( ( (requested_lock == MPI_LOCK_SHARED) && 
                        ((existing_lock == MPID_LOCK_NONE) ||
                         (existing_lock == MPI_LOCK_SHARED) ) )
                 || 
                    ( (requested_lock == MPI_LOCK_EXCLUSIVE) &&
                      (existing_lock == MPID_LOCK_NONE) ) ) {

                grant_lock = 1;
            }
            else {
                grant_lock = 0;
            }

            if (grant_lock == 1)
	    {
                MPIDI_CH3_Pkt_t upkt;
                MPIDI_CH3_Pkt_lock_granted_t * lock_granted_pkt = &upkt.lock_granted;

                /* increment window counter */
                /* FIXME: MT: this has to be done atomically */
                lock_pkt->win_ptr->my_counter++;

                /* set new lock type on window */
                lock_pkt->win_ptr->current_lock_type = requested_lock;

                /* send lock granted packet */
                lock_granted_pkt->type = MPIDI_CH3_PKT_LOCK_GRANTED;
                lock_granted_pkt->lock_granted_flag_ptr = lock_pkt->lock_granted_flag_ptr;
                
                mpi_errno = MPIDI_CH3_iStartMsg(vc, lock_granted_pkt,
                                              sizeof(*lock_granted_pkt), &req);
                /* --BEGIN ERROR HANDLING-- */
                if (mpi_errno != MPI_SUCCESS) {
                    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
						     "**ch3|rmamsg", 0);
                    return mpi_errno;
                }
		/* --END ERROR HANDLING-- */
		if (req != NULL)
		{
		    MPID_Request_release(req);
		}
            }

            else {
                /* queue the lock information */
                MPIDI_Win_lock_queue *curr_ptr, *prev_ptr, *new_ptr;

                curr_ptr = lock_pkt->win_ptr->lock_queue;
                prev_ptr = curr_ptr;
                while (curr_ptr != NULL)
                {
                    prev_ptr = curr_ptr;
                    curr_ptr = curr_ptr->next;
                }

                new_ptr = (MPIDI_Win_lock_queue *) MPIU_Malloc(sizeof(MPIDI_Win_lock_queue));
                /* --BEGIN ERROR HANDLING-- */
                if (!new_ptr)
                {
                    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
                    goto fn_exit;
                }
                /* --END ERROR HANDLING-- */
                if (prev_ptr != NULL)
                    prev_ptr->next = new_ptr;
                else 
                    lock_pkt->win_ptr->lock_queue = new_ptr;
        
                new_ptr->next = NULL;  
                new_ptr->lock_type = requested_lock;
                new_ptr->lock_granted_flag_ptr = lock_pkt->lock_granted_flag_ptr;
                new_ptr->vc = vc;
            }

            *rreqp = NULL;

            break;
        }

	case MPIDI_CH3_PKT_LOCK_GRANTED:
	{
	    MPIDI_CH3_Pkt_lock_granted_t * lock_granted_pkt = &pkt->lock_granted;
	    MPIDI_DBG_PRINTF((30, FCNAME, "received lock granted pkt"));

            /* set the lock_granted flag in the window */
            *(lock_granted_pkt->lock_granted_flag_ptr) = 1;

            *rreqp = NULL;
            MPIDI_CH3_Progress_signal_completion();	

            break;
        }

	case MPIDI_CH3_PKT_FLOW_CNTL_UPDATE:
	{
	    /* --BEGIN ERROR HANDLING-- */
	    MPIDI_DBG_PRINTF((30, FCNAME, "received flow control update pkt"));
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN, "**ch3|flowcntlpkt", 0);
	    *rreqp = NULL;
	    break;
	    /* --END ERROR HANDLING-- */
	}
	
	default:
	{
	    /* --BEGIN ERROR HANDLING-- */
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN, "**ch3|unknownpkt",
					     "**ch3|unknownpkt %d", pkt->type);
	    *rreqp = NULL;
	    break;
	    /* --END ERROR HANDLING-- */
	}
    }

  fn_exit:
    in_routine = FALSE;
    MPIDI_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_HANDLE_ORDERED_RECV_PKT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Post_data_receive
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Post_data_receive(MPIDI_VC * vc, int found, MPID_Request ** rreqp)
{
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPIDI_msg_sz_t userbuf_sz;
    MPID_Datatype * dt_ptr;
    MPIDI_msg_sz_t data_sz;
    MPID_Request * rreq = *rreqp;
    int mpi_errno = MPI_SUCCESS;
    
    MPIDI_DBG_PRINTF((30, FCNAME, "entering"));

    if (rreq->dev.recv_data_sz == 0)
    {
	MPIDI_DBG_PRINTF((30, FCNAME, "null message, %s, decrementing completion counter",
			  (found ? "posted request found" : "unexpected request allocated")));
	/* mark data transfer as complete and decrment CC */
	MPIDI_CH3U_Request_complete(rreq);
	*rreqp = NULL;
	goto fn_exit;
    }
	
    if (found)
    {
	MPIDI_DBG_PRINTF((30, FCNAME, "posted request found"));
	
	MPIDI_CH3U_Datatype_get_info(rreq->dev.user_count, rreq->dev.datatype, dt_contig, userbuf_sz, dt_ptr, dt_true_lb);
		
	if (rreq->dev.recv_data_sz <= userbuf_sz)
	{
	    data_sz = rreq->dev.recv_data_sz;
	}
	else
	{
	    MPIDI_DBG_PRINTF((35, FCNAME, "receive buffer too small; message truncated, msg_sz=" MPIDI_MSG_SZ_FMT ", userbuf_sz="
			      MPIDI_MSG_SZ_FMT, rreq->dev.recv_data_sz, userbuf_sz));
	    rreq->status.MPI_ERROR = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TRUNCATE,
							  "**truncate", "**truncate %d %d %d %d", rreq->status.MPI_SOURCE,
							  rreq->status.MPI_TAG, rreq->dev.recv_data_sz, userbuf_sz );
	    rreq->status.count = userbuf_sz;
	    data_sz = userbuf_sz;
	}

	if (dt_contig && data_sz == rreq->dev.recv_data_sz)
	{
	    /* user buffer is contiguous and large enough to store the
	       entire message */
	    MPIDI_DBG_PRINTF((35, FCNAME, "IOV loaded for contiguous read"));
	    rreq->dev.iov[0].MPID_IOV_BUF = (char*)(rreq->dev.user_buf) + dt_true_lb;
	    rreq->dev.iov[0].MPID_IOV_LEN = data_sz;
	    rreq->dev.iov_count = 1;
	    rreq->dev.ca = MPIDI_CH3_CA_COMPLETE;
	}
	else
	{
	    /* user buffer is not contiguous or is too small to hold
	       the entire message */
	    int mpi_errno;
		    
	    MPIDI_DBG_PRINTF((35, FCNAME, "IOV loaded for non-contiguous read"));
	    MPID_Segment_init(rreq->dev.user_buf, rreq->dev.user_count, rreq->dev.datatype, &rreq->dev.segment, 0);
	    rreq->dev.segment_first = 0;
	    rreq->dev.segment_size = data_sz;
	    mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
						 "**ch3|loadrecviov", 0);
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */
	}
    }
    else /* if (!found) */
    {
	/* TODO: to improve performance, allocate temporary buffer from a specialized buffer pool. */
	MPIDI_DBG_PRINTF((30, FCNAME, "unexpected request allocated"));
		
	rreq->dev.tmpbuf = MPIU_Malloc(rreq->dev.recv_data_sz);
	rreq->dev.tmpbuf_sz = rreq->dev.recv_data_sz;
		
	rreq->dev.iov[0].MPID_IOV_BUF = rreq->dev.tmpbuf;
	rreq->dev.iov[0].MPID_IOV_LEN = rreq->dev.recv_data_sz;
	rreq->dev.iov_count = 1;
	rreq->dev.ca = MPIDI_CH3_CA_UNPACK_UEBUF_AND_COMPLETE;
	rreq->dev.recv_pending_count = 2;
	MPID_Request_initialized_set(rreq);
    }

fn_exit:
    MPIDI_DBG_PRINTF((30, FCNAME, "exiting"));
    return mpi_errno;
}
