/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "ch3i_progress.h"

#undef FUNCNAME
#define FUNCNAME handle_sock_op
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int handle_sock_op(sock_event_t *event_ptr)
{
    int rc, mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_HANDLE_SOCK_OP);

    MPIDI_FUNC_ENTER(MPID_STATE_HANDLE_SOCK_OP);
    switch (event_ptr->op_type)
    {
    case SOCK_OP_READ:
	{
	    MPIDI_CH3I_Connection_t * conn = (MPIDI_CH3I_Connection_t *) event_ptr->user_ptr;

	    MPID_Request * rreq = conn->recv_active;

	    if (event_ptr->error != SOCK_SUCCESS)
	    {
		if (!shutting_down || event_ptr->error != SOCK_EOF)  /* FIXME: this should be handled by the close protocol */
		{
		    connection_recv_fail(conn, event_ptr->error);
		}

		break;
	    }

	    if (conn->recv_active)
	    {
		conn->recv_active = NULL;
		/* decrement the number of active reads */
		MPIDI_CH3I_sock_read_active--;
		MPIDI_CH3U_Handle_recv_req(conn->vc, rreq);
		if (conn->recv_active == NULL)
		{ 
		    connection_post_recv_pkt(conn);
		}
	    }
	    else /* incoming packet header */
	    {

		if (conn->pkt.type < MPIDI_CH3_PKT_END_CH3)
		{
		    conn->recv_active = NULL;
		    MPIDI_CH3U_Handle_recv_pkt(conn->vc, &conn->pkt);
		    if (conn->recv_active == NULL)
		    { 
			connection_post_recv_pkt(conn);
		    }
		}
		else if (conn->pkt.type == MPIDI_CH3I_PKT_SC_OPEN_REQ)
		{
		    int pg_id;
		    int pg_rank;
		    MPIDI_VC * vc;

		    pg_id = conn->pkt.sc_open_req.pg_id;
		    pg_rank = conn->pkt.sc_open_req.pg_rank;
		    vc = &MPIDI_CH3I_Process.pg->vc_table[pg_rank]; /* FIXME: need to lookup process group from pg_id */
		    assert(vc->ssm.pg_rank == pg_rank);

		    if (vc->ssm.conn == NULL || MPIR_Process.comm_world->rank < pg_rank)
		    {
			vc->ssm.state = MPIDI_CH3I_VC_STATE_CONNECTING;
			vc->ssm.sock = conn->sock;
			vc->ssm.conn = conn;
			conn->vc = vc;

			conn->pkt.type = MPIDI_CH3I_PKT_SC_OPEN_RESP;
			conn->pkt.sc_open_resp.ack = TRUE;
		    }
		    else
		    {
			conn->pkt.type = MPIDI_CH3I_PKT_SC_OPEN_RESP;
			conn->pkt.sc_open_resp.ack = FALSE;
		    }

		    conn->state = CONN_STATE_OPEN_LSEND;
		    connection_post_send_pkt(conn);

		}
		else if (conn->pkt.type == MPIDI_CH3I_PKT_SC_OPEN_RESP)
		{
		    if (conn->pkt.sc_open_resp.ack)
		    {
			conn->state = CONN_STATE_CONNECTED;
			conn->vc->ssm.state = MPIDI_CH3I_VC_STATE_CONNECTED;
			assert(conn->vc->ssm.conn == conn);
			assert(conn->vc->ssm.sock == conn->sock);

			connection_post_recv_pkt(conn);
			connection_post_sendq_req(conn);
		    }
		    else
		    {
			conn->vc = NULL;
			conn->state = CONN_STATE_CLOSING;
			sock_post_close(conn->sock);
		    }
		}
		else
		{
		    MPIDI_DBG_Print_packet(&conn->pkt);
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN, "**badpacket", "**badpacket %d",
			conn->pkt.type);
		    return mpi_errno;
		}
	    }

	    break;
	}

    case SOCK_OP_WRITE:
	{
	    MPIDI_CH3I_Connection_t * conn = (MPIDI_CH3I_Connection_t *) event_ptr->user_ptr;

	    if (event_ptr->error != SOCK_SUCCESS)
	    {
		connection_send_fail(conn, event_ptr->error);
		break;
	    }

	    if (conn->send_active)
	    {
		MPID_Request * sreq = conn->send_active;

		conn->send_active = NULL;
		MPIDI_CH3U_Handle_send_req(conn->vc, sreq);
		if (conn->send_active == NULL)
		{
		    if (MPIDI_CH3I_SendQ_head(conn->vc) == sreq)
		    {
			MPIDI_CH3I_SendQ_dequeue(conn->vc);
		    }
		    connection_post_sendq_req(conn);
		}
	    }
	    else /* finished writing internal packet header */
	    {
		if (conn->state == CONN_STATE_OPEN_CSEND)
		{
		    /* finished sending open request packet */
		    /* post receive for open response packet */
		    conn->state = CONN_STATE_OPEN_CRECV;
		    connection_post_recv_pkt(conn);
		}
		else if (conn->state == CONN_STATE_OPEN_LSEND)
		{
		    /* finished sending open response packet */
		    if (conn->pkt.sc_open_resp.ack == TRUE)
		    { 
			/* post receive for packet header */
			conn->state = CONN_STATE_CONNECTED;
			conn->vc->ssm.state = MPIDI_CH3I_VC_STATE_CONNECTED;
			connection_post_recv_pkt(conn);
			connection_post_sendq_req(conn);
		    }
		    else
		    {
			/* head-to-head connections - close this connection */
			conn->state = CONN_STATE_CLOSING;
			rc = sock_post_close(conn->sock);
			assert(rc == SOCK_SUCCESS);
		    }
		}
	    }

	    break;
	}

    case SOCK_OP_ACCEPT:
	{
	    MPIDI_CH3I_Connection_t * conn;

	    conn = connection_alloc();
	    if (conn == NULL)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
		return mpi_errno;
	    }
	    rc = sock_accept(listener_conn->sock, sock_set, conn, &conn->sock);
	    if (rc == SOCK_SUCCESS)
	    { 
		conn->vc = NULL;
		conn->state = CONN_STATE_OPEN_LRECV;
		conn->send_active = NULL;
		conn->recv_active = NULL;

		connection_post_recv_pkt(conn);
	    }
	    else
	    {
		connection_free(conn);
	    }

	    break;
	}

    case SOCK_OP_CONNECT:
	{
	    MPIDI_CH3I_Connection_t * conn = (MPIDI_CH3I_Connection_t *) event_ptr->user_ptr;

	    if (event_ptr->error != SOCK_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**connfailed", "**connfailed %d %d",
		    /* FIXME: pgid */ -1, conn->vc->ssm.pg_rank);
		    return mpi_errno;

	    }

	    conn->state = CONN_STATE_OPEN_CSEND;
	    conn->pkt.type = MPIDI_CH3I_PKT_SC_OPEN_REQ;
	    conn->pkt.sc_open_req.pg_id = -1; /* FIXME: multiple process groups may exist */
	    conn->pkt.sc_open_req.pg_rank = MPIR_Process.comm_world->rank;
	    connection_post_send_pkt(conn);

	    break;
	}

    case SOCK_OP_CLOSE:
	{
	    MPIDI_CH3I_Connection_t * conn = (MPIDI_CH3I_Connection_t *) event_ptr->user_ptr;

	    /* If the conn pointer is NULL then the close was intentional */
	    if (conn != NULL)
	    {
		if (conn->state == CONN_STATE_CLOSING)
		{
		    assert(conn->send_active == NULL);
		    assert(conn->recv_active == NULL);
		    conn->sock = SOCK_INVALID_SOCK;
		    conn->state = CONN_STATE_CLOSED;
		    connection_free(conn);
		}
		else if (conn->state == CONN_STATE_LISTENING && shutting_down)
		{
		    assert(listener_conn == conn);
		    connection_free(listener_conn);
		    listener_conn = NULL;
		    listener_port = 0;
		}
		else
		{ 
		    /* FIXME: an error has occurred */
		}
	    }
	    break;
	}
    }
    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_SOCK_OP);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SSM_VC_post_read
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SSM_VC_post_read(MPIDI_VC * vc, MPID_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_Connection_t * conn = vc->ssm.conn;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_VC_POST_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_VC_POST_READ);
    MPIDI_DBG_PRINTF((60, FCNAME, "entering, vc=%08p, rreq=0x%08x", vc, rreq->handle));

    assert (conn->recv_active == NULL);
    conn->recv_active = rreq;
    sock_post_readv(conn->sock, rreq->ch3.iov + rreq->ssm.iov_offset, rreq->ch3.iov_count - rreq->ssm.iov_offset, NULL);

    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_READ);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SSM_VC_post_write
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SSM_VC_post_write(MPIDI_VC * vc, MPID_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_Connection_t * conn = vc->ssm.conn;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_VC_POST_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_VC_POST_WRITE);
    MPIDI_DBG_PRINTF((60, FCNAME, "entering, vc=%08p, sreq=0x%08x", vc, sreq->handle));

    assert (conn->send_active == NULL);
    assert (!vc->ssm.bShm);

    conn->send_active = sreq;
    sock_post_writev(conn->sock, sreq->ch3.iov + sreq->ssm.iov_offset, sreq->ch3.iov_count - sreq->ssm.iov_offset, NULL);

    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_WRITE);
    return mpi_errno;
}
