/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include "mpidi_ch3_impl.h"
#include "pmi.h"

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
	
    char * key;
    char * val;
    int key_max_sz;
    int val_max_sz;
    
    /*
     * Extract process group related information from PMI and initialize structures that track the process group connections,
     * MPI_COMM_WORLD, and MPI_COMM_SELF
     */
    rc = PMI_Init(has_parent);
    assert(rc == 0);
    rc |= PMI_Get_rank(&pg_rank);
    assert(rc == 0);
    rc |= PMI_Get_size(&pg_size);
    assert(rc == 0);
    
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
    
    /* Allocate and initialize the VC table associated with this process group (and thus COMM_WORLD) */
    vc_table = MPIU_Malloc(sizeof(MPIDI_VC) * pg_size);
    assert(vc_table != NULL);
    pg->ref_count += pg_size;
    for (p = 0; p < pg_size; p++)
    {
	MPIDI_CH3U_VC_init(&vc_table[p], p);
	vc_table[p].sc.pg = pg;
	vc_table[p].sc.pg_rank = p;
	vc_table[p].sc.sendq_head = NULL;
	vc_table[p].sc.sendq_tail = NULL;
	vc_table[p].sc.state = MPIDI_CH3I_VC_STATE_UNCONNECTED;
	vc_table[p].sc.sock = SOCK_INVALID_SOCK;
	vc_table[p].sc.conn = NULL;
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
     */
    key_max_sz = PMI_KVS_Get_key_length_max();
    key = MPIU_Malloc(key_max_sz);
    assert(key != NULL);
    val_max_sz = PMI_KVS_Get_value_length_max();
    val = MPIU_Malloc(val_max_sz);
    assert(val != NULL);

    rc = snprintf(key, key_max_sz, "P%d-businesscard", pg_rank);
    assert(rc > -1 && rc < key_max_sz);
    rc = MPIDI_CH3I_Get_business_card(val, val_max_sz);
    assert(rc == MPI_SUCCESS);
    rc = PMI_KVS_Put(pg->kvs_name, key, val);
    assert(rc == 0);

#   if defined(DEBUG)
    {
	/*dbg_printf("[%d] Published hostname=%s port=%d\n", pg_rank, hostname, port);*/
	dbg_printf("[%d] Business card: <%s>\n", pg_rank, val);
	fflush(stdout);
    }
#   endif

    rc = PMI_KVS_Commit(pg->kvs_name);
    assert(rc == 0);
    rc = PMI_Barrier();
    assert(rc == 0);

    /*MPIU_Free(hostname);*/
    MPIU_Free(val);
    MPIU_Free(key);
    
#   if defined(DEBUG)
    {
	for (p = 0; p < pg_size; p++)
	{
	    rc = snprintf(key, key_max_sz, "P%d-businesscard", p);
	    assert(rc > -1 && rc < key_max_sz);
	    rc = PMI_KVS_Get(pg->kvs_name, key, val);
	    assert(rc == 0);

	    dbg_printf("[%d] businesscard=%s\n", pg_rank, val);
	    fflush(stdout);
	}
    }
#   endif
    
    /* XXX - has_args and has_env need to come from PMI eventually... */
    *has_args = TRUE;
    *has_env = TRUE;

    return MPI_SUCCESS;
}
