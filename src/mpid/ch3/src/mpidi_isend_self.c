/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_Isend_self
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Isend_self(const void * buf, int count, MPI_Datatype datatype, int rank, int tag, MPID_Comm * comm, int context_offset,
		     int type, MPID_Request ** request)
{
    MPIDI_Message_match match;
    MPID_Request * sreq;
    MPID_Request * rreq;
    MPIDI_VC * vc;
#if defined(MPID_USE_SEQUENCE_NUMBERS)
    MPID_Seqnum_t seqnum;
#endif    
    int found;
    int mpi_errno = MPI_SUCCESS;
	
    MPIDI_DBG_PRINTF((15, FCNAME, "sending message to self"));
	
    MPIDI_CH3M_create_sreq(sreq, mpi_errno, goto fn_exit);
    MPIDI_Request_set_type(sreq, type);
    MPIDI_Request_set_msg_type(sreq, MPIDI_REQUEST_SELF_MSG);
    
    match.rank = rank;
    match.tag = tag;
    match.context_id = comm->context_id + context_offset;
    rreq = MPIDI_CH3U_Request_FDP_or_AEU(&match, &found);
    if (rreq == NULL)
    {
	MPIU_Object_set_ref(sreq, 0);
	MPIDI_CH3_Request_destroy(sreq);
	sreq = NULL;
	mpi_errno = MPIR_ERR_MEMALLOCFAILED;
	goto fn_exit;
    }

    vc = comm->vcr[rank];
    MPIDI_CH3U_VC_FAI_send_seqnum(vc, seqnum);
    MPIDI_CH3U_Request_set_seqnum(sreq, seqnum);
    MPIDI_CH3U_Request_set_seqnum(rreq, seqnum);
    
    rreq->status.MPI_SOURCE = rank;
    rreq->status.MPI_TAG = tag;
    
    if (found)
    {
	int data_sz;
	
	MPIDI_DBG_PRINTF((15, FCNAME, "found posted receive request; copying data"));
	    
	MPIDI_CH3U_Buffer_copy(buf, count, datatype, &sreq->status.MPI_ERROR,
			       rreq->ch3.user_buf, rreq->ch3.user_count, rreq->ch3.datatype, &data_sz, &rreq->status.MPI_ERROR);
	rreq->status.count = data_sz;
	MPID_Request_set_completed(rreq);
	MPID_Request_release(rreq);
	/* sreq has never been seen by the user or outside this thread, so it is safe to reset ref_count and cc */
	MPIU_Object_set_ref(sreq, 1);
	sreq->cc = 0;
    }
    else
    {
	if (type != MPIDI_REQUEST_TYPE_RSEND)
	{
	    MPIDI_DBG_PRINTF((15, FCNAME, "added receive request to unexpected queue; attaching send request"));
	    if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
	    {
		MPID_Datatype_get_ptr(datatype, sreq->ch3.datatype_ptr);
		MPID_Datatype_add_ref(sreq->ch3.datatype_ptr);
	    }
	    rreq->partner_request = sreq;
	    rreq->ch3.sender_req_id = sreq->handle;
	}
	else
	{
	    MPIDI_DBG_PRINTF((15, FCNAME, "ready send unable to find matching recv req"));
	    sreq->status.MPI_ERROR = MPI_ERR_UNKNOWN; /* FIXME */
	    rreq->status.MPI_ERROR = MPI_ERR_UNKNOWN; /* FIXME */
	    
	    rreq->partner_request = NULL;
	    rreq->ch3.sender_req_id = MPI_REQUEST_NULL;
	    rreq->status.count = 0;
	    
	    /* sreq has never been seen by the user or outside this thread, so it is safe to reset ref_count and cc */
	    MPIU_Object_set_ref(sreq, 1);
	    sreq->cc = 0;
	}
	    
	MPIDI_Request_set_msg_type(rreq, MPIDI_REQUEST_SELF_MSG);
    }

  fn_exit:
    *request = sreq;
    return mpi_errno;
}
