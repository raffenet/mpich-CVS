/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*
 * MPIDI_CH3U_Handle_recv_req()
 *
 * NOTE: This routine must be reentrant.  Routines like MPIDI_CH3_iRead() are
 * allowed to perform additional up-calls if they complete the requested work
 * immediately. *** Care must be take to avoid deep recursion.  With some
 * thread packages, exceeding the stack space allocated to a thread can result
 * in overwriting the stack of another thread. ***
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Handle_recv_req
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3U_Handle_recv_req(MPIDI_VC * vc, MPID_Request * rreq)
{
    assert(rreq->ch3.ca < MPIDI_CH3_CA_END_CH3);
    
    switch(rreq->ch3.ca)
    {
	case MPIDI_CH3_CA_NONE:
	{
	    /* as the action name says, do nothing... */
	    break;
	}
	
	case MPIDI_CH3_CA_RELOAD_IOV:
	{
	    MPIDI_err_printf(FCNAME, "MPIDI_CH3_CA_RELOAD_IOV UMIMPLEMENTED");
	    abort();
	    break;
	}
	
	case MPIDI_CH3_CA_COPY_COMPLETE:
	{
	    MPIDI_CH3U_Request_unpack_tmp_buf(rreq);
	    MPIU_Free(rreq->ch3.tmp_buf);
	    /* fall through into the next case */
	}
	
	case MPIDI_CH3_CA_COMPLETE:
	{
	    MPIDI_CH3U_Request_complete(rreq);
	    break;
	}
	
	default:
	{
	    MPIDI_err_printf(FCNAME, "action %d UNIMPLEMENTED", rreq->ch3.ca);
	    abort();
	}
    }
}

