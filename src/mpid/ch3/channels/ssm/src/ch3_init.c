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
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

MPIDI_CH3I_Process_t MPIDI_CH3I_Process;

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
    MPID_Comm * comm;
    int p;
    char * key;
    char * val;
    int key_max_sz;
    int val_max_sz;
    char queue_name[100];

    /*
     * Extract process group related information from PMI and initialize structures that track the process group connections,
     * MPI_COMM_WORLD, and MPI_COMM_SELF
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
    
    /* Allocate process group data structure and populate */
    pg = MPIU_Malloc(sizeof(MPIDI_CH3I_Process_group_t));
    if (pg == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }
    pg->size = pg_size;
    pg->kvs_name = MPIU_Malloc(PMI_KVS_Get_name_length_max() + 1);
    if (pg->kvs_name == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }
    mpi_errno = PMI_KVS_Get_my_name(pg->kvs_name);
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get_my_name", "**pmi_kvs_get_my_name %d", mpi_errno);
	return mpi_errno;
    }
    pg->ref_count = 1;
    MPIDI_CH3I_Process.pg = pg;
    MPIDI_CH3I_Process.shm_reading_list = NULL;
    MPIDI_CH3I_Process.shm_writing_list = NULL;
    MPIDI_CH3I_Process.num_cpus = -1;
    
    /* set the global variable defaults */
    pg->nShmEagerLimit = MPIDI_SHM_EAGER_LIMIT;
#ifdef HAVE_SHARED_PROCESS_READ
    pg->nShmRndvLimit = MPIDI_SHM_RNDV_LIMIT;
#endif
    pg->nShmWaitSpinCount = MPIDI_CH3I_SPIN_COUNT_DEFAULT;
    pg->nShmWaitYieldCount = MPIDI_CH3I_YIELD_COUNT_DEFAULT;

    /* Figure out how many processors are available and set the spin count accordingly */
    /* If there were topology information available we could calculate a multi-cpu number */
#ifdef HAVE_WINDOWS_H
    {
	/* if you know the number of processors, calculate the spin count relative to that number */
        SYSTEM_INFO info;
        GetSystemInfo(&info);
        if (info.dwNumberOfProcessors == 1)
            pg->nShmWaitSpinCount = 1;
	/*
        else if (info.dwNumberOfProcessors < (DWORD) num_procs_per_node)
            pg->nShmWaitSpinCount = ( MPIDI_CH3I_SPIN_COUNT_DEFAULT * info.dwNumberOfProcessors ) / num_procs_per_node;
	*/
	if (info.dwNumberOfProcessors > 0)
	    MPIDI_CH3I_Process.num_cpus = info.dwNumberOfProcessors;
    }
#else
#ifdef HAVE_SYSCONF
    {
	int num_cpus;
	num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
	if (num_cpus == 1)
	    pg->nShmWaitSpinCount = 1;
	/*
	else if (num_cpus > 0 && num_cpus < num_procs_per_node)
	    pg->nShmWaitSpinCount = ( MPIDI_CH3I_SPIN_COUNT_DEFAULT * num_cpus ) / num_procs_per_node;
	*/
	if (num_cpus > 0)
	    MPIDI_CH3I_Process.num_cpus = num_cpus;
    }
#endif
#endif

    /* Allocate and initialize the VC table associated with this process group (and thus COMM_WORLD) */
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
	vc_table[p].ssm.pg = pg;
	vc_table[p].ssm.pg_rank = p;
	vc_table[p].ssm.sendq_head = NULL;
	vc_table[p].ssm.sendq_tail = NULL;
	vc_table[p].ssm.recv_active = NULL;
	vc_table[p].ssm.send_active = NULL;
	vc_table[p].ssm.req = NULL;
	vc_table[p].ssm.state = MPIDI_CH3I_VC_STATE_UNCONNECTED;
	vc_table[p].ssm.sock = SOCK_INVALID_SOCK;
	vc_table[p].ssm.conn = NULL;
	vc_table[p].ssm.read_shmq = NULL;
	vc_table[p].ssm.write_shmq = NULL;
	vc_table[p].ssm.shm = NULL;
	vc_table[p].ssm.shm_state = 0;
	vc_table[p].ssm.shm_next_reader = NULL;
	vc_table[p].ssm.shm_next_writer = NULL;
	vc_table[p].ssm.bShm = FALSE;
    }
    pg->vc_table = vc_table;
    
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
	MPID_VCR_Dup(&vc_table[p], &comm->vcr[p]);
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
    MPID_VCR_Dup(&vc_table[pg_rank], &comm->vcr[0]);

    /*
     * Initialize Progress Engine (and setup listener socket)
     */
    mpi_errno = MPIDI_CH3I_Progress_init();
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_progress", 0);
	return mpi_errno;
    }
    
    /*
     * Publish the contact info for this process in the PMI keyval space
     */
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

    /* put the sock business card */
    mpi_errno = MPIU_Snprintf(key, key_max_sz, "P%d-sock_businesscard", pg_rank);
    if (mpi_errno < 0 || mpi_errno > key_max_sz)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**snprintf", "**snprintf %d", mpi_errno);
	return mpi_errno;
    }
    mpi_errno = MPIDI_CH3I_Get_business_card(val, val_max_sz);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_buscard", 0);
	return mpi_errno;
    }
    mpi_errno = PMI_KVS_Put(pg->kvs_name, key, val);
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", mpi_errno);
	return mpi_errno;
    }
    MPIU_Strncpy(pg->shm_hostname, val, 256);
    if (strtok(pg->shm_hostname, ":") == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_strtok_host", "**init_strtok_host %s", val);
	/*MPIDI_err_printf("MPIDI_CH3_Init", "ch3_init: unable to get the host name from the businesscard - %s\n", val);*/
	return mpi_errno;
    }
    /*printf("sock_businesscard: %s\n", val);fflush(stdout);*/

#   if defined(DEBUG)
    {
	dbg_printf("[%d] Sock business card: <%s>\n", pg_rank, val);
	fflush(stdout);
    }
#   endif

    /* put the shmem business card */
    mpi_errno = snprintf(key, key_max_sz, "P%d-shm_businesscard", pg_rank);
    if (mpi_errno < 0 || mpi_errno > key_max_sz)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**snprintf", "**snprintf %d", mpi_errno);
	return mpi_errno;
    }
    mpi_errno = MPIDI_CH3I_BootstrapQ_create(&pg->bootstrapQ);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**boot_create", 0);
	return mpi_errno;
    }
    queue_name[0] = '\0';
    mpi_errno = MPIDI_CH3I_BootstrapQ_tostring(pg->bootstrapQ, queue_name, 100);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**boot_tostring", 0);
	return mpi_errno;
    }
    MPIU_Snprintf(val, val_max_sz, "%s:%s", pg->shm_hostname, queue_name);
    mpi_errno = PMI_KVS_Put(pg->kvs_name, key, val);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", mpi_errno);
	return mpi_errno;
    }
    MPIU_DBG_PRINTF(("shm_businesscard: %s\n", val));

    /* commit the puts */
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

#   if defined(DEBUG)
    {
	for (p = 0; p < pg_size; p++)
	{
	    rc = snprintf(key, key_max_sz, "P%d-sock_businesscard", p);
	    assert(rc > -1 && rc < key_max_sz);
	    rc = PMI_KVS_Get(pg->kvs_name, key, val);
	    assert(rc == 0);

	    dbg_printf("[%d] sock_businesscard=%s\n", pg_rank, val);

	    rc = snprintf(key, key_max_sz, "P%d-shm_businesscard", p);
	    assert(rc > -1 && rc < key_max_sz);
	    rc = PMI_KVS_Get(pg->kvs_name, key, val);
	    assert(rc == 0);

	    dbg_printf("[%d] shm_businesscard=%s\n", pg_rank, val);
	    fflush(stdout);
	}
    }
#   endif
    
    MPIU_Free(val);
    MPIU_Free(key);
    
    *has_args = TRUE;
    *has_env = TRUE;

    return MPI_SUCCESS;
}
