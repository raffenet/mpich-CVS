/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"

#ifdef MPIDI_CH3_USES_SOCK
#include "mpidu_sock.h"

/* brad : this file's functionality is used to do a connect (MPI-2) by way
 *         of a socket.  formerly, it was a part of the sock channel's
 *         ch3_progress.c file but it was copied here and 
 */



#undef FUNCNAME
#define FUNCNAME connection_alloc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int connection_alloc(MPIDI_CH3I_Connection_t ** connp)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_Connection_t * conn = NULL;
    int id_sz;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_ALLOC);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_ALLOC);
    conn = MPIU_Malloc(sizeof(MPIDI_CH3I_Connection_t));
    /* --BEGIN ERROR HANDLING-- */
    if (conn == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**ch3|sock|connallocfailed", NULL);
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */
    conn->pg_id = NULL;
    
    mpi_errno = PMI_Get_id_length_max(&id_sz);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != PMI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_id_length_max",
					 "**pmi_get_id_length_max %d", mpi_errno);
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */
    conn->pg_id = MPIU_Malloc(id_sz + 1);
    /* --BEGIN ERROR HANDLING-- */
    if (conn->pg_id == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", NULL);
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    *connp = conn;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_ALLOC);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    if (conn != NULL)
    {
	if (conn->pg_id != NULL)
	{
	    MPIU_Free(conn->pg_id);
	}
	
	MPIU_Free(conn);
    }

    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
#endif



#undef FUNCNAME
#define FUNCNAME  MPIDI_CH3I_Connect_to_root
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Connect_to_root(char * port_name, MPIDI_VC_t ** new_vc)
{
    int mpi_errno = MPI_SUCCESS;
#ifdef MPIDI_CH3_USES_SOCK   /* brad : TODO originally this function came from the sock
                              *         channel.  the placement and name of these ifdef
                              *         is probably wrong since the connection between
                              *         roots could be via shared memory.
                              */
    /* Used in ch3_comm_connect to connect with the process calling ch3_comm_accept */
    char host_description[MAX_HOST_DESCRIPTION_LEN];
    int port, port_name_tag;
    MPIDI_VC_t * vc;
    MPIDI_CH3I_Connection_t * conn;

    
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT);

    mpi_errno = MPIU_Str_get_string_arg(port_name, MPIDI_CH3I_HOST_DESCRIPTION_KEY, host_description, MAX_HOST_DESCRIPTION_LEN);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**argstr_hostd", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT);
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */
    mpi_errno = MPIU_Str_get_int_arg(port_name, MPIDI_CH3I_PORT_KEY, &port);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**argstr_port", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT);
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    mpi_errno = MPIU_Str_get_int_arg(port_name, MPIDI_CH3I_PORT_NAME_TAG_KEY, &port_name_tag);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**argstr_port_name_tag", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT);
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */    

    
    vc = (MPIDI_VC_t *) MPIU_Malloc(sizeof(MPIDI_VC_t));
    /* --BEGIN ERROR HANDLING-- */
    if (vc == NULL)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", NULL);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */
    /* FIXME - where does this vc get freed? */

    *new_vc = vc;

    MPIDI_VC_Init(vc, NULL, 0);
    vc->ch.sendq_head = NULL;
    vc->ch.sendq_tail = NULL;
    vc->ch.state = MPIDI_CH3I_VC_STATE_UNCONNECTED;
    vc->ch.sock = MPIDU_SOCK_INVALID_SOCK;
    vc->ch.conn = NULL;
    
    mpi_errno = connection_alloc(&conn);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", NULL);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    /* conn->pg_id is not used for this conection */

    mpi_errno = MPIDU_Sock_post_connect(sock_set, conn, host_description, port, &conn->sock);
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
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", NULL);
	}
        vc->ch.state = MPIDI_CH3I_VC_STATE_FAILED;
        MPIU_Free(conn);
        goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

#ifdef MPIDI_CH3_USES_SSHM
    /* TODO - add shared memory version */
#endif

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT);
#endif    
    return mpi_errno;
}
/* MPIDI_CH3I_Connect_to_root() */


