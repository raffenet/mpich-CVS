/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*
 * MPID_Startall()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Startall
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Startall(int count, MPID_Request * requests[])
{
    int i;
    int rc = MPI_SUCCESS;
    int mpi_errno = MPI_SUCCESS;

    for (i = 0; i < count; i++)
    {
	MPID_Request * const preq = requests[i];

	MPIR_Status_set_empty(preq->status);

	switch (MPIDI_Request_get_persistent_type(preq))
	{
	    case MPIDI_REQUEST_PERSISTENT_RECV:
	    {
		rc = MPID_Irecv(
		    preq->ch3.user_buf, preq->ch3.user_count,
		    preq->ch3.datatype, preq->ch3.match.rank,
		    preq->ch3.match.tag, preq->comm,
		    preq->ch3.match.context_id - preq->comm->context_id,
		    &preq->partner_request);
		break;
	    }
	    
	    case MPIDI_REQUEST_PERSISTENT_SEND:
	    {
		rc = MPID_Isend(
		    preq->ch3.user_buf, preq->ch3.user_count,
		    preq->ch3.datatype, preq->ch3.match.rank,
		    preq->ch3.match.tag, preq->comm,
		    preq->ch3.match.context_id - preq->comm->context_id,
		    &preq->partner_request);
		break;
	    }
		
	    case MPIDI_REQUEST_PERSISTENT_RSEND:
	    {
		rc = MPID_Irsend(
		    preq->ch3.user_buf, preq->ch3.user_count,
		    preq->ch3.datatype, preq->ch3.match.rank,
		    preq->ch3.match.tag, preq->comm,
		    preq->ch3.match.context_id - preq->comm->context_id,
		    &preq->partner_request);
		break;
	    }
		
	    case MPIDI_REQUEST_PERSISTENT_SSEND:
	    {
		rc = MPID_Issend(
		    preq->ch3.user_buf, preq->ch3.user_count,
		    preq->ch3.datatype, preq->ch3.match.rank,
		    preq->ch3.match.tag, preq->comm,
		    preq->ch3.match.context_id - preq->comm->context_id,
		    &preq->partner_request);
		break;
	    }
	}
	
	if (rc == MPI_SUCCESS)
	{
	    preq->cc_ptr = &preq->partner_request->cc;
	}
	else
	{
	    /* If a failure occurs atttempt to start the request, then we
	       assume that partner request was not created, and stuff the
	       error code in the persistent request.  The wait and test
	       routines will look at the error code in the persistent request
	       if a partner request is not found. */
	    preq->status.MPI_ERROR = rc;
	}
    }

    return mpi_errno;
}
