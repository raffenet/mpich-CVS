/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*
 * MPIDI_CH3U_Handle_send_pkt()
 *
 * NOTE: This routine must be reentrant safe.  Routines like MPIDI_CH3_iRead()
 * are allowed to perform up-calls if they complete the requested work
 * immediately. *** Care must be take to avoid deep recursion which with some
 * thread packages can result in overwriting the stack of another thread. ***
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Handle_send_req
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Handle_send_req(MPIDI_VC * vc, MPID_Request * req)
{
    int completion = FALSE;
    
    assert(req->ch3.ca < MPIDI_CH3_CA_END_CH3);
    
    switch(req->ch3.ca)
    {
	case MPIDI_CH3_CA_NONE:
	{
	    /* as the action name says, do nothing... */
	    break;
	}
	
	case MPIDI_CH3_CA_RELOAD_IOV:
	{
	    break;
	}
	
	case MPIDI_CH3_CA_COMPLETE:
	{
	    int cc;
		
	    MPIDI_CH3U_Request_decrement_cc(req, &cc);
	    if (cc == 0)
	    {
		MPID_Request_free(req);
		completion = TRUE;
	    }
	    break;
	}
	
	default:
	{
	    MPIDI_dbg_printf(30, FCNAME, "action %d not implemented",
			     req->ch3.ca);
	    abort();
	}
    }

    return completion;
}

