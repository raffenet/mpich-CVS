/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "mpid_nem.h"

/* FIXME: HOMOGENEOUS SYSTEMS ONLY -- no data conversion is performed */

/*
 * MPID_Issend()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Issend
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Issend(const void * buf, int count, MPI_Datatype datatype, int rank, int tag, MPID_Comm * comm, int context_offset,
		MPID_Request ** request)
{
    MPIDI_msg_sz_t data_sz;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPID_Datatype * dt_ptr;
    MPID_Request * sreq;
    MPIDI_VC_t * vc;
#if defined(MPID_USE_SEQUENCE_NUMBERS)
    MPID_Seqnum_t seqnum;
#endif    
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_ISSEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_ISSEND);

    MPIU_DBG_MSG_FMT(CH3_OTHER,VERBOSE,(MPIU_DBG_FDEST,
                 "rank=%d, tag=%d, context=%d", 
                 rank, tag, comm->context_id + context_offset));
    
    if (rank == comm->rank && comm->comm_kind != MPID_INTERCOMM)
    {
	mpi_errno = MPIDI_Isend_self(buf, count, datatype, rank, tag, comm, context_offset, MPIDI_REQUEST_TYPE_SSEND, &sreq);
	goto fn_exit;
    }
    
    MPIDI_Request_create_sreq(sreq, mpi_errno, goto fn_exit);
    MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_SSEND);
    
    if (rank == MPI_PROC_NULL)
    {
	MPIU_Object_set_ref(sreq, 1);
	sreq->cc = 0;
	goto fn_exit;
    }

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    
    MPIDI_Comm_get_vc(comm, rank, &vc);
    
    if (data_sz == 0)
    {
	MPIDI_CH3_Pkt_t upkt;
	MPIDI_CH3_Pkt_eager_sync_send_t * const es_pkt = &upkt.eager_sync_send;

	MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"sending zero length message");

	sreq->cc = 2;
	MPIDI_Request_set_msg_type(sreq, MPIDI_REQUEST_EAGER_MSG);
	sreq->dev.ca = MPIDI_CH3_CA_COMPLETE;
	
	MPIDI_Pkt_init(es_pkt, MPIDI_CH3_PKT_EAGER_SYNC_SEND);
	es_pkt->match.rank = comm->rank;
	es_pkt->match.tag = tag;
	es_pkt->match.context_id = comm->context_id + context_offset;
	es_pkt->sender_req_id = sreq->handle;
	es_pkt->data_sz = 0;

	MPIDI_VC_FAI_send_seqnum(vc, seqnum);
	MPIDI_Pkt_set_seqnum(es_pkt, seqnum);
	MPIDI_Request_set_seqnum(sreq, seqnum);
	
	mpi_errno = MPIDI_CH3_iSend(vc, sreq, es_pkt, sizeof(*es_pkt));
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
	    MPIU_Object_set_ref(sreq, 0);
	    MPIDI_CH3_Request_destroy(sreq);
	    sreq = NULL;
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|eagermsg", 0);
	    goto fn_exit;
	}
	/* --END ERROR HANDLING-- */
	
	goto fn_exit;
    }
    
    /* FIXME: flow control: limit number of outstanding eager messsages containing data and need to be buffered by the receiver */

    if (data_sz + sizeof(MPIDI_CH3_Pkt_eager_sync_send_t) <= MPIDI_CH3_EAGER_MAX_MSG_SIZE)
    {
	MPIDI_CH3_Pkt_t upkt;
	MPIDI_CH3_Pkt_eager_sync_send_t * const es_pkt = &upkt.eager_sync_send;
	MPID_IOV iov[MPID_IOV_LIMIT];
	    
	sreq->cc = 2;
	MPIDI_Request_set_msg_type(sreq, MPIDI_REQUEST_EAGER_MSG);
	sreq->dev.ca = MPIDI_CH3_CA_COMPLETE;
	
	MPIDI_Pkt_init(es_pkt, MPIDI_CH3_PKT_EAGER_SYNC_SEND);
	es_pkt->match.rank = comm->rank;
	es_pkt->match.tag = tag;
	es_pkt->match.context_id = comm->context_id + context_offset;
	es_pkt->sender_req_id = sreq->handle;
	es_pkt->data_sz = data_sz;

	iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)es_pkt;
	iov[0].MPID_IOV_LEN = sizeof(*es_pkt);

	if (dt_contig)
	{
	    MPIU_DBG_MSG_FMT(CH3_OTHER,VERBOSE,(MPIU_DBG_FDEST,
         "sending contiguous sync eager message, data_sz=" MPIDI_MSG_SZ_FMT, 
						data_sz));
	    
	    iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) ((char *)buf + dt_true_lb);
	    iov[1].MPID_IOV_LEN = data_sz;
    
	    MPIDI_VC_FAI_send_seqnum(vc, seqnum);
	    MPIDI_Pkt_set_seqnum(es_pkt, seqnum);
	    MPIDI_Request_set_seqnum(sreq, seqnum);

	    mpi_errno = MPIDI_CH3_iSendv(vc, sreq, iov, 2);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS)
	    {
		MPIU_Object_set_ref(sreq, 0);
		MPIDI_CH3_Request_destroy(sreq);
		sreq = NULL;
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|eagermsg", 0);
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */
	}
	else
	{
	    int iov_n;
	    
	    MPIU_DBG_MSG_FMT(CH3_OTHER,VERBOSE,(MPIU_DBG_FDEST,
  "sending non-contiguous sync eager message, data_sz=" MPIDI_MSG_SZ_FMT, 
						data_sz));
	    
	    MPID_Segment_init(buf, count, datatype, &sreq->dev.segment, 0);
	    sreq->dev.segment_first = 0;
	    sreq->dev.segment_size = data_sz;
	    
	    iov_n = MPID_IOV_LIMIT - 1;
	    mpi_errno = MPIDI_CH3U_Request_load_send_iov(sreq, &iov[1], &iov_n);
	    if (mpi_errno == MPI_SUCCESS)
	    {
		iov_n += 1;
		
		MPIDI_VC_FAI_send_seqnum(vc, seqnum);
		MPIDI_Pkt_set_seqnum(es_pkt, seqnum);
		MPIDI_Request_set_seqnum(sreq, seqnum);

		mpi_errno = MPIDI_CH3_iSendv(vc, sreq, iov, iov_n);
		/* --BEGIN ERROR HANDLING-- */
		if (mpi_errno != MPI_SUCCESS)
		{
		    MPIU_Object_set_ref(sreq, 0);
		    MPIDI_CH3_Request_destroy(sreq);
		    sreq = NULL;
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|eagermsg", 0);
		    goto fn_exit;
		}
		/* --END ERROR HANDLING-- */

		if (sreq->dev.ca != MPIDI_CH3_CA_COMPLETE)
		{
		    sreq->dev.datatype_ptr = dt_ptr;
		    MPID_Datatype_add_ref(dt_ptr);
		}
	    }
	    else
	    {
		/* --BEGIN ERROR HANDLING-- */
		MPIU_Object_set_ref(sreq, 0);
		MPIDI_CH3_Request_destroy(sreq);
		sreq = NULL;
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|loadsendiov", 0);
		goto fn_exit;
		/* --END ERROR HANDLING-- */
	    }
	}
    }
    else
    {
	MPIDI_CH3_Pkt_t upkt;
	MPIDI_CH3_Pkt_rndv_req_to_send_t * const rts_pkt = &upkt.rndv_req_to_send;
#ifndef MPIDI_CH3_CHANNEL_RNDV
	MPID_Request * rts_sreq;
#endif
        MPID_IOV cookie;
        MPID_IOV iov[2];
	
	MPIU_DBG_MSG_FMT(CH3_OTHER,VERBOSE,(MPIU_DBG_FDEST,
		   "sending rndv RTS, data_sz=" MPIDI_MSG_SZ_FMT, data_sz));
	    
	MPIDI_Request_set_msg_type(sreq, MPIDI_REQUEST_RNDV_MSG);
	/* FIXME: The partner request should be set to the rts_sreq so that local cancellation can occur; however, this requires
	   allocating the RTS request early to avoid a race condition. */
	sreq->partner_request = NULL;
	
	MPIDI_Pkt_init(rts_pkt, MPIDI_CH3_PKT_RNDV_REQ_TO_SEND);
	rts_pkt->match.rank = comm->rank;
	rts_pkt->match.tag = tag;
	rts_pkt->match.context_id = comm->context_id + context_offset;
	rts_pkt->sender_req_id = sreq->handle;
	rts_pkt->data_sz = data_sz;

	MPIDI_VC_FAI_send_seqnum(vc, seqnum);
	MPIDI_Pkt_set_seqnum(rts_pkt, seqnum);
	MPIDI_Request_set_seqnum(sreq, seqnum);

#ifdef MPIDI_CH3_CHANNEL_RNDV

	MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"Rendezvous send using iStartRndvMsg");
    
	if (dt_contig) 
	{
	    MPIU_DBG_MSG_FMT(CH3_OTHER,VERBOSE,(MPIU_DBG_FDEST,
                              "  contiguous rndv data, data_sz="
			      MPIDI_MSG_SZ_FMT, data_sz));
		
	    sreq->dev.ca = MPIDI_CH3_CA_COMPLETE;
	    
	    sreq->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) ((char*)sreq->dev.user_buf + dt_true_lb);
	    sreq->dev.iov[0].MPID_IOV_LEN = data_sz;
	    sreq->dev.iov_count = 1;
	}
	else
	{
	    MPID_Segment_init(sreq->dev.user_buf, sreq->dev.user_count,
			      sreq->dev.datatype, &sreq->dev.segment, 0);
	    sreq->dev.iov_count = MPID_IOV_LIMIT;
	    sreq->dev.segment_first = 0;
	    sreq->dev.segment_size = data_sz;
	    mpi_errno = MPIDI_CH3U_Request_load_send_iov(sreq, &sreq->dev.iov[0],
							 &sreq->dev.iov_count);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL,
						 FCNAME, __LINE__, MPI_ERR_OTHER,
						 "**ch3|loadsendiov", 0);
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */
	}
	mpi_errno = MPIDI_CH3_iStartRndvMsg (vc, sreq, &upkt);
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
	    MPIU_Object_set_ref(sreq, 0);
	    MPIDI_CH3_Request_destroy(sreq);
	    sreq = NULL;
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL,
					     FCNAME, __LINE__, MPI_ERR_OTHER,
					     "**ch3|rtspkt", 0);
	    goto fn_exit;
	}
	/* --END ERROR HANDLING-- */
	
#else
        mpi_errno = MPID_nem_lmt_pre_send (vc, sreq, &cookie);
        /* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
	    MPIU_Object_set_ref(sreq, 0);
	    MPIDI_CH3_Request_destroy(sreq);
	    sreq = NULL;
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|rtspkt", 0);
	    goto fn_exit;
	}
	/* --END ERROR HANDLING-- */
        rts_pkt->cookie_len = cookie.MPID_IOV_LEN;
        
        iov[0].MPID_IOV_BUF = rts_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*rts_pkt);
        iov[1] = cookie;
        
	mpi_errno = MPIDI_CH3_iStartMsgv(vc, iov, 2, &rts_sreq);
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
	    MPIU_Object_set_ref(sreq, 0);
	    MPIDI_CH3_Request_destroy(sreq);
	    sreq = NULL;
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|rtspkt", 0);
	    goto fn_exit;
	}
	/* --END ERROR HANDLING-- */
	if (rts_sreq != NULL)
	{
	    if (rts_sreq->status.MPI_ERROR != MPI_SUCCESS)
	    {
		MPIU_Object_set_ref(sreq, 0);
		MPIDI_CH3_Request_destroy(sreq);
		sreq = NULL;
		mpi_errno = MPIR_Err_create_code(rts_sreq->status.MPI_ERROR, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|rtspkt", 0);
		MPID_Request_release(rts_sreq);
		goto fn_exit;
	    }
	    MPID_Request_release(rts_sreq);
	}
#endif
	
	/* FIXME: fill temporary IOV or pack temporary buffer after send to hide some latency.  This requires synchronization
           because the CTS packet could arrive and be processed before the above iStartmsg completes (depending on the progress
           engine, threads, etc.). */
	
	if (dt_ptr != NULL)
	{
	    sreq->dev.datatype_ptr = dt_ptr;
	    MPID_Datatype_add_ref(dt_ptr);
	}
    }

  fn_exit:
    *request = sreq;
    
    MPIU_DBG_STMT(CH3_OTHER,VERBOSE,
    {
	if (sreq != NULL) {
	    MPIU_DBG_MSG_P(CH3_OTHER,VERBOSE,
			   "request allocated, handle=0x%08x", sreq->handle);
	}
    }
		  )
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_ISSEND);
    return mpi_errno;
}
