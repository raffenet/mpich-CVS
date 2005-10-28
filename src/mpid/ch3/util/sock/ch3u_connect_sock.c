/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"

#include "mpidu_sock.h"

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
#define MPIDI_CH3I_HOST_KEY              "host"
#define MPIDI_CH3I_PORT_KEY              "port"

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
    MPIDI_CH3I_Connection_t * conn;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT_SOCK);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT_SOCK);

    MPIU_DBG_MSG_S(CH3_CONNECT,VERBOSE,"Connect to root with portstring %s",
		   port_name );

    mpi_errno = MPIDU_Sock_get_conninfo_from_bc( port_name, host_description,
						 sizeof(host_description),
						 &port );
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
				     int *port )
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIU_Str_get_string_arg(bc, MPIDI_CH3I_HOST_DESCRIPTION_KEY, 
				 host_description, maxlen);
    if (mpi_errno != MPIU_STR_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**argstr_hostd");
    }
    mpi_errno = MPIU_Str_get_int_arg(bc, MPIDI_CH3I_PORT_KEY, port);
    if (mpi_errno != MPIU_STR_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**argstr_port");
    }
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


/*  MPIDI_CH3U_Get_business_card_sock - does socket specific portion of 
 *  getting a business card
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
    return MPI_SUCCESS;
}

