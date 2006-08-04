/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*
 * Send a synchronous eager message.  This is an optimization that you
 * may want to use for programs that make extensive use of MPI_Ssend and
 * MPI_Issend for short messages.
 */

#undef FUNCNAME
#define FUNCNAME MPIDI_EagerSyncNoncontigSend
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
/* MPIDI_CH3_EagerSyncNoncontigSend - Eagerly send noncontiguous data in
   synchronous mode.

   Some implementations may choose to use Rendezvous sends (see ch3u_rndv.c)
   for all Synchronous sends (MPI_Issend and MPI_Ssend).  An eager 
   synchronous send eliminates one of the handshake messages, but 
   most application codes should not be using synchronous sends in
   performance-critical operations.
*/
int MPIDI_CH3_EagerSyncNoncontigSend( MPID_Request **sreq_p, 
				      const void * buf, int count, 
				      MPI_Datatype datatype, int data_sz, 
				      int dt_contig, MPI_Aint dt_true_lb,
				      int rank, 
				      int tag, MPID_Comm * comm, 
				      int context_offset )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_eager_sync_send_t * const es_pkt = &upkt.eager_sync_send;
    MPID_IOV iov[MPID_IOV_LIMIT];
    MPIDI_VC_t * vc;
    MPID_Request *sreq = *sreq_p;
    
    sreq->cc = 2;
    sreq->dev.ca = MPIDI_CH3_CA_COMPLETE;
    
    MPIDI_Pkt_init(es_pkt, MPIDI_CH3_PKT_EAGER_SYNC_SEND);
    es_pkt->match.rank = comm->rank;
    es_pkt->match.tag = tag;
    es_pkt->match.context_id = comm->context_id + context_offset;
    es_pkt->sender_req_id = sreq->handle;
    es_pkt->data_sz = data_sz;

    MPIDI_Comm_get_vc(comm, rank, &vc);
    
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
	    *sreq_p = NULL;
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|eagermsg", 0);
	    goto fn_exit;
	}
	/* --END ERROR HANDLING-- */
    }
    else
    {
	int iov_n;
	
	MPIU_DBG_MSG_D(CH3_OTHER,VERBOSE,
		       "sending non-contiguous sync eager message, data_sz=" MPIDI_MSG_SZ_FMT, 
		       data_sz);
	
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
		*sreq_p = NULL;
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|eagermsg", 0);
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */
	}
	else
	{
	    /* --BEGIN ERROR HANDLING-- */
	    MPIU_Object_set_ref(sreq, 0);
	    MPIDI_CH3_Request_destroy(sreq);
	    *sreq_p = NULL;
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|loadsendiov", 0);
	    goto fn_exit;
	    /* --END ERROR HANDLING-- */
	}
    }

 fn_exit:
    return mpi_errno;
}

/* Send a zero-sized message with eager synchronous.  This is a temporary
   routine, as we may want to replace this with a counterpart to the
   Eager Short message */
int MPIDI_CH3_EagerSyncZero(MPID_Request **sreq_p, int rank, int tag, 
			    MPID_Comm * comm, int context_offset )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_eager_sync_send_t * const es_pkt = &upkt.eager_sync_send;
    MPIDI_VC_t * vc;
    MPID_Request *sreq = *sreq_p;
    
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
    
    MPIDI_Comm_get_vc(comm, rank, &vc);
    MPIDI_VC_FAI_send_seqnum(vc, seqnum);
    MPIDI_Pkt_set_seqnum(es_pkt, seqnum);
    MPIDI_Request_set_seqnum(sreq, seqnum);
    
    mpi_errno = MPIDI_CH3_iSend(vc, sreq, es_pkt, sizeof(*es_pkt));
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
	MPIU_Object_set_ref(sreq, 0);
	MPIDI_CH3_Request_destroy(sreq);
	*sreq_p = NULL;
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|eagermsg", 0);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

 fn_exit:
    return mpi_errno;
}

/* 
 * These routines are called when a receive matches an eager sync send 
 */
int MPIDI_CH3_EagerSyncAck( MPIDI_VC_t *vc, MPID_Request *rreq )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_eager_sync_ack_t * const esa_pkt = &upkt.eager_sync_ack;
    MPID_Request * esa_req;
    
    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"sending eager sync ack");
    MPIDI_Pkt_init(esa_pkt, MPIDI_CH3_PKT_EAGER_SYNC_ACK);
    esa_pkt->sender_req_id = rreq->dev.sender_req_id;
    mpi_errno = MPIDI_CH3_iStartMsg(vc, esa_pkt, sizeof(*esa_pkt), &esa_req);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }
    if (esa_req != NULL)
    {
	MPID_Request_release(esa_req);
    }
 fn_fail:
    return mpi_errno;
}
