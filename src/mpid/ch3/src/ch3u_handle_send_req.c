/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*
 * MPIDI_CH3U_Handle_send_pkt()
 *
 * NOTE: This routine must be reentrant.  Routines like MPIDI_CH3_iWrite() are allowed to perform additional up-calls if they
 * complete the requested work immediately.
 *
 * *** Care must be take to avoid deep recursion.  With some thread packages, exceeding the stack space allocated to a thread can
 * *** result in overwriting the stack of another thread.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Handle_send_req
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Handle_send_req(MPIDI_VC * vc, MPID_Request * sreq)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_HANDLE_SEND_REQ);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_HANDLE_SEND_REQ);

    assert(sreq->ch3.ca < MPIDI_CH3_CA_END_CH3);
    
    switch(sreq->ch3.ca)
    {
	case MPIDI_CH3_CA_COMPLETE:
	{
	    /* mark data transfer as complete and decrement CC */
	    sreq->ch3.iov_count = 0;
	    MPIDI_CH3U_Request_complete(sreq);
	    break;
	}
	
	case MPIDI_CH3_CA_RELOAD_IOV:
	{
	    int mpi_errno;

	    sreq->ch3.iov_count = MPID_IOV_LIMIT;
	    mpi_errno = MPIDI_CH3U_Request_load_send_iov(sreq, sreq->ch3.iov, &sreq->ch3.iov_count);
	    if (mpi_errno == MPI_SUCCESS)
	    {
		MPIDI_CH3_iWrite(vc, sreq);
	    }
	    else
	    {
		MPIDI_ERR_PRINTF((FCNAME, "MPIDI_CH3_CA_RELOAD_IOV failed"));
		MPID_Abort(NULL, mpi_errno);
	    }
	    
	    break;
	}
	
	default:
	{
	    MPIDI_ERR_PRINTF((FCNAME, "action %d UNIMPLEMENTED", sreq->ch3.ca));
	    MPID_Abort(NULL, MPI_ERR_INTERN);
	}
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_HANDLE_SEND_REQ);
    return MPI_SUCCESS;
}

