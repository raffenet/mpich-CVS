/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"
#include "ibu.h"

int MPIDI_CH3I_Setup_connections()
{
    char * key;
    char * val;
    int key_max_sz;
    int val_max_sz;
    int rc;
    int i, dlid;

    key_max_sz = PMI_KVS_Get_key_length_max();
    key = MPIU_Malloc(key_max_sz);
    assert(key != NULL);
    val_max_sz = PMI_KVS_Get_value_length_max();
    val = MPIU_Malloc(val_max_sz);
    assert(val != NULL);
    
    /* create a queue pair connection to each process */
    for (i=0; i<MPIDI_CH3I_Process.pg->size; i++)
    {
	if (MPIDI_CH3I_Process.pg->vc_table[i].ib.pg_rank == MPIR_Process.comm_world->rank)
	    continue;
	/* get the destination lid from the pmi database */
	rc = snprintf(key, key_max_sz, "P%d-lid", MPIDI_CH3I_Process.pg->vc_table[i].ib.pg_rank);
	assert(rc > -1 && rc < key_max_sz);
	rc = PMI_KVS_Get(MPIDI_CH3I_Process.pg->vc_table[i].ib.pg->kvs_name, key, val);
	assert(rc == 0);
	dlid = atoi(val);
	assert(dlid >= 0);
	/* connect to the dlid */
	MPIDI_CH3I_Process.pg->vc_table[i].ib.ibu =
	    ibu_create_qp(MPIDI_CH3I_Process.set, dlid);
	assert(MPIDI_CH3I_Process.pg->vc_table[i].ib.ibu != NULL);
	MPIDI_CH3I_Process.pg->vc_table[i].ib.state = MPIDI_CH3I_VC_STATE_CONNECTED;
    }

    MPIU_Free(val);
    MPIU_Free(key);

    return 0;
}
