/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"

MPIDI_CH3I_Process_t MPIDI_CH3I_Process;

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

    char *pszEnvVariable;
    char shmemkey[100];
    void *shm_addr;
    int i;
    int shm_block;

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
    //MPIDI_CH3I_Process.rank = pg_rank;

    /* set the global variable defaults */
    pg->nShmEagerLimit = MPIDI_SHM_EAGER_LIMIT;
#ifdef HAVE_SHARED_PROCESS_READ
    pg->nShmRndvLimit = MPIDI_SHM_RNDV_LIMIT;
#endif
    pg->addr = NULL;
#ifdef HAVE_SHMGET
    pg->key = -1;
    pg->id = -1;
#elif defined (HAVE_MAPVIEWOFFILE)
    pg->key[0] = '\0';
    pg->id = NULL;
#else
#error *** No shared memory mapping variables specified ***
#endif
    pg->nShmWaitSpinCount = 100;

    /* Allocate and initialize the VC table associated with this process
       group (and thus COMM_WORLD) */
    vc_table = MPIU_Malloc(sizeof(MPIDI_VC) * pg_size);
    assert(vc_table != NULL);
    pg->ref_count += pg_size;
    for (p = 0; p < pg_size; p++)
    {
	MPIDI_CH3U_VC_init(&vc_table[p], p);
	vc_table[p].shm.pg = pg;
	vc_table[p].shm.pg_rank = p;
	vc_table[p].shm.sendq_head = NULL;
	vc_table[p].shm.sendq_tail = NULL;
	vc_table[p].shm.req = (MPID_Request*)MPIU_Malloc(sizeof(MPID_Request));
	vc_table[p].shm.state = MPIDI_CH3I_VC_STATE_UNCONNECTED;
    }
    pg->vc_table = vc_table;
    
    /* save my vc_ptr for easy access */
    MPIDI_CH3I_Process.vc = &vc_table[pg_rank];

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

    /* Initialize Progress Engine */
    rc = MPIDI_CH3I_Progress_init();
    assert(rc == MPI_SUCCESS);
    
    /* Allocate space for pmi keys and values */
    key_max_sz = PMI_KVS_Get_key_length_max()+1;
    key = MPIU_Malloc(key_max_sz);
    assert(key != NULL);
    val_max_sz = PMI_KVS_Get_value_length_max()+1;
    val = MPIU_Malloc(val_max_sz);
    assert(val != NULL);

    /* initialize the shared memory */
    shm_block = sizeof(MPIDI_CH3I_SHM_Queue_t) + MPID_SHMEM_PER_PROCESS;
    if (pg_size > 1)
    {
	pszEnvVariable = getenv("SHMEMKEY");
	if (pszEnvVariable == NULL)
	{
	    MPIU_DBG_PRINTF(("getting SHMEMKEY\n"));
	    rc = PMI_KVS_Get(pg->kvs_name, "SHMEMKEY", val);
	    strcpy(shmemkey, val);
	}
	else
	{
	    strcpy(shmemkey, pszEnvVariable);
	}
	MPIU_DBG_PRINTF(("KEY = %s\n", shmemkey));
#ifdef HAVE_SHMGET
	pg->key = atoi(shmemkey);
#elif defined (HAVE_MAPVIEWOFFILE)
	strcpy(pg->key, "shm");
	strcat(pg->key, shmemkey);
#else
#error *** No shared memory variables specified ***
#endif

	shm_addr = MPIDI_CH3I_SHM_Get_mem_sync( pg, pg_size * shm_block, pg_rank, pg_size );
    }
    else
    {
        shm_addr = MPIDI_CH3I_SHM_Get_mem_sync( pg, shm_block, 0, 1 );
    }

    /* initialize each shared memory queue */
    for (i=0; i<pg_size; i++)
    {
	vc_table[p].shm.shm = (MPIDI_CH3I_SHM_Queue_t*)((char*)pg->addr + (shm_block * i));
	if (pg_rank == 0)
	{
	    vc_table[p].shm.shm->head = NULL;
	    vc_table[p].shm.shm->tail = NULL;
	    vc_table[p].shm.shm->num_free = shm_block - sizeof(MPIDI_CH3I_SHM_Queue_t) - sizeof(MPIDI_CH3I_SHM_Packet_t);
	}
    }

#ifdef HAVE_WINDOWS_H
    {
	// if you know the number of processors, calculate the spin count relative to that number
        SYSTEM_INFO info;
        GetSystemInfo(&info);
        if (info.dwNumberOfProcessors == 1)
            pg->nShmWaitSpinCount = 1;
        else if (info.dwNumberOfProcessors < (DWORD) pg_size)
            pg->nShmWaitSpinCount = ( 100 * info.dwNumberOfProcessors ) / pg_size;
    }
#endif
    
    rc = PMI_KVS_Commit(pg->kvs_name);
    assert(rc == 0);
    rc = PMI_Barrier();
    assert(rc == 0);

    MPIU_Free(val);
    MPIU_Free(key);
    
    /* XXX - has_args and has_env need to come from PMI eventually... */
    *has_args = TRUE;
    *has_env = TRUE;

    return MPI_SUCCESS;
}
