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
    int mpi_errno = MPI_SUCCESS;

    for (i = 0; i < count; i++)
    {
	MPID_Request * const preq = requests[i];

	switch (MPIDI_Request_get_persistent_type(preq))
	{
	    case MPIDI_REQUEST_PERSISTENT_RECV:
	    {
		mpi_errno = MPID_Irecv(
		    preq->ch3.user_buf, preq->ch3.user_count,
		    preq->ch3.datatype, preq->ch3.match.rank,
		    preq->ch3.match.tag, preq->comm,
		    preq->ch3.match.context_id - preq->comm->context_id,
		    &preq->partner_request);
		break;
	    }
	    
	    case MPIDI_REQUEST_PERSISTENT_SEND:
	    {
		mpi_errno = MPID_Isend(
		    preq->ch3.user_buf, preq->ch3.user_count,
		    preq->ch3.datatype, preq->ch3.match.rank,
		    preq->ch3.match.tag, preq->comm,
		    preq->ch3.match.context_id - preq->comm->context_id,
		    &preq->partner_request);
		break;
	    }
		
	    case MPIDI_REQUEST_PERSISTENT_RSEND:
	    {
		mpi_errno = MPID_Irsend(
		    preq->ch3.user_buf, preq->ch3.user_count,
		    preq->ch3.datatype, preq->ch3.match.rank,
		    preq->ch3.match.tag, preq->comm,
		    preq->ch3.match.context_id - preq->comm->context_id,
		    &preq->partner_request);
		break;
	    }
		
	    case MPIDI_REQUEST_PERSISTENT_SSEND:
	    {
		mpi_errno = MPID_Issend(
		    preq->ch3.user_buf, preq->ch3.user_count,
		    preq->ch3.datatype, preq->ch3.match.rank,
		    preq->ch3.match.tag, preq->comm,
		    preq->ch3.match.context_id - preq->comm->context_id,
		    &preq->partner_request);
		break;
	    }
	}
	
	if (mpi_errno != MPI_SUCCESS)
	{
	    return mpi_errno;
	}
    }

    return mpi_errno;
}
