/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"

#include "mpidu_sock.h"

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
/* Include this for AF_INET */
#include <sys/socket.h>
#endif
#ifdef HAVE_ARPA_INET_H
/* Include this for inet_pton prototype */
#include <arpa/inet.h>
#endif

/* FIXME: Describe what these routines do */

/* Partial description: 
   This file contains the routines that are used to create socket connections,
   including the routines used to encode/decode the description of a connection
   into/out of the "business card".

   ToDo: change the "host description" to an "interface address" so that
   socket connections are targeted at particularly interfaces, not
   compute nodes, and that the address is in ready-to-use IP address format, 
   and does not require a gethostbyname lookup.
 */

/*
 * Manage the connection information that is exported to other processes
 * 
 */
#define MPIDI_CH3I_HOST_DESCRIPTION_KEY  "description"
#define MPIDI_CH3I_PORT_KEY              "port"
#define MPIDI_CH3I_IFNAME_KEY            "ifname"

/* FIXME: All of the listener port routines should be in one place.
   It looks like this should be a socket utility function called by
   ch3_progress.c in sock and ssm (and essm if that is still valid), 
   as part of the progress init function.  Note that those progress
   engines also set the port to 0 when shutting down the progress engine,
   though it doesn't look like the port is closed. */

int MPIDI_CH3I_listener_port = 0;
/* Required for (socket version) upcall to Connect_to_root (see FIXME) */
extern MPIDU_Sock_set_t MPIDI_CH3I_sock_set;

/* Allocates a connection and the pg_id field for a connection only.
   Does not initialize any connection fields other than pg_id.
   Called by routines that create connections, used in this
   file and in ch3_progress*.c in various channels.
*/
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Connection_alloc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Connection_alloc(MPIDI_CH3I_Connection_t ** connp)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_Connection_t * conn = NULL;
    int id_sz;
    MPIU_CHKPMEM_DECL(2);
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_ALLOC);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_ALLOC);

    MPIU_CHKPMEM_MALLOC(conn,MPIDI_CH3I_Connection_t*,
			sizeof(MPIDI_CH3I_Connection_t),mpi_errno,"conn");

    mpi_errno = PMI_Get_id_length_max(&id_sz);
    if (mpi_errno != PMI_SUCCESS) {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, 
			     "**pmi_get_id_length_max",
			     "**pmi_get_id_length_max %d", mpi_errno);
    }
    MPIU_CHKPMEM_MALLOC(conn->pg_id,char*,id_sz + 1,mpi_errno,"conn->pg_id");
    *connp = conn;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_ALLOC);
    return mpi_errno;
  fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}


/* FIXME: Why does the name include "to_root"?  */

#undef FUNCNAME
#define FUNCNAME  MPIDI_CH3I_Connect_to_root_sock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Connect_to_root_sock(const char * port_name, 
					 MPIDI_VC_t ** new_vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t * vc;
    MPIU_CHKPMEM_DECL(1);
    char host_description[MAX_HOST_DESCRIPTION_LEN];
    int port, port_name_tag;
    unsigned char ifaddr[4];
    int hasIfaddr = 0;
    MPIDI_CH3I_Connection_t * conn;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT_SOCK);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT_SOCK);

    MPIU_DBG_MSG_S(CH3_CONNECT,VERBOSE,"Connect to root with portstring %s",
		   port_name );

    mpi_errno = MPIDU_Sock_get_conninfo_from_bc( port_name, host_description,
						 sizeof(host_description),
						 &port, ifaddr, &hasIfaddr );
    if (mpi_errno) {
	MPIU_ERR_POP(mpi_errno);
    }
    mpi_errno = MPIDI_GetTagFromPort(port_name, &port_name_tag);
    if (mpi_errno != MPIU_STR_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**argstr_port_name_tag");
    }

    MPIU_DBG_MSG_D(CH3_CONNECT,VERBOSE,"port tag %d",port_name_tag);

    MPIU_CHKPMEM_MALLOC(vc,MPIDI_VC_t *,sizeof(MPIDI_VC_t),mpi_errno,"vc");
    /* FIXME - where does this vc get freed? */

    *new_vc = vc;

    MPIDI_VC_Init(vc, NULL, 0);
    MPIDI_CH3_VC_Init( vc );

    mpi_errno = MPIDI_CH3I_Connection_alloc(&conn);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }

    /* conn->pg_id is not used for this conection */

    /* FIXME: To avoid this global (MPIDI_CH3I_sock_set) which is 
       used only ch3_progress.c and ch3_progress_connect.c in the channels,
       this should be a call into the channel, asking it to setup the
       socket for a connection and return the connection.  That will
       keep the socket set out of the general ch3 code, even if this
       is the socket utility functions. */
    MPIU_DBG_MSG_FMT(CH3_CONNECT,VERBOSE,(MPIU_DBG_FDEST,
	  "posting connect to host %s, port %d", host_description, port ));
    mpi_errno = MPIDU_Sock_post_connect(MPIDI_CH3I_sock_set, conn, 
					host_description, port, &conn->sock);
    if (mpi_errno == MPI_SUCCESS)
    {
        vc->ch.sock = conn->sock;
        vc->ch.conn = conn;
        vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTING;
        conn->vc = vc;
        conn->state = CONN_STATE_CONNECT_ACCEPT;
        conn->send_active = NULL;
        conn->recv_active = NULL;

        /* place the port name tag in the pkt that will eventually be sent to the other side */
        conn->pkt.sc_conn_accept.port_name_tag = port_name_tag;
    }
    /* --BEGIN ERROR HANDLING-- */
    else
    {
	if (MPIR_ERR_GET_CLASS(mpi_errno) == MPIDU_SOCK_ERR_BAD_HOST)
        { 
            mpi_errno = MPIR_Err_create_code(
		MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|badhost",
		"**ch3|sock|badhost %s %d %s", conn->pg_id, conn->vc->pg_rank, port_name);
        }
        else if (MPIR_ERR_GET_CLASS(mpi_errno) == MPIDU_SOCK_ERR_CONN_FAILED)
        { 
            mpi_errno = MPIR_Err_create_code(
		MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|connrefused",
		"**ch3|sock|connrefused %s %d %s", conn->pg_id, conn->vc->pg_rank, port_name);
        }
        else
        {
	    MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER, "**fail");
	}
        vc->ch.state = MPIDI_CH3I_VC_STATE_FAILED;
        MPIU_Free(conn);
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

 fn_exit:
    MPIU_DBG_MSG(CH3_CONNECT,VERBOSE,"Exiting connect_to_root_sock");
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT_SOCK);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

/* ------------------------------------------------------------------------- */
/* Business card management.  These routines insert or extract connection
   information when using sockets from the business card */
/* ------------------------------------------------------------------------- */

/* FIXME: These are small routines; we may want to bring them together 
   into a more specific post-connection-for-sock */
/*
int MPIDI_CH3I_Connection_with_sock( const char *bc, 
				     MPIDI_CH3I_Connection_t **conn )
*/

/* The host_description should be of length MAX_HOST_DESCRIPTION_LEN */

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_get_conninfo_from_bc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDU_Sock_get_conninfo_from_bc( const char *bc, 
				     char *host_description, int maxlen,
				     int *port, void *ifaddr, int *hasIfaddr )
{
    int mpi_errno = MPI_SUCCESS;
#if !defined(HAVE_WINDOWS_H) && defined(HAVE_INET_PTON)
    char ifname[256];
#endif

    mpi_errno = MPIU_Str_get_string_arg(bc, MPIDI_CH3I_HOST_DESCRIPTION_KEY, 
				 host_description, maxlen);
    if (mpi_errno != MPIU_STR_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**argstr_hostd");
    }
    mpi_errno = MPIU_Str_get_int_arg(bc, MPIDI_CH3I_PORT_KEY, port);
    if (mpi_errno != MPIU_STR_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**argstr_port");
    }
    /* ifname is optional */
    /* FIXME: This is a hack to allow Windows to continue to use
       the host description string instead of the interface address
       bytes when posting a socket connection.  This should be fixed 
       by changing the Sock_post_connect to only accept interface
       address.  Note also that Windows does not have the inet_pton 
       routine; the Windows version of this routine will need to 
       be identified or written.  See also channels/sock/ch3_progress.c and
       channels/ssm/ch3_progress_connect.c */
#if !defined(HAVE_WINDOWS_H) && defined(HAVE_INET_PTON)
    mpi_errno = MPIU_Str_get_string_arg(bc, MPIDI_CH3I_IFNAME_KEY, 
					ifname, sizeof(ifname) );
    if (mpi_errno == MPIU_STR_SUCCESS) {
	/* Convert ifname into 4-byte ip address */
	int rc = inet_pton( AF_INET, (const char *)ifname, ifaddr );
	if (rc == 0) {
	    ;/* ifname was not valid */
	}
	else if (rc < 0) {
	    ;/* af_inet not supported */
	}
	else {
	    /* Success */
	    *hasIfaddr = 1;
	}
    }
#endif
    
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


/*  MPIDI_CH3U_Get_business_card_sock - does socket specific portion of 
 *  setting up a business card
 *  
 *  Parameters:
 *     bc_val_p     - business card value buffer pointer, updated to the next 
 *                    available location or freed if published.
 *     val_max_sz_p - ptr to maximum value buffer size reduced by the number 
 *                    of characters written
 *                               
 */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Get_business_card_sock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Get_business_card_sock(char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno;
    int port;
    char host_description[MAX_HOST_DESCRIPTION_LEN];

    mpi_errno = MPIDU_Sock_get_host_description(host_description, MAX_HOST_DESCRIPTION_LEN);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER, "**init_description");
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    port = MPIDI_CH3I_listener_port;
    mpi_errno = MPIU_Str_add_int_arg(bc_val_p, val_max_sz_p, MPIDI_CH3I_PORT_KEY, port);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	if (mpi_errno == MPIU_STR_NOMEM) {
	    MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER, "**buscard_len");
	}
	else {
	    MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER, "**buscard");
	}
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */
    
    mpi_errno = MPIU_Str_add_string_arg(bc_val_p, val_max_sz_p, MPIDI_CH3I_HOST_DESCRIPTION_KEY, host_description);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	if (mpi_errno == MPIU_STR_NOMEM) {
	    MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER, "**buscard_len");
	}
	else {
	    MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER, "**buscard");
	}
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    /* Look up the interface address cooresponding to this host description */
    /* FIXME: We should start switching to getaddrinfo instead of 
       gethostbyname */
    /* FIXME: We don't make use of the ifname in Windows in order to 
       provide backward compatibility with the (undocumented) host
       description string used by the socket connection routine 
       MPIDU_Sock_post_connect.  We need to change to an interface-address
       (already resolved) based description for better scalability and
       to eliminate reliance on fragile DNS services */
#ifndef HAVE_WINDOWS_H
    {
	struct hostent *info;
	char ifname[256];
	unsigned char *p;
	info = gethostbyname( host_description );
	if (info && info->h_addr_list) {
	    p = (unsigned char *)(info->h_addr_list[0]);
	    MPIU_Snprintf( ifname, sizeof(ifname), "%u.%u.%u.%u", 
			   p[0], p[1], p[2], p[3] );
	    MPIU_DBG_MSG_S(CH3_CONNECT,VERBOSE,"ifname = %s",ifname );
	    mpi_errno = MPIU_Str_add_string_arg( bc_val_p, 
						 val_max_sz_p, 
						 MPIDI_CH3I_IFNAME_KEY,
						 ifname );
	    if (mpi_errno != MPIU_STR_SUCCESS) {
		if (mpi_errno == MPIU_STR_NOMEM) {
		    MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER, "**buscard_len");
		}
		else {
		    MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER, "**buscard");
		}
		return mpi_errno;
	    }
	}
    }
#endif
    return MPI_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* Below will be/is the code that is used to create a connection and
 * to handle changes to the state of a connection.  
 */
/* ------------------------------------------------------------------------- */
extern MPIDI_CH3I_Connection_t * MPIDI_CH3I_listener_conn; 
static int connection_post_recv_pkt(MPIDI_CH3I_Connection_t * conn);
static int connection_post_send_pkt(MPIDI_CH3I_Connection_t * conn);
static void connection_post_send_pkt_and_pgid(MPIDI_CH3I_Connection_t * conn);
static int connection_post_sendq_req(MPIDI_CH3I_Connection_t * conn);
static void connection_free(MPIDI_CH3I_Connection_t * conn);

/* */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Sockconn_handle_accept_event
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Sockconn_handle_accept_event( void )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_Connection_t * conn;
    
    mpi_errno = MPIDI_CH3I_Connection_alloc(&conn);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }
    mpi_errno = MPIDU_Sock_accept(MPIDI_CH3I_listener_conn->sock, 
				  MPIDI_CH3I_sock_set, conn, &conn->sock);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,
			    "**ch3|sock|accept");
    }
    
    conn->vc = NULL;
    MPIU_DBG_MSG(CH3_CONNECT,TYPICAL,"Setting state to CONN_STATE_OPEN_LRECV_PKT");
    conn->state = CONN_STATE_OPEN_LRECV_PKT;
    conn->send_active = NULL;
    conn->recv_active = NULL;
    
    mpi_errno = connection_post_recv_pkt(conn);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Sockconn_handle_connect_event
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Sockconn_handle_connect_event( MPIDI_CH3I_Connection_t *conn, 
					     int event_error )
{
    int mpi_errno = MPI_SUCCESS;
    
    /* --BEGIN ERROR HANDLING-- */
    if (event_error != MPI_SUCCESS) {
	mpi_errno = MPIR_Err_create_code(
		  event_error, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, 
		  MPI_ERR_OTHER, "**ch3|sock|connfailed",
		  "**ch3|sock|connfailed %s %d", conn->vc->pg->id, 
		  conn->vc->pg_rank);
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    if (conn->state == CONN_STATE_CONNECTING) {
	MPIU_DBG_MSG(CH3_CONNECT,TYPICAL,"Setting state to CONN_STATE_OPEN_CSEND");
	conn->state = CONN_STATE_OPEN_CSEND;
	MPIDI_Pkt_init(&conn->pkt, MPIDI_CH3I_PKT_SC_OPEN_REQ);
	conn->pkt.sc_open_req.pg_id_len = (int) strlen(MPIDI_Process.my_pg->id) + 1;
	conn->pkt.sc_open_req.pg_rank = MPIR_Process.comm_world->rank;
	
	connection_post_send_pkt_and_pgid(conn);
    }
    else {
	/* CONN_STATE_CONNECT_ACCEPT */
	int port_name_tag;
	
	MPIU_DBG_MSG(CH3_CONNECT,TYPICAL,"Setting state to CONN_STATE_OPEN_CSEND");
	conn->state = CONN_STATE_OPEN_CSEND;
	
	/* pkt contains port name tag. In memory debugging mode, 
	   MPIDI_Pkt_init resets the packet contents. Therefore,
	   save the port name tag and then add it back. */
	port_name_tag = conn->pkt.sc_conn_accept.port_name_tag;
	
	MPIDI_Pkt_init(&conn->pkt, MPIDI_CH3I_PKT_SC_CONN_ACCEPT);
	
	conn->pkt.sc_conn_accept.port_name_tag = port_name_tag;
	
	mpi_errno = connection_post_send_pkt(conn);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_INTERN,
				"**ch3|sock|scconnaccept");
	}
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

int MPIDI_CH3_Sockconn_handle_close_event( MPIDI_CH3I_Connection_t * conn )
{
    int mpi_errno = MPI_SUCCESS;
		
    /* If the conn pointer is NULL then the close was intentional */
    if (conn != NULL) {
	if (conn->state == CONN_STATE_CLOSING) {
	    MPIU_Assert(conn->send_active == NULL);
	    MPIU_Assert(conn->recv_active == NULL);
	    if (conn->vc != NULL) {
		MPIU_DBG_MSG(CH3_CONNECT,TYPICAL,"Setting state to VC_STATE_UNCONNECTED");
		conn->vc->ch.state = MPIDI_CH3I_VC_STATE_UNCONNECTED;
		conn->vc->ch.sock = MPIDU_SOCK_INVALID_SOCK;
		MPIDI_CH3U_Handle_connection(conn->vc, MPIDI_VC_EVENT_TERMINATED);
	    }
	}
	else {
	    MPIU_Assert(conn->state == CONN_STATE_LISTENING);
	    MPIDI_CH3I_listener_conn = NULL;
	    MPIDI_CH3I_listener_port = 0;
	    
	    MPIDI_CH3_Progress_signal_completion();
	    /* MPIDI_CH3I_progress_completion_count++; */
	}
		
	conn->sock = MPIDU_SOCK_INVALID_SOCK;
	MPIU_DBG_MSG(CH3_CONNECT,TYPICAL,"Setting state to CONN_STATE_CLOSED");
	conn->state = CONN_STATE_CLOSED;
	connection_free(conn); 
    }
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* Cycle through the connection setup states */
int MPIDI_CH3_Sockconn_handle_conn_event( MPIDI_CH3I_Connection_t * conn )
{
    int mpi_errno = MPI_SUCCESS;

    if (conn->pkt.type == MPIDI_CH3I_PKT_SC_OPEN_REQ) {
	MPIU_DBG_MSG(CH3_CONNECT,TYPICAL,"Setting state to CONN_STATE_OPEN_LRECV_DATA");
	conn->state = CONN_STATE_OPEN_LRECV_DATA;
	mpi_errno = MPIDU_Sock_post_read(conn->sock, conn->pg_id, conn->pkt.sc_open_req.pg_id_len, 
					 conn->pkt.sc_open_req.pg_id_len, NULL);   
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_POP(mpi_errno);
	}
    }
    else if (conn->pkt.type == MPIDI_CH3I_PKT_SC_CONN_ACCEPT) {
	MPIDI_VC_t *vc; 
	
	vc = (MPIDI_VC_t *) MPIU_Malloc(sizeof(MPIDI_VC_t));
	/* --BEGIN ERROR HANDLING-- */
	if (vc == NULL) {
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
					     "**nomem", NULL);
	    goto fn_fail;
	}
	/* --END ERROR HANDLING-- */
	/* FIXME - where does this vc get freed? */
	
	MPIU_DBG_MSG(CH3_CONNECT,TYPICAL,"Setting state to VC_STATE_CONNECTING");
	MPIDI_VC_Init(vc, NULL, 0);
	vc->ch.sendq_head = NULL;
	vc->ch.sendq_tail = NULL;
	vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTING;
	vc->ch.sock = conn->sock;
	vc->ch.conn = conn;
	conn->vc = vc;
	
	vc->ch.port_name_tag = conn->pkt.sc_conn_accept.port_name_tag;
	
	MPIDI_Pkt_init(&conn->pkt, MPIDI_CH3I_PKT_SC_OPEN_RESP);
	conn->pkt.sc_open_resp.ack = TRUE;
	
	MPIU_DBG_MSG(CH3_CONNECT,TYPICAL,"Setting state to CONN_STATE_OPEN_LSEND");
	conn->state = CONN_STATE_OPEN_LSEND;
	mpi_errno = connection_post_send_pkt(conn);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_INTERN,
				"**ch3|sock|scconnaccept");
	}
	
	/* ENQUEUE vc */
	MPIDI_CH3I_Acceptq_enqueue(vc);

    }
    else if (conn->pkt.type == MPIDI_CH3I_PKT_SC_OPEN_RESP) {
	if (conn->pkt.sc_open_resp.ack) {
	    MPIU_DBG_MSG(CH3_CONNECT,TYPICAL,"Setting state to CONN_STATE_CONNECTED");
	    conn->state = CONN_STATE_CONNECTED;
	    conn->vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTED;
	    MPIU_Assert(conn->vc->ch.conn == conn);
	    MPIU_Assert(conn->vc->ch.sock == conn->sock);
	    
	    mpi_errno = connection_post_recv_pkt(conn);
	    if (mpi_errno != MPI_SUCCESS) {
		MPIU_ERR_POP(mpi_errno);
	    }
	    mpi_errno = connection_post_sendq_req(conn);
	    if (mpi_errno != MPI_SUCCESS) {
		MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_INTERN,
				    "**ch3|sock|scopenresp");
	    }
	}
	else {
	    conn->vc = NULL;
	    MPIU_DBG_MSG(CH3_CONNECT,TYPICAL,"Setting state to CONN_STATE_CLOSING");
	    conn->state = CONN_STATE_CLOSING;
	    MPIDU_Sock_post_close(conn->sock);
	}
    }
    /* --BEGIN ERROR HANDLING-- */
    else {
	MPIDI_DBG_Print_packet(&conn->pkt);
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN,
					 "**ch3|sock|badpacket", "**ch3|sock|badpacket %d", conn->pkt.type);
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */



 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* FIXME: This should really be combined with handle_conn_event */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Sockconn_handle_connopen_event
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Sockconn_handle_connopen_event( MPIDI_CH3I_Connection_t * conn )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_PG_t * pg;
    int pg_rank;
    MPIDI_VC_t * vc;

    /* Look up pg based on conn->pg_id */
    mpi_errno = MPIDI_PG_Find(conn->pg_id, &pg);
    if (pg == NULL) {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,
			     "**pglookup", 
			     "**pglookup %s", conn->pg_id);
    }
    
    pg_rank = conn->pkt.sc_open_req.pg_rank;
    MPIDI_PG_Get_vc(pg, pg_rank, &vc);
    MPIU_Assert(vc->pg_rank == pg_rank);
    
    if (vc->ch.conn == NULL) {
	/* no head-to-head connects, accept the connection */
	MPIU_DBG_MSG(CH3_CONNECT,TYPICAL,"Setting state to VC_STATE_CONNECTING");
	vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTING;
	vc->ch.sock = conn->sock;
	vc->ch.conn = conn;
	conn->vc = vc;
	
	MPIDI_Pkt_init(&conn->pkt, MPIDI_CH3I_PKT_SC_OPEN_RESP);
	conn->pkt.sc_open_resp.ack = TRUE;
    }
    else {
	/* head to head situation */
	if (pg == MPIDI_Process.my_pg) {
	    /* the other process is in the same comm_world; just compare the ranks */
	    if (MPIR_Process.comm_world->rank < pg_rank) {
		/* accept connection */
		MPIU_DBG_MSG(CH3_CONNECT,TYPICAL,"Setting state to VC_STATE_CONNECTING");
		vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTING;
		vc->ch.sock = conn->sock;
		vc->ch.conn = conn;
		conn->vc = vc;
		
		MPIDI_Pkt_init(&conn->pkt, MPIDI_CH3I_PKT_SC_OPEN_RESP);
		conn->pkt.sc_open_resp.ack = TRUE;
	    }
	    else {
		/* refuse connection */
		MPIDI_Pkt_init(&conn->pkt, MPIDI_CH3I_PKT_SC_OPEN_RESP);
		conn->pkt.sc_open_resp.ack = FALSE;
	    }
	}
	else {
	    /* the two processes are in different comm_worlds; compare their unique pg_ids. */
	    if (strcmp(MPIDI_Process.my_pg->id, pg->id) < 0) {
		/* accept connection */
		MPIU_DBG_MSG(CH3_CONNECT,TYPICAL,"Setting state to VC_STATE_CONNECTING");
		vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTING;
		vc->ch.sock = conn->sock;
		vc->ch.conn = conn;
		conn->vc = vc;
		
		MPIDI_Pkt_init(&conn->pkt, MPIDI_CH3I_PKT_SC_OPEN_RESP);
		conn->pkt.sc_open_resp.ack = TRUE;
	    }
	    else {
		/* refuse connection */
		MPIDI_Pkt_init(&conn->pkt, MPIDI_CH3I_PKT_SC_OPEN_RESP);
		conn->pkt.sc_open_resp.ack = FALSE;
	    }
	}
    }
    
    MPIU_DBG_MSG(CH3_CONNECT,TYPICAL,"Setting state to CONN_STATE_OPEN_LSEND");
    conn->state = CONN_STATE_OPEN_LSEND;
    mpi_errno = connection_post_send_pkt(conn);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_INTERN,
			    "**ch3|sock|open_lrecv_data");
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Sockconn_handle_connwrite
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Sockconn_handle_connwrite( MPIDI_CH3I_Connection_t * conn )
{
    int mpi_errno = MPI_SUCCESS;

    if (conn->state == CONN_STATE_OPEN_CSEND) {
	/* finished sending open request packet */
	/* post receive for open response packet */
	MPIU_DBG_MSG(CH3_CONNECT,TYPICAL,"Setting state to CONN_STATE_OPEN_CRECV");
	conn->state = CONN_STATE_OPEN_CRECV;
	mpi_errno = connection_post_recv_pkt(conn);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_POP(mpi_errno);
	}
    }
    else if (conn->state == CONN_STATE_OPEN_LSEND) {
	/* finished sending open response packet */
	if (conn->pkt.sc_open_resp.ack == TRUE) {
	    /* post receive for packet header */
	    MPIU_DBG_MSG(CH3_CONNECT,TYPICAL,"Setting state to CONN_STATE_CONNECTED");
	    conn->state = CONN_STATE_CONNECTED;
	    MPIU_DBG_MSG(CH3_CONNECT,TYPICAL,"Setting state to VC_STATE_CONNECTED");
	    conn->vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTED;
	    mpi_errno = connection_post_recv_pkt(conn);
	    if (mpi_errno != MPI_SUCCESS) {
		MPIU_ERR_POP(mpi_errno);
	    }
	    
	    mpi_errno = connection_post_sendq_req(conn);
	    if (mpi_errno != MPI_SUCCESS) {
		MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_INTERN,
				    "**ch3|sock|openlsend");
	    }
	}
	else {
	    /* head-to-head connections - close this connection */
	    MPIU_DBG_MSG(CH3_CONNECT,TYPICAL,"Setting state to CONN_STATE_CLOSIANG");
	    conn->state = CONN_STATE_CLOSING;
	    mpi_errno = MPIDU_Sock_post_close(conn->sock);
	    if (mpi_errno != MPI_SUCCESS) {
		MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,
				    "**sock_post_close");
	    }
	}
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* ----------------------------------------------------------------------- */
/* FIXME: What does this do? */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_VC_post_connect
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_VC_post_sockconnect(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;
    char key[MPIDI_MAX_KVS_KEY_LEN];
    char val[MPIDI_MAX_KVS_VALUE_LEN];
    char host_description[MAX_HOST_DESCRIPTION_LEN];
    int port;
    unsigned char ifaddr[4];
    int hasIfaddr = 0;
    int rc;
    MPIDI_CH3I_Connection_t * conn = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
    
    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));

    MPIU_Assert(vc->ch.state == MPIDI_CH3I_VC_STATE_UNCONNECTED);
    
    MPIU_DBG_MSG(CH3_CONNECT,TYPICAL,"Setting state to VC_STATE_CONNECTING");
    vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTING;

    rc = MPIU_Snprintf(key, MPIDI_MAX_KVS_KEY_LEN, "P%d-businesscard", vc->pg_rank);
    /* --BEGIN ERROR HANDLING-- */
    if (rc < 0 || rc > MPIDI_MAX_KVS_KEY_LEN)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", NULL);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */
    mpi_errno = MPIDI_KVS_Get(vc->pg->ch.kvs_name, key, val);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }

    mpi_errno = MPIDU_Sock_get_conninfo_from_bc( val, host_description,
						 sizeof(host_description),
						 &port, ifaddr, &hasIfaddr );
    if (mpi_errno) {
	MPIU_ERR_POP(mpi_errno);
    }

    mpi_errno = MPIDI_CH3I_Connection_alloc(&conn);
    if (mpi_errno == MPI_SUCCESS)
    {
	/* FIXME: This is a hack to allow Windows to continue to use
	   the host description string instead of the interface address
	   bytes when posting a socket connection.  This should be fixed 
	   by changing the Sock_post_connect to only accept interface
	   address.  See also channels/ssm/ch3_progress_connect.c */
#ifndef HAVE_WINDOWS_H
	if (hasIfaddr) {
	    mpi_errno = MPIDU_Sock_post_connect_ifaddr(MPIDI_CH3I_sock_set, 
						       conn, ifaddr, port, 
						       &conn->sock);
	}
	else 
#endif
	{
	    mpi_errno = MPIDU_Sock_post_connect(MPIDI_CH3I_sock_set, conn, 
						host_description, port, 
						&conn->sock);
	}
	if (mpi_errno == MPI_SUCCESS)
	{
	    MPIU_DBG_MSG(CH3_CONNECT,TYPICAL,"Setting state to CONN_STATE_CONNECTING");
	    vc->ch.sock = conn->sock;
	    vc->ch.conn = conn;
	    conn->vc = vc;
	    conn->state = CONN_STATE_CONNECTING;
	    conn->send_active = NULL;
	    conn->recv_active = NULL;
	}
	/* --BEGIN ERROR HANDLING-- */
	else
	{
	    MPIU_DBG_MSG(CH3_CONNECT,TYPICAL,"Setting state to VC_STATE_FAILED");
	    vc->ch.state = MPIDI_CH3I_VC_STATE_FAILED;
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|postconnect",
		"**ch3|sock|postconnect %d %d %s", MPIR_Process.comm_world->rank, vc->pg_rank, val);
	    goto fn_fail;
	}
	/* --END ERROR HANDLING-- */
    }
    else {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**ch3|sock|connalloc");
    }

  fn_exit:
    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
    return mpi_errno;
 fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (conn) {
	connection_free(conn);
    }
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
/* end MPIDI_CH3I_VC_post_sockconnect() */


/* FIXME: What does this do? */
/* Guess: Setup a wait-to-read on the socket that was set after the accept 
   was handled */
/* Wrong guess.  */
#undef FUNCNAME
#define FUNCNAME connection_post_recv_pkt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int connection_post_recv_pkt(MPIDI_CH3I_Connection_t * conn)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_POST_RECV_PKT);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_POST_RECV_PKT);

    mpi_errno = MPIDU_Sock_post_read(conn->sock, &conn->pkt, sizeof(conn->pkt),
				     sizeof(conn->pkt), NULL);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER, "**fail");
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_POST_RECV_PKT);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME connection_post_send_pkt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int connection_post_send_pkt(MPIDI_CH3I_Connection_t * conn)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_POST_SEND_PKT);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_POST_SEND_PKT);
    
    mpi_errno = MPIDU_Sock_post_write(conn->sock, &conn->pkt, sizeof(conn->pkt), sizeof(conn->pkt), NULL);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER, "**fail");
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_POST_SEND_PKT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME connection_post_send_pkt_and_pgid
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static void connection_post_send_pkt_and_pgid(MPIDI_CH3I_Connection_t * conn)
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_POST_SEND_PKT_AND_PGID);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_POST_SEND_PKT_AND_PGID);
    
    conn->iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) &conn->pkt;
    conn->iov[0].MPID_IOV_LEN = (int) sizeof(conn->pkt);

    conn->iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) MPIDI_Process.my_pg->id;
    conn->iov[1].MPID_IOV_LEN = (int) strlen(MPIDI_Process.my_pg->id) + 1;

    mpi_errno = MPIDU_Sock_post_writev(conn->sock, conn->iov, 2, NULL);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER, "**fail");
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_POST_SEND_PKT_AND_PGID);
}

#undef FUNCNAME
#define FUNCNAME connection_post_sendq_req
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int connection_post_sendq_req(MPIDI_CH3I_Connection_t * conn)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_POST_SENDQ_REQ);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_POST_SENDQ_REQ);
    /* post send of next request on the send queue */
    conn->send_active = MPIDI_CH3I_SendQ_head(conn->vc); /* MT */
    if (conn->send_active != NULL)
    {
	mpi_errno = MPIDU_Sock_post_writev(conn->sock, conn->send_active->dev.iov, conn->send_active->dev.iov_count, NULL);
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", NULL);
	}
	/* --END ERROR HANDLING-- */
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_POST_SENDQ_REQ);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME connection_free
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static void connection_free(MPIDI_CH3I_Connection_t * conn)
{
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_FREE);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_FREE);

    MPIU_Free(conn->pg_id);
    MPIU_Free(conn);
    
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_FREE);
}
