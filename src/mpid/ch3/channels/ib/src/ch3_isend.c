/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

static void update_request(MPID_Request * sreq, void * hdr, int hdr_sz, int nb)
{
    MPIDI_STATE_DECL(MPID_STATE_UPDATE_REQUEST);

    MPIDI_FUNC_ENTER(MPID_STATE_UPDATE_REQUEST);
    /* memcpy(&sreq->ib.pkt, hdr, hdr_sz); */
    assert(hdr_sz == sizeof(MPIDI_CH3_Pkt_t));
    sreq->ib.pkt = *(MPIDI_CH3_Pkt_t *) hdr;
    sreq->ch3.iov[0].MPID_IOV_BUF = (char *) &sreq->ib.pkt + nb;
    sreq->ch3.iov[0].MPID_IOV_LEN = hdr_sz - nb;
    sreq->ch3.iov_count = 1;
    sreq->ib.iov_offset = 0;
    MPIDI_FUNC_EXIT(MPID_STATE_UPDATE_REQUEST);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iSend
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3_iSend(MPIDI_VC * vc, MPID_Request * sreq, void * hdr, int hdr_sz)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ISEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ISEND);

    MPIU_dbg_printf("ch3_isend\n");
    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));
    assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));

    /* The TCP implementation uses a fixed length header, the size of which is the maximum of all possible packet headers */
    hdr_sz = sizeof(MPIDI_CH3_Pkt_t);
    
    if (vc->ib.state == MPIDI_CH3I_VC_STATE_CONNECTED) /* MT */
    {
	/* Connection already formed.  If send queue is empty attempt to send data, queuing any unsent data. */
	if (MPIDI_CH3I_SendQ_empty(vc)) /* MT */
	{
	    int nb;

	    MPIDI_DBG_PRINTF((55, FCNAME, "send queue empty, attempting to write"));
	    
	    /* MT: need some signalling to lock down our right to use the channel, thus insuring that the progress engine does
               also try to write */
	    do
	    {
		MPIU_dbg_printf("ibu_post_write(%d bytes)\n", hdr_sz);
		ibu_post_write(vc->ib.ibu, hdr, hdr_sz, NULL);
		nb = 0;
	    }
	    while (nb == -1);
	    
	    MPIDI_DBG_PRINTF((55, FCNAME, "wrote %d bytes", nb));
		
	    if (nb == hdr_sz)
	    {
		MPIDI_DBG_PRINTF((55, FCNAME, "write complete, calling MPIDI_CH3U_Handle_send_req()"));
		MPIDI_CH3U_Handle_send_req(vc, sreq);
		if (sreq->ch3.iov_count != 0)
		{
		    /* NOTE: ch3.iov_count is used to detect completion instead of cc because the transfer may be complete, but the
		       request may still be active (see MPI_Ssend()) */
		    MPIDI_CH3I_SendQ_enqueue_head(vc, sreq);
		    MPIDI_CH3I_IB_post_write(vc, sreq);
		}
	    }
	    else if (nb < hdr_sz)
	    {
		MPIDI_DBG_PRINTF((55, FCNAME, "partial write, enqueuing at head"));
		update_request(sreq, hdr, hdr_sz, nb);
		MPIDI_CH3I_SendQ_enqueue_head(vc, sreq);
		MPIDI_CH3I_IB_post_write(vc, sreq);
	    }
	    else if (nb == 0)
	    {
		MPIDI_DBG_PRINTF((55, FCNAME, "unable to write, enqueuing"));
		update_request(sreq, hdr, hdr_sz, 0);
		MPIDI_CH3I_SendQ_enqueue(vc, sreq);
		MPIDI_CH3I_IB_post_write(vc, sreq);
	    }
	    else
	    {
		/* Connection just failed. Mark the request complete and return
                   an error. */
		vc->ib.state = MPIDI_CH3I_VC_STATE_FAILED;
		/* TODO: Create an appropriate error message based on the value
                   of errno */
		sreq->status.MPI_ERROR = MPI_ERR_INTERN;
		 /* MT -CH3U_Request_complete() performs write barrier */
		MPIDI_CH3U_Request_complete(sreq);
	    }
	}
	else
	{
	    MPIDI_DBG_PRINTF((55, FCNAME, "send queue not empty, enqueuing"));
	    update_request(sreq, hdr, hdr_sz, 0);
	    MPIDI_CH3I_SendQ_enqueue(vc, sreq);
	}
    }
    else
    {
	/* Connection failed.  Mark the request complete and return an error. */
	/* TODO: Create an appropriate error message */
	sreq->status.MPI_ERROR = MPI_ERR_INTERN;
	/* MT - CH3U_Request_complete() performs write barrier */
	MPIDI_CH3U_Request_complete(sreq);
    }
    
    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISEND);
}

