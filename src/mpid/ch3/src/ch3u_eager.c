/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*
 * Send an eager message.  To optimize for the important, short contiguous
 * message case, there are separate routines for the contig and non-contig
 * datatype cases.
 */

#undef FUNCNAME
#define FUNCNAME MPIDI_EagerNoncontigSend
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
/* MPIDI_CH3_EagerNoncontigSend - Eagerly send noncontiguous data */
int MPIDI_CH3_EagerNoncontigSend( MPID_Request **sreq_p, 
				  int reqtype, 
				  const void * buf, int count, 
				  MPI_Datatype datatype, int data_sz, 
				  int rank, 
				  int tag, MPID_Comm * comm, 
				  int context_offset )
{
    int mpi_errno = MPI_SUCCESS;
    int iov_n;
    MPIDI_VC_t * vc;
    MPID_Request *sreq = *sreq_p;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_eager_send_t * const eager_pkt = &upkt.eager_send;
    MPID_IOV iov[MPID_IOV_LIMIT];
    
    MPIDI_Pkt_init(eager_pkt, reqtype);
    eager_pkt->match.rank	= comm->rank;
    eager_pkt->match.tag	= tag;
    eager_pkt->match.context_id	= comm->context_id + context_offset;
    eager_pkt->sender_req_id	= MPI_REQUEST_NULL;
    eager_pkt->data_sz		= data_sz;
    
    iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)eager_pkt;
    iov[0].MPID_IOV_LEN = sizeof(*eager_pkt);
    
    MPIU_DBG_MSG_FMT(CH3_OTHER,VERBOSE,(MPIU_DBG_FDEST,
             "sending non-contiguous eager message, data_sz=" MPIDI_MSG_SZ_FMT,
					data_sz));
	    
    MPID_Segment_init(buf, count, datatype, &sreq->dev.segment, 0);
    sreq->dev.segment_first = 0;
    sreq->dev.segment_size = data_sz;
	    
    iov_n = MPID_IOV_LIMIT - 1;
    mpi_errno = MPIDI_CH3U_Request_load_send_iov(sreq, &iov[1], &iov_n);
    if (mpi_errno == MPI_SUCCESS)
    {
	iov_n += 1;
	
	MPIDI_Comm_get_vc(comm, rank, &vc);
	MPIDI_VC_FAI_send_seqnum(vc, seqnum);
	MPIDI_Pkt_set_seqnum(eager_pkt, seqnum);
	MPIDI_Request_set_seqnum(sreq, seqnum);
	
	mpi_errno = MPIDI_CH3_iSendv(vc, sreq, iov, iov_n);
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
	    MPIU_Object_set_ref(sreq, 0);
	    MPIDI_CH3_Request_destroy(sreq);
	    *sreq_p = NULL;
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, 
		         FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|eagermsg", 0);
	    goto fn_exit;
	}
	/* --END ERROR HANDLING-- */

#if 0
	if (sreq->dev.ca != MPIDI_CH3_CA_COMPLETE)
	{
	    /* sreq->dev.datatype_ptr = dt_ptr;
	       MPID_Datatype_add_ref(dt_ptr); -- not necessary for blocking functions */
	}
#endif
    }
    else
    {
	/* --BEGIN ERROR HANDLING-- */
	MPIU_Object_set_ref(sreq, 0);
	MPIDI_CH3_Request_destroy(sreq);
	*sreq_p = NULL;
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, 
		    FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|loadsendiov", 0);
	goto fn_exit;
	/* --END ERROR HANDLING-- */
    }

 fn_exit:
 fn_fail:
    return mpi_errno;
}

/* Send a contiguous eager message.  We'll want to optimize (and possibly
   inline) this 

   Make sure that buf is at the beginning of the data to send; 
   adjust by adding dt_true_lb if necessary 
*/
#undef FUNCNAME
#define FUNCNAME MPIDI_EagerContigSend
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_EagerContigsend( MPID_Request **sreq_p, 
			       int reqtype, 
			       const void * buf, int data_sz, int rank, 
			       int tag, MPID_Comm * comm, int context_offset )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t * vc;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_eager_send_t * const eager_pkt = &upkt.eager_send;
    MPID_Request *sreq = *sreq_p;
    MPID_IOV iov[MPID_IOV_LIMIT];
    
    MPIDI_Pkt_init(eager_pkt, reqtype);
    eager_pkt->match.rank	= comm->rank;
    eager_pkt->match.tag	= tag;
    eager_pkt->match.context_id	= comm->context_id + context_offset;
    eager_pkt->sender_req_id	= MPI_REQUEST_NULL;
    eager_pkt->data_sz		= data_sz;
    
    iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)eager_pkt;
    iov[0].MPID_IOV_LEN = sizeof(*eager_pkt);
    
    MPIU_DBG_MSG_FMT(CH3_OTHER,VERBOSE,(MPIU_DBG_FDEST,
	       "sending contiguous eager message, data_sz=" MPIDI_MSG_SZ_FMT,
					data_sz));
	    
    iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) buf;
    iov[1].MPID_IOV_LEN = data_sz;
    
    MPIDI_Comm_get_vc(comm, rank, &vc);
    MPIDI_VC_FAI_send_seqnum(vc, seqnum);
    MPIDI_Pkt_set_seqnum(eager_pkt, seqnum);
    
    mpi_errno = MPIDI_CH3_iStartMsgv(vc, iov, 2, &sreq);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|eagermsg", 0);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */
    if (sreq != NULL)
    {
	MPIDI_Request_set_seqnum(sreq, seqnum);
	MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_SEND);
	/* sreq->comm = comm;
	   MPIR_Comm_add_ref(comm); -- not necessary for blocking functions */
	
    }
    
 fn_fail:
 fn_exit:
    return mpi_errno;
}

/* Send a contiguous eager message that can be cancelled (e.g., 
   a nonblocking eager send).  We'll want to optimize (and possibly
   inline) this 

   Make sure that buf is at the beginning of the data to send; 
   adjust by adding dt_true_lb if necessary 
*/
#undef FUNCNAME
#define FUNCNAME MPIDI_EagerContigIsend
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_EagerContigIsend( MPID_Request **sreq_p, 
				int reqtype, 
				const void * buf, int data_sz, int rank, 
				int tag, MPID_Comm * comm, int context_offset )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t * vc;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_eager_send_t * const eager_pkt = &upkt.eager_send;
    MPID_Request *sreq = *sreq_p;
    MPID_IOV iov[MPID_IOV_LIMIT];

    MPIU_DBG_MSG_FMT(CH3_OTHER,VERBOSE,(MPIU_DBG_FDEST,
	       "sending contiguous eager message, data_sz=" MPIDI_MSG_SZ_FMT,
					data_sz));
	    
    sreq->dev.ca = MPIDI_CH3_CA_COMPLETE;
    
    MPIDI_Pkt_init(eager_pkt, reqtype);
    eager_pkt->match.rank	= comm->rank;
    eager_pkt->match.tag	= tag;
    eager_pkt->match.context_id	= comm->context_id + context_offset;
    eager_pkt->sender_req_id	= sreq->handle;
    eager_pkt->data_sz		= data_sz;
    
    iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)eager_pkt;
    iov[0].MPID_IOV_LEN = sizeof(*eager_pkt);
    
    iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) buf;
    iov[1].MPID_IOV_LEN = data_sz;
    
    MPIDI_Comm_get_vc(comm, rank, &vc);
    MPIDI_VC_FAI_send_seqnum(vc, seqnum);
    MPIDI_Pkt_set_seqnum(eager_pkt, seqnum);
    MPIDI_Request_set_seqnum(sreq, seqnum);
    
    mpi_errno = MPIDI_CH3_iSendv(vc, sreq, iov, 2 );
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
    
 fn_fail:
 fn_exit:
    return mpi_errno;
}
