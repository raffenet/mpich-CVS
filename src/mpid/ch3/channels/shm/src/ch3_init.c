/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"

MPIDI_CH3I_Process_t MPIDI_CH3I_Process;

/* global variables */

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

    char *pszEnvVariable;
    char shmemkey[100];
    void *shm_addr;
    int shm_size;
    int sizes[SHM_NSIZES];
    int i;

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

    /* set the global variable defaults */
    MPIDI_CH3I_Process.nShmEagerLimit = MPIDI_SHM_EAGER_LIMIT;
#ifdef HAVE_SHARED_PROCESS_READ
    MPIDI_CH3I_Process.nShmRndvLimit = MPIDI_SHM_RNDV_LIMIT;
#ifdef HAVE_WINDOWS_H
    MPIDI_CH3I_Process.pSharedProcessHandles = NULL;
#else
    MPIDI_CH3I_Process.pSharedProcessIDs = NULL;
    MPIDI_CH3I_Process.pSharedProcessFileDescriptors = NULL;
#endif
#endif
    MPIDI_CH3I_Process.addr = NULL;
    MPIDI_CH3I_Process.my_curr_addr = NULL;
#ifdef HAVE_SHMGET
    MPIDI_CH3I_Process.key = -1;
    MPIDI_CH3I_Process.id = -1;
#elif defined (HAVE_MAPVIEWOFFILE)
    MPIDI_CH3I_Process.key[0] = '\0';
    MPIDI_CH3I_Process.id = NULL;
#else
#error *** No shared memory mapping variables specified ***
#endif
    MPIDI_CH3I_Process.size = 0;
    MPIDI_CH3I_Process.my_rem_size = 0;
#ifdef USE_GARBAGE_COLLECTING
    MPIDI_CH3I_Process.gc_count = 0;
    MPIDU_Process_lock_init(&MPIDI_CH3I_Process.gc_lock);     /* lock for garbage collection counter */
#endif
    MPIDU_Process_lock_init(&MPIDI_CH3I_Process.addr_lock);   /* lock for local (remaining) shared memory pool size */

#if 0
    MPIDI_CH3I_Process.recv_queues = NULL;  /* array of recv queues of all processes that share memory */
    MPIDI_CH3I_Process.active_queue = NULL;     /* queue of active rendezvous requests that MPID_SHM_Test must cause progress on */
#endif

    MPIDI_CH3I_Process.free_list = NULL;
#ifdef USE_GARBAGE_COLLECTING
    MPIDI_CH3I_Process.inuse_list = NULL;
#endif
    MPIDI_CH3I_Process.nShmWaitSpinCount = 100;

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
     * Initialize Progress Engine
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
    
    /* initialize the shared memory functions */
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
	MPIDI_CH3I_Process.key = atoi(shmemkey);
#elif defined (HAVE_MAPVIEWOFFILE)
	strcpy(MPIDI_CH3I_Process.key, "shm");
	strcat(MPIDI_CH3I_Process.key, shmemkey);
#else
#error *** No shared memory variables specified ***
#endif

#ifdef HAVE_SHARED_PROCESS_READ
#ifdef HAVE_WINDOWS_H
	MPIDI_CH3I_Process.pSharedProcessHandles = (HANDLE*)MPIU_Malloc(sizeof(HANDLE) * pg_size);
#else
	MPIDI_CH3I_Process.pSharedProcessIDs = (int*)MPIU_Malloc(sizeof(int) * pg_size);
	MPIDI_CH3I_Process.pSharedProcessFileDescriptors = (int*)MPIU_Malloc(sizeof(int) * pg_size);
#endif
#endif
	MPIDI_CH3I_SHM_Get_mem_sync( pg_size * (sizeof(MPIDU_Queue) + MPID_SHMEM_PER_PROCESS), pg_rank, pg_size );
#if 0
	MPIDI_CH3I_Process.recv_queues = (MPIDU_Queue *) MPIDI_CH3I_Process.addr;
#endif

#ifdef HAVE_SHARED_PROCESS_READ
#ifdef HAVE_WINDOWS_H
	MPIU_Free(MPIDI_CH3I_Process.pSharedProcessHandles);
	MPIDI_CH3I_Process.pSharedProcessHandles = NULL;
#else
	MPIU_Free(MPIDI_CH3I_Process.pSharedProcessIDs);
	MPIU_Free(MPIDI_CH3I_Process.pSharedProcessFileDescriptors);
	MPIDI_CH3I_Process.pSharedProcessIDs = NULL;
	MPIDI_CH3I_Process.pSharedProcessFileDescriptors = NULL;
#endif
#endif
    }
    else
    {
        MPIDI_CH3I_SHM_Get_mem_sync( sizeof(MPIDU_Queue) + MPID_SHMEM_PER_PROCESS, 0, 1 );
#if 0
        MPIDI_CH3I_Process.recv_queues = (MPIDU_Queue *) MPIDI_CH3I_Process.addr;
#endif
    }

    if (pg_size > 1)
    {
        shm_addr = ((char *) MPIDI_CH3I_Process.addr) + pg_size * sizeof(MPIDU_Queue);
        shm_size = pg_size * sizeof(MPIDU_Queue);
    }
    else
    {
        shm_addr = NULL;
        shm_size = 0;
    }
    
    MPIDI_CH3I_Process.free_list = (MPIDI_Shm_list *) MPIU_Malloc(SHM_NSIZES * sizeof(MPIDI_Shm_list));
#ifdef USE_GARBAGE_COLLECTING
    MPIDI_CH3I_Process.inuse_list = (MPIDI_Shm_list *) MPIU_Malloc(SHM_NSIZES * sizeof(MPIDI_Shm_list));
#endif
    
    MPIDI_CH3I_Process.my_rem_size = shm_size / pg_size;
    MPIDI_CH3I_Process.my_curr_addr = ((char *)shm_addr) + MPIDI_CH3I_Process.my_rem_size * pg_rank;
    
    MPIDU_Process_lock_init(&MPIDI_CH3I_Process.addr_lock);
    
    sizes[0] = SHM_SIZE1;
    sizes[1] = SHM_SIZE2;
    sizes[2] = SHM_SIZE3;
    sizes[3] = SHM_SIZE4;
    sizes[4] = SHM_SIZE5;
    sizes[5] = SHM_MAX_SIZE;
    
    for (i = 0; i < SHM_NSIZES; i++) 
    {
        MPIDI_CH3I_Process.free_list[i].size = sizes[i];
        MPIDI_CH3I_Process.free_list[i].ptr = NULL;
        MPIDI_CH3I_Process.free_list[i].count = MPIDI_CH3I_Process.free_list[i].max_count = 0;
        MPIDU_Process_lock_init(&(MPIDI_CH3I_Process.free_list[i].thr_lock));
      
#ifdef USE_GARBAGE_COLLECTING
        MPIDI_CH3I_Process.inuse_list[i].size = sizes[i];
        MPIDI_CH3I_Process.inuse_list[i].ptr = NULL;
        MPIDI_CH3I_Process.inuse_list[i].count = MPIDI_CH3I_Process.inuse_list[i].max_count = 0;
        MPIDU_Process_lock_init(&(MPIDI_CH3I_Process.inuse_list[i].thr_lock));
#endif
    }
    
#ifdef USE_GARBAGE_COLLECTING
    /* initialize counter for garbage collection */
    MPIDU_Process_lock_init(&(MPIDI_CH3I_Process.gc_lock));
    MPIDI_CH3I_Process.gc_count = 0;
#endif
    
#if 0 /* UNNECESSARY CODE? */
    if (pg_size > 1)
    {
        /* How do we guarantee that the queues are not used before they are initialized? */
        if (pg_rank == 0)
            MPIDU_Queues_init(MPIDI_CH3I_Process.recv_queues, pg_size);  /* these queues are in shared memory */
        else
#ifdef HAVE_WIN32_SLEEP
            Sleep(100);
#elif defined (HAVE_USLEEP)
            usleep(1000);
#elif defined (HAVE_SLEEP)
            sleep(1);
#else
            ;
#endif
    }
    
    MPIDU_Queue_init(&MPIDI_CH3I_Process.active_queue);  /* these queues are not in shared memory */
#endif /* UNNECESSARY CODE? */ 

#ifdef HAVE_WINDOWS_H
    {
        SYSTEM_INFO info;
        GetSystemInfo(&info);
        if (info.dwNumberOfProcessors == 1)
            MPIDI_CH3I_Process.nShmWaitSpinCount = 1;
        else if (info.dwNumberOfProcessors < (DWORD) pg_size)
            MPIDI_CH3I_Process.nShmWaitSpinCount = ( 100 * info.dwNumberOfProcessors ) / pg_size;
    }
#endif
#if 0
    rc = ibu_init();
    assert(rc == IBU_SUCCESS);
    /* create a completion set for this process */
    rc = ibu_create_set(&MPIDI_CH3I_Process.set);
    assert(rc == IBU_SUCCESS);
    /* get and put the local id for this process in the PMI database */
    port = ibu_get_lid();
    
    rc = snprintf(key, key_max_sz, "P%d-lid", pg_rank);
    assert(rc > -1 && rc < key_max_sz);
    rc = snprintf(val, val_max_sz, "%d", port);
    assert(rc > -1 && rc < val_max_sz);
    rc = PMI_KVS_Put(pg->kvs_name, key, val);
    assert(rc == 0);

    dbg_printf("[%d] Published lid=%d\n", pg_rank, port);
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

    /* for now, connect all the processes at init time */
    MPIDI_CH3I_Setup_connections();

    return MPI_SUCCESS;
}
