/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Cancel_recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPID_Cancel_recv(MPID_Request * rreq)
{
    int cc;
    
    MPIDI_dbg_printf(10, FCNAME, "entering");
    assert(rreq->kind == MPID_REQUEST_RECV);
    /* XXX - need to handle persistent requests */
    
    if (MPIDI_CH3U_Request_FDP(rreq))
    {
	MPIDI_dbg_printf(15, FCNAME, "request 0x%08x cancelled", rreq->handle);
	rreq->status.cancelled = TRUE;
	rreq->status.count = 0;
	/* MT - decrement_cc() used for write barrier */
	MPIDI_CH3U_Request_decrement_cc(rreq, &cc);
	/* NOTE: we should not perform a MPID_Request_release(rreq) as
           MPIDI_CH3U_Request_FDP() releases the one it held internally and the
           reference we have belongs to the caller. */
    }
    else
    {
	MPIDI_dbg_printf(15, FCNAME, "request 0x%08x already matched, "
			 "unable to cancel", rreq->handle);
    }
    
    MPIDI_dbg_printf(10, FCNAME, "exiting");
}
