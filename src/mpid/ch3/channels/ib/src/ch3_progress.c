/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"

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
    /*poll_infos[elem].recv_active = req;*/
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_IB_POST_READ);
}

void MPIDI_CH3I_IB_post_write(MPIDI_VC * vc, MPID_Request * req)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_IB_POST_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_IB_POST_WRITE);
    /*poll_infos[elem].send_active = req;*/
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
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_REQUEST_ADJUST_IOV);
	    return FALSE;
	}
    }
    
    req->ib.iov_offset = offset;

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_REQUEST_ADJUST_IOV);
    return TRUE;
}

#if 0
#undef FUNCNAME
#undef FCNAME
static inline void post_pkt_send(int elem)
{
    MPIDI_STATE_DECL(MPID_STATE_POST_PKT_SEND);

    MPIDI_FUNC_ENTER(MPID_STATE_POST_PKT_SEND);
    poll_infos[elem].req.ch3.iov[0].MPID_IOV_BUF =
	(char *)&poll_infos[elem].req.tcp.pkt;
    poll_infos[elem].req.ch3.iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
    poll_infos[elem].req.ch3.iov_count = 1;
    poll_infos[elem].req.tcp.iov_offset = 0;
    poll_infos[elem].req.ch3.ca = MPIDI_CH3I_CA_HANDLE_PKT;
    poll_infos[elem].send_active = &poll_infos[elem].req;
    poll_fds[elem].events |= POLLOUT;
    /* TODO: try sending pkt immediately - if we are already processing sends for this connection, then the send will be attempted
       as soon as we return to handle_pollout() */
    MPIDI_FUNC_EXIT(MPID_STATE_POST_PKT_SEND);
}

static inline void post_pkt_recv(int elem)
{
    MPIDI_STATE_DECL(MPID_STATE_POST_PKT_RECV);

    MPIDI_FUNC_ENTER(MPID_STATE_POST_PKT_RECV);
    poll_infos[elem].req.ch3.iov[0].MPID_IOV_BUF =
	(char *)&poll_infos[elem].req.tcp.pkt;
    poll_infos[elem].req.ch3.iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
    poll_infos[elem].req.ch3.iov_count = 1;
    poll_infos[elem].req.tcp.iov_offset = 0;
    poll_infos[elem].req.ch3.ca = MPIDI_CH3I_CA_HANDLE_PKT;
    poll_infos[elem].recv_active = &poll_infos[elem].req;
    poll_fds[elem].events |= POLLIN;
    MPIDI_FUNC_EXIT(MPID_STATE_POST_PKT_RECV);
}

#undef FUNCNAME
#define FUNCNAME post_queued_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void post_queued_send(int elem)
{
    MPIDI_VC * vc = poll_infos[elem].vc;
    MPIDI_STATE_DECL(MPID_STATE_POST_QUEUED_SEND);

    MPIDI_FUNC_ENTER(MPID_STATE_POST_QUEUED_SEND);
    
    assert(vc != NULL);
    poll_infos[elem].send_active = MPIDI_CH3I_SendQ_head(vc); /* MT */
    if (poll_infos[elem].send_active != NULL)
    {
	MPIDI_DBG_PRINTF((75, FCNAME, "elem=%d, queued message, send active", elem));
	poll_fds[elem].events |= POLLOUT;
    }
    else
    {
	MPIDI_DBG_PRINTF((75, FCNAME, "elem=%d, queue empty, send deactivated", elem));
	poll_fds[elem].events &= ~POLLOUT;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_POST_QUEUED_SEND);
}

#undef FUNCNAME
#define FUNCNAME handle_error
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static void handle_error(int elem, MPID_Request * req)
{
    MPIDI_VC * vc = poll_infos[elem].vc;
    MPIDI_STATE_DECL(MPID_STATE_HANDLE_ERROR);

    MPIDI_FUNC_ENTER(MPID_STATE_HANDLE_ERROR);
    
    if (errno == ECONNRESET || errno == EPIPE)
    {
	/* FIXME: eof -- mark connection as closed.  if send requests are queued or any data receive requests are active then error
	   them out.  also, any specified source receive requests for this connection can be errored out. */
	if (poll_infos[elem].state != TCP_STATE_DISCONNECTING)
	{
	    DBGMSG((65, "unexpected close"));
		
	    poll_infos[elem].state = TCP_STATE_DISCONNECTING;
	}
    }
    else if (errno == EFAULT)
    {
	req->status.MPI_ERROR = MPI_ERR_BUFFER;
	/* FIXME: drain the rest of the message rather than tearing down
	   the connection */
	req->ch3.iov_count = 0;
	MPIDI_CH3U_Request_complete(req);
	poll_infos[elem].state = TCP_STATE_DISCONNECTING;
    }
    else
    {
	/* FIXME: recover from unknown problem */
	assert(errno == EWOULDBLOCK || errno == EAGAIN ||
	       errno == ENOMEM || errno == ECONNRESET || errno == EPIPE ||
	       errno == EFAULT);
	abort();
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_ERROR);
}


#undef FUNCNAME
#define FUNCNAME handle_pollin
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void handle_pollin(int elem)
{
    MPIDI_VC * vc = poll_infos[elem].vc;
    MPIDI_STATE_DECL(MPID_STATE_HANDLE_POLLIN);
    MPIDI_STATE_DECL(MPID_STATE_READV);

    MPIDI_FUNC_ENTER(MPID_STATE_HANDLE_POLLIN);
    
    MPIDI_DBG_PRINTF((60, FCNAME, "entering, elem=%d", elem));
    while (poll_infos[elem].recv_active != NULL)
    {
	MPID_Request * req = poll_infos[elem].recv_active;
	int nb;

	do
	{
	    assert(req->tcp.iov_offset < req->ch3.iov_count);
	    MPIDI_FUNC_ENTER(MPID_STATE_READV);
	    nb = readv(poll_fds[elem].fd, req->ch3.iov + req->tcp.iov_offset, req->ch3.iov_count - req->tcp.iov_offset);
	    MPIDI_FUNC_EXIT(MPID_STATE_READV);
	}
	while(nb == -1 && errno == EINTR);
		
	MPIDI_DBG_PRINTF((65, FCNAME, "read returned %d", nb));
			 
	if (nb > 0)
	{
	    if (MPIDI_CH3I_Request_adjust_iov(req, nb))
	    {
		/* Read operation complete */
		MPIDI_CA_t ca = req->ch3.ca;
			
		poll_fds[elem].events &= ~POLLIN;
		poll_infos[elem].recv_active = NULL;
			
		if (ca == MPIDI_CH3I_CA_HANDLE_PKT)
		{
		    MPIDI_CH3_Pkt_t * pkt = &req->tcp.pkt;
		    
		    if (pkt->type < MPIDI_CH3_PKT_END_CH3)
		    {
			MPIDI_DBG_PRINTF((65, FCNAME, "received CH3 packet %d, calllng CH3U_Handle_recv_pkt()", pkt->type));
			
			MPIDI_CH3U_Handle_recv_pkt(vc, pkt);
			if (poll_infos[elem].recv_active == NULL)
			{
			    DBGMSG((65, "complete; posting new recv packet"));
			    post_pkt_recv(elem);
			    break;
			}
		    }
		    else if (pkt->type == MPIDI_CH3I_PKT_TCP_OPEN_REQ)
		    {
			MPIDI_CH3I_Pkt_TCP_open_req_t * ipkt = &pkt->tcp_open_req;
			MPIDI_CH3I_Pkt_TCP_open_resp_t * opkt = &pkt->tcp_open_resp;
			MPIDI_VC * vc;
			int pg_rank;
    
			/* XXX - For now we assume that only one process group exists.  Eventually we need to find the correct
			   process group from ipkt->pg_id.  Once we have multiple process groups, rank is not the right value to
			   use in deciding which connection persists.  Some sort of global rank would work. A comparison of secret
			   (integer) keys published in PMI-KVS along with a backup voting algorithm if the keys are equal might
			   also work. */
			pg_rank = ipkt->pg_rank;
			assert(pg_rank < MPIDI_CH3I_Process.pg->size);
			vc = &MPIDI_CH3I_Process.pg->vc_table[pg_rank];
			assert(vc->tcp.pg_rank == pg_rank);
			DBGMSG((65, "received TCP_OPEN_REQ packet"));
			
			/* NOTE: opkt and ipkt point at the same memory! */
			opkt->type = MPIDI_CH3I_PKT_TCP_OPEN_RESP;
			if (vc->tcp.poll_elem < 0 ||
			    MPIR_Process.comm_world->rank < pg_rank)
			{
			    opkt->ack = TRUE;
			    vc->tcp.state = MPIDI_CH3I_VC_STATE_CONNECTING;
			    vc->tcp.poll_elem = elem;
			    vc->tcp.fd = poll_fds[elem].fd;
			    poll_infos[elem].vc = vc;
			}
			else
			{
			    opkt->ack = FALSE;
			}
			MPIDI_DBG_PRINTF((65, FCNAME, "sending TCP_OPEN_RESP packet, ack=%s, pg_rank=%d, vc=0x%08x, elem=%d",
					  opkt->ack ? "ACK" : "NAK", vc->tcp.pg_rank, (unsigned) vc, elem));
			post_pkt_send(elem);
		    }
		    else if (pkt->type == MPIDI_CH3I_PKT_TCP_OPEN_RESP)
		    {
			MPIDI_CH3I_Pkt_TCP_open_resp_t * ipkt = &pkt->tcp_open_resp;
			
			MPIDI_DBG_PRINTF((65, FCNAME, "received TCP_OPEN_RESP packet, ack=%s, pg_rank=%d, vc=0x%08x, elem=%d",
					  ipkt->ack ? "ACK" : "NAK", vc->tcp.pg_rank, (unsigned) vc, elem));
			if (ipkt->ack)
			{
			    vc->tcp.fd = poll_fds[elem].fd;
			    DBGMSG((65, "changing state to CONNECTED"));
			    poll_infos[elem].state = TCP_STATE_CONNECTED;
			    vc->tcp.state = MPIDI_CH3I_VC_STATE_CONNECTED;
			    post_pkt_recv(elem);
			    post_queued_send(elem);
			}
			else
			{
			    /* close connection and free poll element - actual work done by make_progress() */
			    DBGMSG((65, "changing state to DISCONNECTING"));
			    poll_infos[elem].state = TCP_STATE_DISCONNECTING;
			}
		    }
		}
		else if (ca == MPIDI_CH3_CA_COMPLETE)
		{
		    MPIDI_DBG_PRINTF((65, FCNAME, "received requested data, decrementing CC"));
		    /* mark data transfer as complete adn decrment CC */
		    req->ch3.iov_count = 0;
		    MPIDI_CH3U_Request_complete(req);
		    post_pkt_recv(elem);
		    break;
		}
		else if (ca < MPIDI_CH3_CA_END_CH3)
		{
		    /* XXX - This code assumes that if another read is not posted by the device during the callback, then the
                       device is not expecting any more data for request.  As a result, the channels posts a read for another
                       packet */
		    MPIDI_DBG_PRINTF((65, FCNAME, "finished receiving iovec, calling CH3U_Handle_recv_req()"));
		    MPIDI_CH3U_Handle_recv_req(vc, req);
		    if (poll_infos[elem].recv_active == NULL)
		    {
			DBGMSG((65, "request (assumed) complete"));
			DBGMSG((65, "posting new recv packet"));
			post_pkt_recv(elem);
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
		assert(req->tcp.iov_offset < req->ch3.iov_count);
		break;
	    }
	}
	else if (nb == 0) 
	{
	    errno = EPIPE;
	    handle_error(elem, req);
	    break;
	}
	
	else if (errno == EWOULDBLOCK || errno == EAGAIN || errno == ENOMEM)
	{
	    break;
	}
	else
	{
	    MPIDI_DBG_PRINTF((65, FCNAME, "Read args were elem=%d, fd=%d, iov=%x, count=%d\n", elem, poll_fds[elem].fd,
			      req->ch3.iov + req->tcp.iov_offset, req->ch3.iov_count - req->tcp.iov_offset));
	    handle_error(elem, req);
	    break;
	}
    }

    if (poll_infos[elem].state == TCP_STATE_LISTENING)
    {
	/* Incoming connection on listener socket -- only the listener should have NULL vc and recv_active elements */
	int fd;
	struct sockaddr_in addr;
	socklen_t addr_len;

	addr_len = sizeof(struct sockaddr_in);
	fd = accept(poll_fds[elem].fd, (struct sockaddr *) &addr, &addr_len);
	if (fd >= 0)
	{
	    long flags;
	    int nodelay;
	    int rc;
	    
	    flags = fcntl(fd, F_GETFL, 0);
	    assert(flags != -1);
	    rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	    assert(rc != -1);
	    
	    nodelay = 1;
	    rc = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay,
			    sizeof(nodelay));
	    assert(rc == 0);
    
	    /* If a new socket has been formed, then we must wait for a description packet from the connecting side (so we know who
	       he is).  This involves allocating and initializing a new polling element and receive request. */
	    elem = poll_elem_alloc(fd);
	    assert(elem >= 0);
	    poll_infos[elem].vc = NULL;
	    poll_infos[elem].state = TCP_STATE_OPEN_EXCHANGE;
	    poll_infos[elem].send_active = NULL;
	    poll_infos[elem].recv_active = NULL;
	    poll_fds[elem].events = 0;

	    MPIDI_DBG_PRINTF((65, FCNAME, "new connection accepted, elem=%d", elem));
	    /* post a receive for a TCP_OPEN_REQ packet */
	    MPIDI_DBG_PRINTF((65, FCNAME, "posting receive for TCP_OPEN_REQ, elem=%d", elem));
	    post_pkt_recv(elem);
	}
	else
	{
	    assert(errno == EAGAIN || errno == EWOULDBLOCK || errno == ECONNABORTED || errno == EPROTO || errno == EINTR);
	}
    }

    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));

    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_POLLIN);
}

#undef FUNCNAME
#define FUNCNAME handle_pollout
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void handle_pollout(int elem)
{
    MPIDI_VC * vc = poll_infos[elem].vc;
    MPIDI_STATE_DECL(MPID_STATE_HANDLE_POLLOUT);
    MPIDI_STATE_DECL(MPID_STATE_WRITEV);

    MPIDI_FUNC_ENTER(MPID_STATE_HANDLE_POLLOUT);
    
    MPIDI_DBG_PRINTF((60, FCNAME, "entering, elem=%d", elem));
    while(poll_infos[elem].send_active != NULL)
    {
	MPID_Request * req = poll_infos[elem].send_active;
	int nb;
		
	do
	{
	    assert(req->tcp.iov_offset < req->ch3.iov_count);
	    MPIDI_FUNC_ENTER(MPID_STATE_WRITEV);
	    nb = writev(poll_fds[elem].fd, req->ch3.iov + req->tcp.iov_offset, req->ch3.iov_count - req->tcp.iov_offset);
	    MPIDI_FUNC_EXIT(MPID_STATE_WRITEV);
	}
	while(nb == -1 && errno == EINTR);

	MPIDI_DBG_PRINTF((65, FCNAME, "wrote %d bytes", nb));
	
	if (nb > 0)
	{
	    if (MPIDI_CH3I_Request_adjust_iov(req, nb))
	    {
		/* Write operation complete */
		MPIDI_CA_t ca = req->ch3.ca;
			
		poll_fds[elem].events &= ~POLLOUT;
		poll_infos[elem].send_active = NULL;
		
		if (ca == MPIDI_CH3_CA_COMPLETE)
		{
		    DBGMSG((65, "sent requested data, decrementing CC"));
		    MPIDI_CH3I_SendQ_dequeue(vc);
		    post_queued_send(elem);
		    /* mark data transfer as complete adn decrment CC */
		    req->ch3.iov_count = 0;
		    MPIDI_CH3U_Request_complete(req);
		}
		else if (ca == MPIDI_CH3I_CA_HANDLE_PKT)
		{
		    MPIDI_CH3_Pkt_t * pkt = &req->tcp.pkt;
		    
		    if (pkt->type < MPIDI_CH3_PKT_END_CH3)
		    {
			post_queued_send(elem);
		    }
		    else if (pkt->type == MPIDI_CH3I_PKT_TCP_OPEN_REQ)
		    {
			DBGMSG((65, "finished sending TCP_OPEN_REQ"));
			DBGMSG((65, "posting receive for TCP_OPEN_RESP"));
			post_pkt_recv(elem);
		    }
		    else if (pkt->type == MPIDI_CH3I_PKT_TCP_OPEN_RESP)
		    {
			DBGMSG((65, "finished sending TCP_OPEN_RESP"));
			if (pkt->tcp_open_resp.ack == TRUE)
			{
			    DBGMSG((65, "changing state to CONNECTED"));
			    poll_infos[elem].state = TCP_STATE_CONNECTED;
			    vc->tcp.state = MPIDI_CH3I_VC_STATE_CONNECTED;
			    post_pkt_recv(elem);
			    post_queued_send(elem);
			}
			else
			{
			    /* close connection and free poll element - actual work done by make_progress() */
			    DBGMSG((65, "changing state to DISCONNECTING"));
			    poll_infos[elem].state = TCP_STATE_DISCONNECTING;
			}
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
		    if (poll_infos[elem].send_active == NULL)
		    {
			/* NOTE: This code assumes that if another write is not posted by the device during the callback, then the
			   device has completed the current request.  As a result, the current request is dequeded and next request
			   in the queue is processed. */
			DBGMSG((65, "request (assumed) complete"));
			DBGMSG((65, "dequeuing req and posting next send"));
			MPIDI_CH3I_SendQ_dequeue(vc);
			post_queued_send(elem);
		    }
		}
		else
		{
		    assert(ca < MPIDI_CH3I_CA_END_TCP);
		}
	    }
	    else
	    {
		assert(req->tcp.iov_offset < req->ch3.iov_count);
		break;
	    }
	}
	else
	{
	    assert(nb != 0);

	    handle_error(elem, req);
	    break;
	}
    }

    if (poll_infos[elem].state == TCP_STATE_CONNECTING)
    {
	struct sockaddr_in addr;
	socklen_t addr_len;
	int rc;

	poll_fds[elem].events &= ~POLLOUT;
	poll_infos[elem].state = TCP_STATE_OPEN_EXCHANGE;
	
	addr_len = sizeof(struct sockaddr_in);
	rc = getpeername(poll_fds[elem].fd, (struct sockaddr *) &addr, &addr_len);
	if (rc == 0)
	{
	    /* Connection established.  It's time to introduce ourselves. */
	    MPIDI_CH3I_Pkt_TCP_open_req_t * pkt = &poll_infos[elem].req.tcp.pkt.tcp_open_req;

	    DBGMSG((65, "connect() succeeded"));
	    DBGMSG((65, "sending TCP_OPEN_REQ packet"));
	    pkt->type = MPIDI_CH3I_PKT_TCP_OPEN_REQ;
	    pkt->pg_id = -1; /* FIXME: MPIDI_CH3I_Process.pg.??? */
	    pkt->pg_rank = MPIR_Process.comm_world->rank;
	    post_pkt_send(elem);
	}
	else
	{
	    /* FIXME: if getpeername() returns ENOTCONN, then we can now use
	    getsockopt() to find out what the errno associated with the
	    connect(). */
	    assert(errno == ENOTCONN);
	    assert(errno != ENOTCONN);
	}
    }

    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));

    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_POLLOUT);
}
#endif

#undef FUNCNAME
#define FUNCNAME make_progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void make_progress(int is_blocking)
{
    MPIDI_STATE_DECL(MPID_STATE_MAKE_PROGRESS);
    MPIDI_STATE_DECL(MPID_STATE_POLL);

    MPIDI_FUNC_ENTER(MPID_STATE_MAKE_PROGRESS);
/*
    handle_pollin(elem);
    handle_pollout(elem);
*/
    MPIDI_FUNC_EXIT(MPID_STATE_MAKE_PROGRESS);
}
