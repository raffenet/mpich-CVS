/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "ch3i_progress.h"

/*
 * MPIDI_CH3I_Request_adjust_iov()
 *
 * Adjust the iovec in the request by the supplied number of bytes.  If the iovec has been consumed, return true; otherwise return
 * false.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Request_adjust_iov
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Request_adjust_iov(MPID_Request * req, MPIDI_msg_sz_t nb)
{
    int offset = req->ssm.iov_offset;
    const int count = req->ch3.iov_count;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_REQUEST_ADJUST_IOV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_REQUEST_ADJUST_IOV);
    
    while (offset < count)
    {
	if (req->ch3.iov[offset].MPID_IOV_LEN <= (unsigned int)nb)
	{
	    nb -= req->ch3.iov[offset].MPID_IOV_LEN;
	    offset++;
	}
	else
	{
	    (char *) req->ch3.iov[offset].MPID_IOV_BUF += nb;
	    req->ch3.iov[offset].MPID_IOV_LEN -= nb;
	    req->ssm.iov_offset = offset;
	    MPIDI_DBG_PRINTF((60, FCNAME, "adjust_iov returning FALSE"));
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_REQUEST_ADJUST_IOV);
	    return FALSE;
	}
    }
    
    req->ssm.iov_offset = offset;

    MPIDI_DBG_PRINTF((60, FCNAME, "adjust_iov returning TRUE"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_REQUEST_ADJUST_IOV);
    return TRUE;
}

#undef FUNCNAME
#define FUNCNAME handle_shm_read
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void handle_shm_read(MPIDI_VC *vc, int nb)
{
    MPID_Request * req;
    MPIDI_STATE_DECL(MPID_STATE_HANDLE_SHM_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_HANDLE_SHM_READ);
    
    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));

    req = vc->ssm.recv_active;
    if (req == NULL)
    {
	MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
	MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_SHM_READ);
	return;
    }

    if (nb > 0)
    {
	if (MPIDI_CH3I_Request_adjust_iov(req, nb))
	{
	    /* Read operation complete */
	    MPIDI_CA_t ca = req->ch3.ca;
	    
	    vc->ssm.recv_active = NULL;

	    if (ca == MPIDI_CH3_CA_COMPLETE)
	    {
		MPIDI_DBG_PRINTF((65, FCNAME, "received requested data, decrementing CC"));
		/* mark data transfer as complete adn decrment CC */
		req->ch3.iov_count = 0;
		MPIDI_CH3U_Request_complete(req);
		vc->ssm.shm_reading_pkt = TRUE;
		MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
		MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_SHM_READ);
		return;
	    }
	    else if (ca < MPIDI_CH3_CA_END_CH3)
	    {
		/* XXX - This code assumes that if another read is not posted by the device during the callback, then the
		   device is not expecting any more data for request.  As a result, the channels posts a read for another
		   packet */
		MPIDI_DBG_PRINTF((65, FCNAME, "finished receiving iovec, calling CH3U_Handle_recv_req()"));
		/* decrement the number of active reads */
		MPIDI_CH3I_shm_read_active--;
		MPIDI_CH3U_Handle_recv_req(vc, req);
		if (vc->ssm.recv_active == NULL)
		{
		    MPIDI_DBG_PRINTF((65, FCNAME, "request (assumed) complete, posting new recv packet"));
		    vc->ssm.shm_reading_pkt = TRUE;
		    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
		    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_SHM_READ);
		    return;
		}
	    }
	    else
	    {
		assert(ca != MPIDI_CH3I_CA_HANDLE_PKT);
		assert(ca < MPIDI_CH3_CA_END_CH3);
	    }
	}
	else
	{
	    assert(req->ssm.iov_offset < req->ch3.iov_count);
	    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
	    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_SHM_READ);
	    return;
	}
    }
    else
    {
	MPIDI_DBG_PRINTF((65, FCNAME, "Read args were iov=%x, count=%d",
	    req->ch3.iov + req->ssm.iov_offset, req->ch3.iov_count - req->ssm.iov_offset));
    }
    
    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_SHM_READ);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_write_progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_write_progress(MPIDI_VC * vc)
{
    int error;
    int nb;
    int total = 0;
    MPIDI_STATE_DECL(MPID_STATE_HANDLE_SHM_WRITTEN);

    MPIDI_FUNC_ENTER(MPID_STATE_HANDLE_SHM_WRITTEN);
    
    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));
    while (vc->ssm.send_active != NULL)
    {
	MPID_Request * req = vc->ssm.send_active;

	/*
	if (req->ssm.iov_offset >= req->ch3.iov_count)
	{
	    MPIDI_DBG_PRINTF((60, FCNAME, "iov_offset(%d) >= iov_count(%d)", req->ssm.iov_offset, req->ch3.iov_count));
	}
	*/
	assert(req->ssm.iov_offset < req->ch3.iov_count);
	/* Check here or inside shm_writev?
	if (vc->ssm.write_shmq->packet[vc->ssm.write_shmq->tail_index].avail == MPIDI_CH3I_PKT_EMPTY)
	    error = MPIDI_CH3I_SHM_writev(vc, req->ch3.iov + req->ssm.iov_offset, req->ch3.iov_count - req->ssm.iov_offset, &nb);
	else
	    nb = 0;
	*/
	error = MPIDI_CH3I_SHM_writev(vc, req->ch3.iov + req->ssm.iov_offset, req->ch3.iov_count - req->ssm.iov_offset, &nb);

	if (nb > 0)
	{
	    MPIDI_DBG_PRINTF((60, FCNAME, "shm_writev returned %d", nb));
	    total += nb;
	    if (MPIDI_CH3I_Request_adjust_iov(req, nb))
	    {
		/* Write operation complete */
		MPIDI_CA_t ca = req->ch3.ca;
			
		vc->ssm.send_active = NULL;
		
		if (ca == MPIDI_CH3_CA_COMPLETE)
		{
		    MPIDI_DBG_PRINTF((65, FCNAME, "sent requested data, decrementing CC"));
		    MPIDI_CH3I_SendQ_dequeue(vc);
		    vc->ssm.send_active = MPIDI_CH3I_SendQ_head(vc);
		    /* mark data transfer as complete and decrment CC */
		    req->ch3.iov_count = 0;
		    MPIDI_CH3U_Request_complete(req);
		}
		else if (ca == MPIDI_CH3I_CA_HANDLE_PKT)
		{
		    MPIDI_CH3_Pkt_t * pkt = &req->ssm.pkt;
		    
		    if (pkt->type < MPIDI_CH3_PKT_END_CH3)
		    {
			MPIDI_DBG_PRINTF((65, FCNAME, "setting ssm.send_active"));
			vc->ssm.send_active = MPIDI_CH3I_SendQ_head(vc);
		    }
		    else
		    {
			MPIDI_DBG_PRINTF((71, FCNAME, "unknown packet type %d", pkt->type));
		    }
		}
		else if (ca < MPIDI_CH3_CA_END_CH3)
		{
		    MPIDI_DBG_PRINTF((65, FCNAME, "finished sending iovec, calling CH3U_Handle_send_req()"));
		    MPIDI_CH3U_Handle_send_req(vc, req);
		    if (vc->ssm.send_active == NULL)
		    {
			/* NOTE: This code assumes that if another write is not posted by the device during the callback, then the
			   device has completed the current request.  As a result, the current request is dequeded and next request
			   in the queue is processed. */
			MPIDI_DBG_PRINTF((65, FCNAME, "request (assumed) complete"));
			MPIDI_DBG_PRINTF((65, FCNAME, "dequeuing req and posting next send"));
			MPIDI_CH3I_SendQ_dequeue(vc);
			vc->ssm.send_active = MPIDI_CH3I_SendQ_head(vc);
		    }
		}
		else
		{
		    MPIDI_DBG_PRINTF((65, FCNAME, "ca = %d", ca));
		    assert(ca < MPIDI_CH3I_CA_END_SSM_CHANNEL);
		}
	    }
	    else
	    {
		MPIDI_DBG_PRINTF((65, FCNAME, "iovec updated by %d bytes but not complete", nb));
		assert(req->ssm.iov_offset < req->ch3.iov_count);
		break;
	    }
	}
	else
	{
#ifdef MPICH_DBG_OUTPUT
	    if (nb != 0)
	    {
		MPIDI_DBG_PRINTF((65, FCNAME, "shm_writev returned %d bytes", nb));
	    }
#endif
	    break;
	}
    }

    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));

    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_SHM_WRITTEN);
    return total;
}
