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
#define FUNCNAME MPID_Isend
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Isend(const void * buf, int count, MPI_Datatype datatype, int rank,
	       int tag, MPID_Comm * comm, int context_offset,
               MPID_Request ** request)
{
    long dt_sz;
    int dt_contig;
    MPID_Request * sreq;

    MPIDI_dbg_printf(10, FCNAME, "entering");
    MPIDI_dbg_printf(15, FCNAME, "rank=%d, tag=%d, context=%d", rank, tag,
		     comm->context_id + context_offset);
    
    sreq = MPIDI_CH3_Request_create();
    assert(sreq != NULL);
    sreq->ref_count = 2;
    sreq->kind = MPID_REQUEST_SEND;
    sreq->cc = 1;
    sreq->cc_ptr = &(sreq->cc);
    sreq->comm = comm;
    sreq->ch3.match.rank = rank;
    sreq->ch3.match.tag = tag;
    sreq->ch3.match.context_id = comm->context_id + context_offset;
    sreq->ch3.user_buf = (void *) buf;
    sreq->ch3.user_count = count;
    sreq->ch3.datatype = datatype;
    sreq->ch3.vc = comm->vcr[rank];
    
    if (count == 0)
    {
	    MPIDI_CH3_Pkt_t upkt;
	    MPIDI_CH3_Pkt_eager_send_t * const pkt = &upkt.eager_send;

	    sreq->ch3.ca = MPIDI_CH3_CA_COMPLETE;
	    
	    MPIDI_dbg_printf(15, FCNAME, "sending zero length message");
	    pkt->type = MPIDI_CH3_PKT_EAGER_SEND;
	    pkt->match.rank = comm->rank;
	    pkt->match.tag = tag;
	    pkt->match.context_id = comm->context_id + context_offset;
	    pkt->data_sz = 0;

	    MPIDI_CH3_iSend(comm->vcr[rank], sreq, pkt, sizeof(*pkt));

	    goto fn_exit;
    }
    
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

    /* TODO - flow control: limit number of outstanding eager messsages */
    
    if (count * dt_sz + sizeof(MPIDI_CH3_Pkt_eager_send_t) <=
	MPIDI_CH3_EAGER_MAX_MSG_SIZE)
    {
	sreq->ch3.ca = MPIDI_CH3_CA_COMPLETE;
	
	/* eager message send */
	if (dt_contig)
	{
	    MPIDI_CH3_Pkt_t upkt;
	    MPIDI_CH3_Pkt_eager_send_t * const pkt = &upkt.eager_send;
	    struct iovec iov[2];

	    pkt->type = MPIDI_CH3_PKT_EAGER_SEND;
	    pkt->match.rank = comm->rank;
	    pkt->match.tag = tag;
	    pkt->match.context_id = comm->context_id + context_offset;
	    pkt->data_sz = count * dt_sz;

	    MPIDI_dbg_printf(15, FCNAME, "sending eager contiguous message, "
			     "data_sz=%ld", pkt->data_sz);
	    iov[0].iov_base = pkt;
	    iov[0].iov_len = sizeof(*pkt);
	    iov[1].iov_base = (void *) buf;
	    iov[1].iov_len = count * dt_sz;
	    MPIDI_CH3_iSendv(comm->vcr[rank], sreq, iov, 2);
	}
	else
	{
	    assert(dt_contig);
	    abort();
	}
    }
    else
    {
	/* rendezvous protocol */
	MPIDI_CH3_Pkt_t upkt;
	MPIDI_CH3_Pkt_rndv_req_to_send_t * const pkt = &upkt.rndv_req_to_send;
	    
	sreq->ch3.ca = MPIDI_CH3_CA_NONE;
	
	pkt->type = MPIDI_CH3_PKT_RNDV_REQ_TO_SEND;
	pkt->match.rank = comm->rank;
	pkt->match.tag = tag;
	pkt->match.context_id = comm->context_id + context_offset;
	pkt->data_sz = count * dt_sz;
	pkt->req_id_sender = sreq->handle;

	MPIDI_CH3_iSend(comm->vcr[rank], sreq, pkt, sizeof(*pkt));
    }

  fn_exit:
    MPIDI_dbg_printf(10, FCNAME, "exiting");
    *request = sreq;
    return MPI_SUCCESS;
}

