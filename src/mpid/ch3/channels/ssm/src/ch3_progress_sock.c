/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "ch3i_progress.h"

#undef FUNCNAME
#define FUNCNAME connection_post_send_pkt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void connection_post_send_pkt_and_pgid(MPIDI_CH3I_Connection_t * conn)
{
    int rc;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_POST_SEND_PKT_AND_PGID);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_POST_SEND_PKT_AND_PGID);
    
    conn->iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) &conn->pkt;
    conn->iov[0].MPID_IOV_LEN = (int) sizeof(conn->pkt);

    conn->iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) MPIDI_CH3I_Process.pg->pg_id;
    conn->iov[1].MPID_IOV_LEN = (int) strlen(MPIDI_CH3I_Process.pg->pg_id) + 1;

    rc = MPIDU_Sock_post_writev(conn->sock, conn->iov, 2, NULL);
    if (rc != MPI_SUCCESS)
    {
	connection_send_fail(conn, rc);
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_POST_SEND_PKT_AND_PGID);
}


#undef FUNCNAME
#define FUNCNAME handle_sock_op
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int handle_sock_op(MPIDU_Sock_event_t *event_ptr)
{
    int mpi_errno;
    int complete;
    MPIDI_STATE_DECL(MPID_STATE_HANDLE_SOCK_OP);

    MPIDI_FUNC_ENTER(MPID_STATE_HANDLE_SOCK_OP);
    switch (event_ptr->op_type)
    {
    case MPIDU_SOCK_OP_READ:
	{
	    MPIDI_CH3I_Connection_t * conn = (MPIDI_CH3I_Connection_t *) event_ptr->user_ptr;

	    MPID_Request * rreq = conn->recv_active;

	    if (event_ptr->error != MPIDU_SOCK_SUCCESS)
	    {
		if (!shutting_down || MPIR_ERR_GET_CLASS(event_ptr->error) != MPIDU_SOCK_ERR_CONN_CLOSED /*event_ptr->error != SOCK_EOF*/)  /* FIXME: this should be handled by the close protocol */
		{
		    connection_recv_fail(conn, event_ptr->error);
		}

		break;
	    }

	    if (conn->recv_active)
	    {
		/* decrement the number of active reads */
		MPIDI_CH3I_sock_read_active--;
		mpi_errno = MPIDI_CH3U_Handle_recv_req(conn->vc, rreq, &complete);
		if (mpi_errno != MPI_SUCCESS)
		{
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS);
		    return mpi_errno;
		}
		if (complete)
		{ 
		    conn->recv_active = NULL;
		    connection_post_recv_pkt(conn);
		}
		else
		{
		    mpi_errno = MPIDU_Sock_post_readv(conn->sock, rreq->dev.iov, rreq->dev.iov_count, NULL);
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
			    "**ch3|sock|postread", "ch3|sock|postread %p %p %p",
			    rreq, conn, conn->vc);
		    }
		}
	    }
	    else if (conn->state == CONN_STATE_OPEN_LRECV_DATA)
	    {
		MPIDI_CH3I_Process_group_t * pg;
		int pg_rank;
		MPIDI_VC * vc;

		/* Look up pg based on conn->pg_id */

		pg = MPIDI_CH3I_Process.pg;
		while (strcmp(pg->pg_id, conn->pg_id))
		{
		    pg = pg->next;
		    if (pg == NULL)
		    {
			mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pglookup", "**pglookup %s", conn->pg_id);
			MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS);
			return mpi_errno;
		    }
		}

		pg_rank = conn->pkt.sc_open_req.pg_rank;
		vc = &(pg->vc_table[pg_rank]);
		assert(vc->ch.pg_rank == pg_rank);

		if (vc->ch.conn == NULL)
		{
		    /* no head-to-head connects, accept the
		    connection */
		    vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTING;
		    vc->ch.sock = conn->sock;
		    vc->ch.conn = conn;
		    conn->vc = vc;

		    conn->pkt.type = MPIDI_CH3I_PKT_SC_OPEN_RESP;
		    conn->pkt.sc_open_resp.ack = TRUE;
		}
		else
		{
		    /* head to head situation */
		    if (pg == MPIDI_CH3I_Process.pg)
		    {
			/* the other process is in the same
			comm_world; just compare the ranks */
			if (MPIR_Process.comm_world->rank < pg_rank)
			{
			    /* accept connection */
			    vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTING;
			    vc->ch.sock = conn->sock;
			    vc->ch.conn = conn;
			    conn->vc = vc;

			    conn->pkt.type = MPIDI_CH3I_PKT_SC_OPEN_RESP;
			    conn->pkt.sc_open_resp.ack = TRUE;
			}
			else
			{
			    /* refuse connection */
			    conn->pkt.type = MPIDI_CH3I_PKT_SC_OPEN_RESP;
			    conn->pkt.sc_open_resp.ack = FALSE;
			}
		    }
		    else
		    { 
			/* the two processes are in different
			comm_worlds; compare their unique
			pg_ids. */
			if (strcmp(MPIDI_CH3I_Process.pg->pg_id, pg->pg_id) < 0)
			{
			    /* accept connection */
			    vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTING;
			    vc->ch.sock = conn->sock;
			    vc->ch.conn = conn;
			    conn->vc = vc;

			    conn->pkt.type = MPIDI_CH3I_PKT_SC_OPEN_RESP;
			    conn->pkt.sc_open_resp.ack = TRUE;
			}
			else
			{
			    /* refuse connection */
			    conn->pkt.type = MPIDI_CH3I_PKT_SC_OPEN_RESP;
			    conn->pkt.sc_open_resp.ack = FALSE;
			}
		    }
		}

		conn->state = CONN_STATE_OPEN_LSEND;
		connection_post_send_pkt(conn);
	    }
	    else /* incoming packet header */
	    {
		if (conn->pkt.type < MPIDI_CH3_PKT_END_CH3)
		{
		    mpi_errno = MPIDI_CH3U_Handle_recv_pkt(conn->vc, &conn->pkt, &rreq);
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
			MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS);
			return mpi_errno;
		    }
		    if (rreq == NULL)
		    {
			conn->recv_active = NULL;
			connection_post_recv_pkt(conn);
		    }
		    else
		    {
			conn->recv_active = rreq;
			mpi_errno = MPIDU_Sock_post_readv(conn->sock, rreq->dev.iov, rreq->dev.iov_count, NULL);
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
				"**ch3|sock|postread", "ch3|sock|postread %p %p %p",
				rreq, conn, conn->vc);
			}
		    }
		}
		else if (conn->pkt.type == MPIDI_CH3I_PKT_SC_OPEN_REQ)
		{
		    int rc;

		    conn->state = CONN_STATE_OPEN_LRECV_DATA;
		    rc = MPIDU_Sock_post_read(conn->sock,
			conn->pg_id,
			conn->pkt.sc_open_req.pg_id_len, 
			conn->pkt.sc_open_req.pg_id_len, NULL);   
		    if (rc != MPI_SUCCESS)
		    {
			connection_recv_fail(conn, rc);
		    }
		}
		else if (conn->pkt.type == MPIDI_CH3I_PKT_SC_CONN_ACCEPT)
		{
		    MPIDI_VC *vc; 

		    vc = (MPIDI_VC *) MPIU_Malloc(sizeof(MPIDI_VC));
		    if (vc == NULL)
		    {
			mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
			goto fn_exit;
		    }
		    /* FIXME - where does this vc get freed? */

		    vc->ch.pg = NULL;
		    vc->ch.pg_rank = 0;
		    vc->ch.sendq_head = NULL;
		    vc->ch.sendq_tail = NULL;

		    vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTING;
		    vc->ch.sock = conn->sock;
		    vc->ch.conn = conn;
		    conn->vc = vc;

		    conn->pkt.type = MPIDI_CH3I_PKT_SC_OPEN_RESP;
		    conn->pkt.sc_open_resp.ack = TRUE;

		    conn->state = CONN_STATE_OPEN_LSEND;
		    connection_post_send_pkt(conn);

		    /* ENQUEUE vc */
		    MPIDI_CH3I_Acceptq_enqueue(vc);
		}
		else if (conn->pkt.type == MPIDI_CH3I_PKT_SC_OPEN_RESP)
		{
		    if (conn->pkt.sc_open_resp.ack)
		    {
			conn->state = CONN_STATE_CONNECTED;
			conn->vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTED;
#ifdef MPICH_DBG_OUTPUT
			/*
			assert(conn->vc->ch.conn == conn);
			assert(conn->vc->ch.sock == conn->sock);
			*/
			if (conn->vc->ch.conn != conn)
			{
			    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**bad_conn", "**bad_conn %p %p", conn->vc->ch.conn, conn);
			    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_SOCK_OP);
			    return mpi_errno;
			}
			if (conn->vc->ch.sock != conn->sock)
			{
			    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**bad_sock", "**bad_sock %d %d",
				MPIDU_Sock_get_sock_id(conn->vc->ch.sock), MPIDU_Sock_get_sock_id(conn->sock));
			    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_SOCK_OP);
			    return mpi_errno;
			}
#endif

			connection_post_recv_pkt(conn);
			connection_post_sendq_req(conn);
		    }
		    else
		    {
			conn->vc = NULL;
			conn->state = CONN_STATE_CLOSING;
			MPIDU_Sock_post_close(conn->sock);
		    }
		}
		else
		{
		    MPIDI_DBG_Print_packet(&conn->pkt);
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN, "**badpacket", "**badpacket %d",
			conn->pkt.type);
		    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_SOCK_OP);
		    return mpi_errno;
		}
	    }

	    break;
	}

    case MPIDU_SOCK_OP_WRITE:
	{
	    MPIDI_CH3I_Connection_t * conn = (MPIDI_CH3I_Connection_t *) event_ptr->user_ptr;

	    if (event_ptr->error != MPIDU_SOCK_SUCCESS)
	    {
		connection_send_fail(conn, event_ptr->error);
		break;
	    }

	    if (conn->send_active)
	    {
		MPID_Request * sreq = conn->send_active;

		mpi_errno = MPIDI_CH3U_Handle_send_req(conn->vc, sreq, &complete);
		if (mpi_errno != MPI_SUCCESS)
		{
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS);
		    return mpi_errno;
		}
		if (complete)
		{
		    MPIDI_CH3I_SendQ_dequeue(conn->vc);
		    connection_post_sendq_req(conn);
		}
		else
		{
		    MPIDI_DBG_PRINTF((55, FCNAME, "posting writev, vc=0x%p, sreq=0x%08x", conn->vc, sreq->handle));
		    mpi_errno = MPIDU_Sock_post_writev(conn->sock, sreq->dev.iov, sreq->dev.iov_count, NULL);
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
			    "**ch3|sock|postwrite", "ch3|sock|postwrite %p %p %p",
			    sreq, conn, conn->vc);
			MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS);
			return mpi_errno;
		    }
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
			conn->vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTED;
			connection_post_recv_pkt(conn);
			connection_post_sendq_req(conn);
		    }
		    else
		    {
			/* head-to-head connections - close this connection */
			conn->state = CONN_STATE_CLOSING;
			mpi_errno = MPIDU_Sock_post_close(conn->sock);
			/*assert(rc == SOCK_SUCCESS);*/
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**sock_post_close", 0);
			    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_SOCK_OP);
			    return mpi_errno;
			}
		    }
		}
	    }

	    break;
	}

    case MPIDU_SOCK_OP_ACCEPT:
	{
	    MPIDI_CH3I_Connection_t * conn;

	    conn = connection_alloc();
	    if (conn == NULL)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
		MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_SOCK_OP);
		return mpi_errno;
	    }
	    mpi_errno = MPIDU_Sock_accept(listener_conn->sock, sock_set, conn, &conn->sock);
	    if (mpi_errno == MPI_SUCCESS)
	    { 
		conn->vc = NULL;
		conn->state = CONN_STATE_OPEN_LRECV_PKT;
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

    case MPIDU_SOCK_OP_CONNECT:
	{
	    MPIDI_CH3I_Connection_t * conn = (MPIDI_CH3I_Connection_t *) event_ptr->user_ptr;

	    if (event_ptr->error != MPIDU_SOCK_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**connfailed", "**connfailed %d %d",
		    /* FIXME: pgid */ -1, conn->vc->ch.pg_rank);
		MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_SOCK_OP);
		return mpi_errno;
	    }

	    if (conn->state == CONN_STATE_CONNECTING)
	    {
		conn->state = CONN_STATE_OPEN_CSEND;
		conn->pkt.type = MPIDI_CH3I_PKT_SC_OPEN_REQ;
		conn->pkt.sc_open_req.pg_id_len = (int)
		    strlen(MPIDI_CH3I_Process.pg->pg_id) + 1;
		conn->pkt.sc_open_req.pg_rank = MPIR_Process.comm_world->rank;

		connection_post_send_pkt_and_pgid(conn);
	    }
	    else
	    {  /* CONN_STATE_CONNECT_ACCEPT */
		conn->state = CONN_STATE_OPEN_CSEND;
		conn->pkt.type = MPIDI_CH3I_PKT_SC_CONN_ACCEPT;
		/* pkt contains nothing */
		connection_post_send_pkt(conn);
	    }

	    break;
	}

    case MPIDU_SOCK_OP_CLOSE:
	{
	    MPIDI_CH3I_Connection_t * conn = (MPIDI_CH3I_Connection_t *) event_ptr->user_ptr;

	    /* If the conn pointer is NULL then the close was intentional */
	    if (conn != NULL)
	    {
		if (conn->state == CONN_STATE_CLOSING)
		{
#ifdef MPICH_DBG_OUTPUT
		    /*
		    assert(conn->send_active == NULL);
		    assert(conn->recv_active == NULL);
		    */
		    if (conn->send_active != NULL || conn->recv_active != NULL)
		    {
			mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**conn_still_active", 0);
			MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_SOCK_OP);
			return mpi_errno;
		    }
#endif
		    conn->sock = MPIDU_SOCK_INVALID_SOCK;
		    conn->state = CONN_STATE_CLOSED;
		    connection_free(conn);
		}
		else if (conn->state == CONN_STATE_LISTENING && shutting_down)
		{
#ifdef MPICH_DBG_OUTPUT
		    /*assert(listener_conn == conn);*/
		    if (listener_conn != conn)
		    {
			mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**invalid_listener", "**invalid_listener %p", conn);
			MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_SOCK_OP);
			return mpi_errno;
		    }
#endif
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
fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_SOCK_OP);
    return MPI_SUCCESS;
}
