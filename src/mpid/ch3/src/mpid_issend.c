/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/* XXX - HOMOGENEOUS SYSTEMS ONLY -- no data conversion is performed */

/*
 * MPID_Issend()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Issend
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Issend(const void * buf, int count, MPI_Datatype datatype, int rank,
		int tag, MPID_Comm * comm, int context_offset,
		MPID_Request ** request)
{
    MPIDI_msg_sz_t data_sz;
    int dt_contig;
    MPID_Request * sreq;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_ISSEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_ISSEND);

    MPIDI_DBG_PRINTF((10, FCNAME, "entering"));
    MPIDI_DBG_PRINTF((15, FCNAME, "rank=%d, tag=%d, context=%d", rank, tag,
		      comm->context_id + context_offset));
    
    MPIDI_CH3M_create_send_request(sreq, mpi_errno, goto fn_exit);
    
    if (rank == MPI_PROC_NULL)
    {
	sreq->ref_count = 1;
	sreq->cc = 0;
	goto fn_exit;
    }

    sreq->ch3.vc = comm->vcr[rank];
    
    if (rank == comm->rank && comm->comm_kind != MPID_INTERCOMM)
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
	    MPID_Request_set_complete(sreq);
	    MPID_Request_release(rreq);
	    MPID_Request_release(sreq);
	}
	else
	{
	    MPIDI_DBG_PRINTF((15, FCNAME, "added receive request to unexpected"
			      " queue; attaching send request"));
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
	MPIDI_CH3_Pkt_eager_sync_send_t * const es_pkt = &upkt.eager_sync_send;

	MPIDI_DBG_PRINTF((15, FCNAME, "sending zero length message"));

	sreq->cc = 2;
	sreq->ch3.ca = MPIDI_CH3_CA_COMPLETE;
	
	es_pkt->type = MPIDI_CH3_PKT_EAGER_SYNC_SEND;
	es_pkt->match.rank = comm->rank;
	es_pkt->match.tag = tag;
	es_pkt->match.context_id = comm->context_id + context_offset;
	es_pkt->sender_req_id = sreq->handle;
	es_pkt->data_sz = 0;

	MPIDI_CH3_iSend(comm->vcr[rank], sreq, es_pkt, sizeof(*es_pkt));
	goto fn_exit;
    }
    
    /* TODO - flow control: limit number of outstanding eager messsages that
       contain data which may need to be buffered by the receiver */

    /* TODO - handle case where data_sz is greater than what can be stored in
       iov.MPID_IOV_LEN.  probably just hand off to segment code. */
    
    if (data_sz + sizeof(MPIDI_CH3_Pkt_eager_sync_send_t) <=
	MPIDI_CH3_EAGER_MAX_MSG_SIZE)
    {
	MPIDI_CH3_Pkt_t upkt;
	MPIDI_CH3_Pkt_eager_sync_send_t * const es_pkt = &upkt.eager_sync_send;
	MPID_IOV iov[MPID_IOV_LIMIT];
	    
	sreq->cc = 2;
	sreq->ch3.ca = MPIDI_CH3_CA_COMPLETE;
	
	es_pkt->type = MPIDI_CH3_PKT_EAGER_SYNC_SEND;
	es_pkt->match.rank = comm->rank;
	es_pkt->match.tag = tag;
	es_pkt->match.context_id = comm->context_id + context_offset;
	es_pkt->sender_req_id = sreq->handle;
	es_pkt->data_sz = data_sz;

	iov[0].MPID_IOV_BUF = es_pkt;
	iov[0].MPID_IOV_LEN = sizeof(*es_pkt);

	if (dt_contig)
	{
	    MPIDI_DBG_PRINTF((15, FCNAME, "sending contiguous sync eager "
			      "message, data_sz=" MPIDI_MSG_SZ_FMT, data_sz));
	    
	    iov[1].MPID_IOV_BUF = (void *) buf;
	    iov[1].MPID_IOV_LEN = data_sz;
	    MPIDI_CH3_iSendv(sreq->ch3.vc, sreq, iov, 2);
	}
	else
	{
	    int iov_n;
	    
	    MPIDI_DBG_PRINTF((15, FCNAME, "sending non-contiguous sync eager "
			      "message, data_sz=" MPIDI_MSG_SZ_FMT, data_sz));
	    
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
    if (sreq != NULL)
    {
	MPIDI_DBG_PRINTF((15, FCNAME, "request allocated, handle=0x%08x",
			  sreq->handle));
    }
    MPIDI_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_ISSEND);
    return mpi_errno;
}
