/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"

int MPIDI_CH3I_Setup_connections()
{
    char * key;
    char * val;
    int key_max_sz;
    int val_max_sz;
/*    int rc;*/
    int i;
    MPIDI_VC *vc;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SETUP_CONNECTIONS);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SETUP_CONNECTIONS);

    key_max_sz = PMI_KVS_Get_key_length_max();
    key = MPIU_Malloc(key_max_sz);
    assert(key != NULL);
    val_max_sz = PMI_KVS_Get_value_length_max();
    val = MPIU_Malloc(val_max_sz);
    assert(val != NULL);
    
    /* create a queue pair connection to each process */
    for (i=0; i<MPIDI_CH3I_Process.pg->size; i++)
    {
	vc = &MPIDI_CH3I_Process.pg->vc_table[i];

	if (vc->shm.pg_rank == MPIR_Process.comm_world->rank)
	    continue;
#if 0
	/* get the destination lid from the pmi database */
	rc = snprintf(key, key_max_sz, "P%d-lid", vc->ib.pg_rank);
	assert(rc > -1 && rc < key_max_sz);
	rc = PMI_KVS_Get(vc->ib.pg->kvs_name, key, val);
	assert(rc == 0);
	dlid = atoi(val);
	assert(dlid >= 0);
	/* connect to the dlid */
	MPIU_DBG_PRINTF(("calling ibu_create_qp(%d)\n", dlid));
	vc->ib.ibu = ibu_create_qp(MPIDI_CH3I_Process.set, dlid);
	assert(vc->ib.ibu != NULL);
	if (vc->ib.ibu == NULL)
	    err_printf("CH3I_Setup_connections: ibu_create_qp failed.\n");
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
	ibu_post_read(vc->ib.ibu, &vc->ib.req->ib.pkt, sizeof(MPIDI_CH3_Pkt_t), NULL);
#endif
    }

    PMI_Barrier();

    MPIU_Free(val);
    MPIU_Free(key);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SETUP_CONNECTIONS);
    return 0;
}
