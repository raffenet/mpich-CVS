/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"
#include "mpidu_sock.h"

volatile unsigned int MPIDI_CH3I_progress_completions = 0;

enum conn_state
{
    CONN_STATE_UNCONNECTED,
    CONN_STATE_LISTENING,
    CONN_STATE_CONNECTING,
    CONN_STATE_CONNECT_ACCEPT, 
    CONN_STATE_OPEN_CSEND,
    CONN_STATE_OPEN_CRECV,
    CONN_STATE_OPEN_LRECV,
    CONN_STATE_OPEN_LSEND,
    CONN_STATE_CONNECTED,
    CONN_STATE_CLOSING,
    CONN_STATE_CLOSED,
    CONN_STATE_FAILED
};

typedef struct MPIDI_CH3I_Connection
{
    MPIDI_VC * vc;
    MPIDU_Sock_t sock;
    enum conn_state state;
    MPIDI_CH3I_Process_group_t * remote_pg_ptr;
    MPID_Request * send_active;
    MPID_Request * recv_active;
    MPIDI_CH3_Pkt_t pkt;
} MPIDI_CH3I_Connection_t;

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
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS);

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
	assert(rc == MPI_SUCCESS);

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
		    conn->recv_active = NULL;
		    mpi_errno = MPIDI_CH3U_Handle_recv_req(conn->vc, rreq);
		    if (mpi_errno != MPI_SUCCESS)
		    {
			MPID_Abort(NULL, mpi_errno, 13);
		    }
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
			mpi_errno = MPIDI_CH3U_Handle_recv_pkt(conn->vc, &conn->pkt);
			if (mpi_errno != MPI_SUCCESS)
			{
			    MPID_Abort(NULL, mpi_errno, 13);
			}
			if (conn->recv_active == NULL)
			{ 
			    connection_post_recv_pkt(conn);
			}
		    }
		    else if (conn->pkt.type == MPIDI_CH3I_PKT_SC_OPEN_REQ)
		    {
			MPIDI_CH3I_Process_group_t * pg;
			int pg_rank;
			MPIDI_VC * vc;

			pg = conn->pkt.sc_open_req.pg_ptr;
			pg_rank = conn->pkt.sc_open_req.pg_rank;
#ifdef OLD
			vc = &MPIDI_CH3I_Process.pg->vc_table[pg_rank]; /* FIXME: need to lookup process group from pg_id */
#endif
                        vc = &(pg->vc_table[pg_rank]);
			assert(vc->sc.pg_rank == pg_rank);

			if (vc->sc.conn == NULL) {
                            /* no head-to-head connects, accept the
                               connection */
			    vc->sc.state = MPIDI_CH3I_VC_STATE_CONNECTING;
			    vc->sc.sock = conn->sock;
			    vc->sc.conn = conn;
			    conn->vc = vc;
			    
			    conn->pkt.type = MPIDI_CH3I_PKT_SC_OPEN_RESP;
			    conn->pkt.sc_open_resp.ack = TRUE;
                        }
                        else { /* head to head situation */
                            if (pg == MPIDI_CH3I_Process.pg) {
                                /* the other process is in the same
                                   comm_world; just compare the ranks */
                                if (MPIR_Process.comm_world->rank < pg_rank) { 
                                    /* accept connection */
                                    vc->sc.state = MPIDI_CH3I_VC_STATE_CONNECTING;
                                    vc->sc.sock = conn->sock;
                                    vc->sc.conn = conn;
                                    conn->vc = vc;
			    
                                    conn->pkt.type = MPIDI_CH3I_PKT_SC_OPEN_RESP;
                                    conn->pkt.sc_open_resp.ack = TRUE;
                                }
                                else {
                                    /* refuse connection */
                                    conn->pkt.type = MPIDI_CH3I_PKT_SC_OPEN_RESP;
                                    conn->pkt.sc_open_resp.ack = FALSE;
                                }
                            }
                            else { 
                                /* the two processes are in different
                                   comm_worlds; compare their unique
                                   pg_ids. For now, we are using the
                                   kvs_name in the pg structure as the
                                   unique pg_id */
                                if (strcmp(MPIDI_CH3I_Process.pg->kvs_name, 
                                           pg->kvs_name) < 0) {
                                    /* accept connection */
                                    vc->sc.state = MPIDI_CH3I_VC_STATE_CONNECTING;
                                    vc->sc.sock = conn->sock;
                                    vc->sc.conn = conn;
                                    conn->vc = vc;
                                    
                                    conn->pkt.type = MPIDI_CH3I_PKT_SC_OPEN_RESP;
                                    conn->pkt.sc_open_resp.ack = TRUE;
                                }
                                else {
                                    /* refuse connection */
                                    conn->pkt.type = MPIDI_CH3I_PKT_SC_OPEN_RESP;
                                    conn->pkt.sc_open_resp.ack = FALSE;
                                }
                            }
                        }

			conn->state = CONN_STATE_OPEN_LSEND;
			connection_post_send_pkt(conn);
			
		    }
                    else if (conn->pkt.type == MPIDI_CH3I_PKT_SC_CONN_ACCEPT) {
                        MPIDI_VC *vc; 

                        vc = (MPIDI_VC *) MPIU_Malloc(sizeof(MPIDI_VC));
                        if (vc == NULL)
                        {
                            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
                            goto fn_exit;
                        }
                        /* FIXME - where does this vc get freed? */

                        vc->sc.pg = NULL;
                        vc->sc.pg_rank = 0;
                        vc->sc.sendq_head = NULL;
                        vc->sc.sendq_tail = NULL;

                        vc->sc.state = MPIDI_CH3I_VC_STATE_CONNECTING;
                        vc->sc.sock = conn->sock;
                        vc->sc.conn = conn;
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
			    conn->vc->sc.state = MPIDI_CH3I_VC_STATE_CONNECTED;
			    assert(conn->vc->sc.conn == conn);
			    assert(conn->vc->sc.sock == conn->sock);
			    
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
			MPID_Abort(NULL, mpi_errno, 13);
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
		    
		    conn->send_active = NULL;
		    mpi_errno = MPIDI_CH3U_Handle_send_req(conn->vc, sreq);
		    if (mpi_errno != MPI_SUCCESS)
		    {
			MPID_Abort(NULL, mpi_errno, 13);
		    }
		    if (conn->send_active == NULL)
		    {
			MPIDI_CH3I_SendQ_dequeue(conn->vc);
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
			    conn->vc->sc.state = MPIDI_CH3I_VC_STATE_CONNECTED;
			    connection_post_recv_pkt(conn);
			    connection_post_sendq_req(conn);
			}
			else
			{
			    /* head-to-head connections - close this connection */
			    conn->state = CONN_STATE_CLOSING;
			    rc = MPIDU_Sock_post_close(conn->sock);
			    assert(rc == MPI_SUCCESS);
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
	    
	    case MPIDU_SOCK_OP_CONNECT:
	    {
		MPIDI_CH3I_Connection_t * conn = (MPIDI_CH3I_Connection_t *) event.user_ptr;

		if (event.error != MPI_SUCCESS)
		{
		    int mpi_errno;

		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|connfailed",
						     "**ch3|sock|connfailed %d %d", /* FIXME: pgid */ -1, conn->vc->sc.pg_rank);
		    MPID_Abort(NULL, mpi_errno, 13);

		}

		if (conn->state == CONN_STATE_CONNECTING) {
                    conn->state = CONN_STATE_OPEN_CSEND;
                    conn->pkt.type = MPIDI_CH3I_PKT_SC_OPEN_REQ;
#ifdef OLD
                    conn->pkt.sc_open_req.pg_id = -1; /* FIXME: multiple process groups may exist */
#endif
                    conn->pkt.sc_open_req.pg_ptr = conn->remote_pg_ptr;
                    conn->pkt.sc_open_req.pg_rank = MPIR_Process.comm_world->rank;
                }
                else {  /* CONN_STATE_CONNECT_ACCEPT */
                    conn->state = CONN_STATE_OPEN_CSEND;
                    conn->pkt.type = MPIDI_CH3I_PKT_SC_CONN_ACCEPT;
                    /* pkt contains nothing */
                }

		connection_post_send_pkt(conn);
		    
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
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS);
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
	NMPI_Barrier(MPI_COMM_WORLD); /* FIXME: this barrier may not be necessary */
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

/* no longer static because it is needed in ch3_comm_connect.c */
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

/* retrieves the pg_ptr from the front of the business card and
   increments the business card ptr to point to the real business
   card. not static because it is needed in ch3_comm_connect.c */
static int GetPGptr(char **business_card, MPIDI_CH3I_Process_group_t **remote_pg_ptr)
{
    sscanf(*business_card, "%p", remote_pg_ptr);
    *business_card = strchr(*business_card, ':') + 1;

/*    printf("pg_ptr = %p, biz_card = %s\n", *remote_pg_ptr, *business_card);
    fflush(stdout);
*/
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
    MPIDI_CH3I_Process_group_t * remote_pg_ptr;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
    
    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));

    assert(vc->sc.state == MPIDI_CH3I_VC_STATE_UNCONNECTED);
    
    vc->sc.state = MPIDI_CH3I_VC_STATE_CONNECTING;
    
    key_max_sz = PMI_KVS_Get_key_length_max();
    key = MPIU_Malloc(key_max_sz);
    assert(key != NULL);
    val_max_sz = PMI_KVS_Get_value_length_max();
    val = MPIU_Malloc(val_max_sz);
    assert(val != NULL);

    rc = snprintf(key, key_max_sz, "P%d-businesscard", vc->sc.pg_rank);
    assert(rc > -1 && rc < key_max_sz);
    rc = PMI_KVS_Get(vc->sc.pg->kvs_name, key, val);
    assert(rc == MPI_SUCCESS);

    val_p = val;
    rc = GetPGptr(&val_p, &remote_pg_ptr);
    assert(rc == MPI_SUCCESS);

    rc = GetHostAndPort(host, &port, val_p);
    assert(rc == MPI_SUCCESS);
    /*printf("GetHostAndPort returned: host %s, port %d\n", host, port);fflush(stdout);*/

    conn = connection_alloc();
    conn->remote_pg_ptr = remote_pg_ptr;

    rc = MPIDU_Sock_post_connect(sock_set, conn, host, port, &conn->sock);
    if (rc == MPI_SUCCESS)
    {
	vc->sc.sock = conn->sock;
	vc->sc.conn = conn;
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
					     "**ch3|sock|hostlookup %d %d %s", remote_pg_ptr, conn->vc->sc.pg_rank, val);
	}
	else if (rc == SOCK_ERR_CONN_REFUSED)
	{ 
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|connrefused",
					     "**ch3|sock|connrefused %d %d %s", remote_pg_ptr, conn->vc->sc.pg_rank, val);
	}
	else
	{
	    mpi_errno = MPIDI_CH3I_sock_errno_to_mpi_errno(FCNAME, rc);
	}

	MPID_Abort(NULL, mpi_errno, 13);
#endif
	mpi_errno = MPIR_Err_create_code(rc, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	
	vc->sc.state = MPIDI_CH3I_VC_STATE_FAILED;
	connection_free(conn);
    }
    
    MPIU_Free(val);
    MPIU_Free(key);

    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_VC_post_read
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_VC_post_read(MPIDI_VC * vc, MPID_Request * rreq)
{
    MPIDI_CH3I_Connection_t * conn = vc->sc.conn;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_VC_POST_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_VC_POST_READ);
    MPIDI_DBG_PRINTF((60, FCNAME, "entering, vc=0x%p, rreq=0x%08x", vc, rreq->handle));

    assert (conn->recv_active == NULL);
    conn->recv_active = rreq;
    mpi_errno = MPIDU_Sock_post_readv(conn->sock, rreq->ch3.iov + rreq->sc.iov_offset, rreq->ch3.iov_count - rreq->sc.iov_offset, NULL);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
    }
    
    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_READ);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_VC_post_write
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_VC_post_write(MPIDI_VC * vc, MPID_Request * sreq)
{
    MPIDI_CH3I_Connection_t * conn = vc->sc.conn;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_VC_POST_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_VC_POST_WRITE);
    MPIDI_DBG_PRINTF((60, FCNAME, "entering, vc=0x%p, sreq=0x%08x", vc, sreq->handle));
    
    assert (conn->send_active == NULL);
    conn->send_active = sreq;
    mpi_errno = MPIDU_Sock_post_writev(conn->sock, sreq->ch3.iov + sreq->sc.iov_offset,
				  sreq->ch3.iov_count - sreq->sc.iov_offset,  NULL);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
    }

    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_WRITE);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME  MPIDI_CH3I_Connect_to_root
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int  MPIDI_CH3I_Connect_to_root(char *port_name, MPIDI_VC **new_vc)
{
    /* used in ch3_comm_connect to connect with the process calling
       ch3_comm_accept */
    char *port_name_p;
    char host[MAXHOSTNAMELEN];
    MPIDI_CH3I_Process_group_t *pg;
    int port, rc, mpi_errno;
    MPIDI_VC *vc;
    MPIDI_CH3I_Connection_t * conn;
    
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT);

    port_name_p = port_name;
    
    mpi_errno = GetPGptr(&port_name_p, &pg);
    /* pg should be NULL since it is not used for this connection */
    if (mpi_errno != MPI_SUCCESS) goto fn_exit;
    
    mpi_errno = GetHostAndPort(host, &port, port_name_p);
    if (mpi_errno != MPI_SUCCESS) goto fn_exit;
    
    conn = connection_alloc();
    conn->remote_pg_ptr = pg;  /* NULL, not used */

    vc = (MPIDI_VC *) MPIU_Malloc(sizeof(MPIDI_VC));
    if (vc == NULL)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	goto fn_exit;
    }
    /* FIXME - where does this vc get freed? */

    *new_vc = vc;

    vc->sc.pg = pg;
    vc->sc.pg_rank = 0;
    vc->sc.sendq_head = NULL;
    vc->sc.sendq_tail = NULL;
    vc->sc.state = MPIDI_CH3I_VC_STATE_UNCONNECTED;
    vc->sc.sock = MPIDU_SOCK_INVALID_SOCK;
    vc->sc.conn = NULL;
    
    rc = MPIDU_Sock_post_connect(sock_set, conn, host, port, &conn->sock);
    if (rc == MPI_SUCCESS)
    {
        vc->sc.sock = conn->sock;
        vc->sc.conn = conn;
        vc->sc.state = MPIDI_CH3I_VC_STATE_CONNECTING;
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
                                             "**ch3|sock|hostlookup %d %d %s", pg, conn->vc->sc.pg_rank, port_name);
        }
        else if (rc == SOCK_ERR_CONN_REFUSED)
        { 
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|connrefused",
                                             "**ch3|sock|connrefused %d %d %s", pg, conn->vc->sc.pg_rank, port_name);
        }
        else
        {
            mpi_errno = MPIDI_CH3I_sock_errno_to_mpi_errno(FCNAME, rc);
        }
#endif
	mpi_errno = MPIR_Err_create_code(rc, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
        vc->sc.state = MPIDI_CH3I_VC_STATE_FAILED;
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
    int mpi_errno;
    
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_ALLOC);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_ALLOC);
    conn = MPIU_Malloc(sizeof(MPIDI_CH3I_Connection_t));
    if (conn == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|connallocfailed", 0);
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
	
	rc = MPIDU_Sock_post_writev(conn->sock, conn->send_active->ch3.iov, conn->send_active->ch3.iov_count, NULL);
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
	    
	    conn->vc->sc.state = MPIDI_CH3I_VC_STATE_FAILED;
	    MPIDI_CH3U_VC_send_failure(conn->vc, mpi_errno);
	}
    }
#   endif

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
