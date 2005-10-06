/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"


#undef FUNCNAME
#define FUNCNAME  MPIDI_CH3I_Connect_to_root
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Connect_to_root(const char * port_name, MPIDI_VC_t ** new_vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t * vc;
    MPIU_CHKPMEM_DECL(1);
#ifdef MPIDI_CH3_USES_SOCK
    /* Used in ch3_comm_connect to connect with the process calling ch3_comm_accept */
    char host_description[MAX_HOST_DESCRIPTION_LEN];
    int port, port_name_tag;
    MPIDI_CH3I_Connection_t * conn;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT);

    mpi_errno = MPIU_Str_get_string_arg(port_name, MPIDI_CH3I_HOST_DESCRIPTION_KEY, host_description, MAX_HOST_DESCRIPTION_LEN);
    if (mpi_errno != MPIU_STR_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**argstr_hostd");
    }
    mpi_errno = MPIU_Str_get_int_arg(port_name, MPIDI_CH3I_PORT_KEY, &port);
    if (mpi_errno != MPIU_STR_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**argstr_port");
    }
    mpi_errno = MPIDI_GetTagFromPort(port_name, &port_name_tag);
    if (mpi_errno != MPIU_STR_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**argstr_port_name_tag");
    }

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
    mpi_errno = MPIDU_Sock_post_connect(MPIDI_CH3I_sock_set, conn, host_description, port, &conn->sock);
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

/* FIXME: Heres an example of a problem with the large number of "USES" 
   codes - a channel might define several USES and then code with #elif 
   will give you just one */
#elif defined(MPIDI_CH3_USES_SSHM)
    int port_name_tag;
    int connected;
    MPIDI_CH3_Pkt_t pkt;
    int num_written;
    char *cached_pg_id, *dummy_id = "";
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT);

    mpi_errno = MPIDI_GetTagFromPort(port_name, &port_name_tag);
    if (mpi_errno != MPIU_STR_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**argstr_port_name_tag");
    }

    if (*new_vc != NULL)
    {
	vc = *new_vc;
    }
    else
    {
	MPIU_CHKPMEM_MALLOC(vc,MPIDI_VC_t *,sizeof(MPIDI_VC_t),mpi_errno,"vc");
	/* FIXME - where does this vc get freed? */

	*new_vc = vc;

	MPIDI_VC_Init(vc, NULL, 0);
	MPIDI_CH3_VC_Init(vc);
    }

    vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTING;

    connected = FALSE;
    /* Make is so that the pg_id is not matched on the other side by sending a dummy value */
    cached_pg_id = MPIDI_Process.my_pg->id;
    MPIDI_Process.my_pg->id = dummy_id;
    mpi_errno = MPIDI_CH3I_Shm_connect(vc, port_name, &connected);
    MPIDI_Process.my_pg->id = cached_pg_id;
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER, "**fail");
	vc->ch.state = MPIDI_CH3I_VC_STATE_FAILED;
	goto fn_exit;
    }
    if (!connected)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "unable to establish a shared memory queue connection");
	goto fn_exit;
    }

    MPIDI_CH3I_SHM_Add_to_writer_list(vc);
    vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTED;
    vc->ch.shm_reading_pkt = TRUE;
    vc->ch.send_active = MPIDI_CH3I_SendQ_head(vc); /* MT */

    MPIDI_Pkt_init(&pkt, MPIDI_CH3I_PKT_SC_CONN_ACCEPT);
    pkt.sc_conn_accept.port_name_tag = port_name_tag;

    mpi_errno = MPIDI_CH3I_SHM_write(vc, &pkt, sizeof(MPIDI_CH3_Pkt_t), &num_written);
    if (mpi_errno != MPI_SUCCESS || num_written != sizeof(MPIDI_CH3_Pkt_t)) {
	MPIU_ERR_POP(mpi_errno);
    }

#else
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT);

    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**notimpl", "**notimpl %s", FCNAME);
#endif

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}
/* MPIDI_CH3I_Connect_to_root() */
