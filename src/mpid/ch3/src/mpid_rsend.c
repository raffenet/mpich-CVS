/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/* XXX - HOMOGENEOUS SYSTEMS ONLY -- no data conversion is performed */

/*
 * MPID_Rsend()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Rsend
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Rsend(const void * buf, int count, MPI_Datatype datatype,
	       int rank, int tag, MPID_Comm * comm, int context_offset,
	       MPID_Request ** request)
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_ready_send_t * const ready_pkt = &upkt.ready_send;
    MPIDI_msg_sz_t data_sz;
    int dt_contig;
    MPID_Request * sreq;
    int mpi_errno = MPI_SUCCESS;    

    MPIDI_DBG_PRINTF((10, FCNAME, "entering"));
    MPIDI_DBG_PRINTF((15, FCNAME, "rank=%d, tag=%d, context=%d", rank, tag,
		      comm->context_id + context_offset));
    
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
	sreq = MPIDI_CH3_iStartMsg(
	    comm->vcr[rank], ready_pkt, sizeof(*ready_pkt));
	if (sreq != NULL)
	{
	    sreq->comm = comm;
	}
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
	    
	    MPIDI_DBG_PRINTF((15, FCNAME, "sending non-contiguous ready-mode "
			      "message, data_sz=" MPIDI_MSG_SZ_FMT, data_sz));
	    
	    sreq = MPIDI_CH3_Request_create();
	    if (sreq == NULL)
	    {
		MPIDI_DBG_PRINTF((15, FCNAME,
				  "send request allocation failed"));
		mpi_errno = MPI_ERR_NOMEM;
		goto fn_exit;
	    }
	    
	    sreq->ref_count = 2;
	    sreq->kind = MPID_REQUEST_SEND;
	    sreq->comm = comm;
	    sreq->ch3.match.rank = rank;
	    sreq->ch3.match.tag = tag;
	    sreq->ch3.match.context_id = comm->context_id + context_offset;
	    sreq->ch3.user_buf = (void *) buf;
	    sreq->ch3.user_count = count;
	    sreq->ch3.datatype = datatype;
	    sreq->ch3.vc = comm->vcr[rank];
	    
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
		MPID_Request_release(sreq);
		sreq = NULL;
		goto fn_exit;
	    }
	}
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
    return mpi_errno;
}
