/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"
#include "mpidu_sock.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif

volatile unsigned int MPIDI_CH3I_progress_completions = 0;

static MPIDU_Sock_set_t sock_set; 
static int listener_port = 0;
static MPIDI_CH3I_Connection_t * listener_conn = NULL;

static int shutting_down = FALSE;


static inline MPIDI_CH3I_Connection_t * connection_alloc(void);
static inline void connection_free(MPIDI_CH3I_Connection_t * conn);
static inline void connection_post_sendq_req(MPIDI_CH3I_Connection_t * conn);
static inline void connection_post_send_pkt(MPIDI_CH3I_Connection_t * conn);
static inline void connection_post_recv_pkt(MPIDI_CH3I_Connection_t * conn);
static void connection_send_fail(MPIDI_CH3I_Connection_t * conn, int sock_errno);
static void connection_recv_fail(MPIDI_CH3I_Connection_t * conn, int sock_errno);
static inline void connection_post_send_pkt_and_pgid(MPIDI_CH3I_Connection_t * conn);

#if !defined(MPIDI_CH3_Progress_start)
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Progress_start
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3_Progress_start()
{
    /* MT - This function is empty for the single-threaded implementation */
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS_START);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS_START);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS_START);
}
#endif

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress(int is_blocking)
{
    int rc;
    MPIDU_Sock_event_t event;
    unsigned completions = MPIDI_CH3I_progress_completions;
    int mpi_errno = MPI_SUCCESS;
    int complete;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS);

#   if defined(MPICH_DBG_OUTPUT)
    {
	if (is_blocking)
	{
	    MPIDI_DBG_PRINTF((50, FCNAME, "entering, blocking=%s", is_blocking ? "true" : "false"));
	}
    }
#   endif
    
    do
    {
	rc = MPIDU_Sock_wait(sock_set, is_blocking ? MPIDU_SOCK_INFINITE_TIME : 0, &event);
	if (MPIR_ERR_GET_CLASS(rc) == MPIDU_SOCK_ERR_TIMEOUT)
	{
	    break;
	}
	if (rc != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(rc, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**progress_sock_wait", 0);
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS);
	    return mpi_errno;
	}

	switch (event.op_type)
	{
	    case MPIDU_SOCK_OP_READ:
	    {
		MPIDI_CH3I_Connection_t * conn = (MPIDI_CH3I_Connection_t *) event.user_ptr;
		
		MPID_Request * rreq = conn->recv_active;
		
		if (event.error != MPI_SUCCESS)
		{
		    if (!shutting_down || MPIR_ERR_GET_CLASS(event.error) != MPIDU_SOCK_ERR_CONN_CLOSED)  /* FIXME: this should be handled by the close protocol */
		    {
			connection_recv_fail(conn, event.error);
		    }
		    
		    break;
		}
		
		if (conn->recv_active)
		{
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
                            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
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
			    assert(conn->vc->ch.conn == conn);
			    assert(conn->vc->ch.sock == conn->sock);
			    
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
			int mpi_errno;

			MPIDI_DBG_Print_packet(&conn->pkt);
			mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN,
							 "**ch3|sock|badpacket", "**ch3|sock|badpacket %d", conn->pkt.type);
			MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS);
			return mpi_errno;
		    }
		}

		break;
	    }
	    
	    case MPIDU_SOCK_OP_WRITE:
	    {
		MPIDI_CH3I_Connection_t * conn = (MPIDI_CH3I_Connection_t *) event.user_ptr;

		if (event.error != MPI_SUCCESS)
		{
		    connection_send_fail(conn, event.error);
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
			    rc = MPIDU_Sock_post_close(conn->sock);
			    if (rc != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code(rc, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**sock_post_close", 0);
				MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS);
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
		rc = MPIDU_Sock_accept(listener_conn->sock, sock_set, conn, &conn->sock);
		if (rc == MPI_SUCCESS)
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
		MPIDI_CH3I_Connection_t * conn = (MPIDI_CH3I_Connection_t *) event.user_ptr;

		if (event.error != MPI_SUCCESS)
		{
		    int mpi_errno;

		    mpi_errno = MPIR_Err_create_code(event.error, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|connfailed",
						     "**ch3|sock|connfailed %d %d", /* FIXME: pgid */ -1, conn->vc->ch.pg_rank);
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS);
		    return mpi_errno;
		    /*MPID_Abort(NULL, mpi_errno, 13);*/
		}

		if (conn->state == CONN_STATE_CONNECTING) {
                    conn->state = CONN_STATE_OPEN_CSEND;
                    conn->pkt.type = MPIDI_CH3I_PKT_SC_OPEN_REQ;
                    conn->pkt.sc_open_req.pg_id_len = (int)
                        strlen(MPIDI_CH3I_Process.pg->pg_id) + 1;
                    conn->pkt.sc_open_req.pg_rank = MPIR_Process.comm_world->rank;

                    connection_post_send_pkt_and_pgid(conn);
                }
                else {  /* CONN_STATE_CONNECT_ACCEPT */
                    conn->state = CONN_STATE_OPEN_CSEND;
                    conn->pkt.type = MPIDI_CH3I_PKT_SC_CONN_ACCEPT;
                    /* pkt contains nothing */
                    connection_post_send_pkt(conn);
                }
		    
		break;
	    }
	    
	    case MPIDU_SOCK_OP_CLOSE:
	    {
		MPIDI_CH3I_Connection_t * conn = (MPIDI_CH3I_Connection_t *) event.user_ptr;
		
		/* If the conn pointer is NULL then the close was intentional */
		if (conn != NULL)
		{
		    if (conn->state == CONN_STATE_CLOSING)
		    {
			assert(conn->send_active == NULL);
			assert(conn->recv_active == NULL);
			conn->sock = MPIDU_SOCK_INVALID_SOCK;
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
    }
    while (completions == MPIDI_CH3I_progress_completions && is_blocking);

#   if defined(MPICH_DBG_OUTPUT)
    {
	if (is_blocking)
	{
	    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));
	}
    }
#   endif

 fn_exit:    
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Progress_poke
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Progress_poke()
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS_POKE);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS_POKE);
    
    mpi_errno = MPIDI_CH3I_Progress(FALSE);
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS_POKE);
    return mpi_errno;
}

#if !defined(MPIDI_CH3_Progress_end)
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Progress_end
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3_Progress_end()
{
    /* MT: This function is empty for the single-threaded implementation */
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS_END);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS_END);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS_END);
}
#endif

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress_init()
{
    int rc;
    MPIDU_Sock_t sock;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_INIT);

    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));

    rc = MPIDU_Sock_init();
    if (rc != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(rc, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	goto fn_exit;
    }
    
    /* create sock set */
    rc = MPIDU_Sock_create_set(&sock_set);
    if (rc != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(rc, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	goto fn_exit;
    }
    
    /* establish non-blocking listener */
    listener_conn = connection_alloc();
    listener_conn->sock = NULL;
    listener_conn->vc = NULL;
    listener_conn->state = CONN_STATE_LISTENING;
    listener_conn->send_active = NULL;
    listener_conn->recv_active = NULL;
    
    rc = MPIDU_Sock_listen(sock_set, listener_conn, &listener_port, &sock);
    if (rc != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(rc, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	goto fn_exit;
    }
    
    listener_conn->sock = sock;
    
  fn_exit:
    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS_INIT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress_finalize()
{
    /* MT: in a multi-threaded environment, finalize() should signal any thread(s) blocking on MPIDU_Sock_wait() and wait for those
       threads to complete before destroying the progress engine data structures.  We may need to add a function to the sock
       interface for this. */
    
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_FINALIZE);
    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));

    MPIR_Nest_incr();
    {
	mpi_errno = NMPI_Barrier(MPI_COMM_WORLD); /* FIXME: this barrier may not be necessary */
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**progress_finalize", 0);
	}
	shutting_down = TRUE;
	NMPI_Barrier(MPI_COMM_WORLD);
    }
    MPIR_Nest_decr();
    
    /* Shut down the listener */
    mpi_errno = MPIDU_Sock_post_close(listener_conn->sock);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
    }

    /* XXX: Wait for listener sock to close */
    
    /* FIXME: Cleanly shutdown other socks and free connection structures. (close protocol?) */

    MPIDU_Sock_destroy_set(sock_set);
    MPIDU_Sock_finalize();

    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS_FINALIZE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Listener_get_port
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
short MPIDI_CH3I_Listener_get_port()
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_LISTENER_GET_PORT);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_LISTENER_GET_PORT);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_LISTENER_GET_PORT);
    return listener_port;
}

static unsigned int GetIP(char *pszIP)
{
    unsigned int nIP;
    unsigned int a,b,c,d;
    if (pszIP == NULL)
	return 0;
    sscanf(pszIP, "%u.%u.%u.%u", &a, &b, &c, &d);
    /*printf("mask: %u.%u.%u.%u\n", a, b, c, d);fflush(stdout);*/
    nIP = (d << 24) | (c << 16) | (b << 8) | a;
    return nIP;
}

static unsigned int GetMask(char *pszMask)
{
    int i, nBits;
    unsigned int nMask = 0;
    unsigned int a,b,c,d;

    if (pszMask == NULL)
	return 0;

    if (strstr(pszMask, "."))
    {
	sscanf(pszMask, "%u.%u.%u.%u", &a, &b, &c, &d);
	/*printf("mask: %u.%u.%u.%u\n", a, b, c, d);fflush(stdout);*/
	nMask = (d << 24) | (c << 16) | (b << 8) | a;
    }
    else
    {
	nBits = atoi(pszMask);
	for (i=0; i<nBits; i++)
	{
	    nMask = nMask << 1;
	    nMask = nMask | 0x1;
	}
    }
    /*
    unsigned int a, b, c, d;
    a = ((unsigned char *)(&nMask))[0];
    b = ((unsigned char *)(&nMask))[1];
    c = ((unsigned char *)(&nMask))[2];
    d = ((unsigned char *)(&nMask))[3];
    printf("mask: %u.%u.%u.%u\n", a, b, c, d);fflush(stdout);
    */
    return nMask;
}

static int GetHostAndPort(char *host, int *port, char *business_card)
{
    char pszNetMask[50];
    char *pEnv, *token;
    unsigned int nNicNet, nNicMask;
    char *temp, *pszHost, *pszIP, *pszPort;
    unsigned int ip;

    pEnv = getenv("MPICH_NETMASK");
    if (pEnv != NULL)
    {
	MPIU_Strncpy(pszNetMask, pEnv, 50);
	token = strtok(pszNetMask, "/");
	if (token != NULL)
	{
	    token = strtok(NULL, "\n");
	    if (token != NULL)
	    {
		nNicNet = GetIP(pszNetMask);
		nNicMask = GetMask(token);

		/* parse each line of the business card and match the ip address with the network mask */
		temp = MPIU_Strdup(business_card);
		token = strtok(temp, ":\r\n");
		while (token)
		{
		    pszHost = token;
		    pszIP = strtok(NULL, ":\r\n");
		    pszPort = strtok(NULL, ":\r\n");
		    ip = GetIP(pszIP);
		    /*msg_printf("masking '%s'\n", pszIP);*/
		    if ((ip & nNicMask) == nNicNet)
		    {
			/* the current ip address matches the requested network so return these values */
			MPIU_Strncpy(host, pszIP, MAXHOSTNAMELEN); /*pszHost);*/
			*port = atoi(pszPort);
			MPIU_Free(temp);
			return MPI_SUCCESS;
		    }
		    token = strtok(NULL, ":\r\n");
		}
		if (temp)
		    MPIU_Free(temp);
	    }
	}
    }

    temp = MPIU_Strdup(business_card);
    if (temp == NULL)
    {
	/*MPIDI_err_printf("GetHostAndPort", "MPIU_Strdup failed\n");*/
	return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|strdup", 0);
    }
    /* move to the host part */
    token = strtok(temp, ":");
    if (token == NULL)
    {
	MPIU_Free(temp);
	/*MPIDI_err_printf("GetHostAndPort", "invalid business card\n");*/
	return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|badbuscard",
				    "**ch3|sock|badbuscard %s", business_card);
    }
    /*strcpy(host, token);*/
    /* move to the ip part */
    token = strtok(NULL, ":");
    if (token == NULL)
    {
	MPIU_Free(temp);
	/*MPIDI_err_printf("GetHostAndPort", "invalid business card\n");*/
	return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|badbuscard",
				    "**ch3|sock|badbuscard %s", business_card);
    }
    MPIU_Strncpy(host, token, MAXHOSTNAMELEN); /* use the ip string instead of the hostname, it's more reliable */
    /* move to the port part */
    token = strtok(NULL, ":");
    if (token == NULL)
    {
	MPIU_Free(temp);
	/*MPIDI_err_printf("GetHostAndPort", "invalid business card\n");*/
	return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|badbuscard",
				    "**ch3|sock|badbuscard %s", business_card);
    }
    *port = atoi(token);
    MPIU_Free(temp);

    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_VC_post_connect
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_VC_post_connect(MPIDI_VC * vc)
{
    char * key;
    char * val;
    char * val_p;
    int key_max_sz;
    int val_max_sz;
    char host[MAXHOSTNAMELEN];
    int port;
    int rc;
    MPIDI_CH3I_Connection_t * conn;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
    
    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));

    assert(vc->ch.state == MPIDI_CH3I_VC_STATE_UNCONNECTED);
    
    vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTING;
    
    mpi_errno = PMI_KVS_Get_key_length_max(&key_max_sz);
    if (mpi_errno != PMI_SUCCESS)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get_key_length_max", "**pmi_kvs_get_key_length_max %d", mpi_errno);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
        return mpi_errno;
    }
    key = MPIU_Malloc(key_max_sz);
    if (key == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
	return mpi_errno;
    }
    mpi_errno = PMI_KVS_Get_value_length_max(&val_max_sz);
    if (mpi_errno != PMI_SUCCESS)
    {
    }
    val = MPIU_Malloc(val_max_sz);
    if (val == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
	return mpi_errno;
    }

    rc = snprintf(key, key_max_sz, "P%d-businesscard", vc->ch.pg_rank);
    if (rc < 0 || rc > key_max_sz)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
	return mpi_errno;
    }
    rc = PMI_KVS_Get(vc->ch.pg->kvs_name, key, val, val_max_sz);
    if (rc != PMI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get %d", rc);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
	return mpi_errno;
    }

    val_p = val;

    rc = GetHostAndPort(host, &port, val_p);
    if (rc != MPI_SUCCESS)
    {
	/*printf("GetHostAndPort failed: <%s>\n", val_p);fflush(stdout);*/
	mpi_errno = MPIR_Err_create_code(rc, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
	return mpi_errno;
    }
    /*printf("GetHostAndPort returned: host %s, port %u\n", host, (unsigned int)port);fflush(stdout);*/

    conn = connection_alloc();
    rc = MPIDU_Sock_post_connect(sock_set, conn, host, port, &conn->sock);
    if (rc == MPI_SUCCESS)
    {
	vc->ch.sock = conn->sock;
	vc->ch.conn = conn;
	conn->vc = vc;
	conn->state = CONN_STATE_CONNECTING;
	conn->send_active = NULL;
	conn->recv_active = NULL;
    }
    else
    {
#if 0
	if (rc == SOCK_ERR_HOST_LOOKUP)
	{ 
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|hostlookup",
					     "**ch3|sock|hostlookup %s %d %s", conn->pg_id, conn->vc->ch.pg_rank, val);
	}
	else if (rc == SOCK_ERR_CONN_REFUSED)
	{ 
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|connrefused",
					     "**ch3|sock|connrefused %s %d %s", conn->pg_id, conn->vc->ch.pg_rank, val);
	}
	else
	{
	    mpi_errno = MPIDI_CH3I_sock_errno_to_mpi_errno(FCNAME, rc);
	}

	MPID_Abort(NULL, mpi_errno, 13);
#endif
	mpi_errno = MPIR_Err_create_code(rc, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	
	vc->ch.state = MPIDI_CH3I_VC_STATE_FAILED;
	connection_free(conn);
    }
    
    MPIU_Free(val);
    MPIU_Free(key);

    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME  MPIDI_CH3I_Connect_to_root
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int  MPIDI_CH3I_Connect_to_root(char *port_name, MPIDI_VC **new_vc)
{
    /* Used in ch3_comm_connect to connect with the process calling
       ch3_comm_accept */
    char *port_name_p;
    char host[MAXHOSTNAMELEN];
    int port, rc, mpi_errno;
    MPIDI_VC *vc;
    MPIDI_CH3I_Connection_t * conn;
    
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT);

    port_name_p = port_name;
    
    mpi_errno = GetHostAndPort(host, &port, port_name_p);
    if (mpi_errno != MPI_SUCCESS) goto fn_exit;
    
    vc = (MPIDI_VC *) MPIU_Malloc(sizeof(MPIDI_VC));
    if (vc == NULL)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	goto fn_exit;
    }
    /* FIXME - where does this vc get freed? */

    *new_vc = vc;

    vc->ch.pg = NULL; /* not used */
    vc->ch.pg_rank = 0;
    vc->ch.sendq_head = NULL;
    vc->ch.sendq_tail = NULL;
    vc->ch.state = MPIDI_CH3I_VC_STATE_UNCONNECTED;
    vc->ch.sock = MPIDU_SOCK_INVALID_SOCK;
    vc->ch.conn = NULL;
    
    conn = connection_alloc();
    /* conn->pg_id is not used for this conection */

    rc = MPIDU_Sock_post_connect(sock_set, conn, host, port, &conn->sock);
    if (rc == MPI_SUCCESS)
    {
        vc->ch.sock = conn->sock;
        vc->ch.conn = conn;
        vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTING;
        conn->vc = vc;
        conn->state = CONN_STATE_CONNECT_ACCEPT;
        conn->send_active = NULL;
        conn->recv_active = NULL;
    }
    else
    {
#if 0
        if (rc == SOCK_ERR_HOST_LOOKUP)
        { 
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|hostlookup",
                                             "**ch3|sock|hostlookup %s %d %s", conn->pg_id, conn->vc->ch.pg_rank, port_name);
        }
        else if (rc == SOCK_ERR_CONN_REFUSED)
        { 
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|connrefused",
                                             "**ch3|sock|connrefused %s %d %s", conn->pg_id, conn->vc->ch.pg_rank, port_name);
        }
        else
        {
            mpi_errno = MPIDI_CH3I_sock_errno_to_mpi_errno(FCNAME, rc);
        }
#endif
	mpi_errno = MPIR_Err_create_code(rc, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
        vc->ch.state = MPIDI_CH3I_VC_STATE_FAILED;
        MPIU_Free(conn);
        goto fn_exit;
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT);
    return mpi_errno;
}



#undef FUNCNAME
#define FUNCNAME connection_alloc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline MPIDI_CH3I_Connection_t * connection_alloc(void)
{
    MPIDI_CH3I_Connection_t * conn;
    int mpi_errno, id_sz;
    
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_ALLOC);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_ALLOC);
    conn = MPIU_Malloc(sizeof(MPIDI_CH3I_Connection_t));
    if (conn == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|connallocfailed", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_ALLOC);
	/*return mpi_errno;*/
	MPID_Abort(NULL, mpi_errno, 13);
    }

    mpi_errno = PMI_Get_id_length_max(&id_sz);
    if (mpi_errno != PMI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_id_length_max", "**pmi_get_id_length_max %d", mpi_errno);
	/* return mpi_errno;*/
	MPID_Abort(NULL, mpi_errno, 13);
    }
    conn->pg_id = MPIU_Malloc(id_sz + 1);
    if (conn->pg_id == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_ALLOC);
	/*return mpi_errno;*/
	MPID_Abort(NULL, mpi_errno, 13);
    }

    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_ALLOC);
    return conn;
}

#undef FUNCNAME
#define FUNCNAME connection_free
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void connection_free(MPIDI_CH3I_Connection_t * conn)
{
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_FREE);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_FREE);

    MPIU_Free(conn->pg_id);
    MPIU_Free(conn);
    
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_FREE);
}

#undef FUNCNAME
#define FUNCNAME connection_post_sendq_req
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void connection_post_sendq_req(MPIDI_CH3I_Connection_t * conn)
{
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_POST_SENDQ_REQ);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_POST_SENDQ_REQ);
    /* post send of next request on the send queue */
    conn->send_active = MPIDI_CH3I_SendQ_head(conn->vc); /* MT */
    if (conn->send_active != NULL)
    {
	int rc;
	
	rc = MPIDU_Sock_post_writev(conn->sock, conn->send_active->dev.iov, conn->send_active->dev.iov_count, NULL);
	if (rc != MPI_SUCCESS)
	{
	    connection_send_fail(conn, rc);
	}
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_POST_SENDQ_REQ);
}

#undef FUNCNAME
#define FUNCNAME connection_post_send_pkt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void connection_post_send_pkt(MPIDI_CH3I_Connection_t * conn)
{
    int rc;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_POST_SEND_PKT);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_POST_SEND_PKT);
    
    rc = MPIDU_Sock_post_write(conn->sock, &conn->pkt, sizeof(conn->pkt), sizeof(conn->pkt), NULL);
    if (rc != MPI_SUCCESS)
    {
	connection_send_fail(conn, rc);
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_POST_SEND_PKT);
}

#undef FUNCNAME
#define FUNCNAME connection_post_recv_pkt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void connection_post_recv_pkt(MPIDI_CH3I_Connection_t * conn)
{
    int rc;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_POST_RECV_PKT);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_POST_RECV_PKT);

    rc = MPIDU_Sock_post_read(conn->sock, &conn->pkt, sizeof(conn->pkt), sizeof(conn->pkt), NULL);
    if (rc != MPI_SUCCESS)
    {
	connection_recv_fail(conn, rc);
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_POST_RECV_PKT);
}


#undef FUNCNAME
#define FUNCNAME connection_post_send_pkt_and_pgid
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
#define FUNCNAME connection_send_fail
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static void connection_send_fail(MPIDI_CH3I_Connection_t * conn, int sock_errno)
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_SEND_FAIL);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_SEND_FAIL);

    mpi_errno = MPIR_Err_create_code(sock_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);

#   if 0
    {
	conn->state = CONN_STATE_FAILED;
	if (conn->vc != NULL)
	{
	    conn->vc->ch.state = MPIDI_CH3I_VC_STATE_FAILED;
	    MPIDI_CH3U_VC_send_failure(conn->vc, mpi_errno);
	}
    }
#   endif

    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_SEND_FAIL);
    /*return mpi_errno;*/
    if (conn->send_active)
    {
	MPID_Abort(conn->send_active->comm, mpi_errno, 13);
    }
    else
    {
	MPID_Abort(NULL, mpi_errno, 13);
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_SEND_FAIL);
}

#undef FUNCNAME
#define FUNCNAME connection_recv_fail
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static void connection_recv_fail(MPIDI_CH3I_Connection_t * conn, int sock_errno)
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_RECV_FAIL);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_RECV_FAIL);

    mpi_errno = MPIR_Err_create_code(sock_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
    MPID_Abort(NULL, mpi_errno, 13);
    
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_RECV_FAIL);
}
