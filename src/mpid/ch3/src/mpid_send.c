/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/* FIXME - HOMOGENEOUS SYSTEMS ONLY -- no data conversion is performed */

/*
 * MPID_Send()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Send(const void * buf, int count, MPI_Datatype datatype, int rank, int tag, MPID_Comm * comm, int context_offset,
	      MPID_Request ** request)
{
    MPIDI_msg_sz_t data_sz;
    int dt_contig;
    MPID_Datatype * dt_ptr;
    MPID_Request * sreq = NULL;
    MPIDI_VC * vc;
#if defined(MPID_USE_SEQUENCE_NUMBERS)
    MPID_Seqnum_t seqnum;
#endif    
    int mpi_errno = MPI_SUCCESS;    
    MPIDI_STATE_DECL(MPID_STATE_MPID_SEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEND);

    MPIDI_DBG_PRINTF((10, FCNAME, "entering"));
    MPIDI_DBG_PRINTF((15, FCNAME, "rank=%d, tag=%d, context=%d", rank, tag, comm->context_id + context_offset));

    if (rank == comm->rank && comm->comm_kind != MPID_INTERCOMM)
    {
	mpi_errno = MPIDI_Isend_self(buf, count, datatype, rank, tag, comm, context_offset, MPIDI_REQUEST_TYPE_SEND, &sreq);
#       if defined(MPICH_SINGLE_THREADED)
	{
	    if (sreq != NULL && sreq->cc != 0)
	    {
		MPIDI_ERR_PRINTF((FCNAME, "DEADLOCK - MPI_Send() to self, no matching received posted."));
		abort();
	    }
	}
#	endif
	goto fn_exit;
    }

    if (rank == MPI_PROC_NULL)
    {
	goto fn_exit;
    }

    MPIDI_CH3U_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr);
    
    vc = comm->vcr[rank];
    
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
	
	MPIDI_CH3U_VC_FAI_send_seqnum(vc, seqnum);
	MPIDI_CH3U_Pkt_set_seqnum(eager_pkt, seqnum);
	
	sreq = MPIDI_CH3_iStartMsg(vc, eager_pkt, sizeof(*eager_pkt));
	if (sreq != NULL)
	{
	    MPIDI_CH3U_Request_set_seqnum(sreq, seqnum);
	    MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_SEND);
	    /* sreq->comm = comm;
	      MPIR_Comm_add_ref(comm); -- not necessary for blocking functions */
	}
	
	goto fn_exit;
    }
    
    /* FIXME: flow control: limit number of outstanding eager messsages containing data and need to be buffered by the receiver */

    if (data_sz + sizeof(MPIDI_CH3_Pkt_eager_send_t) <=	MPIDI_CH3_EAGER_MAX_MSG_SIZE)
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

	iov[0].MPID_IOV_BUF = (void*)eager_pkt;
	iov[0].MPID_IOV_LEN = sizeof(*eager_pkt);

	if (dt_contig)
	{
	    MPIDI_DBG_PRINTF((15, FCNAME, "sending contiguous eager message, data_sz=" MPIDI_MSG_SZ_FMT, data_sz));
	    
	    iov[1].MPID_IOV_BUF = (void *) buf;
	    iov[1].MPID_IOV_LEN = data_sz;
	    
	    MPIDI_CH3U_VC_FAI_send_seqnum(vc, seqnum);
	    MPIDI_CH3U_Pkt_set_seqnum(eager_pkt, seqnum);
	    
	    sreq = MPIDI_CH3_iStartMsgv(vc, iov, 2);
	    if (sreq != NULL)
	    {
		MPIDI_CH3U_Request_set_seqnum(sreq, seqnum);
		MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_SEND);
		/* sreq->comm = comm;
		   MPIR_Comm_add_ref(comm); -- not necessary for blocking functions */

	    }
	}
	else
	{
	    int iov_n;
	    
	    MPIDI_DBG_PRINTF((15, FCNAME, "sending non-contiguous eager message, data_sz=" MPIDI_MSG_SZ_FMT, data_sz));
	    
	    MPIDI_CH3M_create_sreq(sreq, mpi_errno, goto fn_exit);
	    MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_SEND);
	    
	    MPID_Segment_init(buf, count, datatype, &sreq->ch3.segment);
	    sreq->ch3.segment_first = 0;
	    sreq->ch3.segment_size = data_sz;
	    
	    iov_n = MPID_IOV_LIMIT - 1;
	    mpi_errno = MPIDI_CH3U_Request_load_send_iov(sreq, &iov[1], &iov_n);
	    if (mpi_errno == MPI_SUCCESS)
	    {
		iov_n += 1;
		
		MPIDI_CH3U_VC_FAI_send_seqnum(vc, seqnum);
		MPIDI_CH3U_Pkt_set_seqnum(eager_pkt, seqnum);
		MPIDI_CH3U_Request_set_seqnum(sreq, seqnum);
		
		if (sreq->ch3.ca != MPIDI_CH3_CA_COMPLETE)
		{
		    /* sreq->ch3.datatype_ptr = dt_ptr;
		       MPID_Datatype_add_ref(dt_ptr); -- no necessary for blocking functions */
		}
		
		MPIDI_CH3_iSendv(vc, sreq, iov, iov_n);
	    }
	    else
	    {
		MPIU_Object_set_ref(sreq, 0);
		MPIDI_CH3_Request_destroy(sreq);
		sreq = NULL;
	    }
	}
    }
    else
    {
	MPIDI_CH3_Pkt_t upkt;
	MPIDI_CH3_Pkt_rndv_req_to_send_t * const rts_pkt = &upkt.rndv_req_to_send;
	MPID_Request * rts_sreq;
	
	MPIDI_DBG_PRINTF((15, FCNAME, "sending rndv RTS, data_sz=" MPIDI_MSG_SZ_FMT, data_sz));
	    
	MPIDI_CH3M_create_sreq(sreq, mpi_errno, goto fn_exit);
	MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_SEND);
	sreq->partner_request = NULL;
	
	/* FIXME - Since the request is never returned to the user and they can't do things like cancel it or wait on it, we may
           not need to fill in all of the fields.  For example, it may be completely unnecessary to supply the matching
           information.  Also, some of the fields can be set after the message has been sent.  These issues should be looked at
           more closely when we are trying to squeeze those last few nanoseconds out of the code.  */
	
	rts_pkt->type = MPIDI_CH3_PKT_RNDV_REQ_TO_SEND;
	rts_pkt->match.rank = comm->rank;
	rts_pkt->match.tag = tag;
	rts_pkt->match.context_id = comm->context_id + context_offset;
	rts_pkt->sender_req_id = sreq->handle;
	rts_pkt->data_sz = data_sz;
	
	MPIDI_CH3U_VC_FAI_send_seqnum(vc, seqnum);
	MPIDI_CH3U_Pkt_set_seqnum(rts_pkt, seqnum);
	MPIDI_CH3U_Request_set_seqnum(sreq, seqnum);

	rts_sreq = MPIDI_CH3_iStartMsg(vc, rts_pkt, sizeof(*rts_pkt));
	if (rts_sreq != NULL)
	{
	    MPID_Request_release(rts_sreq);
	}

	/* FIXME: fill temporary IOV or pack temporary buffer after send to hide some latency.  This requires synchronization
           because the CTS packet could arrive and be processed before the above iStartmsg completes (depending on the progress
           engine, threads, etc.). */
	
	if (dt_ptr != NULL)
	{
	    /* sreq->ch3.datatype_ptr = dt_ptr;
	       MPID_Datatype_add_ref(dt_ptr);  -- no necessary for blocking send */
	}

    }

  fn_exit:
    *request = sreq;
#ifdef MPICH_DBG_OUTPUT
    if (mpi_errno == MPI_SUCCESS)
    {
	if (sreq)
	{
	    MPIDI_DBG_PRINTF((15, FCNAME, "request allocated, handle=0x%08x", sreq->handle));
	}
	else
	{
	    MPIDI_DBG_PRINTF((15, FCNAME, "operation complete, no requests allocated"));
	}
    }
#endif
    MPIDI_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEND);
    return mpi_errno;
}
