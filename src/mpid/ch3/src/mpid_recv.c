/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int MPID_Recv(void * buf, int count, MPI_Datatype datatype,
	      int rank, int tag, MPID_Comm * comm, int context_offset,
	      MPI_Status * status, MPID_Request ** request)
{
    MPID_Request * req;
    int found;

    req = MPIDI_CH3U_Request_FUOAP(
	rank, tag, comm->context_id + context_offset, &found);
    assert(req != NULL);

    if (found)
    {
	/* Message was found in the unexepected queue */

	printf("MPID_Recv: matching message found\n");
	fflush(stdout);

#if 0	/* only zero length messages supported */
	
	/* XXX - this check needs to be thread safe; it may be already assuming
           req->cc is declared volatile */
	/* NOTE - req->cc is used here instead of req->cc_ptr.  We are assuming
	   that for simple sends and receives the request's internal completion
	   counter will always be used. */
	if (req->cc == 0)
	{
	    /* The entire message has arrived */

	    if ((req->mpid_state & MPID_REQUEST_STATE_MSG_MASK) <=
		MPID_REQUEST_STATE_EAGER_MSG)
	    {
		/* This is an eager send message.  We need to copy the data and
                   then free the buffer and the request. */
		
		int dt_sz;
		int dt_contig;
    
		if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN)
		{
		    dt_sz = MPID_Datatype_get_size(datatype);
		    dt_contig = TRUE;
		}
		else
		{
		    assert(HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN);
		    abort();
		}
		
		if (dt_contig)
		{
		    memcpy(buf, req->ch3.recv_buf, req->ch3.recv_sz);
		}
		else
		{
		    assert(dt_contig);
		    abort();
		}
		
		*status = req->status;
		MPIU_Free(req->ch3.recv_buf);
		MPIDI_CH3_Request_free(req);
		
		return status->MPI_ERROR;
	    }
	    else
	    {
		/* A rendezvous request-to-send (RTS) message has arrived.  We
		   need to repopulate the reequest with the information
		   supplied in the arguments and then send a clear-to-send
		   message to the remote process. */
		req->ch3.user_buf = buf;
		req->ch3.user_count = count;
		req->ch3.datatype = datatype;
		req->ch3.vc = comm->vcr[rank];

		/* TODO - send Rndv RTS */
		
		*request = req;
	    }
	}
	else
	{
	    /* The data is still being transfered across the net.  We'll leave
               it to the progress engine to handle once the entire message has
               arrived. */
	    req->ch3.user_buf = buf;
	    req->ch3.user_count = count;
	    req->ch3.datatype = datatype;
	    req->ch3.vc = comm->vcr[rank];
	    *request = req;
	}
#endif	
    }
    else
    {
	/* Message has yet to arrived.  The request has been placed on the list
           of posted receive requests.  The request still needs to be populated
           with information supplied in the arguments. */
	req->ch3.user_buf = buf;
	req->ch3.user_count = count;
	req->ch3.datatype = datatype;
	req->ch3.vc = comm->vcr[rank];
	*request = req;
	/* XXX - thread safety? message could arrive while populating req */
    }
    
    return MPI_SUCCESS;
}
