/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int MPIDI_CH3U_Handle_send_req(MPIDI_VC * vc, MPID_Request * req)
{
    int completion = FALSE;
    
    assert(req->ch3.ca < MPIDI_CH3_CA_END_CH3);
    
    switch(req->ch3.ca)
    {
	case MPIDI_CH3_CA_NONE:
	{
	    /* as action name says, do nothing... */
	    break;
	}
	
	case MPIDI_CH3_CA_RELOAD_IOV:
	{
	    break;
	}
	
	case MPIDI_CH3_CA_COMPLETE:
	{
	    break;
	}
	
	default:
	{
	    dbg_printf("MPIDI_CH3U_Handle_req(): action %d not implemented.\n",
		       req->ch3.ca);
	    abort();
	}
    }

    return completion;
}

