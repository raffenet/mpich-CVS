/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"
#include "ibu.h"

MPIDI_CH3I_Process_t MPIDI_CH3I_Process;

/* XXX - all calls to assert() need to be turned into real error checking and
   return meaningful errors */

int MPIDI_CH3_Init(int * has_args, int * has_env, int * has_parent)
{
    int rc;
    MPIDI_CH3I_Process_group_t * pg;
    
    int pg_rank;
    int pg_size;
    MPIDI_VC * vc_table;
    MPID_Comm * comm;
    int p;
	
    int port;
    char * key;
    char * val;
    int key_max_sz;
    int val_max_sz;
    
    /*
     * Extract process group related information from PMI and initialize
     * structures that track the process group connections, MPI_COMM_WORLD, and
     * MPI_COMM_SELF
     */
    rc = PMI_Init(has_parent);
    assert(rc == 0);
    rc |= PMI_Get_rank(&pg_rank);
    assert(rc == 0);
    rc |= PMI_Get_size(&pg_size);
    assert(rc == 0);
    
    /*MPIU_Timer_init(pg_rank, pg_size);*/

    /* Allocate process group data structure and populate */
    pg = MPIU_Malloc(sizeof(MPIDI_CH3I_Process_group_t));
    assert(pg != NULL);
    pg->size = pg_size;
    pg->kvs_name = MPIU_Malloc(PMI_KVS_Get_name_length_max() + 1);
    assert(pg->kvs_name != NULL);
    rc = PMI_KVS_Get_my_name(pg->kvs_name);
    assert(rc == 0);
    pg->ref_count = 1;
    MPIDI_CH3I_Process.pg = pg;
    
    /* Allocate and initialize the VC table associated with this process
       group (and thus COMM_WORLD) */
    vc_table = MPIU_Malloc(sizeof(MPIDI_VC) * pg_size);
    assert(vc_table != NULL);
    pg->ref_count += pg_size;
    for (p = 0; p < pg_size; p++)
    {
	MPIDI_CH3U_VC_init(&vc_table[p], p);
	vc_table[p].ib.pg = pg;
	vc_table[p].ib.pg_rank = p;
	vc_table[p].ib.sendq_head = NULL;
	vc_table[p].ib.sendq_tail = NULL;
	vc_table[p].ib.req = (MPID_Request*)MPIU_Malloc(sizeof(MPID_Request));
	vc_table[p].ib.state = MPIDI_CH3I_VC_STATE_UNCONNECTED;
    }
    pg->vc_table = vc_table;
    
    /* Initialize MPI_COMM_WORLD object */
    comm = MPIR_Process.comm_world;
    comm->rank = pg_rank;
    comm->remote_size = comm->local_size = pg_size;
    rc = MPID_VCRT_Create(comm->remote_size, &comm->vcrt);
    assert(rc == MPI_SUCCESS);
    rc = MPID_VCRT_Get_ptr(comm->vcrt, &comm->vcr);
    assert(rc == MPI_SUCCESS);
    for (p = 0; p < pg_size; p++)
    {
	MPID_VCR_Dup(&vc_table[p], &comm->vcr[p]);
    }
    
    /* Initialize MPI_COMM_SELF object */
    comm = MPIR_Process.comm_self;
    comm->rank = 0;
    comm->remote_size = comm->local_size = 1;
    rc = MPID_VCRT_Create(comm->remote_size, &comm->vcrt);
    assert(rc == MPI_SUCCESS);
    rc = MPID_VCRT_Get_ptr(comm->vcrt, &comm->vcr);
    assert(rc == MPI_SUCCESS);
    MPID_VCR_Dup(&vc_table[pg_rank], &comm->vcr[0]);

    /*
     * Initialize Progress Engine (and setup listener socket)
     */
    rc = MPIDI_CH3I_Progress_init();
    assert(rc == MPI_SUCCESS);
    
    /*
     * Publish the contact info for this process in the PMI keyval space
     *
     * XXX - need to check sizes of values to insure array overruns do not occur
     */
    key_max_sz = PMI_KVS_Get_key_length_max()+1;
    key = MPIU_Malloc(key_max_sz);
    assert(key != NULL);
    val_max_sz = PMI_KVS_Get_value_length_max()+1;
    val = MPIU_Malloc(val_max_sz);
    assert(val != NULL);
    
    /* initialize the infinband functions */
    rc = ibu_init();
    assert(rc == VAPI_OK);
    /* create a completion set for this process */
    rc = ibu_create_set(&MPIDI_CH3I_Process.set);
    assert(rc == VAPI_OK);
    /* get and put the local id for this process in the PMI database */
    port = ibu_get_lid();

    rc = snprintf(key, key_max_sz, "P%d-lid", pg_rank);
    assert(rc > -1 && rc < key_max_sz);
    rc = snprintf(val, val_max_sz, "%d", port);
    assert(rc > -1 && rc < val_max_sz);
    rc = PMI_KVS_Put(pg->kvs_name, key, val);
    assert(rc == 0);

    dbg_printf("[%d] Published lid=%d\n", pg_rank, port);
    
    rc = PMI_KVS_Commit(pg->kvs_name);
    assert(rc == 0);
    rc = PMI_Barrier();
    assert(rc == 0);

    /*
    {
	for (p = 0; p < pg_size; p++)
	{
	    rc = snprintf(key, key_max_sz, "P%d-lid", p);
	    assert(rc > -1 && rc < key_max_sz);
	    rc = PMI_KVS_Get(pg->kvs_name, key, val);
	    assert(rc == 0);
	    
	    dbg_printf("[%d] port[%d]=%s\n", pg_rank, p, val);
	    fflush(stdout);
	}
    }
    */
    
    MPIU_Free(val);
    MPIU_Free(key);
    
    /* XXX - has_args and has_env need to come from PMI eventually... */
    *has_args = TRUE;
    *has_env = TRUE;

    /* for now, connect all the processes at init time */
    MPIDI_CH3I_Setup_connections();

    return MPI_SUCCESS;
}



