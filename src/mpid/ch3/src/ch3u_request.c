/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Request_FUOAP
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
MPID_Request * MPIDI_CH3U_Request_FUOAP(
    int source, int tag, int context_id, int * found)
{
    MPID_Request * req;
    MPID_Request * prev_req;

    if (tag != MPI_ANY_TAG && source != MPI_ANY_SOURCE)
    {
	prev_req = NULL;
	req = MPIDI_Process.recv_unexpected_head;
	while(req != NULL)
	{
	    if (req->ch3.match.context_id == context_id &&
		req->ch3.match.rank == source &&
		req->ch3.match.tag == tag)
	    {
		if (prev_req != NULL)
		{
		    prev_req->ch3.next = req->ch3.next;
		}
		else
		{
		    MPIDI_Process.recv_unexpected_head = req->ch3.next;
		}
		if (req->ch3.next == NULL)
		{
		    MPIDI_Process.recv_unexpected_tail = prev_req;
		}
		*found = 1;
		return req;
	    }
	    
	    prev_req = req;
	    req = req->ch3.next;
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
	
	prev_req = NULL;
	req = MPIDI_Process.recv_unexpected_head;
	while (req != NULL)
	{
	    if (req->ch3.match.context_id == match.context_id &&
		(req->ch3.match.rank && mask.rank) == match.rank &&
		(req->ch3.match.tag && mask.tag) == match.tag)
	    {
		if (prev_req != NULL)
		{
		    prev_req->ch3.next = req->ch3.next;
		}
		else
		{
		    MPIDI_Process.recv_unexpected_head = req->ch3.next;
		}
		if (req->ch3.next == NULL)
		{
		    MPIDI_Process.recv_unexpected_tail = prev_req;
		}
		*found = 1;
		return req;
	    }
	    
	    prev_req = req;
	    req = req->ch3.next;
	}
    }

    /* A matching request was not found in the unexpected queue, so we need to
       allocate a new request and add it to the posted queue */
    req = MPIDI_CH3_Request_create();
    if (req != NULL)
    {
	req->ref_count = 2;
	req->kind = MPID_REQUEST_RECV;
	req->cc = 1;
	req->cc_ptr = &(req->cc);
	req->ch3.match.tag = tag;
	req->ch3.match.rank = source;
	req->ch3.match.context_id = context_id;
	req->ch3.next = NULL;
	
	if (MPIDI_Process.recv_posted_tail != NULL)
	{
	    MPIDI_Process.recv_posted_tail->ch3.next = req;
	}
	else
	{
	    MPIDI_Process.recv_posted_head = req;
	}
	MPIDI_Process.recv_posted_tail = req;
    }
    
    *found = 0;
    return req;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Request_FPOAU
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
MPID_Request * MPIDI_CH3U_Request_FPOAU(
    MPIDI_Message_match * match, int * found)
{
    MPID_Request * req;
    MPID_Request * prev_req;

    prev_req = NULL;
    req = MPIDI_Process.recv_posted_head;
    while (req != NULL)
    {
	if (req->ch3.match.context_id == match->context_id)
	{
	    if (req->ch3.match.rank == match->rank ||
		req->ch3.match.rank == MPI_ANY_SOURCE)
	    {
		if (req->ch3.match.tag == match->tag ||
		    req->ch3.match.tag == MPI_ANY_TAG)
		{
		    if (prev_req != NULL)
		    {
			prev_req->ch3.next = req->ch3.next;
		    }
		    else
		    {
			MPIDI_Process.recv_posted_head = req->ch3.next;
		    }
		    if (req->ch3.next == NULL)
		    {
			MPIDI_Process.recv_posted_tail = prev_req;
		    }
		    *found = 1;
		    return req;
		}
	    }
	}
	    
	prev_req = req;
	req = req->ch3.next;
    }

    /* A matching request was not found in the posted queue, so we need to
       allocate a new request and add it to the unexpected queue */
    req = MPIDI_CH3_Request_create();
    if (req != NULL)
    {
	req->ref_count = 2;
	req->kind = MPID_REQUEST_RECV;
	req->cc = 1;
	req->cc_ptr = &(req->cc);
	req->ch3.match = *match;
	req->ch3.next = NULL;
	
	if (MPIDI_Process.recv_unexpected_tail != NULL)
	{
	    MPIDI_Process.recv_unexpected_tail->ch3.next = req;
	}
	else
	{
	    MPIDI_Process.recv_unexpected_head = req;
	}
	MPIDI_Process.recv_unexpected_tail = req;
    }
    
    *found = 0;
    return req;
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
