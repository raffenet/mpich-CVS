/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"
#include "ibu.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Setup_connections
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Setup_connections()
{
    int mpi_errno;
    char * key;
    char * val;
    int key_max_sz;
    int val_max_sz;
    int i, dlid;
    MPIDI_VC *vc;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SETUP_CONNECTIONS);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SETUP_CONNECTIONS);

    key_max_sz = PMI_KVS_Get_key_length_max();
    key = MPIU_Malloc(key_max_sz);
    if (key == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }
    val_max_sz = PMI_KVS_Get_value_length_max();
    val = MPIU_Malloc(val_max_sz);
    if (val == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }
    
    /* create a queue pair connection to each process */
    for (i=0; i<MPIDI_CH3I_Process.pg->size; i++)
    {
	vc = &MPIDI_CH3I_Process.pg->vc_table[i];

	if (vc->ib.pg_rank == MPIR_Process.comm_world->rank)
	    continue;
	/* get the destination lid from the pmi database */
	mpi_errno = MPIU_Snprintf(key, key_max_sz, "P%d-lid", vc->ib.pg_rank);
	if (mpi_errno < 0 || mpi_errno > key_max_sz)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**snprintf", "**snprintf %d", mpi_errno);
	    return mpi_errno;
	}
	mpi_errno = PMI_KVS_Get(vc->ib.pg->kvs_name, key, val);
	if (mpi_errno != 0)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get %d", mpi_errno);
	    return mpi_errno;
	}
	dlid = atoi(val);
	if (dlid < 0)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**arg", 0);
	    return mpi_errno;
	}
	/* connect to the dlid */
	MPIU_DBG_PRINTF(("calling ibu_create_qp(%d)\n", dlid));
	vc->ib.ibu = ibu_create_qp(MPIDI_CH3I_Process.set, dlid);
	if (vc->ib.ibu == NULL)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	    return mpi_errno;
	}
	/* set the user pointer to be a pointer to the VC */
	ibu_set_user_ptr(vc->ib.ibu, &MPIDI_CH3I_Process.pg->vc_table[i]);
	/* set the state to connected */
	vc->ib.state = MPIDI_CH3I_VC_STATE_CONNECTED;
	/* post a read of the first packet */
	MPIU_DBG_PRINTF(("posting first packet receive of %d bytes\n", sizeof(MPIDI_CH3_Pkt_t)));
	vc->ib.req->ch3.iov[0].MPID_IOV_BUF = (void *)&vc->ib.req->ib.pkt;
	vc->ib.req->ch3.iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
	vc->ib.req->ch3.iov_count = 1;
	vc->ib.req->ib.iov_offset = 0;
	vc->ib.req->ch3.ca = MPIDI_CH3I_CA_HANDLE_PKT;
	vc->ib.recv_active = vc->ib.req;
	mpi_errno = ibu_post_read(vc->ib.ibu, &vc->ib.req->ib.pkt, sizeof(MPIDI_CH3_Pkt_t), NULL);
    }

    PMI_Barrier();

    MPIU_Free(val);
    MPIU_Free(key);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SETUP_CONNECTIONS);
    return 0;
}
