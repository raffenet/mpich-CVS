/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"

#ifdef MPIDI_CH3_USES_SOCK
#include "mpidu_sock.h"

int MPIDI_CH3I_Connection_alloc(MPIDI_CH3I_Connection_t **);

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Connection_alloc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Connection_alloc(MPIDI_CH3I_Connection_t ** connp)
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
#define FUNCNAME  MPIDI_CH3I_Initialize_tmp_comm
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Initialize_tmp_comm(MPID_Comm **comm_pptr, MPIDI_VC_t *vc_ptr, int is_low_group)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *tmp_comm, *commself_ptr;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_INITIALIZE_TMP_COMM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_INITIALIZE_TMP_COMM);

    MPID_Comm_get_ptr( MPI_COMM_SELF, commself_ptr );
    mpi_errno = MPIR_Comm_create(commself_ptr, &tmp_comm);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    /* fill in all the fields of tmp_comm. */

    tmp_comm->context_id = 4095;  /* FIXME - we probably need a unique context_id. */
    tmp_comm->remote_size = 1;

    /* Fill in new intercomm */
    tmp_comm->attributes   = NULL;
    tmp_comm->local_size   = 1;
    tmp_comm->rank         = 0;
    tmp_comm->local_group  = NULL;
    tmp_comm->remote_group = NULL;
    tmp_comm->comm_kind    = MPID_INTERCOMM;
    tmp_comm->local_comm   = NULL;
    tmp_comm->is_low_group = is_low_group;
    tmp_comm->coll_fns     = NULL;

    /* No pg structure needed since vc has already been set up (connection has been established). */

    /* Point local vcr, vcrt at those of commself_ptr */
    tmp_comm->local_vcrt = commself_ptr->vcrt;
    MPID_VCRT_Add_ref(commself_ptr->vcrt);
    tmp_comm->local_vcr  = commself_ptr->vcr;

    /* No pg needed since connection has already been formed. 
       FIXME - ensure that the comm_release code does not try to
       free an unallocated pg */

    /* Set up VC reference table */
    mpi_errno = MPID_VCRT_Create(tmp_comm->remote_size, &tmp_comm->vcrt);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_vcrt", 0);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */
    mpi_errno = MPID_VCRT_Get_ptr(tmp_comm->vcrt, &tmp_comm->vcr);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_getptr", 0);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    MPID_VCR_Dup(vc_ptr, tmp_comm->vcr);

    *comm_pptr = tmp_comm;

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_INITIALIZE_TMP_COMM);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME  MPIDI_CH3I_Connect_to_root
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Connect_to_root(char * port_name, MPIDI_VC_t ** new_vc)
{
    int mpi_errno = MPI_SUCCESS;
#ifdef MPIDI_CH3_USES_SOCK
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
#ifdef MPIDI_CH3_USES_SSHM
    vc->ch.recv_active = NULL;
    vc->ch.send_active = NULL;
    vc->ch.req = NULL;
    vc->ch.read_shmq = NULL;
    vc->ch.write_shmq = NULL;
    vc->ch.shm = NULL;
    vc->ch.shm_state = 0;
    vc->ch.shm_next_reader = NULL;
    vc->ch.shm_next_writer = NULL;
    vc->ch.shm_read_connected = 0;
    vc->ch.bShm = FALSE;
#endif

    mpi_errno = MPIDI_CH3I_Connection_alloc(&conn);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", NULL);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    /* conn->pg_id is not used for this conection */

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
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", NULL);
	}
        vc->ch.state = MPIDI_CH3I_VC_STATE_FAILED;
        MPIU_Free(conn);
        goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

#elif defined(MPIDI_CH3_USES_SSHM)
    int port_name_tag;
    MPIDI_VC_t * vc;
    int connected;
    MPIDI_CH3_Pkt_t pkt;
    int num_written;
    char *cached_pg_id, *dummy_id = "";
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT);

    mpi_errno = MPIU_Str_get_int_arg(port_name, MPIDI_CH3I_PORT_NAME_TAG_KEY, &port_name_tag);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**argstr_port_name_tag", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT);
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    if (*new_vc != NULL)
    {
	vc = *new_vc;
    }
    else
    {
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
	vc->ch.recv_active = NULL;
	vc->ch.send_active = NULL;
	vc->ch.req = NULL;
	vc->ch.read_shmq = NULL;
	vc->ch.write_shmq = NULL;
	vc->ch.shm = NULL;
	vc->ch.shm_state = 0;
	vc->ch.shm_next_reader = NULL;
	vc->ch.shm_next_writer = NULL;
	vc->ch.shm_read_connected = 0;
    }

    vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTING;

    connected = FALSE;
    /* Make is so that the pg_id is not matched on the other side by sending a dummy value */
    cached_pg_id = MPIDI_Process.my_pg->id;
    MPIDI_Process.my_pg->id = dummy_id;
    mpi_errno = MPIDI_CH3I_Shm_connect(vc, port_name, &connected);
    MPIDI_Process.my_pg->id = cached_pg_id;
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
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
    if (mpi_errno != MPI_SUCCESS || num_written != sizeof(MPIDI_CH3_Pkt_t))
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	goto fn_exit;
    }

#else
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT);

    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**notimpl", "**notimpl %s", FCNAME);
#endif

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT);
    return mpi_errno;
}
/* MPIDI_CH3I_Connect_to_root() */


