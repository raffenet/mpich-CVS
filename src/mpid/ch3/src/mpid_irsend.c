/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/* FIXME: HOMOGENEOUS SYSTEMS ONLY -- no data conversion is performed */

/*
 * MPID_Irsend()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Irsend
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Irsend(const void * buf, int count, MPI_Datatype datatype, int rank, int tag, MPID_Comm * comm, int context_offset,
		MPID_Request ** request)
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_ready_send_t * const ready_pkt = &upkt.ready_send;
    MPIDI_msg_sz_t data_sz;
    int dt_contig;
    MPID_Request * sreq;
    MPID_IOV iov[MPID_IOV_LIMIT];
    MPIDI_VC * vc;
#if defined(MPID_USE_SEQUENCE_NUMBERS)
    MPID_Seqnum_t seqnum;
#endif    
    int mpi_errno = MPI_SUCCESS;    
    MPIDI_STATE_DECL(MPID_STATE_MPID_IRSEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_IRSEND);

    MPIDI_DBG_PRINTF((10, FCNAME, "entering"));
    MPIDI_DBG_PRINTF((15, FCNAME, "rank=%d, tag=%d, context=%d", rank, tag, comm->context_id + context_offset));
    
    if (rank == comm->rank && comm->comm_kind != MPID_INTERCOMM)
    {
	mpi_errno = MPIDI_Isend_self(buf, count, datatype, rank, tag, comm, context_offset, MPIDI_REQUEST_TYPE_RSEND, &sreq);
	goto fn_exit;
    }
    
    MPIDI_CH3M_create_send_request(sreq, mpi_errno, goto fn_exit);
    MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_RSEND);
    
    if (rank == MPI_PROC_NULL)
    {
	sreq->ref_count = 1;
	sreq->cc = 0;
	goto fn_exit;
    }
    
    MPIDI_CH3U_Datatype_get_info(count, datatype, dt_contig, data_sz);

    vc = comm->vcr[rank];
    
    ready_pkt->type = MPIDI_CH3_PKT_READY_SEND;
    ready_pkt->match.rank = comm->rank;
    ready_pkt->match.tag = tag;
    ready_pkt->match.context_id = comm->context_id + context_offset;
    ready_pkt->sender_req_id = MPI_REQUEST_NULL;
    ready_pkt->data_sz = data_sz;

    if (data_sz == 0)
    {
	MPIDI_DBG_PRINTF((15, FCNAME, "sending zero length message"));

	MPIDI_CH3U_VC_FAI_send_seqnum(vc, seqnum);
	MPIDI_CH3U_Pkt_set_seqnum(ready_pkt, seqnum);
	MPIDI_CH3U_Request_set_seqnum(sreq, seqnum);
	
	MPIDI_CH3_iSend(vc, sreq, ready_pkt, sizeof(*ready_pkt));
	goto fn_exit;
    }
    
    /* FIXME: flow control: limit number of outstanding eager messsages containing data and need to be buffered by the receiver */

    /* FIXME: handle case where data_sz is greater than what can be stored in iov.MPID_IOV_LEN.  hand off to segment code? */
    
    iov[0].MPID_IOV_BUF = ready_pkt;
    iov[0].MPID_IOV_LEN = sizeof(*ready_pkt);

    if (dt_contig)
    {
	MPIDI_DBG_PRINTF((15, FCNAME, "sending contiguous ready-mode message, data_sz=" MPIDI_MSG_SZ_FMT, data_sz));
	    
	iov[1].MPID_IOV_BUF = (void *) buf;
	iov[1].MPID_IOV_LEN = data_sz;

	MPIDI_CH3U_VC_FAI_send_seqnum(vc, seqnum);
	MPIDI_CH3U_Pkt_set_seqnum(ready_pkt, seqnum);
	MPIDI_CH3U_Request_set_seqnum(sreq, seqnum);

	MPIDI_CH3_iSendv(vc, sreq, iov, 2);
    }
    else
    {
	int iov_n;
	    
	MPIDI_DBG_PRINTF((15, FCNAME, "sending non-contiguous ready-mode message, data_sz=" MPIDI_MSG_SZ_FMT, data_sz));
	    
	MPID_Segment_init(buf, count, datatype, &sreq->ch3.segment);
	sreq->ch3.segment_first = 0;
	sreq->ch3.segment_size = data_sz;
	    
	iov_n = MPID_IOV_LIMIT - 1;
	mpi_errno = MPIDI_CH3U_Request_load_send_iov(sreq, &iov[1], &iov_n);
	if (mpi_errno == MPI_SUCCESS)
	{
	    iov_n += 1;

	    MPIDI_CH3U_VC_FAI_send_seqnum(vc, seqnum);
	    MPIDI_CH3U_Pkt_set_seqnum(ready_pkt, seqnum);
	    MPIDI_CH3U_Request_set_seqnum(sreq, seqnum);
	    
	    MPIDI_CH3_iSendv(vc, sreq, iov, iov_n);
	}
	else
	{
	    sreq->ref_count = 0;
	    MPIDI_CH3_Request_destroy(sreq);
	    sreq = NULL;
	}
    }
 
  fn_exit:
    *request = sreq;
    if (sreq != NULL)
    {
	MPIDI_DBG_PRINTF((15, FCNAME, "request allocated, handle=0x%08x", sreq->handle));
    }
    MPIDI_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_IRSEND);
    return mpi_errno;
}
