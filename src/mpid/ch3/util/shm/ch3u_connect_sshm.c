/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"


#undef FUNCNAME
#define FUNCNAME  MPIDI_CH3I_Connect_to_root_sshm
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Connect_to_root_sshm(const char * port_name, 
				    MPIDI_VC_t ** new_vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t * vc;
    MPIU_CHKPMEM_DECL(1);
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

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}
