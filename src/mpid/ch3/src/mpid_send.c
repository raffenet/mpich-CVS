/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/* XXX - HOMOGENEOUS SYSTEMS ONLY -- no data conversion is performed */

/*
 * MPID_Send()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Send(const void * buf, int count, MPI_Datatype datatype,
	      int rank, int tag, MPID_Comm * comm, int context_offset,
	      MPID_Request ** request)
{
    MPIDI_msg_sz_t data_sz;
    int dt_contig;
    MPID_Request * sreq = NULL;
    int mpi_errno = MPI_SUCCESS;    
    MPIDI_STATE_DECL(MPID_STATE_MPID_SEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEND);

    MPIDI_DBG_PRINTF((10, FCNAME, "entering"));
    MPIDI_DBG_PRINTF((15, FCNAME, "rank=%d, tag=%d, context=%d", rank, tag,
		      comm->context_id + context_offset));

    if (rank == comm->rank)
    {
	MPIDI_Message_match match;
	MPID_Request * rreq;
	int found;
	
	MPIDI_DBG_PRINTF((15, FCNAME, "sending message to self"));
	
	match.rank = rank;
	match.tag = tag;
	match.context_id = comm->context_id + context_offset;
	rreq = MPIDI_CH3U_Request_FDP_or_AEU(&match, &found);
	if (rreq == NULL)
	{
	    mpi_errno = MPI_ERR_NOMEM;
	    goto fn_exit;
	}
	
	if (found)
	{
	    MPIDI_DBG_PRINTF((15, FCNAME, "found posted receive request; "
			      "copying data"));
	    
	    MPIDI_CH3U_Buffer_copy(
		buf, count, datatype, &mpi_errno,
		rreq->ch3.user_buf,rreq->ch3.user_count, rreq->ch3.datatype,
		&data_sz, &rreq->status.MPI_ERROR);
	    rreq->status.MPI_SOURCE = rank;
	    rreq->status.MPI_TAG = tag;
	    rreq->status.count = data_sz;
	    MPID_Request_set_complete(rreq);
	    MPID_Request_release(rreq);
	}
	else
	{
	    MPIDI_DBG_PRINTF((15, FCNAME, "added receive request to unexpected"
			      " queue; attaching send request"));
	    
	    MPIDI_CH3M_create_send_request(sreq, mpi_errno,
	    {
		rreq->status.count = 0;
		rreq->status.MPI_SOURCE = rank;
		rreq->status.MPI_TAG = tag;
		rreq->status.MPI_ERROR = mpi_errno;
		MPID_Request_set_complete(rreq);
		MPID_Request_release(rreq);
		goto fn_exit;
	    });

	    rreq->partner_request = sreq;
	    MPIDI_Request_set_msg_type(rreq, MPIDI_REQUEST_SELF_MSG);
	}

	goto fn_exit;
    }
    
    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN)
    {
	dt_contig = TRUE;
	data_sz = count * MPID_Datatype_get_basic_size(datatype);
	MPIDI_DBG_PRINTF((15, FCNAME, "basic datatype: dt_contig=%d, "
			  "dt_sz=%d, data_sz=" MPIDI_MSG_SZ_FMT, dt_contig,
			  MPID_Datatype_get_basic_size(datatype), data_sz));
    }
    else
    {
	MPID_Datatype * dtp;
	
	MPID_Datatype_get_ptr(datatype, dtp);
	dt_contig = dtp->is_contig;
	data_sz = count * dtp->size;
	MPIDI_DBG_PRINTF((15, FCNAME, "user defined datatype: dt_contig=%d, "
			  "dt_sz=%d, data_sz=" MPIDI_MSG_SZ_FMT, dt_contig,
			  dtp->size, data_sz));
    }

    if (data_sz == 0)
    {
	MPIDI_CH3_Pkt_t upkt;
	MPIDI_CH3_Pkt_eager_send_t * const eager_pkt = &upkt.eager_send;

	MPIDI_DBG_PRINTF((15, FCNAME, "sending zero length message"));
	eager_pkt->type = MPIDI_CH3_PKT_EAGER_SEND;
	eager_pkt->match.rank = comm->rank;
	eager_pkt->match.tag = tag;
	eager_pkt->match.context_id = comm->context_id + context_offset;
	eager_pkt->sender_req_id = MPI_REQUEST_NULL;
	eager_pkt->data_sz = 0;

	sreq = MPIDI_CH3_iStartMsg(
	    comm->vcr[rank], eager_pkt, sizeof(*eager_pkt));
	if (sreq != NULL)
	{
	    sreq->comm = comm;
	}
	
	goto fn_exit;
    }
    
    /* TODO - flow control: limit number of outstanding eager messsages that
       contain data which may need to be buffered by the receiver */

    /* TODO - handle case where data_sz is greater than what can be stored in
       iov.MPID_IOV_LEN.  probably just hand off to segment code. */
    
    if (data_sz + sizeof(MPIDI_CH3_Pkt_eager_send_t) <=
	MPIDI_CH3_EAGER_MAX_MSG_SIZE)
    {
	MPIDI_CH3_Pkt_t upkt;
	MPIDI_CH3_Pkt_eager_send_t * const eager_pkt = &upkt.eager_send;
	MPID_IOV iov[MPID_IOV_LIMIT];
	    
	eager_pkt->type = MPIDI_CH3_PKT_EAGER_SEND;
	eager_pkt->match.rank = comm->rank;
	eager_pkt->match.tag = tag;
	eager_pkt->match.context_id = comm->context_id + context_offset;
	eager_pkt->sender_req_id = MPI_REQUEST_NULL;
	eager_pkt->data_sz = data_sz;

	iov[0].MPID_IOV_BUF = eager_pkt;
	iov[0].MPID_IOV_LEN = sizeof(*eager_pkt);

	if (dt_contig)
	{
	    MPIDI_DBG_PRINTF((15, FCNAME, "sending contiguous eager message, "
			      "data_sz=" MPIDI_MSG_SZ_FMT, data_sz));
	    
	    iov[1].MPID_IOV_BUF = (void *) buf;
	    iov[1].MPID_IOV_LEN = data_sz;
	    sreq = MPIDI_CH3_iStartMsgv(comm->vcr[rank], iov, 2);
	    if (sreq != NULL)
	    {
		/* XXX - what else needs to be set? */
		sreq->comm = comm;
	    }
	}
	else
	{
	    int iov_n;
	    
	    MPIDI_DBG_PRINTF((15, FCNAME, "sending non-contiguous eager "
			      "message, data_sz=" MPIDI_MSG_SZ_FMT, data_sz));
	    
	    MPIDI_CH3M_create_send_request(sreq, mpi_errno, goto fn_exit);
	    
	    MPID_Segment_init(buf, count, datatype, &sreq->ch3.segment);
	    sreq->ch3.segment_first = 0;
	    sreq->ch3.segment_size = data_sz;
	    
	    iov_n = MPID_IOV_LIMIT - 1;
	    mpi_errno = MPIDI_CH3U_Request_load_send_iov(
		sreq, &iov[1], &iov_n);
	    if (mpi_errno == MPI_SUCCESS)
	    {
		iov_n += 1;
		MPIDI_CH3_iSendv(sreq->ch3.vc, sreq, iov, iov_n);
	    }
	    else
	    {
		MPIDI_CH3_Request_destroy(sreq);
		sreq = NULL;
		goto fn_exit;
	    }
	}
    }
    else
    {
	MPIDI_CH3_Pkt_t upkt;
	MPIDI_CH3_Pkt_rndv_req_to_send_t * const rts_pkt =
	    &upkt.rndv_req_to_send;
	MPID_Request * rts_sreq;
	
	MPIDI_DBG_PRINTF((15, FCNAME, "sending rndv RTS, data_sz="
			  MPIDI_MSG_SZ_FMT, data_sz));
	    
	MPIDI_CH3M_create_send_request(sreq, mpi_errno, goto fn_exit);
	
	/* XXX - Since the request is never returned to the user and they can't
           do things like cancel it or wait on it, we may not need to fill in
           all of the fields.  For example, it may be completely unnecessary to
           supply the matching information.  Also, some of the fields can be
           set after the message has been sent.  These issues should be looked
           at more closely when we are trying to squeeze those last few
           nanoseconds out of the code.  */
	
	rts_pkt->type = MPIDI_CH3_PKT_RNDV_REQ_TO_SEND;
	rts_pkt->match.rank = comm->rank;
	rts_pkt->match.tag = tag;
	rts_pkt->match.context_id = comm->context_id + context_offset;
	rts_pkt->sender_req_id = sreq->handle;
	rts_pkt->data_sz = data_sz;

	rts_sreq = MPIDI_CH3_iStartMsg(comm->vcr[rank], rts_pkt,
				       sizeof(*rts_pkt));
	if (rts_sreq != NULL)
	{
	    MPID_Request_release(rts_sreq);
	}

	/* TODO: fill temporary IOV or pack temporary buffer after send to hide
           some latency.  This require synchronization because CTS could arrive
           and be processed before the above iSend completes (depending on the
           progress engine, threads, etc.). */
    }

  fn_exit:
    *request = sreq;
    if (mpi_errno == MPI_SUCCESS)
    {
	if (sreq)
	{
	    MPIDI_DBG_PRINTF((15, FCNAME, "request allocated, handle=0x%08x",
			      sreq->handle));
	}
	else
	{
	    MPIDI_DBG_PRINTF((15, FCNAME, "operation complete, no requests "
			      "allocated"));
	}
    }
    MPIDI_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEND);
    return mpi_errno;
}
