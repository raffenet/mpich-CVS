/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "ch3i_progress.h"

#undef FUNCNAME
#define FUNCNAME adjust_iov
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int adjust_iov(MPID_IOV ** iovp, int * countp, MPIU_Size_t nb)
{
    MPID_IOV * const iov = *iovp;
    const int count = *countp;
    int offset = 0;
    
    while (offset < count)
    {
	if (iov[offset].MPID_IOV_LEN <= nb)
	{
	    nb -= iov[offset].MPID_IOV_LEN;
	    offset++;
	}
	else
	{
	    iov[offset].MPID_IOV_BUF = (char *) iov[offset].MPID_IOV_BUF + nb;
	    iov[offset].MPID_IOV_LEN -= nb;
	    break;
	}
    }

    *iovp += offset;
    *countp -= offset;

    return (*countp == 0);
}

#undef FUNCNAME
#define FUNCNAME connection_post_send_pkt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void connection_post_send_pkt_and_pgid(MPIDI_CH3I_Connection_t * conn)
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_POST_SEND_PKT_AND_PGID);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_POST_SEND_PKT_AND_PGID);
    
    conn->iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) &conn->pkt;
    conn->iov[0].MPID_IOV_LEN = (int) sizeof(conn->pkt);

    conn->iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) MPIDI_Process.my_pg->id;
    conn->iov[1].MPID_IOV_LEN = (int) strlen(MPIDI_Process.my_pg->id) + 1;

    mpi_errno = MPIDU_Sock_post_writev(conn->sock, conn->iov, 2, NULL);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = connection_send_fail(conn, mpi_errno);
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_POST_SEND_PKT_AND_PGID);
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_handle_sock_event
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress_handle_sock_event(MPIDU_Sock_event_t * event)
{
    int complete;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_HANDLE_SOCK_EVENT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_HANDLE_SOCK_EVENT);

    switch (event->op_type)
    {
	case MPIDU_SOCK_OP_READ:
	{
	    MPIDI_CH3I_Connection_t * conn = (MPIDI_CH3I_Connection_t *) event->user_ptr;
		
	    MPID_Request * rreq = conn->recv_active;
		
	    if (event->error != MPI_SUCCESS)
	    {
		/* FIXME: the following should be handled by the close protocol */
		if (!shutting_down || MPIR_ERR_GET_CLASS(event->error) != MPIDU_SOCK_ERR_CONN_CLOSED)
		{
		    mpi_errno = connection_recv_fail(conn, event->error);
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
						     "**fail", NULL);
		    goto fn_exit;
		}
		    
		break;
	    }
		
	    if (conn->state == CONN_STATE_CONNECTED)
	    {
		if (conn->recv_active == NULL)
		{
		    MPIU_Assert(conn->pkt.type < MPIDI_CH3_PKT_END_CH3);
			
		    mpi_errno = MPIDI_CH3U_Handle_recv_pkt(conn->vc, &conn->pkt, &rreq);
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
							 "**fail", NULL);
			goto fn_exit;
		    }

		    if (rreq == NULL)
		    {
			if (conn->state != CONN_STATE_CLOSING)
			{
			    /* conn->recv_active = NULL;  -- already set to NULL */
			    mpi_errno = connection_post_recv_pkt(conn);
			    if (mpi_errno != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN,
				    "**fail", NULL);
				goto fn_exit;
			    }
			}
		    }
		    else
		    {
			for(;;)
			{
			    MPID_IOV * iovp;
			    MPIU_Size_t nb;
				
			    iovp = rreq->dev.iov;
			    
			    mpi_errno = MPIDU_Sock_readv(conn->sock, iovp, rreq->dev.iov_count, &nb);
			    if (mpi_errno != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
								 "**ch3|sock|immedread", "ch3|sock|immedread %p %p %p",
								 rreq, conn, conn->vc);
				goto fn_exit;
			    }

			    MPIDI_DBG_PRINTF((55, FCNAME, "immediate readv, vc=0x%p nb=%d, rreq=0x%08x",
					      conn->vc, rreq->handle, nb));
				
			    if (nb > 0 && adjust_iov(&iovp, &rreq->dev.iov_count, nb))
			    {
				mpi_errno = MPIDI_CH3U_Handle_recv_req(conn->vc, rreq, &complete);
				if (mpi_errno != MPI_SUCCESS)
				{
				    mpi_errno = MPIR_Err_create_code(
					mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", NULL);
				    goto fn_exit;
				}

				if (complete)
				{
				    /* conn->recv_active = NULL; -- already set to NULL */
				    mpi_errno = connection_post_recv_pkt(conn);
				    if (mpi_errno != MPI_SUCCESS)
				    {
					mpi_errno = MPIR_Err_create_code(
					    mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN, "**fail", NULL);
					goto fn_exit;
				    }

				    break;
				}
			    }
			    else
			    {
				MPIDI_DBG_PRINTF((55, FCNAME, "posting readv, vc=0x%p, rreq=0x%08x", conn->vc, rreq->handle));
				conn->recv_active = rreq;
				mpi_errno = MPIDU_Sock_post_readv(conn->sock, iovp, rreq->dev.iov_count, NULL);
				if (mpi_errno != MPI_SUCCESS)
				{
				    mpi_errno = MPIR_Err_create_code(
					mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|postread",
					"ch3|sock|postread %p %p %p", rreq, conn, conn->vc);
				    goto fn_exit;
				}

				break;
			    }
			}
		    }
		}
		else /* incoming data */
		{
		    mpi_errno = MPIDI_CH3U_Handle_recv_req(conn->vc, rreq, &complete);
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
							 "**fail", NULL);
			goto fn_exit;
		    }
			
		    if (complete)
		    {
			conn->recv_active = NULL;
			mpi_errno = connection_post_recv_pkt(conn);
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN,
							     "**fail", NULL);
			    goto fn_exit;
			}
		    }
		    else /* more data to be read */
		    {
			for(;;)
			{
			    MPID_IOV * iovp;
			    MPIU_Size_t nb;
				
			    iovp = rreq->dev.iov;
			    
			    mpi_errno = MPIDU_Sock_readv(conn->sock, iovp, rreq->dev.iov_count, &nb);
			    if (mpi_errno != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
								 "**ch3|sock|immedread", "ch3|sock|immedread %p %p %p",
								 rreq, conn, conn->vc);
				goto fn_exit;
			    }

			    MPIDI_DBG_PRINTF((55, FCNAME, "immediate readv, vc=0x%p nb=%d, rreq=0x%08x",
					      conn->vc, rreq->handle, nb));
				
			    if (nb > 0 && adjust_iov(&iovp, &rreq->dev.iov_count, nb))
			    {
				mpi_errno = MPIDI_CH3U_Handle_recv_req(conn->vc, rreq, &complete);
				if (mpi_errno != MPI_SUCCESS)
				{
				    mpi_errno = MPIR_Err_create_code(
					mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", NULL);
				    goto fn_exit;
				}

				if (complete)
				{
				    conn->recv_active = NULL;
				    mpi_errno = connection_post_recv_pkt(conn);
				    if (mpi_errno != MPI_SUCCESS)
				    {
					mpi_errno = MPIR_Err_create_code(
					    mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN, "**fail", NULL);
					goto fn_exit;
				    }

				    break;
				}
			    }
			    else
			    {
				MPIDI_DBG_PRINTF((55, FCNAME, "posting readv, vc=0x%p, rreq=0x%08x", conn->vc, rreq->handle));
				/* conn->recv_active = rreq;  -- already set to current request */
				mpi_errno = MPIDU_Sock_post_readv(conn->sock, iovp, rreq->dev.iov_count, NULL);
				if (mpi_errno != MPI_SUCCESS)
				{
				    mpi_errno = MPIR_Err_create_code(
					mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|postread",
					"ch3|sock|postread %p %p %p", rreq, conn, conn->vc);
				    goto fn_exit;
				}

				break;
			    }
			}
		    }
		}
	    }
	    else if (conn->state == CONN_STATE_OPEN_LRECV_DATA)
	    {
		MPIDI_PG_t * pg;
		int pg_rank;
		MPIDI_VC_t * vc;

		/* Look up pg based on conn->pg_id */
		mpi_errno = MPIDI_PG_Find(conn->pg_id, &pg);
		if (pg == NULL)
		{
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
						     "**pglookup", "**pglookup %s", conn->pg_id);
		    goto fn_exit;
		}

		pg_rank = conn->pkt.sc_open_req.pg_rank;
		MPIDI_PG_Get_vc(pg, pg_rank, &vc);
		MPIU_Assert(vc->pg_rank == pg_rank);
                    
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
		    if (pg == MPIDI_Process.my_pg)
		    {
			/* the other process is in the same comm_world; just compare the ranks */
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
			/* the two processes are in different comm_worlds; compare their unique pg_ids. */
			if (strcmp(MPIDI_Process.my_pg->id, pg->id) < 0)
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
		mpi_errno = connection_post_send_pkt(conn);
		if (mpi_errno != MPI_SUCCESS)
		{
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN,
						     "**ch3|sock|open_lrecv_data", NULL);
		    goto fn_exit;
		}
	    }
	    else /* Handling some internal connection establishment or tear down packet */
	    { 
		if (conn->pkt.type == MPIDI_CH3I_PKT_SC_OPEN_REQ)
		{
		    conn->state = CONN_STATE_OPEN_LRECV_DATA;
		    mpi_errno = MPIDU_Sock_post_read(conn->sock, conn->pg_id, conn->pkt.sc_open_req.pg_id_len, 
						     conn->pkt.sc_open_req.pg_id_len, NULL);   
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = connection_recv_fail(conn, mpi_errno);
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
							 "**fail", NULL);
			goto fn_exit;
		    }
		}
		else if (conn->pkt.type == MPIDI_CH3I_PKT_SC_CONN_ACCEPT)
		{
		    MPIDI_VC_t *vc; 

		    vc = (MPIDI_VC_t *) MPIU_Malloc(sizeof(MPIDI_VC_t));
		    if (vc == NULL)
		    {
			mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
							 "**nomem", NULL);
			goto fn_exit;
		    }
		    /* FIXME - where does this vc get freed? */

		    MPIDI_VC_Init(vc, NULL, 0);
		    vc->ch.sendq_head = NULL;
		    vc->ch.sendq_tail = NULL;
		    vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTING;
		    vc->ch.sock = conn->sock;
		    vc->ch.conn = conn;
		    conn->vc = vc;
                        
		    conn->pkt.type = MPIDI_CH3I_PKT_SC_OPEN_RESP;
		    conn->pkt.sc_open_resp.ack = TRUE;
                        
		    conn->state = CONN_STATE_OPEN_LSEND;
		    mpi_errno = connection_post_send_pkt(conn);
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN,
							 "**ch3|sock|scconnaccept", NULL);
			goto fn_exit;
		    }

		    /* ENQUEUE vc */
		    MPIDI_CH3I_Acceptq_enqueue(vc);

		}
		else if (conn->pkt.type == MPIDI_CH3I_PKT_SC_OPEN_RESP)
		{
		    if (conn->pkt.sc_open_resp.ack)
		    {
			conn->state = CONN_STATE_CONNECTED;
			conn->vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTED;
			assert(conn->vc->ch.conn == conn);
			assert(conn->vc->ch.sock == conn->sock);
			    
			mpi_errno = connection_post_recv_pkt(conn);
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN,
							     "**fail", NULL);
			    goto fn_exit;
			}
			mpi_errno = connection_post_sendq_req(conn);
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN,
							     "**ch3|sock|scopenresp", NULL);
			    goto fn_exit;
			}
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
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN,
						     "**ch3|sock|badpacket", "**ch3|sock|badpacket %d", conn->pkt.type);
		    goto fn_exit;
		}
	    }

	    break;
	}
	    
	case MPIDU_SOCK_OP_WRITE:
	{
	    MPIDI_CH3I_Connection_t * conn = (MPIDI_CH3I_Connection_t *) event->user_ptr;

	    if (event->error != MPI_SUCCESS)
	    {
		mpi_errno = connection_send_fail(conn, event->error);
		goto fn_exit;
	    }
		
	    if (conn->send_active)
	    {
		MPID_Request * sreq = conn->send_active;

		mpi_errno = MPIDI_CH3U_Handle_send_req(conn->vc, sreq, &complete);
		if (mpi_errno != MPI_SUCCESS)
		{
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
						     "**fail", NULL);
		    goto fn_exit;
		}
		    
		if (complete)
		{
		    MPIDI_CH3I_SendQ_dequeue(conn->vc);
		    mpi_errno = connection_post_sendq_req(conn);
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN,
							 "**fail", NULL);
			goto fn_exit;
		    }
		}
		else /* more data to send */
		{
		    for(;;)
		    {
			MPID_IOV * iovp;
			MPIU_Size_t nb;
				
			iovp = sreq->dev.iov;
			    
			mpi_errno = MPIDU_Sock_writev(conn->sock, iovp, sreq->dev.iov_count, &nb);
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
							     "**ch3|sock|immedwrite", "ch3|sock|immedwrite %p %p %p",
							     sreq, conn, conn->vc);
			    goto fn_exit;
			}

			MPIDI_DBG_PRINTF((55, FCNAME, "immediate writev, vc=0x%p, sreq=0x%08x, nb=%d",
					  conn->vc, sreq->handle, nb));
			    
			if (nb > 0 && adjust_iov(&iovp, &sreq->dev.iov_count, nb))
			{
			    mpi_errno = MPIDI_CH3U_Handle_send_req(conn->vc, sreq, &complete);
			    if (mpi_errno != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code(
				    mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", NULL);
				goto fn_exit;
			    }

			    if (complete)
			    {
				MPIDI_CH3I_SendQ_dequeue(conn->vc);
				mpi_errno = connection_post_sendq_req(conn);
				if (mpi_errno != MPI_SUCCESS)
				{
				    mpi_errno = MPIR_Err_create_code(
					mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN, "**fail", NULL);
				    goto fn_exit;
				}

				break;
			    }
			}
			else
			{
			    MPIDI_DBG_PRINTF((55, FCNAME, "posting writev, vc=0x%p, sreq=0x%08x", conn->vc, sreq->handle));
			    mpi_errno = MPIDU_Sock_post_writev(conn->sock, iovp, sreq->dev.iov_count, NULL);
			    if (mpi_errno != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code(
				    mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|postwrite",
				    "ch3|sock|postwrite %p %p %p", sreq, conn, conn->vc);
				goto fn_exit;
			    }

			    break;
			}
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
		    mpi_errno = connection_post_recv_pkt(conn);
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN,
							 "**fail", NULL);
			goto fn_exit;
		    }
		}
		else if (conn->state == CONN_STATE_OPEN_LSEND)
		{
		    /* finished sending open response packet */
		    if (conn->pkt.sc_open_resp.ack == TRUE)
		    { 
			/* post receive for packet header */
			conn->state = CONN_STATE_CONNECTED;
			conn->vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTED;
			mpi_errno = connection_post_recv_pkt(conn);
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN,
							     "**fail", NULL);
			    goto fn_exit;
			}
			
			mpi_errno = connection_post_sendq_req(conn);
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN,
							     "**ch3|sock|openlsend", NULL);
			    goto fn_exit;
			}
		    }
		    else
		    {
			/* head-to-head connections - close this connection */
			conn->state = CONN_STATE_CLOSING;
			mpi_errno = MPIDU_Sock_post_close(conn->sock);
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
							     "**sock_post_close", NULL);
			    goto fn_exit;
			}
		    }
		}
	    }

	    break;
	}
	    
	case MPIDU_SOCK_OP_ACCEPT:
	{
	    MPIDI_CH3I_Connection_t * conn;

	    mpi_errno = connection_alloc(&conn);
	    if (mpi_errno != MPI_SUCCESS)
	    { 
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
						 "**ch3|sock|accept", NULL);
		goto fn_exit;
	    }
	    mpi_errno = MPIDU_Sock_accept(MPIDI_CH3I_listener_conn->sock, sock_set, conn, &conn->sock);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
						 "**ch3|sock|accept", NULL);
		connection_free(conn);
		goto fn_exit;
	    }
	    
	    conn->vc = NULL;
	    conn->state = CONN_STATE_OPEN_LRECV_PKT;
	    conn->send_active = NULL;
	    conn->recv_active = NULL;

	    mpi_errno = connection_post_recv_pkt(conn);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN,
						 "**fail", NULL);
		goto fn_exit;
	    }
		
	    break;
	}
	    
	case MPIDU_SOCK_OP_CONNECT:
	{
	    MPIDI_CH3I_Connection_t * conn = (MPIDI_CH3I_Connection_t *) event->user_ptr;

	    if (event->error != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(
		    event->error, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|connfailed",
		    "**ch3|sock|connfailed %s %d", conn->vc->pg->id, conn->vc->pg_rank);
		goto fn_exit;
	    }

	    if (conn->state == CONN_STATE_CONNECTING)
	    {
		conn->state = CONN_STATE_OPEN_CSEND;
		conn->pkt.type = MPIDI_CH3I_PKT_SC_OPEN_REQ;
		conn->pkt.sc_open_req.pg_id_len = (int) strlen(MPIDI_Process.my_pg->id) + 1;
		conn->pkt.sc_open_req.pg_rank = MPIR_Process.comm_world->rank;

		connection_post_send_pkt_and_pgid(conn);
	    }
	    else
	    {
		/* CONN_STATE_CONNECT_ACCEPT */
		conn->state = CONN_STATE_OPEN_CSEND;
		conn->pkt.type = MPIDI_CH3I_PKT_SC_CONN_ACCEPT;
		/* pkt contains nothing */
		mpi_errno = connection_post_send_pkt(conn);
		if (mpi_errno != MPI_SUCCESS)
		{
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN,
						     "**ch3|sock|scconnaccept", NULL);
		    goto fn_exit;
		}
	    }
		    
	    break;
	}
	    
	case MPIDU_SOCK_OP_CLOSE:
	{
	    MPIDI_CH3I_Connection_t * conn = (MPIDI_CH3I_Connection_t *) event->user_ptr;
		
	    /* If the conn pointer is NULL then the close was intentional */
	    if (conn != NULL)
	    {
		if (conn->state == CONN_STATE_CLOSING)
		{
		    MPIU_Assert(conn->send_active == NULL);
		    MPIU_Assert(conn->recv_active == NULL);
		    if (conn->vc != NULL)
		    {
			conn->vc->ch.state = MPIDI_CH3I_VC_STATE_UNCONNECTED;
			conn->vc->ch.sock = MPIDU_SOCK_INVALID_SOCK;
			MPIDI_CH3U_Handle_connection(conn->vc, MPIDI_VC_EVENT_TERMINATED);
		    }
		}
		else
		{
		    MPIU_Assert(conn->state == CONN_STATE_LISTENING);
		    MPIDI_CH3I_listener_conn = NULL;
		    MPIDI_CH3I_listener_port = 0;
		    
		    /* MPIDI_CH3_Progress_signal_completion(); */
		    MPIDI_CH3I_progress_completion_count++;
		}
		
		conn->sock = MPIDU_SOCK_INVALID_SOCK;
		conn->state = CONN_STATE_CLOSED;
		connection_free(conn);
	    }
	    
	    break;
	}

	case MPIDU_SOCK_OP_WAKEUP:
	{
	    /* MPIDI_CH3_Progress_signal_completion(); */
	    MPIDI_CH3I_progress_completion_count++;
	    break;
	}
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_HANDLE_SOCK_EVENT);
    return mpi_errno;
}
