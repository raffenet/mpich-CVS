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
		(rreq->ch3.match.rank && mask.rank) == match.rank &&
		(rreq->ch3.match.tag && mask.tag) == match.tag)
	    {
		MPIDI_CH3_Request_add_ref(rreq);
		return rreq;
	    }
	    
	    rreq = rreq->ch3.next;
	}
    }
    
    return NULL;
}

/*
 * MPIDI_CH3U_Request_FDUOAEP()
 *
 * Find a request in the unexpected queue and dequeue it; otherwise return
 * NULL.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Request_FDU
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
MPID_Request * MPIDI_CH3U_Request_FDU(int source, int tag, int context_id)
{
    MPID_Request * rreq;
    MPID_Request * prev_rreq;

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
		(rreq->ch3.match.rank && mask.rank) == match.rank &&
		(rreq->ch3.match.tag && mask.tag) == match.tag)
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
		return rreq;
	    }
	    
	    prev_rreq = rreq;
	    rreq = rreq->ch3.next;
	}
    }

    return NULL;
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

    rreq = MPIDI_CH3U_Request_FDU(source, tag, context_id);
    if (rreq != NULL)
    {
	*found = TRUE;
	return rreq;
    }
    
    /* A matching request was not found in the unexpected queue, so we need to
       allocate a new request and add it to the posted queue */
    rreq = MPIDI_CH3_Request_create();
    if (rreq != NULL)
    {
	rreq->ref_count = 2;
	rreq->kind = MPID_REQUEST_RECV;
	rreq->cc = 1;
	rreq->cc_ptr = &(rreq->cc);
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
    return rreq;
}


/*
 * MPIDI_CH3U_Request_FDP()
 *
 * Find a request in the posted queue and dequeue it, or return NULL
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Request_FDP
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
MPID_Request * MPIDI_CH3U_Request_FDP(MPIDI_Message_match * match)
{
    MPID_Request * rreq;
    MPID_Request * prev_rreq;

    prev_rreq = NULL;
    rreq = MPIDI_Process.recv_posted_head;
    while (rreq != NULL)
    {
	if (rreq->ch3.match.context_id == match->context_id)
	{
	    if (rreq->ch3.match.rank == match->rank ||
		rreq->ch3.match.rank == MPI_ANY_SOURCE)
	    {
		if (rreq->ch3.match.tag == match->tag ||
		    rreq->ch3.match.tag == MPI_ANY_TAG)
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
		    return rreq;
		}
	    }
	}
	    
	prev_rreq = rreq;
	rreq = rreq->ch3.next;
    }

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

    rreq = MPIDI_CH3U_Request_FDP(match);
    if (rreq != NULL)
    {
	*found = TRUE;
	return rreq;
    }

    /* A matching request was not found in the posted queue, so we need to
       allocate a new request and add it to the unexpected queue */
    rreq = MPIDI_CH3_Request_create();
    if (rreq != NULL)
    {
	rreq->ref_count = 2;
	rreq->kind = MPID_REQUEST_RECV;
	rreq->cc = 1;
	rreq->cc_ptr = &(rreq->cc);
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
    return rreq;
}

/*
 * MPIDI_CH3U_Request_adjust_iov()
 *
 * Adjust the iovec in the request by the supplied number of bytes.  If the
 * iovec has been consumed, return true; otherwise return false.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Request_adjust_iov
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Request_adjust_iov(MPID_Request * req, int nb)
{
    int offset = req->ch3.iov_offset;
    const int count = req->ch3.iov_count;
    
    while (offset < count)
    {
	if (req->ch3.iov[offset].iov_len <= nb)
	{
	    nb -= req->ch3.iov[offset].iov_len;
	    offset++;
	}
	else
	{
	    req->ch3.iov[offset].iov_base += nb;
	    req->ch3.iov[offset].iov_len -= nb;
	    req->ch3.iov_offset = offset;
	    return FALSE;
	}
    }
    
    req->ch3.iov_offset = offset;
    return TRUE;
}


/*
 * MPIDI_CH3U_Request_copy_tmp_data()
 *
 * Copy data from a temporary buffer attached to the receive request into the
 * user data buffer.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Request_copy_tmp_data
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3U_Request_copy_tmp_data(MPID_Request * rreq)
{
    long dt_sz;
    int dt_contig;
	    
    if (HANDLE_GET_KIND(rreq->ch3.datatype) == HANDLE_KIND_BUILTIN)
    {
	dt_sz = MPID_Datatype_get_size(rreq->ch3.datatype);
	dt_contig = TRUE;
    }
    else
    {
	MPIDI_err_printf(FCNAME, "only basic datatypes are supported");
	abort();
    }
		    
    if (dt_contig) 
    {
	if (rreq->ch3.recv_data_sz <= dt_sz * rreq->ch3.user_count)
	{
	    memcpy(rreq->ch3.user_buf, rreq->ch3.tmp_buf,
		   rreq->ch3.recv_data_sz);
	}
	else
	{
	    MPIDI_err_printf(FCNAME, "receive buffer overflow");
	    abort();
	    /* TODO: handle buffer overflow properly */
	}
    }
    else
    {
	MPIDI_err_printf(FCNAME, "only contiguous data is supported");
	abort();
    }
		
    MPIU_Free(rreq->ch3.tmp_buf);
    rreq->ch3.tmp_buf = NULL;
    rreq->ch3.tmp_sz = 0;
}
