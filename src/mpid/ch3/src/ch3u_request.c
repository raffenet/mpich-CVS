/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*
 * MPIDI_CH3U_Request_FU()
 *
 * Find a request in the unexpected queue; or return NULL.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Request_FU
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
MPID_Request * MPIDI_CH3U_Request_FU(int source, int tag, int context_id)
{
    MPID_Request * rreq;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_REQUEST_FU);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_REQUEST_FU);

    if (tag != MPI_ANY_TAG && source != MPI_ANY_SOURCE)
    {
	rreq = MPIDI_Process.recv_unexpected_head;
	while(rreq != NULL)
	{
	    if (rreq->ch3.match.context_id == context_id &&
		rreq->ch3.match.rank == source &&
		rreq->ch3.match.tag == tag)
	    {
		MPIDI_CH3_Request_add_ref(rreq);
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_REQUEST_FU);
		return rreq;
	    }
	    
	    rreq = rreq->ch3.next;
	}
    }
    else
    {
	MPIDI_Message_match match;
	MPIDI_Message_match mask;

	match.context_id = context_id;
	mask.context_id = ~0;
	if (tag == MPI_ANY_TAG)
	{
	    match.tag = 0;
	    mask.tag = 0;
	}
	else
	{
	    match.tag = tag;
	    mask.tag = ~0;
	}
	if (source == MPI_ANY_SOURCE)
	{
	    match.rank = 0;
	    mask.rank = 0;
	}
	else
	{
	    match.rank = source;
	    mask.rank = ~0;
	}
	
	rreq = MPIDI_Process.recv_unexpected_head;
	while (rreq != NULL)
	{
	    if (rreq->ch3.match.context_id == match.context_id &&
		(rreq->ch3.match.rank & mask.rank) == match.rank &&
		(rreq->ch3.match.tag & mask.tag) == match.tag)
	    {
		MPIDI_CH3_Request_add_ref(rreq);
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_REQUEST_FU);
		return rreq;
	    }
	    
	    rreq = rreq->ch3.next;
	}
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_REQUEST_FU);
    return NULL;
}

/*
 * MPIDI_CH3U_Request_FDU()
 *
 * Find a request in the unexpected queue and dequeue it; otherwise return
 * NULL.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Request_FDU
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
MPID_Request * MPIDI_CH3U_Request_FDU(MPI_Request sreq_id,
				      MPIDI_Message_match * match)
{
    MPID_Request * prev_rreq;
    MPID_Request * cur_rreq;
    MPID_Request * matching_prev_rreq;
    MPID_Request * matching_cur_rreq;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_REQUEST_FDU);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_REQUEST_FDU);
    matching_prev_rreq = NULL;
    matching_cur_rreq = NULL;
    prev_rreq = NULL;
    cur_rreq = MPIDI_Process.recv_unexpected_head;
    while(cur_rreq != NULL)
    {
	if (cur_rreq->ch3.sender_req_id == sreq_id &&
	    cur_rreq->ch3.match.context_id == match->context_id &&
	    cur_rreq->ch3.match.rank == match->rank &&
	    cur_rreq->ch3.match.tag == match->tag)
	{
	    matching_prev_rreq = prev_rreq;
	    matching_cur_rreq = cur_rreq;
	}
	    
	prev_rreq = cur_rreq;
	cur_rreq = cur_rreq->ch3.next;
    }

    if (matching_cur_rreq != NULL)
    {
	if (matching_prev_rreq != NULL)
	{
	    matching_prev_rreq->ch3.next = matching_cur_rreq->ch3.next;
	}
	else
	{
	    MPIDI_Process.recv_unexpected_head = matching_cur_rreq->ch3.next;
	}
	
	if (matching_cur_rreq->ch3.next == NULL)
	{
	    MPIDI_Process.recv_unexpected_tail = matching_prev_rreq;
	}

	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_REQUEST_FDU);
	return matching_cur_rreq;
    }
    else
    {
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_REQUEST_FDU);
	return NULL;
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_REQUEST_FDU);
}


/*
 * MPIDI_CH3U_Request_FDU_or_AEP()
 *
 * Atomically find a request in the unexpected queue and dequeue it, or
 * allocate a new request and enqueue it in the posted queue
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Request_FDU_or_AEP
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
MPID_Request * MPIDI_CH3U_Request_FDU_or_AEP(
    int source, int tag, int context_id, int * found)
{
    MPID_Request * rreq;
    MPID_Request * prev_rreq;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_REQUEST_FDU_OR_AEP);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_REQUEST_FDU_OR_AEP);
    if (tag != MPI_ANY_TAG && source != MPI_ANY_SOURCE)
    {
	prev_rreq = NULL;
	rreq = MPIDI_Process.recv_unexpected_head;
	while(rreq != NULL)
	{
	    if (rreq->ch3.match.context_id == context_id &&
		rreq->ch3.match.rank == source &&
		rreq->ch3.match.tag == tag)
	    {
		if (prev_rreq != NULL)
		{
		    prev_rreq->ch3.next = rreq->ch3.next;
		}
		else
		{
		    MPIDI_Process.recv_unexpected_head = rreq->ch3.next;
		}
		if (rreq->ch3.next == NULL)
		{
		    MPIDI_Process.recv_unexpected_tail = prev_rreq;
		}
		*found = TRUE;
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_REQUEST_FDU_OR_AEP);
		return rreq;
	    }
	    
	    prev_rreq = rreq;
	    rreq = rreq->ch3.next;
	}
    }
    else
    {
	MPIDI_Message_match match;
	MPIDI_Message_match mask;

	match.context_id = context_id;
	mask.context_id = ~0;
	if (tag == MPI_ANY_TAG)
	{
	    match.tag = 0;
	    mask.tag = 0;
	}
	else
	{
	    match.tag = tag;
	    mask.tag = ~0;
	}
	if (source == MPI_ANY_SOURCE)
	{
	    match.rank = 0;
	    mask.rank = 0;
	}
	else
	{
	    match.rank = source;
	    mask.rank = ~0;
	}
	
	prev_rreq = NULL;
	rreq = MPIDI_Process.recv_unexpected_head;
	while (rreq != NULL)
	{
	    if (rreq->ch3.match.context_id == match.context_id &&
		(rreq->ch3.match.rank & mask.rank) == match.rank &&
		(rreq->ch3.match.tag & mask.tag) == match.tag)
	    {
		if (prev_rreq != NULL)
		{
		    prev_rreq->ch3.next = rreq->ch3.next;
		}
		else
		{
		    MPIDI_Process.recv_unexpected_head = rreq->ch3.next;
		}
		if (rreq->ch3.next == NULL)
		{
		    MPIDI_Process.recv_unexpected_tail = prev_rreq;
		}
		*found = TRUE;
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_REQUEST_FDU_OR_AEP);
		return rreq;
	    }
	    
	    prev_rreq = rreq;
	    rreq = rreq->ch3.next;
	}
    }

    /* A matching request was not found in the unexpected queue, so we need to
       allocate a new request and add it to the posted queue */
    rreq = MPIDI_CH3_Request_create();
    if (rreq != NULL)
    {
	rreq->ref_count = 2;
	rreq->kind = MPID_REQUEST_RECV;
	rreq->ch3.match.tag = tag;
	rreq->ch3.match.rank = source;
	rreq->ch3.match.context_id = context_id;
	rreq->ch3.next = NULL;
	
	if (MPIDI_Process.recv_posted_tail != NULL)
	{
	    MPIDI_Process.recv_posted_tail->ch3.next = rreq;
	}
	else
	{
	    MPIDI_Process.recv_posted_head = rreq;
	}
	MPIDI_Process.recv_posted_tail = rreq;
    }
    
    *found = FALSE;
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_REQUEST_FDU_OR_AEP);
    return rreq;
}


/*
 * MPIDI_CH3U_Request_DP()
 *
 * Given an existing request, dequeue that request from the posted queue, or
 * return NULL if the request was not in the posted queued
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Request_DP
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Request_DP(MPID_Request * rreq)
{
    MPID_Request * cur_rreq;
    MPID_Request * prev_rreq;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_REQUEST_DP);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_REQUEST_DP);
    prev_rreq = NULL;
    cur_rreq = MPIDI_Process.recv_posted_head;
    while (rreq != NULL)
    {
	if (cur_rreq == rreq)
	{
	    if (prev_rreq != NULL)
	    {
		prev_rreq->ch3.next = cur_rreq->ch3.next;
	    }
	    else
	    {
		MPIDI_Process.recv_posted_head = cur_rreq->ch3.next;
	    }
	    if (cur_rreq->ch3.next == NULL)
	    {
		MPIDI_Process.recv_posted_tail = prev_rreq;
	    }
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_REQUEST_DP);
	    return TRUE;
	}
	    
	prev_rreq = rreq;
	rreq = rreq->ch3.next;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_REQUEST_DP);
    return FALSE;
}

/*
 * MPIDI_CH3U_Request_FDP
 *
 * Locate a request in the posted queue and dequeue it, or return NULL.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Request_FDP
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
MPID_Request * MPIDI_CH3U_Request_FDP(
    MPIDI_Message_match * match)
{
    MPID_Request * rreq;
    MPID_Request * prev_rreq;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_REQUEST_FDP_OR_AEU);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_REQUEST_FDP_OR_AEU);
    prev_rreq = NULL;
    rreq = MPIDI_Process.recv_posted_head;
    while (rreq != NULL)
    {
	if ((rreq->ch3.match.context_id == match->context_id) &&
	    (rreq->ch3.match.rank == match->rank ||
	     rreq->ch3.match.rank == MPI_ANY_SOURCE) &&
	    (rreq->ch3.match.tag == match->tag ||
	     rreq->ch3.match.tag == MPI_ANY_TAG))
	{
	    if (prev_rreq != NULL)
	    {
		prev_rreq->ch3.next = rreq->ch3.next;
	    }
	    else
	    {
		MPIDI_Process.recv_posted_head = rreq->ch3.next;
	    }
	    if (rreq->ch3.next == NULL)
	    {
		MPIDI_Process.recv_posted_tail = prev_rreq;
	    }
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_REQUEST_FDP_OR_AEU);
	    return rreq;
	}
	    
	prev_rreq = rreq;
	rreq = rreq->ch3.next;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_REQUEST_FDP_OR_AEU);
    return NULL;
}


/*
 * MPIDI_CH3U_Request_FDP_or_AEU()
 *
 * Locate a request in the posted queue and dequeue it, or allocate a new
 * request and enqueue it in the unexpected queue
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Request_FDP_or_AEU
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
MPID_Request * MPIDI_CH3U_Request_FDP_or_AEU(
    MPIDI_Message_match * match, int * found)
{
    MPID_Request * rreq;
    MPID_Request * prev_rreq;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_REQUEST_FDP_OR_AEU);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_REQUEST_FDP_OR_AEU);
    prev_rreq = NULL;
    rreq = MPIDI_Process.recv_posted_head;
    while (rreq != NULL)
    {
	if ((rreq->ch3.match.context_id == match->context_id) &&
	    (rreq->ch3.match.rank == match->rank ||
	     rreq->ch3.match.rank == MPI_ANY_SOURCE) &&
	    (rreq->ch3.match.tag == match->tag ||
	     rreq->ch3.match.tag == MPI_ANY_TAG))
	{
	    if (prev_rreq != NULL)
	    {
		prev_rreq->ch3.next = rreq->ch3.next;
	    }
	    else
	    {
		MPIDI_Process.recv_posted_head = rreq->ch3.next;
	    }
	    if (rreq->ch3.next == NULL)
	    {
		MPIDI_Process.recv_posted_tail = prev_rreq;
	    }
	    *found = TRUE;
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_REQUEST_FDP_OR_AEU);
	    return rreq;
	}
	    
	prev_rreq = rreq;
	rreq = rreq->ch3.next;
    }

    /* A matching request was not found in the posted queue, so we need to
       allocate a new request and add it to the unexpected queue */
    rreq = MPIDI_CH3_Request_create();
    if (rreq != NULL)
    {
	rreq->ref_count = 2;
	rreq->kind = MPID_REQUEST_RECV;
	rreq->ch3.match = *match;
	rreq->ch3.next = NULL;
	
	if (MPIDI_Process.recv_unexpected_tail != NULL)
	{
	    MPIDI_Process.recv_unexpected_tail->ch3.next = rreq;
	}
	else
	{
	    MPIDI_Process.recv_unexpected_head = rreq;
	}
	MPIDI_Process.recv_unexpected_tail = rreq;
    }
    
    *found = FALSE;
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_REQUEST_FDP_OR_AEU);
    return rreq;
}


/*
 * MPIDI_CH3U_Request_load_send_iov()
 *
 * Fill the provided IOV with the next (or remaining) portion of data described
 * by the segment contained in the request structure.  If the density of IOV is
 * not sufficient, pack the data into a send/receive buffer and point the IOV
 * at the buffer.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Request_load_send_iov
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Request_load_send_iov(
    MPID_Request * const sreq, MPID_IOV * const iov, int * const iov_n)
{
    MPIDI_msg_sz_t last;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_REQUEST_LOAD_SEND_IOV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_REQUEST_LOAD_SEND_IOV);
    last = sreq->ch3.segment_size;
    MPIDI_DBG_PRINTF((40, FCNAME, "pre-pv: first=" MPIDI_MSG_SZ_FMT
		      ", last=" MPIDI_MSG_SZ_FMT ", iov_n=%d",
		      sreq->ch3.segment_first, last, *iov_n));
    assert(sreq->ch3.segment_first < last);
    assert(last > 0);
    assert(*iov_n <= MPID_IOV_LIMIT);
    MPID_Segment_pack_vector(&sreq->ch3.segment, sreq->ch3.segment_first,
			     &last, iov, iov_n);
    assert(*iov_n > 0);
    assert(*iov_n <= MPID_IOV_LIMIT);
    MPIDI_DBG_PRINTF((40, FCNAME, "post-pv: first=" MPIDI_MSG_SZ_FMT
		      ", last=" MPIDI_MSG_SZ_FMT ", iov_n=%d",
		      sreq->ch3.segment_first, last, *iov_n));
    
    if (last == sreq->ch3.segment_size)
    {
	MPIDI_DBG_PRINTF((40, FCNAME, "remaining data loaded into IOV"));
	sreq->ch3.ca = MPIDI_CH3_CA_COMPLETE;
    }
    else if ((last - sreq->ch3.segment_first) / *iov_n
	     >= MPIDI_IOV_DENSITY_MIN)
    {
	MPIDI_DBG_PRINTF((40, FCNAME, "more data loaded into IOV"));
	sreq->ch3.segment_first = last;
	sreq->ch3.ca = MPIDI_CH3_CA_RELOAD_IOV;
    }
    else
    {
	MPIDI_msg_sz_t data_sz;
	
	MPIDI_DBG_PRINTF((40, FCNAME, "low density.  using SRBuf."));
	    
	data_sz = sreq->ch3.segment_size - sreq->ch3.segment_first;
	if (!MPIDI_Request_get_srbuf_flag(sreq))
	{
	    MPIDI_CH3U_SRBuf_alloc(sreq, data_sz);
	    if (sreq->ch3.tmpbuf_sz == 0)
	    {
		MPIDI_DBG_PRINTF((40, FCNAME, "SRBuf allocation failure"));
		mpi_errno = MPI_ERR_NOMEM;
		sreq->status.MPI_ERROR = mpi_errno;
		goto fn_exit;
	    }
	}
		    
	last = (data_sz <= sreq->ch3.tmpbuf_sz) ? sreq->ch3.segment_size :
	    sreq->ch3.segment_first + sreq->ch3.tmpbuf_sz;
	MPIDI_DBG_PRINTF((40, FCNAME, "pre-pack: first=" MPIDI_MSG_SZ_FMT
			   ", last=" MPIDI_MSG_SZ_FMT,
			   sreq->ch3.segment_first, last));
	MPID_Segment_pack(&sreq->ch3.segment, sreq->ch3.segment_first,
			  &last, sreq->ch3.tmpbuf);
	MPIDI_DBG_PRINTF((40, FCNAME, "post-pack: first=" MPIDI_MSG_SZ_FMT
			   ", last=" MPIDI_MSG_SZ_FMT,
			   sreq->ch3.segment_first, last));
	iov[0].MPID_IOV_BUF = sreq->ch3.tmpbuf;
	iov[0].MPID_IOV_LEN = last - sreq->ch3.segment_first;
	*iov_n = 1;
	if (last == sreq->ch3.segment_size)
	{
	    MPIDI_DBG_PRINTF((40, FCNAME, "remaining data packed into SRBuf"));
	    sreq->ch3.ca = MPIDI_CH3_CA_COMPLETE;
	}
	else 
	{
	    MPIDI_DBG_PRINTF((40, FCNAME, "more data packed into SRBuf"));
	    sreq->ch3.segment_first = last;
	    sreq->ch3.ca = MPIDI_CH3_CA_RELOAD_IOV;
	}
    }
    
  fn_exit:

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_REQUEST_LOAD_SEND_IOV);
    return mpi_errno;
}

/*
 * MPIDI_CH3U_Request_load_recv_iov()
 *
 * Fill the request's IOV with the next (or remaining) portion of data
 * described by the segment (also contained in the request structure).  If the
 * density of IOV is not sufficient, allocate a send/receive buffer and point
 * the IOV at the buffer.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Request_load_recv_iov
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Request_load_recv_iov(
    MPID_Request * const rreq)
{
    MPIDI_msg_sz_t last;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_REQUEST_LOAD_RECV_IOV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_REQUEST_LOAD_RECV_IOV);
    if (rreq->ch3.segment_first < rreq->ch3.segment_size)
    {
	/* still reading data that needs to go into the user buffer */
	
	if (MPIDI_Request_get_srbuf_flag(rreq))
	{
	    MPIDI_msg_sz_t data_sz;
	    MPIDI_msg_sz_t tmpbuf_sz;

	    /* Once a SRBuf is in use, we continue to use it since a small
	       amount of data may already be present at the beginning of the
	       buffer.  This data is left over from the previous unpack, most
	       like a result of alignment issues.  NOTE: we could force the use
	       of the SRBuf only when (rreq->ch3.tmpbuf_off > 0)... */
	    
	    data_sz = rreq->ch3.segment_size - rreq->ch3.segment_first;
	    tmpbuf_sz = rreq->ch3.tmpbuf_sz - rreq->ch3.tmpbuf_off;
	    if (tmpbuf_sz < data_sz)
	    {
		data_sz = tmpbuf_sz;
	    }
	    rreq->ch3.iov[0].MPID_IOV_BUF = (char *) rreq->ch3.tmpbuf +
		rreq->ch3.tmpbuf_off;
	    rreq->ch3.iov[0].MPID_IOV_LEN = data_sz;
	    rreq->ch3.iov_count = 1;
	    if (rreq->ch3.segment_first + data_sz + rreq->ch3.tmpbuf_off ==
		rreq->ch3.recv_data_sz)
	    {
		MPIDI_DBG_PRINTF((35, FCNAME, "updating rreq to read the "
				  "remaining data into the SRBuf"));
		rreq->ch3.ca = MPIDI_CH3_CA_UNPACK_SRBUF_AND_COMPLETE;
	    }
	    else
	    {
		MPIDI_DBG_PRINTF((35, FCNAME, "updating rreq to read more"
			      "data into the SRBuf"));
		rreq->ch3.ca = MPIDI_CH3_CA_UNPACK_SRBUF_AND_RELOAD_IOV;
	    }
	    goto fn_exit;
	}
	
	last = rreq->ch3.segment_size;
	MPIDI_DBG_PRINTF((40, FCNAME, "pre-upv: first=" MPIDI_MSG_SZ_FMT
			  ", last=" MPIDI_MSG_SZ_FMT ", iov_n=%d",
			  rreq->ch3.segment_first, last, rreq->ch3.iov_count));
	assert(rreq->ch3.segment_first < last);
	assert(last > 0);
	rreq->ch3.iov_count = MPID_IOV_LIMIT;
	MPID_Segment_unpack_vector(&rreq->ch3.segment, rreq->ch3.segment_first,
				   &last, rreq->ch3.iov, &rreq->ch3.iov_count);
	MPIDI_DBG_PRINTF((40, FCNAME, "post-upv: first=" MPIDI_MSG_SZ_FMT
			  ", last=" MPIDI_MSG_SZ_FMT ", iov_n=%d",
			  rreq->ch3.segment_first, last, rreq->ch3.iov_count));
	assert(rreq->ch3.iov_count <= MPID_IOV_LIMIT);
	
	if (rreq->ch3.iov_count == 0)
	{
	    /* If the data can't be unpacked, the we have a mis-match between
	       the datatype and the amount of data received.  Adjust the
	       segment info so that the remaining data is received and thrown
	       away. */
	    rreq->status.MPI_ERROR = MPI_ERR_UNKNOWN;
	    rreq->ch3.segment_size = rreq->ch3.segment_first;
	    mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
	    goto fn_exit;
	}

	if (last == rreq->ch3.recv_data_sz)
	{
	    MPIDI_DBG_PRINTF((35, FCNAME, "updating rreq to read the remaining"
			      " data directly into the user buffer"));
	    rreq->ch3.ca = MPIDI_CH3_CA_COMPLETE;
	}
	else if (last == rreq->ch3.segment_size
		 || (last - rreq->ch3.segment_first) / rreq->ch3.iov_count
		 >= MPIDI_IOV_DENSITY_MIN)
	{
	    MPIDI_DBG_PRINTF((35, FCNAME, "updating rreq to read more data "
			      "directly into the user buffer"));
	    rreq->ch3.segment_first = last;
	    rreq->ch3.ca = MPIDI_CH3_CA_RELOAD_IOV;
	}
	else
	{
	    /* Too little data would have been received using an IOV.  We will
	       start receiving data into a SRBuf and unpacking it later. */
	    
	    assert(MPIDI_Request_get_srbuf_flag(rreq) == FALSE);
	    
	    MPIDI_CH3U_SRBuf_alloc(
		rreq, rreq->ch3.segment_size - rreq->ch3.segment_first);
	    rreq->ch3.tmpbuf_off = 0;
	    if (rreq->ch3.tmpbuf_sz == 0)
	    {
		/* XXX - we should drain the data off the pipe here, but we
		   don't have a buffer to drain it into.  should this be a
		   fatal error? */
		MPIDI_DBG_PRINTF((40, FCNAME, "SRBuf allocation failure"));
		mpi_errno = MPI_ERR_NOMEM;
		rreq->status.MPI_ERROR = mpi_errno;
		goto fn_exit;
	    }

	    /* fill in the IOV using a recursive call */
	    mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
	}
    }
    else
    {
	/* receive and toss any extra data that does not fit in the user's
           buffer */
	MPIDI_msg_sz_t data_sz;

	data_sz = rreq->ch3.recv_data_sz - rreq->ch3.segment_first;
	if (!MPIDI_Request_get_srbuf_flag(rreq))
	{
	    MPIDI_CH3U_SRBuf_alloc(rreq, data_sz);
	    if (rreq->ch3.tmpbuf_sz == 0)
	    {
		MPIDI_DBG_PRINTF((40, FCNAME, "SRBuf allocation failure"));
		mpi_errno = MPI_ERR_NOMEM;
		rreq->status.MPI_ERROR = mpi_errno;
		goto fn_exit;
	    }
	}

	if (data_sz <= rreq->ch3.tmpbuf_sz)
	{
	    MPIDI_DBG_PRINTF((35, FCNAME, "updating rreq to read overflow "
			      "data into the SRBuf and complete"));
	    rreq->ch3.iov[0].MPID_IOV_LEN = data_sz;
	    rreq->ch3.ca = MPIDI_CH3_CA_COMPLETE;
	}
	else
	{
	    MPIDI_DBG_PRINTF((35, FCNAME, "updating rreq to read overflow "
			      "data into the SRBuf and reload IOV"));
	    rreq->ch3.iov[0].MPID_IOV_LEN = rreq->ch3.tmpbuf_sz;
	    rreq->ch3.segment_first += rreq->ch3.tmpbuf_sz;
	    rreq->ch3.ca = MPIDI_CH3_CA_RELOAD_IOV;
	}
	
	rreq->ch3.iov[0].MPID_IOV_BUF = rreq->ch3.tmpbuf;
	rreq->ch3.iov_count = 1;
    }
    
  fn_exit:

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_REQUEST_LOAD_RECV_IOV);
    return mpi_errno;
}

/*
 * MPIDI_CH3U_Request_unpack_srbuf
 *
 * Unpack data from a send/receive buffer into the user buffer.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Request_unpack_srbuf
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Request_unpack_srbuf(MPID_Request * rreq)
{
    int last;
    int tmpbuf_last;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_REQUEST_UNPACK_SRBUF);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_REQUEST_UNPACK_SRBUF);

    tmpbuf_last = rreq->ch3.segment_first + rreq->ch3.tmpbuf_sz;
    if (rreq->ch3.segment_size < tmpbuf_last)
    {
	tmpbuf_last = rreq->ch3.segment_size;
    }
    last = tmpbuf_last;
    MPID_Segment_unpack(&rreq->ch3.segment, rreq->ch3.segment_first, &last,
			rreq->ch3.tmpbuf);
    if (last == 0 || last == rreq->ch3.segment_first)
    {
	/* If no data can be unpacked, then we have a datatype processing
	   problem.  Adjust the segment info so that the remaining data is
	   received and thrown away. */
	rreq->status.MPI_ERROR = MPI_ERR_UNKNOWN;
	rreq->status.count = rreq->ch3.segment_first;
	rreq->ch3.segment_size = rreq->ch3.segment_first;
	rreq->ch3.segment_first += tmpbuf_last;
    }
    else if (tmpbuf_last == rreq->ch3.segment_size)
    {
	if (last != tmpbuf_last)
	{
	    /* received data was not entirely consumed by unpack() because too
	       few bytes remained to fill the next basic datatype.  Note: the
	       segment_first field is set to segment_last so that if this is a
	       truncated message, extra data will be read off the pipe. */
	    rreq->status.count = last;
	    rreq->ch3.segment_size = last;
	    rreq->ch3.segment_first = tmpbuf_last;
	    mpi_errno = MPI_ERR_UNKNOWN;
	}
    }
    else
    {
	rreq->ch3.tmpbuf_off = tmpbuf_last - last;
	if (rreq->ch3.tmpbuf_off > 0)
	{
	    /* move any remaining data to the beginning of the buffer.  Note:
               memmove() is used since the data regions could overlap. */
	    memmove(rreq->ch3.tmpbuf, (char *) rreq->ch3.tmpbuf +
		    (last - rreq->ch3.segment_first), rreq->ch3.tmpbuf_off);
	}
	rreq->ch3.segment_first = last;
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_REQUEST_UNPACK_SRBUF);
    return MPI_SUCCESS;
}

/*
 * MPIDI_CH3U_Request_unpack_uebuf
 *
 * Copy/unpack data from an "unexpected eager buffer" into the user buffer.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Request_unpack_uebuf
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Request_unpack_uebuf(MPID_Request * rreq)
{
    int dt_contig;
    MPIDI_msg_sz_t userbuf_sz;
    MPIDI_msg_sz_t unpack_sz;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_REQUEST_UNPACK_UEBUF);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_REQUEST_UNPACK_UEBUF);

    if (HANDLE_GET_KIND(rreq->ch3.datatype) == HANDLE_KIND_BUILTIN)
    {
	dt_contig = TRUE;
	userbuf_sz = rreq->ch3.user_count *
	    MPID_Datatype_get_basic_size(rreq->ch3.datatype);
    }
    else
    {
	MPID_Datatype * dtp;

	MPID_Datatype_get_ptr(rreq->ch3.datatype, dtp);
	dt_contig = dtp->is_contig;
	userbuf_sz = rreq->ch3.user_count * dtp->size;
    }
    
    if (rreq->ch3.recv_data_sz <= userbuf_sz)
    {
	unpack_sz = rreq->ch3.recv_data_sz;
    }
    else
    {
	MPIDI_DBG_PRINTF((40, FCNAME, "receive buffer overflow; message "
			  "truncated, msg_sz=" MPIDI_MSG_SZ_FMT ", buf_sz="
			  MPIDI_MSG_SZ_FMT, rreq->ch3.recv_data_sz,
			  userbuf_sz));
	unpack_sz = userbuf_sz;
	rreq->status.count = userbuf_sz;
	mpi_errno = MPI_ERR_TRUNCATE;
    }
	
    if (dt_contig)
    {
	/* TODO - check that amount of data is consistent with datatype.  In
           other words, if we were to use Segment_unpack() would last = unpack?
           If not we should return an error (unless configured with
           --enable-fast) */
	memcpy(rreq->ch3.user_buf, rreq->ch3.tmpbuf, unpack_sz);
    }
    else
    {
	MPID_Segment seg;
	int last;

	MPID_Segment_init(rreq->ch3.user_buf, rreq->ch3.user_count,
			  rreq->ch3.datatype, &seg);
	last = unpack_sz;
	MPID_Segment_unpack(&seg, 0, &last, rreq->ch3.tmpbuf);
	if (last != unpack_sz && mpi_errno == MPI_SUCCESS)
	{
	    /* received data was not entirely consumed by unpack() because
	       too few bytes remained to fill the next basic datatype */
	    rreq->status.count = last;
	    mpi_errno = MPI_ERR_UNKNOWN;
	}
    }
		
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_REQUEST_UNPACK_UEBUF);
    return mpi_errno;
}


