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
    int mpi_errno = MPI_SUCCESS;
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
	    sreq->ch3.iov_count = MPID_IOV_LIMIT;
	    mpi_errno = MPIDI_CH3U_Request_load_send_iov(sreq, sreq->ch3.iov, &sreq->ch3.iov_count);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|loadsendiov", 0);
		goto fn_exit;
	    }
	    mpi_errno = MPIDI_CH3_iWrite(vc, sreq);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|senddata", 0);
		goto fn_exit;
	    }
	    
	    break;
	}
	
	default:
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN, "**ch3|badca",
					     "**ch3|badca %d", sreq->ch3.ca);
	    break;
	}
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_HANDLE_SEND_REQ);
    return mpi_errno;
}

