/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"

#define DBGMSG(a)

volatile unsigned int MPIDI_CH3I_progress_completions = 0;

static inline void make_progress(int is_blocking);

void MPIDI_CH3_Progress_start()
{
    /* MT - This function is empty for the single-threaded implementation */
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Progress(int is_blocking)
{
    unsigned register count;
    unsigned completions = MPIDI_CH3I_progress_completions;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS);

    MPIDI_DBG_PRINTF((50, FCNAME, "entering, blocking=%s", is_blocking ? "true" : "false"));
    do
    {
	make_progress(is_blocking);
    }
    while (completions == MPIDI_CH3I_progress_completions && is_blocking);

    count = MPIDI_CH3I_progress_completions - completions;
    MPIDI_DBG_PRINTF((50, FCNAME, "exiting, count=%d", count));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS);
    return count;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Progress_poke
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3_Progress_poke()
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS_POKE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS_POKE);
    make_progress(0);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS_POKE);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Progress_end
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3_Progress_end()
{
    /* MT - This function is empty for the single-threaded implementation */
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress_init()
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS_INIT);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS_INIT);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress_finalize()
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS_FINALIZE);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS_FINALIZE);
    return MPI_SUCCESS;
}

void MPIDI_CH3I_IB_post_read(MPIDI_VC * vc, MPID_Request * req)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_IB_POST_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_IB_POST_READ);
    MPIU_dbg_printf("ch3_ib_post_read\n");
    vc->ib.recv_active = req;
    ibu_post_readv(vc->ib.ibu, req->ch3.iov + req->ib.iov_offset, req->ch3.iov_count - req->ib.iov_offset, NULL);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_IB_POST_READ);
}

void MPIDI_CH3I_IB_post_write(MPIDI_VC * vc, MPID_Request * req)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_IB_POST_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_IB_POST_WRITE);
    MPIU_dbg_printf("ch3_ib_post_write\n");
    vc->ib.send_active = req;
    ibu_post_writev(vc->ib.ibu, req->ch3.iov + req->ib.iov_offset, req->ch3.iov_count - req->ib.iov_offset, NULL);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_IB_POST_WRITE);
}

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
int MPIDI_CH3I_Request_adjust_iov(MPID_Request * req, MPIDI_msg_sz_t nb)
{
    int offset = req->ib.iov_offset;
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
	    req->ib.iov_offset = offset;
	    MPIU_dbg_printf("adjust_iov returning FALSE\n");
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_REQUEST_ADJUST_IOV);
	    return FALSE;
	}
    }
    
    req->ib.iov_offset = offset;

    MPIU_dbg_printf("adjust_iov returning TRUE\n");
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_REQUEST_ADJUST_IOV);
    return TRUE;
}

static inline void post_pkt_recv(MPIDI_VC *vc)
{
    MPIDI_STATE_DECL(MPID_STATE_POST_PKT_RECV);

    MPIDI_FUNC_ENTER(MPID_STATE_POST_PKT_RECV);
    vc->ib.req->ch3.iov[0].MPID_IOV_BUF = (void *)&vc->ib.req->ib.pkt;
    vc->ib.req->ch3.iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
    vc->ib.req->ch3.iov_count = 1;
    vc->ib.req->ib.iov_offset = 0;
    vc->ib.req->ch3.ca = MPIDI_CH3I_CA_HANDLE_PKT;
    vc->ib.recv_active = vc->ib.req;
    MPIDI_FUNC_EXIT(MPID_STATE_POST_PKT_RECV);
}

#undef FUNCNAME
#define FUNCNAME post_queued_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void post_queued_send(MPIDI_VC * vc)
{
    MPIDI_STATE_DECL(MPID_STATE_POST_QUEUED_SEND);

    MPIDI_FUNC_ENTER(MPID_STATE_POST_QUEUED_SEND);
    
    assert(vc != NULL);
    vc->ib.send_active = MPIDI_CH3I_SendQ_head(vc); /* MT */
    if (vc->ib.send_active != NULL)
    {
	MPIDI_DBG_PRINTF((75, FCNAME, "queued message, send active"));
	assert(vc->ib.send_active->ib.iov_offset < vc->ib.send_active->ch3.iov_count);
	ibu_post_writev(vc->ib.ibu, vc->ib.send_active->ch3.iov + vc->ib.send_active->ib.iov_offset, vc->ib.send_active->ch3.iov_count - vc->ib.send_active->ib.iov_offset, NULL);
    }
    else
    {
	MPIDI_DBG_PRINTF((75, FCNAME, "queue empty, send deactivated"));
    }

    MPIDI_FUNC_EXIT(MPID_STATE_POST_QUEUED_SEND);
}

#undef FUNCNAME
#define FUNCNAME handle_read
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void handle_read(MPIDI_VC *vc, int nb)
{
    MPIDI_STATE_DECL(MPID_STATE_HANDLE_POLLIN);

    MPIDI_FUNC_ENTER(MPID_STATE_HANDLE_POLLIN);
    
    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));
    while (vc->ib.recv_active != NULL)
    {
	MPID_Request * req = vc->ib.recv_active;

	/*
	assert(req->ib.iov_offset < req->ch3.iov_count);
	nb = readv(poll_fds[elem].fd, req->ch3.iov + req->ib.iov_offset, req->ch3.iov_count - req->ib.iov_offset);
	*/

	MPIDI_DBG_PRINTF((65, FCNAME, "read returned %d", nb));
			 
	if (nb > 0)
	{
	    if (MPIDI_CH3I_Request_adjust_iov(req, nb))
	    {
		/* Read operation complete */
		MPIDI_CA_t ca = req->ch3.ca;
			
		vc->ib.recv_active = NULL;
			
		if (ca == MPIDI_CH3I_CA_HANDLE_PKT)
		{
		    MPIDI_CH3_Pkt_t * pkt = &req->ib.pkt;
		    
		    if (pkt->type < MPIDI_CH3_PKT_END_CH3)
		    {
			MPIDI_DBG_PRINTF((65, FCNAME, "received CH3 packet %d, calllng CH3U_Handle_recv_pkt()", pkt->type));
			
			MPIDI_CH3U_Handle_recv_pkt(vc, pkt);
			if (vc->ib.recv_active == NULL)
			{
			    DBGMSG((65, "complete; posting new recv packet"));
			    post_pkt_recv(vc);
			    break;
			}
		    }
		}
		else if (ca == MPIDI_CH3_CA_COMPLETE)
		{
		    MPIDI_DBG_PRINTF((65, FCNAME, "received requested data, decrementing CC"));
		    /* mark data transfer as complete adn decrment CC */
		    req->ch3.iov_count = 0;
		    MPIDI_CH3U_Request_complete(req);
		    post_pkt_recv(vc);
		    break;
		}
		else if (ca < MPIDI_CH3_CA_END_CH3)
		{
		    /* XXX - This code assumes that if another read is not posted by the device during the callback, then the
                       device is not expecting any more data for request.  As a result, the channels posts a read for another
                       packet */
		    MPIDI_DBG_PRINTF((65, FCNAME, "finished receiving iovec, calling CH3U_Handle_recv_req()"));
		    MPIDI_CH3U_Handle_recv_req(vc, req);
		    if (vc->ib.recv_active == NULL)
		    {
			DBGMSG((65, "request (assumed) complete"));
			DBGMSG((65, "posting new recv packet"));
			post_pkt_recv(vc);
			break;
		    }
		}
		else
		{
		    assert(ca < MPIDI_CH3_CA_END_CH3);
		}
	    }
	    else
	    {
		assert(req->ib.iov_offset < req->ch3.iov_count);
		break;
	    }
	}
	else if (nb == 0) 
	{
	    /*
	    errno = EPIPE;
	    handle_error(elem, req);
	    */
	    break;
	}
	/*
	else if (errno == EWOULDBLOCK || errno == EAGAIN || errno == ENOMEM)
	{
	    break;
	}
	*/
	else
	{
	    MPIDI_DBG_PRINTF((65, FCNAME, "Read args were iov=%x, count=%d\n",
			      req->ch3.iov + req->ib.iov_offset, req->ch3.iov_count - req->ib.iov_offset));
	    /*handle_error(elem, req);*/
	    break;
	}

	/* There is no active reading for infiniband so set the next num_bytes to zero */
	nb = 0;
    }

    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));

    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_POLLIN);
}

#undef FUNCNAME
#define FUNCNAME handle_written
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void handle_written(MPIDI_VC * vc, int nb)
{
    MPIDI_STATE_DECL(MPID_STATE_HANDLE_WRITTEN);

    MPIDI_FUNC_ENTER(MPID_STATE_HANDLE_WRITTEN);
    
    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));
    while (vc->ib.send_active != NULL)
    {
	MPID_Request * req = vc->ib.send_active;

	/*
	assert(req->ib.iov_offset < req->ch3.iov_count);
	nb = writev(poll_fds[elem].fd, req->ch3.iov + req->ib.iov_offset, req->ch3.iov_count - req->ib.iov_offset);
	*/

	if (nb > 0)
	{
	    if (MPIDI_CH3I_Request_adjust_iov(req, nb))
	    {
		/* Write operation complete */
		MPIDI_CA_t ca = req->ch3.ca;
			
		vc->ib.send_active = NULL;
		
		if (ca == MPIDI_CH3_CA_COMPLETE)
		{
		    DBGMSG((65, "sent requested data, decrementing CC"));
		    MPIDI_CH3I_SendQ_dequeue(vc);
		    post_queued_send(vc);
		    /* mark data transfer as complete and decrment CC */
		    req->ch3.iov_count = 0;
		    MPIDI_CH3U_Request_complete(req);
		}
		else if (ca == MPIDI_CH3I_CA_HANDLE_PKT)
		{
		    MPIDI_CH3_Pkt_t * pkt = &req->ib.pkt;
		    
		    if (pkt->type < MPIDI_CH3_PKT_END_CH3)
		    {
			post_queued_send(vc);
		    }
		    else
		    {
			MPIDI_DBG_PRINTF((71, FCNAME, "unknown packet type %d", pkt->type));
		    }
		}
		else if (ca < MPIDI_CH3_CA_END_CH3)
		{
		    DBGMSG((65, "finished sending iovec, calling CH3U_Handle_send_req()"));
		    MPIDI_CH3U_Handle_send_req(vc, req);
		    if (vc->ib.send_active == NULL)
		    {
			/* NOTE: This code assumes that if another write is not posted by the device during the callback, then the
			   device has completed the current request.  As a result, the current request is dequeded and next request
			   in the queue is processed. */
			DBGMSG((65, "request (assumed) complete"));
			DBGMSG((65, "dequeuing req and posting next send"));
			MPIDI_CH3I_SendQ_dequeue(vc);
			post_queued_send(vc);
		    }
		}
		else
		{
		    assert(ca < MPIDI_CH3I_CA_END_IB);
		}
	    }
	    else
	    {
		assert(req->ib.iov_offset < req->ch3.iov_count);
		break;
	    }
	}
	else
	{
	    /*assert(nb != 0);*/
	    /*handle_error(elem, req);*/
	    break;
	}
	nb = 0;
    }

    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));

    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_WRITTEN);
}

#undef FUNCNAME
#define FUNCNAME make_progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void make_progress(int is_blocking)
{
    ibu_wait_t out;
    int rc;
    MPIDI_STATE_DECL(MPID_STATE_MAKE_PROGRESS);
    MPIDI_STATE_DECL(MPID_STATE_POLL);

    MPIDI_FUNC_ENTER(MPID_STATE_MAKE_PROGRESS);

    do 
    {
	rc = ibu_wait(MPIDI_CH3I_Process.set, 0, &out);
	assert(rc != IBU_FAIL);
	switch (out.op_type)
	{
	case IBU_OP_TIMEOUT:
	    break;
	case IBU_OP_READ:
	    MPIU_dbg_printf("make_progress: read finished %d bytes\n", out.num_bytes);
	    handle_read(out.user_ptr, out.num_bytes);
	    return;
	    break;
	case IBU_OP_WRITE:
	    MPIU_dbg_printf("make_progress: write finished %d bytes\n", out.num_bytes);
	    handle_written(out.user_ptr, out.num_bytes);
	    return;
	    break;
	case IBU_OP_CLOSE:
	    break;
	default:
	    assert(FALSE);
	    break;
	}
    } while (is_blocking);

    MPIDI_FUNC_EXIT(MPID_STATE_MAKE_PROGRESS);
}
