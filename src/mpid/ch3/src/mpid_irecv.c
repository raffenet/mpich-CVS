/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*
 * MPID_Isend()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Irecv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Irecv(void * buf, int count, MPI_Datatype datatype, int rank,
	       int tag, MPID_Comm * comm, int context_offset,
               MPID_Request ** request)
{
    MPID_Request * rreq;
    int found;
    int mpi_errno = MPI_SUCCESS;

    MPIDI_dbg_printf(10, FCNAME, "entering");
    MPIDI_dbg_printf(15, FCNAME, "rank=%d, tag=%d, context=%d", rank, tag,
		     comm->context_id + context_offset);
    
    rreq = MPIDI_CH3U_Request_FUOAP(
	rank, tag, comm->context_id + context_offset, &found);
    assert(rreq != NULL);

    /* MT - thread safety? message could arrive while populating req */
    rreq->comm = comm;
    rreq->ch3.user_buf = buf;
    rreq->ch3.user_count = count;
    rreq->ch3.datatype = datatype;
    rreq->ch3.vc = comm->vcr[rank];

    if (found)
    {
	/* Message was found in the unexepected queue */
	MPIDI_dbg_printf(15, FCNAME, "request found in unexpected queue");

	/* MT - this check needs to be thread safe */
	/* NOTE - rreq->cc is used here instead of rreq->cc_ptr.  We are
	   assuming that for simple sends and receives the request's internal
	   completion counter will always be used. */
	if (MPIDI_Request_get_msg_type(rreq) == MPIDI_REQUEST_EAGER_MSG)
	{
	    /* This is an eager message */
	    MPIDI_dbg_printf(15, FCNAME, "eager message in the request");
	    
	    if (rreq->cc == 0)
	    {
		/* All of the data has arrived, we need to copy the data and
	           then free the buffer and the request. */
		if (count > 0)
		{
		    MPIDI_CH3U_Request_copy_tmp_data(rreq);
		}
		
		mpi_errno = rreq->status.MPI_ERROR;
		goto fn_exit;
	    }
	    else
	    {
		/* The data is still being transfered across the net.  We'll
		   leave it to the progress engine to handle once the entire
		   message has arrived. */
		rreq->ch3.ca = MPIDI_CH3_CA_COPY_COMPLETE;
	    }
	}
	else
	{
	    /* A rendezvous request-to-send (RTS) message has arrived.  We need
	       to send a clear-to-send message to the remote process. */
	    MPID_Request * cts_req;
	    MPIDI_CH3_Pkt_t upkt;
	    MPIDI_CH3_Pkt_rndv_clr_to_send_t * cts_pkt =
		&upkt.rndv_clr_to_send;
		
	    MPIDI_dbg_printf(15, FCNAME, "rndv RTS in the request, "
			     "sending rndv CTS");
	    
	    cts_pkt->type = MPIDI_CH3_PKT_RNDV_CLR_TO_SEND;
	    cts_pkt->req_id_sender = rreq->ch3.rndv_req_id;
	    cts_pkt->req_id_receiver = rreq->handle;
	    cts_req = MPIDI_CH3_iStartMsg(rreq->ch3.vc,
					  cts_pkt, sizeof(*cts_pkt));
	    if (cts_req != NULL)
	    {
		/* XXX: Ideally we could specify that a req not be returned.
		   This would avoid our having to decrement the reference count
		   on a req we don't want/need. */
		MPID_Request_free(cts_req);
	    }
	}
    }
    else
    {
	/* Message has yet to arrived.  The request has been placed on the list
           of posted receive requests and populated with information supplied
           in the arguments. */
	MPIDI_dbg_printf(15, FCNAME, "request allocated in posted queue");
    }

  fn_exit:
    *request = rreq;
    MPIDI_dbg_printf(10, FCNAME, "exiting");
    return mpi_errno;
}

