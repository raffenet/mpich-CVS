/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

MPIDI_CH3I_Process_t MPIDI_CH3I_Process;

static void generate_shm_string(char *str)
{
#ifdef USE_WINDOWS_SHM
    UUID guid;
    UuidCreate(&guid);
    sprintf(str, "%08lX-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X",
	guid.Data1, guid.Data2, guid.Data3,
	guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
	guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
    MPIU_DBG_PRINTF(("GUID = %s\n", str));
#elif defined (USE_POSIX_SHM)
    sprintf(str, "/mpich_shm_%d", getpid());
#elif defined (USE_SYSV_SHM)
    sprintf(str, "%d", getpid());
#else
#error No shared memory subsystem defined
#endif
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Init(int * has_args, int * has_env, int * has_parent)
{
    int mpi_errno;
    MPIDI_CH3I_Process_group_t * pg;
    
    int pg_rank;
    int pg_size;
    MPIDI_VC * vc_table;
    MPID_Comm * comm, *intercomm, *commworld;
    int p;
	
    char * key;
    char * val;
    int key_max_sz;
    int val_max_sz;
    int name_sz;

    char shmemkey[MPIDI_MAX_SHM_NAME_LENGTH];
    int i, j, k;
    int shm_block;
    char local_host[100];
#ifdef HAVE_WINDOWS_H
    DWORD host_len;
#endif

    /*
     * Extract process group related information from PMI and initialize
     * structures that track the process group connections, MPI_COMM_WORLD, and
     * MPI_COMM_SELF
     */
    mpi_errno = PMI_Init(has_parent);
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_init", "**pmi_init %d", mpi_errno);
	return mpi_errno;
    }
    mpi_errno = PMI_Get_rank(&pg_rank);
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_rank", "**pmi_get_rank %d", mpi_errno);
	return mpi_errno;
    }
    mpi_errno = PMI_Get_size(&pg_size);
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_size", "**pmi_get_size %d", mpi_errno);
	return mpi_errno;
    }
    
    /*MPIU_Timer_init(pg_rank, pg_size);*/
    MPIU_dbg_init(pg_rank);

    /* Allocate process group data structure and populate */
    pg = MPIU_Malloc(sizeof(MPIDI_CH3I_Process_group_t));
    if (pg == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }
    pg->size = pg_size;
    pg->rank = pg_rank;
    mpi_errno = PMI_KVS_Get_name_length_max(&name_sz);
    if (mpi_errno != PMI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %d", mpi_errno);
	return mpi_errno;
    }
    pg->kvs_name = MPIU_Malloc(name_sz + 1);
    if (pg->kvs_name == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }
    mpi_errno = PMI_KVS_Get_my_name(pg->kvs_name, name_sz);
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get_my_name", "**pmi_kvs_get_my_name %d", mpi_errno);
	return mpi_errno;
    }
    pg->ref_count = 1;
    MPIDI_CH3I_Process.pg = pg;

    /* set the global variable defaults */
    pg->nShmEagerLimit = MPIDI_SHM_EAGER_LIMIT;
#ifdef HAVE_SHARED_PROCESS_READ
    pg->nShmRndvLimit = MPIDI_SHM_RNDV_LIMIT;
#ifdef HAVE_WINDOWS_H
    pg->pSharedProcessHandles = NULL;
#else
    pg->pSharedProcessIDs = NULL;
    pg->pSharedProcessFileDescriptors = NULL;
#endif
#endif
    pg->addr = NULL;
#ifdef USE_POSIX_SHM
    pg->key[0] = '\0';
    pg->id = -1;
#elif defined (USE_SYSV_SHM)
    pg->key = -1;
    pg->id = -1;
#elif defined (USE_WINDOWS_SHM)
    pg->key[0] = '\0';
    pg->id = NULL;
#else
#error No shared memory subsystem defined
#endif
    pg->nShmWaitSpinCount = MPIDI_CH3I_SPIN_COUNT_DEFAULT;
    pg->nShmWaitYieldCount = MPIDI_CH3I_YIELD_COUNT_DEFAULT;

    /* Allocate and initialize the VC table associated with this process
       group (and thus COMM_WORLD) */
    vc_table = MPIU_Malloc(sizeof(MPIDI_VC) * pg_size);
    if (vc_table == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }
    pg->ref_count += pg_size;
    for (p = 0; p < pg_size; p++)
    {
	MPIDI_CH3U_VC_init(&vc_table[p], p);
	vc_table[p].ch.pg = pg;
	vc_table[p].ch.pg_rank = p;
	vc_table[p].ch.sendq_head = NULL;
	vc_table[p].ch.sendq_tail = NULL;
	vc_table[p].ch.req = (MPID_Request*)MPIU_Malloc(sizeof(MPID_Request));
	vc_table[p].ch.state = MPIDI_CH3I_VC_STATE_IDLE;
	vc_table[p].ch.shm_reading_pkt = TRUE;
	vc_table[p].ch.shm_state = 0;
	vc_table[p].ch.recv_active = NULL;
	vc_table[p].ch.send_active = NULL;
#ifdef USE_SHM_UNEX
	vc_table[p].ch.unex_finished_next = NULL;
	vc_table[p].ch.unex_list = NULL;
#endif
	vc_table[p].ch.shm = NULL;
	vc_table[p].ch.read_shmq = NULL;
	vc_table[p].ch.write_shmq = NULL;
    }
    pg->vc_table = vc_table;
    
    /* save my vc_ptr for easy access */
    MPIDI_CH3I_Process.vc = &vc_table[pg_rank];

    /* set MPIDI_Process->lpid_counter to pg_size */
    MPIDI_Process.lpid_counter = pg_size;

    /* Initialize MPI_COMM_WORLD object */
    comm = MPIR_Process.comm_world;
    comm->rank = pg_rank;
    comm->remote_size = comm->local_size = pg_size;
    mpi_errno = MPID_VCRT_Create(comm->remote_size, &comm->vcrt);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_vcrt", 0);
	return mpi_errno;
    }
    mpi_errno = MPID_VCRT_Get_ptr(comm->vcrt, &comm->vcr);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_getptr", 0);
	return mpi_errno;
    }
    for (p = 0; p < pg_size; p++)
    {
	mpi_errno = MPID_VCR_Dup(&vc_table[p], &comm->vcr[p]);
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_vcrdup", 0);
	    return mpi_errno;
	}
    }

    /* Initialize MPI_COMM_SELF object */
    comm = MPIR_Process.comm_self;
    comm->rank = 0;
    comm->remote_size = comm->local_size = 1;
    mpi_errno = MPID_VCRT_Create(comm->remote_size, &comm->vcrt);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_vcrt", 0);
	return mpi_errno;
    }
    mpi_errno = MPID_VCRT_Get_ptr(comm->vcrt, &comm->vcr);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_getptr", 0);
	return mpi_errno;
    }
    mpi_errno = MPID_VCR_Dup(&vc_table[pg_rank], &comm->vcr[0]);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_vcrdup", 0);
	return mpi_errno;
    }

    /* Initialize Progress Engine */
    mpi_errno = MPIDI_CH3I_Progress_init();
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_progress", 0);
	return mpi_errno;
    }

    /* Allocate space for pmi keys and values */
    mpi_errno = PMI_KVS_Get_key_length_max(&key_max_sz);
    if (mpi_errno != PMI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %d", mpi_errno);
	return mpi_errno;
    }
    key_max_sz++;
    key = MPIU_Malloc(key_max_sz);
    if (key == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }
    mpi_errno = PMI_KVS_Get_value_length_max(&val_max_sz);
    if (mpi_errno != PMI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %d", mpi_errno);
	return mpi_errno;
    }
    val_max_sz++;
    val = MPIU_Malloc(val_max_sz);
    if (val == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }

    /* initialize the shared memory */
    shm_block = sizeof(MPIDI_CH3I_SHM_Queue_t) * pg_size; 

    if (pg_size > 1)
    {
	if (pg_rank == 0)
	{
	    /* Put the shared memory key */
	    generate_shm_string(shmemkey);
	    if (MPIU_Strncpy(key, "SHMEMKEY", key_max_sz))
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**strncpy", 0);
		return mpi_errno;
	    }
	    if (MPIU_Strncpy(val, shmemkey, val_max_sz))
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**strncpy", 0);
		return mpi_errno;
	    }
	    mpi_errno = PMI_KVS_Put(pg->kvs_name, key, val);
	    if (mpi_errno != 0)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", mpi_errno);
		return mpi_errno;
	    }

	    /* Put the hostname to make sure everyone is on the same host */
	    if (MPIU_Strncpy(key, "SHMHOST", key_max_sz))
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**strncpy", 0);
		return mpi_errno;
	    }
#ifdef HAVE_WINDOWS_H
	    host_len = val_max_sz;
	    GetComputerName(val, &host_len);
#else
	    gethostname(val, val_max_sz); /* Don't call this under Windows because it requires the socket library */
#endif
	    mpi_errno = PMI_KVS_Put(pg->kvs_name, key, val);
	    if (mpi_errno != 0)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", mpi_errno);
		return mpi_errno;
	    }

	    mpi_errno = PMI_KVS_Commit(pg->kvs_name);
	    if (mpi_errno != 0)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_commit", "**pmi_kvs_commit %d", mpi_errno);
		return mpi_errno;
	    }
	    mpi_errno = PMI_Barrier();
	    if (mpi_errno != 0)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", mpi_errno);
		return mpi_errno;
	    }
	}
	else
	{
	    /* Get the shared memory key */
	    if (MPIU_Strncpy(key, "SHMEMKEY", key_max_sz))
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**strncpy", 0);
		return mpi_errno;
	    }
	    mpi_errno = PMI_Barrier();
	    if (mpi_errno != 0)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", mpi_errno);
		return mpi_errno;
	    }
	    mpi_errno = PMI_KVS_Get(pg->kvs_name, key, val, val_max_sz);
	    if (mpi_errno != 0)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get %d", mpi_errno);
		return mpi_errno;
	    }
	    if (MPIU_Strncpy(shmemkey, val, val_max_sz))
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**strncpy", 0);
		return mpi_errno;
	    }
	    /* Get the root host and make sure local process is on the same node */
	    if (MPIU_Strncpy(key, "SHMHOST", key_max_sz))
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**strncpy", 0);
		return mpi_errno;
	    }
	    mpi_errno = PMI_KVS_Get(pg->kvs_name, key, val, val_max_sz);
	    if (mpi_errno != 0)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get %d", mpi_errno);
		return mpi_errno;
	    }
#ifdef HAVE_WINDOWS_H
	    host_len = 100;
	    GetComputerName(local_host, &host_len);
#else
	    gethostname(local_host, 100); /* Don't call this under Windows because it requires the socket library */
#endif
	    if (strcmp(val, local_host))
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmhost", "**shmhost %s %s", local_host, val);
		return mpi_errno;
	    }
	}

	MPIU_DBG_PRINTF(("KEY = %s\n", shmemkey));
#if defined(USE_POSIX_SHM) || defined(USE_WINDOWS_SHM)
	if (MPIU_Strncpy(pg->key, shmemkey, MPIDI_MAX_SHM_NAME_LENGTH))
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**strncpy", 0);
	    return mpi_errno;
	}
#elif defined (USE_SYSV_SHM)
	pg->key = atoi(shmemkey);
#else
#error No shared memory subsystem defined
#endif

	mpi_errno = MPIDI_CH3I_SHM_Get_mem( pg, pg_size * shm_block, pg_rank, pg_size, TRUE );
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmgetmem", 0);
	    return mpi_errno;
	}
    }
    else
    {
	mpi_errno = MPIDI_CH3I_SHM_Get_mem( pg, shm_block, 0, 1, FALSE );
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmgetmem", 0);
	    return mpi_errno;
	}
    }

    /* initialize each shared memory queue */
    for (i=0; i<pg_size; i++)
    {
#ifdef HAVE_SHARED_PROCESS_READ
#ifdef HAVE_WINDOWS_H
	if (pg->pSharedProcessHandles)
	    vc_table[i].ch.hSharedProcessHandle = pg->pSharedProcessHandles[i];
#else
	if (pg->pSharedProcessIDs)
	{
	    vc_table[i].ch.nSharedProcessID = pg->pSharedProcessIDs[i];
	    vc_table[i].ch.nSharedProcessFileDescriptor = pg->pSharedProcessFileDescriptors[i];
	}
#endif
#endif
	if (i == pg_rank)
	{
	    vc_table[i].ch.shm = (MPIDI_CH3I_SHM_Queue_t*)((char*)pg->addr + (shm_block * i));
	    for (j=0; j<pg_size; j++)
	    {
		vc_table[i].ch.shm[j].head_index = 0;
		vc_table[i].ch.shm[j].tail_index = 0;
		for (k=0; k<MPIDI_CH3I_NUM_PACKETS; k++)
		{
		    vc_table[i].ch.shm[j].packet[k].offset = 0;
		    vc_table[i].ch.shm[j].packet[k].avail = MPIDI_CH3I_PKT_AVAILABLE;
		}
	    }
	}
	else
	{
	    /*vc_table[i].ch.shm += pg_rank;*/
	    vc_table[i].ch.shm = NULL;
	    vc_table[i].ch.write_shmq = (MPIDI_CH3I_SHM_Queue_t*)((char*)pg->addr + (shm_block * i)) + pg_rank;
	    vc_table[i].ch.read_shmq = (MPIDI_CH3I_SHM_Queue_t*)((char*)pg->addr + (shm_block * pg_rank)) + i;
	    /* post a read of the first packet header */
	    vc_table[i].ch.shm_reading_pkt = TRUE;
	}
    }

#ifdef HAVE_WINDOWS_H
    {
	/* if you know the number of processors, calculate the spin count relative to that number */
        SYSTEM_INFO info;
        GetSystemInfo(&info);
        if (info.dwNumberOfProcessors == 1)
            pg->nShmWaitSpinCount = 1;
        else if (info.dwNumberOfProcessors < (DWORD) pg_size)
            pg->nShmWaitSpinCount = ( MPIDI_CH3I_SPIN_COUNT_DEFAULT * info.dwNumberOfProcessors ) / pg_size;
    }
#else
    /* figure out how many processors are available and set the spin count accordingly */
#if defined(HAVE_SYSCONF) && defined(_SC_NPROCESSORS_ONLN)
    {
	int num_cpus;
	num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
	if (num_cpus == 1)
	    pg->nShmWaitSpinCount = 1;
	else if (num_cpus > 0 && num_cpus < pg_size)
            pg->nShmWaitSpinCount = 1;
	    /* pg->nShmWaitSpinCount = ( MPIDI_CH3I_SPIN_COUNT_DEFAULT * num_cpus ) / pg_size; */
    }
#else
    /* if the number of cpus cannot be determined, set the spin count to 1 */
    pg->nShmWaitSpinCount = 1;
#endif
#endif

    mpi_errno = PMI_KVS_Commit(pg->kvs_name);
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_commit", "**pmi_kvs_commit %d", mpi_errno);
	return mpi_errno;
    }
    mpi_errno = PMI_Barrier();
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", mpi_errno);
	return mpi_errno;
    }
#ifdef USE_POSIX_SHM
    if (shm_unlink(pg->key))
    {
	/* Everyone is unlinking the same object so failure is ok? */
	/*
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shm_unlink", "**shm_unlink %s %d", pg->key, errno);
	return mpi_errno;
	*/
    }
#elif defined (USE_SYSV_SHM)
    if (shmctl(pg->id, IPC_RMID, NULL))
    {
	/* Everyone is removing the same object so failure is ok? */
	if (errno != EINVAL)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmctl", "**shmctl %d %d", pg->id, errno);
	    return mpi_errno;
	}
    }
#endif

    /* XXX - has_args and has_env need to come from PMI eventually... */
    *has_args = TRUE;
    *has_env = TRUE;

    if (*has_parent)
    {
        /* This process was spawned. Create intercommunicator with parents. */

        if (pg_rank == 0)
	{
            /* get the port name of the root of the parents */
            mpi_errno = PMI_KVS_Get(pg->kvs_name, "PARENT_ROOT_PORT_NAME", val, val_max_sz);
            if (mpi_errno != 0)
            {
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get_parent %d", mpi_errno);
                return mpi_errno;
            }
        }

        /* do a connect with the root */
        MPID_Comm_get_ptr(MPI_COMM_WORLD, commworld);
        mpi_errno = MPIDI_CH3_Comm_connect(val, 0, commworld, &intercomm);
        if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "spawned group unable to connect back to the parent");
	    return mpi_errno;
	}

	MPIU_Strncpy(intercomm->name, "MPI_COMM_PARENT", MPI_MAX_OBJECT_NAME);
        MPIR_Process.comm_parent = intercomm;

        /* TODO: Check that this intercommunicator gets freed in
           MPI_Finalize if not already freed.  */
    }

    MPIU_Free(val);
    MPIU_Free(key);
    
    return MPI_SUCCESS;
}
