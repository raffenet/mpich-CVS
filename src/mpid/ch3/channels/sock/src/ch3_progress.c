/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"
#include "sock.h"

volatile unsigned int MPIDI_CH3I_progress_completions = 0;

enum conn_state
{
    CONN_STATE_UNCONNECTED,
    CONN_STATE_LISTENING,
    CONN_STATE_CONNECTING,
    CONN_STATE_OPEN_CSEND,
    CONN_STATE_OPEN_CRECV,
    CONN_STATE_OPEN_LRECV,
    CONN_STATE_OPEN_LSEND,
    CONN_STATE_CONNECTED,
    CONN_STATE_CLOSING,
    CONN_STATE_CLOSED,
    CONN_STATE_FAILED
};


typedef struct MPIDI_CH3I_conn_info
{
    MPIDI_VC * vc;
    sock_t sock;
    enum conn_state state;
    MPID_Request * send_active;
    MPID_Request * recv_active;
    MPIDI_CH3_Pkt_t pkt;
} MPIDI_CH3I_conn_info_t;

static sock_set_t sock_set;
static int listener_port = 0;
static MPIDI_CH3I_conn_info_t * listener_info = NULL;

static int shutting_down = FALSE;


static inline MPIDI_CH3I_conn_info_t * conn_info_create(void);
static inline void conn_info_destroy(MPIDI_CH3I_conn_info_t * conn_info);

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
    int rc;
    int register count;
    sock_event_t event;
    unsigned completions = MPIDI_CH3I_progress_completions;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS);

    MPIDI_DBG_PRINTF((50, FCNAME, "entering, blocking=%s", is_blocking ? "true" : "false"));
    do
    {
	rc = sock_wait(sock_set, is_blocking ? SOCK_INFINITE_TIME : 0, &event);
	if (rc == SOCK_ERR_TIMEOUT)
	{
	    break;
	}
	assert(rc == SOCK_SUCCESS);

	switch (event.op_type)
	{
	    case SOCK_OP_TIMEOUT:
	    {
		goto fn_exit;
	    }
	    
	    case SOCK_OP_READ:
	    {
		MPIDI_CH3I_conn_info_t * info = (MPIDI_CH3I_conn_info_t *) event.user_ptr;
		
		if (info->recv_active)
		{
		}
		else /* incoming packet header */
		{
		    
		}

		break;
	    }
	    
	    case SOCK_OP_WRITE:
	    {
		MPIDI_CH3I_conn_info_t * info = (MPIDI_CH3I_conn_info_t *) event.user_ptr;

		if (event.error != SOCK_SUCCESS)
		{
		    /* XXX: this is not right */
		    info->state = CONN_STATE_FAILED;
		    if (info->vc != NULL)
		    { 
			info->vc->sc.state = MPIDI_CH3I_VC_STATE_FAILED;
			/* MPIDI_CH3U_VC_propagate_error(info->vc, MPI_ERR_UNKNOWN); */
		    }
		    break;
		}
		
		if (info->send_active)
		{
		    MPID_Request * sreq = info->send_active;
		    
		    info->send_active = FALSE;
		    MPIDI_CH3U_Handle_send_req(info->vc, sreq);
		    if (info->send_active == FALSE)
		    { 
			/* post send of next request on the send queue */
			info->send_active = MPIDI_CH3I_SendQ_head(info->vc); /* MT */
			if (info->send_active != NULL)
			{
			    sock_post_write(info->sock, info->send_active->ch3.iov, info->send_active->ch3.iov_count, NULL);
			}
		    }
		}
		else /* finished writing internal packet header */
		{
		    if (info->state == CONN_STATE_OPEN_CSEND)
		    {
			/* finished sending open request packet */
			/* post receive for open response packet */
			info->state = CONN_STATE_OPEN_CRECV;
			sock_post_read(info->sock, &info->pkt, sizeof(info->pkt), NULL);
		    }
		    else if (info->state == CONN_STATE_OPEN_LSEND)
		    {
			/* finished sending open response packet */
			if (info->pkt.sc_open_resp.ack == TRUE)
			{ 
			    /* post receive for packet header */
			    info->state = CONN_STATE_CONNECTED;
			    sock_post_read(info->sock, &info->pkt, sizeof(info->pkt), NULL);
			    
			    /* post send of next request on the send queue */
			    info->send_active = MPIDI_CH3I_SendQ_head(info->vc); /* MT */
			    if (info->send_active != NULL)
			    {
				sock_post_write(info->sock, info->send_active->ch3.iov, info->send_active->ch3.iov_count, NULL);
			    }
			}
			else
			{
			    /* head-to-head connections - close this connection */
			    info->state = CONN_STATE_CLOSING;
			    rc = sock_post_close(info->sock);
			    assert(rc == SOCK_SUCCESS);
			}
		    }
		}

		break;
	    }
	    
	    case SOCK_OP_ACCEPT:
	    {
		MPIDI_CH3I_conn_info_t * info;

		info = conn_info_create();
		if (info != NULL)
		{ 
		    rc = sock_accept(listener_info->sock, sock_set, info, &info->sock);
		    if (rc == SOCK_SUCCESS)
		    { 
			info->vc = NULL;
			info->state = CONN_STATE_OPEN_LRECV;
			info->send_active = NULL;
			info->recv_active = NULL;

			sock_post_read(info->sock, &info->pkt, sizeof(info->pkt), NULL);
		    }
		    else
		    {
			conn_info_destroy(info);
		    }
		}
		else
		{
		    sock_t sock;
		    
		    if (sock_accept(listener_info->sock, sock_set, NULL, &sock) == SOCK_SUCCESS)
		    { 
			sock_post_close(sock);
		    }
		}
		
		break;
	    }
	    
	    case SOCK_OP_CONNECT:
	    {
		break;
	    }
	    
	    case SOCK_OP_CLOSE:
	    {
		MPIDI_CH3I_conn_info_t * info = (MPIDI_CH3I_conn_info_t *) event.user_ptr;
		
		/* If the info pointer is NULL then the close was intentional */
		if (info != NULL)
		{
		    if (info->state == CONN_STATE_CLOSING)
		    {
			assert(info->send_active == NULL);
			assert(info->recv_active == NULL);
			info->sock = SOCK_INVALID_SOCK;
			info->state = CONN_STATE_CLOSED;
			conn_info_destroy(info);
		    }
		    else if (info->state == CONN_STATE_LISTENING && shutting_down)
		    {
			assert(listener_info == info);
			conn_info_destroy(listener_info);
			listener_info = NULL;
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

  fn_exit:
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
    MPIDI_CH3_Progress(FALSE);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS_POKE);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Progress_end
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3_Progress_end()
{
    /* MT: This function is empty for the single-threaded implementation */
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress_init()
{
    int rc;
    sock_t sock;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS_INIT);

    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));

    /* create sock set */
    rc = sock_create_set(&sock_set);
    if (rc != SOCK_SUCCESS)
    {
	if (rc == SOCK_ERR_NOMEM)
	{ 
	    mpi_errno = MPIR_ERR_MEMALLOCFAILED;
	}
	else
	{
	    mpi_errno = MPI_ERR_OTHER;
	}

	goto fn_exit;
    }
    
    /* establish non-blocking listener */
    listener_info = conn_info_create();
    listener_info->sock = NULL;
    listener_info->vc = NULL;
    listener_info->state = CONN_STATE_LISTENING;
    listener_info->send_active = NULL;
    listener_info->recv_active = NULL;
    
    rc = sock_listen(sock_set, listener_info, &listener_port, &sock);
    if (rc != SOCK_SUCCESS)
    {
	if (rc == SOCK_ERR_NOMEM)
	{ 
	    mpi_errno = MPIR_ERR_MEMALLOCFAILED;
	}
	else
	{
	    mpi_errno = MPI_ERR_OTHER;
	}

	goto fn_exit;
    }
    
    listener_info->sock = sock;
    
  fn_exit:
    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS_INIT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress_finalize()
{
    /* MT: in a multi-threaded environment, finalize() should signal any thread(s) blocking on sock_wait() and wait for those
       threads to complete before destroying the progress engine data structures.  We may need to add a function to the sock
       interface for this. */
    
    int rc;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS_FINALIZE);
    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));

    MPI_Barrier(MPI_COMM_WORLD); /* FIXME: this barrier may not be necessary */
    shutting_down = TRUE;
    MPI_Barrier(MPI_COMM_WORLD);
    
    /* Shut down the listener */
    rc = sock_post_close(listener_info->sock);
    assert(rc == SOCK_SUCCESS);

    /* XXX: Wait for listener sock to close */
    
    /* FIXME: Cleanly shutdown other socks and free info structures. (close protocol?) */

    sock_destroy_set(sock_set);

    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS_FINALIZE);
    return MPI_SUCCESS;
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

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_VC_post_connect
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_VC_post_connect(MPIDI_VC * vc)
{
    char * key;
    char * val;
    int key_max_sz;
    int val_max_sz;
    int port;
    int rc;
    MPIDI_CH3I_conn_info_t * info;
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
    
    rc = snprintf(key, key_max_sz, "P%d-port", vc->sc.pg_rank);
    assert(rc > -1 && rc < key_max_sz);
    rc = PMI_KVS_Get(vc->sc.pg->kvs_name, key, val);
    assert(rc == 0);
    rc = sscanf(val, "%d", &port);
    
    rc = snprintf(key, key_max_sz, "P%d-hostname", vc->sc.pg_rank);
    assert(rc > -1 && rc < key_max_sz);
    rc = PMI_KVS_Get(vc->sc.pg->kvs_name, key, val);
    assert(rc == 0);

    info = conn_info_create();
    if (info == NULL)
    {
	mpi_errno = MPIR_ERR_MEMALLOCFAILED;
	goto fn_exit;
    }

    rc = sock_post_connect(sock_set, info, val, port, &info->sock);
    if (rc == SOCK_SUCCESS)
    {
	vc->sc.sock = info->sock;
	vc->sc.info = info;
	info->vc = vc;
	info->state = CONN_STATE_CONNECTING;
	info->send_active = NULL;
	info->recv_active = NULL;
    }
    else
    { 
	vc->sc.state = MPIDI_CH3I_VC_STATE_FAILED;
	conn_info_destroy(info);
    }
    
    MPIU_Free(val);
    MPIU_Free(key);

  fn_exit:
    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
    return mpi_errno;
}

void MPIDI_CH3I_VC_post_read(MPIDI_VC * vc, MPID_Request * req)
{
    MPIDI_CH3I_conn_info_t * info = vc->sc.info;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_VC_POST_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_VC_POST_READ);

    assert (info->recv_active == NULL);
    info->recv_active = req;
    sock_post_readv(info->sock, req->ch3.iov, req->ch3.iov_count, NULL);
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_READ);
}

void MPIDI_CH3I_VC_post_write(MPIDI_VC * vc, MPID_Request * req)
{
    MPIDI_CH3I_conn_info_t * info = vc->sc.info;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_VC_POST_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_VC_POST_WRITE);
    
    assert (info->send_active == NULL);
    info->send_active = req;
    sock_post_writev(info->sock, req->ch3.iov, req->ch3.iov_count, NULL);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_WRITE);
}

MPIDI_CH3I_conn_info_t * conn_info_create()
{
    return MPIU_Malloc(sizeof(MPIDI_CH3I_conn_info_t));
}

void conn_info_destroy(MPIDI_CH3I_conn_info_t * info)
{
    MPIU_Free(info);
}
