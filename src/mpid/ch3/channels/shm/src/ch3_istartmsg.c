/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/*static MPID_Request * create_request(void * hdr, int hdr_sz, int nb)*/
#undef create_request
#define create_request(sreq, hdr, hdr_sz, nb) \
{ \
    MPIDI_STATE_DECL(MPID_STATE_CREATE_REQUEST); \
    MPIDI_FUNC_ENTER(MPID_STATE_CREATE_REQUEST); \
    sreq = MPIDI_CH3_Request_create(); \
    assert(sreq != NULL); \
    MPIU_Object_set_ref(sreq, 2); \
    sreq->kind = MPID_REQUEST_SEND; \
    assert(hdr_sz == sizeof(MPIDI_CH3_Pkt_t)); \
    sreq->shm.pkt = *(MPIDI_CH3_Pkt_t *) hdr; \
    sreq->ch3.iov[0].MPID_IOV_BUF = (char *) &sreq->shm.pkt + nb; \
    sreq->ch3.iov[0].MPID_IOV_LEN = hdr_sz - nb; \
    sreq->ch3.iov_count = 1; \
    sreq->shm.iov_offset = 0; \
    sreq->ch3.ca = MPIDI_CH3_CA_COMPLETE; \
    MPIDI_FUNC_EXIT(MPID_STATE_CREATE_REQUEST); \
}

/*
 * MPIDI_CH3_iStartMsg() attempts to send the message immediately.  If the
 * entire message is successfully sent, then NULL is returned.  Otherwise a
 * request is allocated, the header is copied into the request, and a pointer
 * to the request is returned.  An error condition also results in a request be
 * allocated and the errror being returned in the status field of the
 * request.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iStartMsg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
MPID_Request * MPIDI_CH3_iStartMsg(MPIDI_VC * vc, void * hdr, int hdr_sz)
{
    MPID_Request * sreq = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ISTARTMSG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ISTARTMSG);

    MPIU_DBG_PRINTF(("ch3_istartmsg\n"));
    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));
    assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));

    /* The TCP implementation uses a fixed length header, the size of which is
       the maximum of all possible packet headers */
    hdr_sz = sizeof(MPIDI_CH3_Pkt_t);
    
    /* Connection already formed.  If send queue is empty attempt to send
       data, queuing any unsent data. */
    if (MPIDI_CH3I_SendQ_empty(vc)) /* MT */
    {
	int nb;
	
	/* MT - need some signalling to lock down our right to use the
	   channel, thus insuring that the progress engine does also try to
	   write */
	
	//nb = MPIDI_CH3I_SHM_write(vc->shm.shm, hdr, hdr_sz);
	nb = MPIDI_CH3I_SHM_write(vc, hdr, hdr_sz);
	
	if (nb == hdr_sz)
	{
	    MPIDI_DBG_PRINTF((55, FCNAME, "data sent immediately"));
	    /* done.  get us out of here as quickly as possible. */
	}
	else if (nb >= 0)
	{
	    MPIDI_DBG_PRINTF((55, FCNAME,
		"send delayed, request enqueued"));
	    create_request(sreq, hdr, hdr_sz, nb);
	    MPIDI_CH3I_SendQ_enqueue_head(vc, sreq);
	    vc->shm.send_active = sreq;
	}
	else
	{
	    MPIDI_DBG_PRINTF((55, FCNAME, "ERROR - connection failed, "
		"errno=%d:%s", errno, strerror(errno)));
	    sreq = MPIDI_CH3_Request_create();
	    assert(sreq != NULL);
	    sreq->kind = MPID_REQUEST_SEND;
	    sreq->cc = 0;
	    /* TODO: Create an appropriate error message based on the value
	             of errno */
	    sreq->status.MPI_ERROR = MPI_ERR_INTERN;
	}
    }
    else
    {
	MPIDI_DBG_PRINTF((55, FCNAME, "send in progress, request enqueued"));
	create_request(sreq, hdr, hdr_sz, 0);
	MPIDI_CH3I_SendQ_enqueue(vc, sreq);
    }
    
    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISTARTMSG);
    return sreq;
}
