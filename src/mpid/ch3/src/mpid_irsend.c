/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/* XXX - HOMOGENEOUS SYSTEMS ONLY -- no data conversion is performed */

/*
 * MPID_Irsend()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Irsend
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Irsend(const void * buf, int count, MPI_Datatype datatype, int rank,
		int tag, MPID_Comm * comm, int context_offset,
		MPID_Request ** request)
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_ready_send_t * const ready_pkt = &upkt.ready_send;
    MPIDI_msg_sz_t data_sz;
    int dt_contig;
    MPID_Request * sreq;
    int mpi_errno = MPI_SUCCESS;    
    MPIDI_STATE_DECL(MPID_STATE_MPID_IRSEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_IRSEND);

    MPIDI_DBG_PRINTF((10, FCNAME, "entering"));
    MPIDI_DBG_PRINTF((15, FCNAME, "rank=%d, tag=%d, context=%d", rank, tag,
		      comm->context_id + context_offset));
    
    if (rank == MPI_PROC_NULL)
    {
	sreq = MPIDI_CH3_Request_create();
	if (sreq != NULL)
	{
	    sreq->ref_count = 1;
	    sreq->cc = 0;
	    sreq->kind = MPID_REQUEST_SEND;
	    MPIR_Status_set_empty(&sreq->status);
	    sreq->status.MPI_SOURCE = MPI_PROC_NULL;
	    /* DEBUG: the following are provided for debugging purposes only */
	    sreq->comm = comm;
	    sreq->ch3.match.rank = rank;
	    sreq->ch3.match.tag = tag;
	    sreq->ch3.match.context_id = comm->context_id + context_offset;
	    sreq->ch3.user_buf = (void *) buf;
	    sreq->ch3.user_count = count;
	    sreq->ch3.datatype = datatype;
	}
	else
	{
	    mpi_errno = MPI_ERR_NOMEM;
	}
	goto fn_exit;
    }
    
    MPIDI_CH3M_create_send_request(sreq, mpi_errno, goto fn_exit);
    
    if (rank == comm->rank)
    {
	MPIDI_Message_match match;
	MPID_Request * rreq;
	
	MPIDI_DBG_PRINTF((15, FCNAME, "sending message to self"));
	
	match.rank = rank;
	match.tag = tag;
	match.context_id = comm->context_id + context_offset;
	rreq = MPIDI_CH3U_Request_FDP(&match);
	if (rreq == NULL)
	{
	    mpi_errno = MPI_ERR_NOMEM;
	    goto fn_exit;
	}
	
	if (rreq != NULL)
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
	    MPIDI_DBG_PRINTF((15, FCNAME, "unable to find matching recv req"));
	    mpi_errno = MPI_ERR_UNKNOWN;
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

    ready_pkt->type = MPIDI_CH3_PKT_READY_SEND;
    ready_pkt->match.rank = comm->rank;
    ready_pkt->match.tag = tag;
    ready_pkt->match.context_id = comm->context_id + context_offset;
    ready_pkt->sender_req_id = MPI_REQUEST_NULL;
    ready_pkt->data_sz = data_sz;

    if (data_sz == 0)
    {
	MPIDI_DBG_PRINTF((15, FCNAME, "sending zero length message"));
	MPIDI_CH3_iSend(sreq->ch3.vc, sreq, ready_pkt, sizeof(*ready_pkt));
    }
    else
    {
	MPID_IOV iov[MPID_IOV_LIMIT];
	    
	/* TODO - handle case where data_sz is greater than what can be stored
	   in iov.MPID_IOV_LEN.  probably just hand off to segment code. */
    
	iov[0].MPID_IOV_BUF = ready_pkt;
	iov[0].MPID_IOV_LEN = sizeof(*ready_pkt);

	if (dt_contig)
	{
	    MPIDI_DBG_PRINTF((15, FCNAME, "sending contiguous ready-mode "
			      "message, data_sz=" MPIDI_MSG_SZ_FMT, data_sz));
	    
	    iov[1].MPID_IOV_BUF = (void *) buf;
	    iov[1].MPID_IOV_LEN = data_sz;
	    MPIDI_CH3_iSendv(sreq->ch3.vc, sreq, iov, 2);
	}
	else
	{
	    int iov_n;
	    
	    MPIDI_DBG_PRINTF((15, FCNAME, "sending non-contiguous ready-mode "
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
 
  fn_exit:
    *request = sreq;
    if (sreq != NULL)
    {
	MPIDI_DBG_PRINTF((15, FCNAME, "request allocated, handle=0x%08x",
			  sreq->handle));
    }
    MPIDI_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_IRSEND);
    return mpi_errno;
}
