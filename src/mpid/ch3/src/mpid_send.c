/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/* XXX - HOMOGENEOUS SYSTEMS ONLY -- no data conversion is performed */
int MPID_Send(const void * buf, int count, MPI_Datatype datatype,
	      int rank, int tag, MPID_Comm * comm, int context_offset,
	      MPID_Request ** request)
{
    int dt_sz;
    int dt_contig;
    MPID_Request * req;

    if (count == 0)
    {
	    MPIDI_CH3_Pkt_t rpkt;
	    MPIDI_CH3_Pkt_eager_send_t * const pkt = &(rpkt.eager_send);

	    pkt->type = MPIDI_CH3_PKT_EAGER_SEND;
	    pkt->match.context_id = comm->context_id + context_offset;
	    pkt->match.rank = rank;
	    pkt->match.tag = tag;
	    pkt->data_sz = 0;

	    req = MPIDI_CH3_iStartMsg(comm->vcr[rank], pkt, sizeof(rpkt));
	    if (req)
	    {
		req->comm = comm;
	    }

	    *request = req;
	    return MPI_SUCCESS;
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
	/* eager message send */
	if (dt_contig)
	{
	    MPIDI_CH3_Pkt_t rpkt;
	    MPIDI_CH3_Pkt_eager_send_t * const pkt = &(rpkt.eager_send);
	    struct iovec iov[2];

	    pkt->type = MPIDI_CH3_PKT_EAGER_SEND;
	    pkt->match.context_id = comm->context_id + context_offset;
	    pkt->match.rank = rank;
	    pkt->match.tag = tag;
	    pkt->data_sz = count * dt_sz;

	    iov[0].iov_base = pkt;
	    iov[0].iov_len = sizeof(rpkt);
	    iov[1].iov_base = (void *) buf;
	    iov[1].iov_len = count * dt_sz;
	    req = MPIDI_CH3_iStartMsgv(comm->vcr[rank], iov, 2);
	    if (req != NULL)
	    {
		/* XXX - is there a race condition here? */
		req->comm = comm;
		/* XXX - what other infor needs to go into the request? */
	    }
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
	MPIDI_CH3_Pkt_t rpkt;
	MPIDI_CH3_Pkt_rndv_req_to_send_t * const pkt =
	    &(rpkt.rndv_req_to_send);
	    
	req = MPIDI_CH3_Request_create();
	assert(req != NULL);
	req->cc = 1;
	req->cc_ptr = &(req->cc);
	req->comm = comm;
	req->ch3.match.tag = tag;
	req->ch3.match.rank = rank;
	req->ch3.match.context_id = comm->context_id + context_offset;
	req->ch3.user_buf = (void *) buf;
	req->ch3.user_count = count;
	req->ch3.datatype = datatype;
	req->ch3.vc = comm->vcr[rank];
	req->ch3.ca = MPIDI_CH3_CA_NONE;
	
	/* XXX - what other information needs to go into the request? */
	
	pkt->type = MPIDI_CH3_PKT_RNDV_REQ_TO_SEND;
	pkt->match.tag = tag;
	pkt->match.rank = rank;
	pkt->match.context_id = comm->context_id + context_offset;
	pkt->data_sz = count * dt_sz;
	pkt->req_id_sender = req->handle;

	MPIDI_CH3_iSend(comm->vcr[rank], req, pkt, sizeof(rpkt));
    }
    
    *request = req;
    return MPI_SUCCESS;
}
