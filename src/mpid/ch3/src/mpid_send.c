/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/* XXX - HOMOGENEOUS SYSTEMS ONLY -- no data conversion is performed */

#undef FUNCNAME
#define FUNCNAME MPID_Send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Send(const void * buf, int count, MPI_Datatype datatype,
	      int rank, int tag, MPID_Comm * comm, int context_offset,
	      MPID_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    long dt_sz;
    long data_sz;
    MPID_Request * sreq;

    MPIDI_DBG_PRINTF((10, FCNAME, "entering"));
    MPIDI_DBG_PRINTF((15, FCNAME, "rank=%d, tag=%d, context=%d", rank, tag,
		      comm->context_id + context_offset));
    
    MPID_Datatype_get_size_macro(datatype, dt_sz);
    data_sz = count * dt_sz;
    MPIDI_DBG_PRINTF((10, FCNAME, "dt_sz=%ld", dt_sz));

    if (data_sz == 0)
    {
	MPIDI_CH3_Pkt_t upkt;
	MPIDI_CH3_Pkt_eager_send_t * const eager_pkt = &upkt.eager_send;

	MPIDI_DBG_PRINTF((15, FCNAME, "sending zero length message"));
	eager_pkt->type = MPIDI_CH3_PKT_EAGER_SEND;
	eager_pkt->match.rank = comm->rank;
	eager_pkt->match.tag = tag;
	eager_pkt->match.context_id = comm->context_id + context_offset;
	eager_pkt->sender_req_id = MPI_REQUEST_NULL;
	eager_pkt->data_sz = 0;

	sreq = MPIDI_CH3_iStartMsg(
	    comm->vcr[rank], eager_pkt, sizeof(*eager_pkt));
	if (sreq != NULL)
	{
	    sreq->comm = comm;
	}
	
	goto fn_exit;
    }
    
    /* TODO - flow control: limit number of outstanding eager messsages that
       contain data which may need to be buffered by the receiver */

    /* TODO - handle case where data_sz is greater than what can be stored in
       iov.MPID_IOV_LEN.  probably just hand off to segment code. */
    
    if (data_sz + sizeof(MPIDI_CH3_Pkt_eager_send_t) <=
	MPIDI_CH3_EAGER_MAX_MSG_SIZE)
    {
	MPIDI_CH3_Pkt_t upkt;
	MPIDI_CH3_Pkt_eager_send_t * const eager_pkt = &upkt.eager_send;
	MPID_IOV iov[MPID_IOV_LIMIT];
	    
	eager_pkt->type = MPIDI_CH3_PKT_EAGER_SEND;
	eager_pkt->match.rank = comm->rank;
	eager_pkt->match.tag = tag;
	eager_pkt->match.context_id = comm->context_id + context_offset;
	eager_pkt->sender_req_id = MPI_REQUEST_NULL;
	eager_pkt->data_sz = data_sz;

	iov[0].MPID_IOV_BUF = eager_pkt;
	iov[0].MPID_IOV_LEN = sizeof(*eager_pkt);
	
	if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN)
	{
	    MPIDI_DBG_PRINTF((15, FCNAME, "sending eager contiguous message, "
			      "data_sz=%ld", eager_pkt->data_sz));
	    iov[1].MPID_IOV_BUF = (void *) buf;
	    iov[1].MPID_IOV_LEN = data_sz;
	    sreq = MPIDI_CH3_iStartMsgv(comm->vcr[rank], iov, 2);
	    if (sreq != NULL)
	    {
		/* XXX - what else needs to be set? */
		sreq->comm = comm;
	    }
	}
	else
	{
	    MPID_Segment seg;
	    int last;
	    int iov_n;
	    
	    /* two choices here: fill iovec or pack message into a buffer.  in
	       either case, if we have more data to send, we need to use a
	       completion operation that takes care of the rest of the
	       data. */
	
	    MPID_Segment_init(buf, count, datatype, &seg);
	    iov_n = MPID_IOV_LIMIT - 1;
	    last = data_sz;
	    MPID_Segment_pack_vector(&seg, 0, &last, &iov[1], &iov_n);
	    assert(iov_n > 0);
	    assert(last > 0);
	    
	    iov_n += 1;
	    
	    if (last == data_sz)
	    {
		/* XXX - should the above expression also contain "&& last /
                   iov_n >= MPIDI_IOV_DENSITY_MIN"?  */
		
		/* send entire message using IOV */
		MPIDI_DBG_PRINTF((15, FCNAME, "sending eager non-contiguous "
				  "message using single IOV, data_sz=%ld, "
				  "iov_n=%d", eager_pkt->data_sz, iov_n));
		sreq = MPIDI_CH3_iStartMsgv(comm->vcr[rank], iov, iov_n);
		if (sreq != NULL)
		{
		    /* XXX - what else needs to be set? */
		    sreq->comm = comm;
		}
	    }
	    else
	    {
		sreq = MPIDI_CH3_Request_create();
		assert(sreq != NULL);
		sreq->ref_count = 2;
		sreq->kind = MPID_REQUEST_SEND;
		sreq->comm = comm;
		sreq->ch3.match.rank = rank;
		sreq->ch3.match.tag = tag;
		sreq->ch3.match.context_id = comm->context_id + context_offset;
		sreq->ch3.user_buf = (void *) buf;
		sreq->ch3.user_count = count;
		sreq->ch3.datatype = datatype;
		sreq->ch3.vc = comm->vcr[rank];

		if (last / (iov_n - 1) >= MPIDI_IOV_DENSITY_MIN)
		{
		    /* send partial message using IOV */
		    MPIDI_DBG_PRINTF((15, FCNAME,
				      "sending eager non-contiguous message"
				      "using multiple IOVs, data_sz=%ld, "
				      "iov_n=%d, iov_sz=%d",
				      eager_pkt->data_sz, iov_n, last));
		    sreq->ch3.segment = seg;
		    sreq->ch3.segment_first = last;
		    sreq->ch3.segment_size = data_sz;
		    sreq->ch3.ca = MPIDI_CH3_CA_RELOAD_IOV;
		}
		else
		{
		    /* allocate temporary buffer and pack */
		    MPIDI_CH3U_SRBuf_alloc(&sreq->ch3.tmp_buf, data_sz,
					   &sreq->ch3.tmp_sz);
		    assert (sreq->ch3.tmp_sz > 0);
		    MPIDI_Request_set_tmpbuf_flag(sreq, TRUE);

		    last = (data_sz <= sreq->ch3.tmp_sz) ? data_sz :
			    sreq->ch3.tmp_sz;
		    MPID_Segment_pack(&seg, 0, &last, sreq->ch3.tmp_buf);
		    iov[1].MPID_IOV_BUF = sreq->ch3.tmp_buf;
		    iov[1].MPID_IOV_LEN = last;
		    iov_n = 2;
		    sreq->ch3.segment = seg;
		    MPIDI_DBG_PRINTF((15, FCNAME,
				      "sending eager non-contiguous message"
				      "using packed buffer, data_sz=%ld, "
				      "pack_sz=%d", eager_pkt->data_sz, last));

		    if (last == data_sz)
		    {
			sreq->ch3.ca = MPIDI_CH3_CA_COMPLETE;
		    }
		    else 
		    {
			sreq->ch3.segment = seg;
			sreq->ch3.segment_first = last;
			sreq->ch3.segment_size = data_sz;
			sreq->ch3.ca = MPIDI_CH3_CA_RELOAD_IOV;
		    }
			
		}
		
		MPIDI_CH3_iSendv(comm->vcr[rank], sreq, iov, iov_n);
	    }
	}
    }
    else
    {
	MPIDI_CH3_Pkt_t upkt;
	MPIDI_CH3_Pkt_rndv_req_to_send_t * const rts_pkt =
	    &upkt.rndv_req_to_send;
	
	sreq = MPIDI_CH3_Request_create();
	assert(sreq != NULL);
	sreq->ref_count = 2;
	sreq->kind = MPID_REQUEST_SEND;
	sreq->comm = comm;
	sreq->ch3.match.rank = rank;
	sreq->ch3.match.tag = tag;
	sreq->ch3.match.context_id = comm->context_id + context_offset;
	sreq->ch3.user_buf = (void *) buf;
	sreq->ch3.user_count = count;
	sreq->ch3.datatype = datatype;
	sreq->ch3.vc = comm->vcr[rank];
	sreq->ch3.ca = MPIDI_CH3_CA_NONE;
	
	/* XXX - Since the request is never returned to the user and they can't
           do things like cancel it or wait on it, we may not need to fill in
           all of the fields.  For example, it may be completely unnecessary to
           supply the matching information.  Also, some of the fields can be
           set after the message has been sent.  These issues should be looked
           at more closely when we are trying to squeeze those last few
           nanoseconds out of the code.  */
	
	rts_pkt->type = MPIDI_CH3_PKT_RNDV_REQ_TO_SEND;
	rts_pkt->match.rank = comm->rank;
	rts_pkt->match.tag = tag;
	rts_pkt->match.context_id = comm->context_id + context_offset;
	rts_pkt->sender_req_id = sreq->handle;
	rts_pkt->data_sz = data_sz;

	MPIDI_CH3_iSend(comm->vcr[rank], sreq, rts_pkt, sizeof(*rts_pkt));

	/* TODO: fill temporary IOV or pack temporary buffer after send to hide
           some latency */
    }

  fn_exit:
    *request = sreq;
    if (sreq)
    {
	MPIDI_DBG_PRINTF((15, FCNAME, "request allocated, handle=0x%08x",
			  sreq->handle));
    }
    else
    {
	MPIDI_DBG_PRINTF((15, FCNAME, "operation complete, no requests "
			  "allocated"));
    }
    MPIDI_DBG_PRINTF((10, FCNAME, "exiting"));
    return mpi_errno;
}
