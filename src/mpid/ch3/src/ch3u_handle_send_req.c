/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*
 * MPIDI_CH3U_Handle_send_pkt()
 *
 * NOTE: This routine must be reentrant.  Routines like MPIDI_CH3_iRead() are
 * allowed to perform additional up-calls if they complete the requested work
 * immediately. *** Care must be take to avoid deep recursion.  With some
 * thread packages, exceeding the stack space allocated to a thread can result
 * in overwriting the stack of another thread. ***
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Handle_send_req
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3U_Handle_send_req(MPIDI_VC * vc, MPID_Request * sreq)
{
    assert(sreq->ch3.ca < MPIDI_CH3_CA_END_CH3);
    
    switch(sreq->ch3.ca)
    {
	case MPIDI_CH3_CA_NONE:
	{
	    /* as the action name says, do nothing... */
	    break;
	}
	
	case MPIDI_CH3_CA_RELOAD_IOV:
	{
	    int size;
	    int last;
	    
	    /* two choices here: fill iovec or pack message into a buffer.  in
	       either case, if we have more data to send, we need to use a
	       completion operation that takes care of the rest of the
	       data. */
	
	    sreq->ch3.iov_count = MPID_IOV_LIMIT;
	    last = sreq->ch3.segment_size;
	    MPID_Segment_pack_vector(
		&sreq->ch3.segment, sreq->ch3.segment_first, &last,
		sreq->ch3.iov, &sreq->ch3.iov_count);
	    assert(last > sreq->ch3.segment_first);
	    assert(sreq->ch3.iov_count > 0);
	    
	    if (last / sreq->ch3.iov_count < MPIDI_IOV_DENSITY_MIN &&
		last != sreq->ch3.segment_size)
	    {
		size = sreq->ch3.segment_size - sreq->ch3.segment_first;
		
		/* allocate temporary buffer and pack */
		if (!MPIDI_Request_get_srbuf_flag(sreq))
		{
		    MPIDI_CH3U_SRBuf_alloc(sreq, size);
		    assert (sreq->ch3.tmp_sz > 0);
		}

		last = (size <= sreq->ch3.tmp_sz) ? sreq->ch3.segment_size
		    : sreq->ch3.segment_first + sreq->ch3.tmp_sz;
		MPID_Segment_pack(
		    &sreq->ch3.segment, sreq->ch3.segment_first, &last,
		    sreq->ch3.tmp_buf);
	    }
	    
	    if (last == sreq->ch3.segment_size)
	    {
		sreq->ch3.ca = MPIDI_CH3_CA_COMPLETE;
	    }
	    else 
	    {
		sreq->ch3.segment_first = last;
		sreq->ch3.ca = MPIDI_CH3_CA_RELOAD_IOV;
	    }
	    
	    MPIDI_CH3_iWrite(sreq->ch3.vc, sreq);
	    
	    break;
	}
	
	case MPIDI_CH3_CA_COMPLETE:
	{
	    MPIDI_CH3U_Request_complete(sreq);
	    break;
	}
	
	default:
	{
	    MPIDI_err_printf(FCNAME, "action %d UNIMPLEMENTED", sreq->ch3.ca);
	    abort();
	}
    }
}

