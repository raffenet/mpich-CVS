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

/* MPIDI_CH3I_Process_t MPIDI_CH3I_Process;*/

static int MPIDI_CH3I_PG_Compare_ids(void * id1, void * id2);
static int MPIDI_CH3I_PG_Destroy(MPIDI_PG_t * pg, void * id);

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Init(int * has_args, int * has_env, int * has_parent, MPIDI_PG_t ** pg_p, int * pg_rank_p,
                    char **publish_bc_p, char **bc_key_p, char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    int pg_size;
    int p;

    MPIDI_STATE_DECL(MPID_STATE_MPID_CH3_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_CH3_INIT);
    
/*     printf("before MPIDI_CH3U_Init_sock\n"); */
    /* initialize aspects specific to sockets.  do NOT publish business card yet  */
    mpi_errno = MPIDI_CH3U_Init_sock(has_args, has_env, has_parent, pg_p, pg_rank_p,
                               NULL, bc_key_p, bc_val_p, val_max_sz_p);
/*     printf("after MPIDI_CH3U_Init_sock\n"); */
    if (mpi_errno != MPI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }

/*     printf("before MPIDI_CH3U_Init_sshm\n"); */
    /* initialize aspects specific to sshm.  now publish business card   */
    mpi_errno = MPIDI_CH3U_Init_sshm(has_args, has_env, has_parent, pg_p, pg_rank_p,
                               publish_bc_p, bc_key_p, bc_val_p, val_max_sz_p);
/*     printf("after MPIDI_CH3U_Init_sshm\n"); */
    if (mpi_errno != MPI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }

    /* brad : these variables are no longer used but instead Init occurs via generic
     *         PMI code in mpid_init.c and also upcalls to ch3u_init_sock.c and
     *         ch3u_init_sshm.c to do the same thing
     */
 
#if 0  /* brad : start */
    int pmi_errno = PMI_SUCCESS;
    MPIDI_PG_t * pg = NULL;
    int pg_rank;
    int pg_size;
    char * pg_id;
    int pg_id_sz;
    int p;
    char * key;
    char * val;
    int key_max_sz;
    int val_max_sz;
    char queue_name[100];
    char * business_card, * bc_orig;
    int bc_length;
    char host_description[256];
    int port;
    int kvs_name_sz;

    srand(getpid());

    MPIDI_CH3I_Process.parent_port_name = NULL;
    MPIDI_CH3I_Process.acceptq_head = NULL;
    MPIDI_CH3I_Process.acceptq_tail = NULL;
#   if (USE_THREAD_IMPL == MPICH_THREAD_IMPL_NOT_IMPLEMENTED)
    {
	MPID_Thread_lock_init(&MPIDI_CH3I_Process.acceptq_mutex);
    }
#   endif

    /* brad : this code is all independent and now in MPIDI_CH3I_PMI_Init.  the upcalls
     *         above get the code specific to sockets and shared memory.
     */
    
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
#endif /* brad : end if 0.  need pg_size for pg->vct[p].ch.bShm initialization */
    
    mpi_errno = PMI_Get_size(&pg_size);
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_size", "**pmi_get_size %d", mpi_errno);
	return mpi_errno;
    }
#if 0  /* brad : below is common */
    
    /* brad : common code in MPIDI_CH3I_PMI_Init does appnum adjustments here */
    
    /*
     * Get the process group id
     */
    pmi_errno = PMI_Get_id_length_max(&pg_id_sz);
    if (pmi_errno != PMI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**pmi_get_id_length_max", "**pmi_get_id_length_max %d", pmi_errno);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }

    pg_id = MPIU_Malloc(pg_id_sz + 1);
    if (pg_id == NULL)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", NULL);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
    
    pmi_errno = PMI_Get_id(pg_id, pg_id_sz);
    if (pmi_errno != PMI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_id",
					 "**pmi_get_id %d", pmi_errno);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }


    /* printf("[%d] is process %d\n", pg_rank, getpid()); */
    
    /*
     * Initialize the process group tracking subsystem
     */
    mpi_errno = MPIDI_PG_Init(MPIDI_CH3I_PG_Compare_ids, MPIDI_CH3I_PG_Destroy);
    if (mpi_errno != MPI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**dev|pg_init", NULL);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
    
    /*
     * Create a new structure to track the process group
     */
    mpi_errno = MPIDI_PG_Create(pg_size, pg_id, &pg);
    if (mpi_errno != MPI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**dev|pg_create", NULL);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
    pg->ch.kvs_name = NULL;
    
    /*
     * Get the name of the key-value space (KVS)
     */
    pmi_errno = PMI_KVS_Get_name_length_max(&kvs_name_sz);
    if (pmi_errno != PMI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**pmi_kvs_get_name_length_max", "**pmi_kvs_get_name_length_max %d", pmi_errno);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
    
    pg->ch.kvs_name = MPIU_Malloc(kvs_name_sz + 1);
    if (pg->ch.kvs_name == NULL)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", NULL);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
    
    pmi_errno = PMI_KVS_Get_my_name(pg->ch.kvs_name, kvs_name_sz);
    if (pmi_errno != PMI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**pmi_kvs_get_my_name", "**pmi_kvs_get_my_name %d", pmi_errno);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }

    /* brad : specific to this channel.  similar in essm and sshm */
    MPIDI_CH3I_Process.shm_reading_list = NULL;
    MPIDI_CH3I_Process.shm_writing_list = NULL;
    MPIDI_CH3I_Process.num_cpus = -1;
    
    /* set the global variable defaults */
    pg->ch.nShmEagerLimit = MPIDI_SHM_EAGER_LIMIT;
#ifdef HAVE_SHARED_PROCESS_READ
    pg->ch.nShmRndvLimit = MPIDI_SHM_RNDV_LIMIT;
#endif
    pg->ch.nShmWaitSpinCount = MPIDI_CH3I_SPIN_COUNT_DEFAULT;
    pg->ch.nShmWaitYieldCount = MPIDI_CH3I_YIELD_COUNT_DEFAULT;

    /* Figure out how many processors are available and set the spin count accordingly */
    /* If there were topology information available we could calculate a multi-cpu number */
#ifdef HAVE_WINDOWS_H
    {
	/* if you know the number of processors, calculate the spin count relative to that number */
        SYSTEM_INFO info;
        GetSystemInfo(&info);
        if (info.dwNumberOfProcessors == 1)
            pg->ch.nShmWaitSpinCount = 1;
	/*
        else if (info.dwNumberOfProcessors < (DWORD) num_procs_per_node)
            pg->ch.nShmWaitSpinCount = ( MPIDI_CH3I_SPIN_COUNT_DEFAULT * info.dwNumberOfProcessors ) / num_procs_per_node;
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
	    pg->ch.nShmWaitSpinCount = 1;
	/*
	else if (num_cpus > 0 && num_cpus < num_procs_per_node)
	    pg->ch.nShmWaitSpinCount = ( MPIDI_CH3I_SPIN_COUNT_DEFAULT * num_cpus ) / num_procs_per_node;
	*/
	if (num_cpus > 0)
	    MPIDI_CH3I_Process.num_cpus = num_cpus;
    }
#endif
#endif
#ifndef HAVE_WINDOWS_H
    pg->ch.nShmWaitSpinCount = 1;   /* brad - does this mean non-windows systems are all 1? */
    g_nLockSpinCount = 1;
#endif


#endif /* brad : VCs initialized in MPIDI_CH3U_init_* upcalls.  still need to initialize bShm.  */
    
    /* Allocate and initialize the VC table associated with this process group (and thus COMM_WORLD) */
    for (p = 0; p < pg_size; p++)
    {
#if 0 /* brad : bShm only */
	pg->vct[p].ch.sendq_head = NULL;
	pg->vct[p].ch.sendq_tail = NULL;
	pg->vct[p].ch.state = MPIDI_CH3I_VC_STATE_UNCONNECTED;
	pg->vct[p].ch.sock = MPIDU_SOCK_INVALID_SOCK;
	pg->vct[p].ch.conn = NULL;
	pg->vct[p].ch.recv_active = NULL;
	pg->vct[p].ch.send_active = NULL;
	pg->vct[p].ch.req = NULL;
	pg->vct[p].ch.read_shmq = NULL;
	pg->vct[p].ch.write_shmq = NULL;
	pg->vct[p].ch.shm = NULL;
	pg->vct[p].ch.shm_state = 0;
	pg->vct[p].ch.shm_next_reader = NULL;
	pg->vct[p].ch.shm_next_writer = NULL;
#endif /* brad : bShm only */
/*         printf("\tvc init (%d)\n", p); */
	(*pg_p)->vct[p].ch.bShm = FALSE;
    }

#if 0 /* brad : below is common in MPIDI_CH3I_PMI_Init */
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
    mpi_errno = PMI_KVS_Get_key_length_max(&key_max_sz);
    if (mpi_errno != PMI_SUCCESS)
    {
    }
    key = MPIU_Malloc(key_max_sz);
    if (key == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }
    mpi_errno = PMI_KVS_Get_value_length_max(&val_max_sz);
    if (mpi_errno != PMI_SUCCESS)
    {
    }
    /* brad : separating malloc's for val and business_card not done in new MPIDI_CH3I_PMI_Init.  if
     *         still required for business card, they can be malloced locally in business card
     *         related functions done in upcalls (MPIDI_CH3U_Get_business_card_*())
     */
    val = MPIU_Malloc(val_max_sz);
    if (val == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }
    business_card = MPIU_Malloc(val_max_sz);
    if (business_card == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }
    bc_length = val_max_sz;
    bc_orig = business_card;

    /* add the sock business card pieces */
    /*
    mpi_errno = MPIU_Snprintf(key, key_max_sz, "P%d-sock_businesscard", pg_rank);
    if (mpi_errno < 0 || mpi_errno > key_max_sz)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**snprintf", "**snprintf %d", mpi_errno);
	return mpi_errno;
    }
    */

    /* brad : the sock portion of the business card creation is now handled by the upcall
     *          MPIDI_CH3U_Get_business_card_sock within MPIDI_CH3I_Get_business_card
     */
    port = MPIDI_CH3I_Listener_get_port();
    mpi_errno = MPIDU_Sock_get_host_description(host_description, 256);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_description", 0);
	return mpi_errno;
    }
    mpi_errno = MPIU_Str_add_int_arg(&business_card, &bc_length, MPIDI_CH3I_PORT_KEY, port);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	if (mpi_errno == MPIU_STR_NOMEM)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**buscard_len", 0);
	}
	else
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**buscard", 0);
	}
	return mpi_errno;
    }
    mpi_errno = MPIU_Str_add_string_arg(&business_card, &bc_length, MPIDI_CH3I_HOST_DESCRIPTION_KEY, host_description);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	if (mpi_errno == MPIU_STR_NOMEM)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**buscard_len", 0);
	}
	else
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**buscard", 0);
	}
	return mpi_errno;
    }
    /*
    mpi_errno = MPIDI_CH3I_Get_business_card(val, val_max_sz);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_buscard", 0);
	return mpi_errno;
    }
    mpi_errno = PMI_KVS_Put(pg->ch.kvs_name, key, val);
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", mpi_errno);
	return mpi_errno;
    }
    */
    /* FIXME: Note that gethostname is not in POSIX */
    /* FIXME: consider using sizeof(pg->ch.shm_hostname) instead of
       MAXHOSTNAMELEN, in case the system and the ssm/include header files
       have a different interpretation of the size of MAXHOSTNAMELEN */
#ifdef HAVE_WINDOWS_H
    {
	DWORD len = sizeof(pg->ch.shm_hostname);
	/*GetComputerName(pg->ch.shm_hostname, &len);*/
	GetComputerNameEx(ComputerNameDnsFullyQualified, pg->ch.shm_hostname, &len);
    }
#else
    gethostname(pg->ch.shm_hostname, sizeof(pg->ch.shm_hostname));
#endif

#ifdef USE_MQSHM

    MPIU_Strncpy(key, "bootstrapQ_name", key_max_sz );
    if (pg_rank == 0)
    {
	mpi_errno = MPIDI_CH3I_BootstrapQ_create_unique_name(queue_name, 100);
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**boot_create", 0);
	    return mpi_errno;
	}
	mpi_errno = MPIDI_CH3I_BootstrapQ_create_named(&pg->ch.bootstrapQ, queue_name, 1);
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**boot_create", 0);
	    return mpi_errno;
	}
	/*printf("root process created bootQ: '%s'\n", queue_name);fflush(stdout);*/
	MPIU_Strncpy(val, queue_name, val_max_sz);
	mpi_errno = PMI_KVS_Put(pg->ch.kvs_name, key, val);
	if (mpi_errno != 0)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", mpi_errno);
	    return mpi_errno;
	}
	mpi_errno = PMI_KVS_Commit(pg->ch.kvs_name);
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
	mpi_errno = PMI_Barrier();
	if (mpi_errno != 0)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", mpi_errno);
	    return mpi_errno;
	}
	mpi_errno = PMI_KVS_Get(pg->ch.kvs_name, key, val, val_max_sz);
	if (mpi_errno != 0)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get %d", mpi_errno);
	    return mpi_errno;
	}
	MPIU_Strncpy(queue_name, val, val_max_sz);
	/*printf("process %d got bootQ name: '%s'\n", pg_rank, queue_name);fflush(stdout);*/
	mpi_errno = MPIDI_CH3I_BootstrapQ_create_named(&pg->ch.bootstrapQ, queue_name, 1);
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**boot_create", 0);
	    return mpi_errno;
	}
    }
    mpi_errno = PMI_Barrier();
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", mpi_errno);
	return mpi_errno;
    }
    mpi_errno = MPIDI_CH3I_BootstrapQ_unlink(pg->ch.bootstrapQ);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**boot_unlink", 0);
	return mpi_errno;
    }

#else

    mpi_errno = MPIDI_CH3I_BootstrapQ_create(&pg->ch.bootstrapQ);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**boot_create", 0);
	return mpi_errno;
    }

#endif
    queue_name[0] = '\0';
    mpi_errno = MPIDI_CH3I_BootstrapQ_tostring(pg->ch.bootstrapQ, queue_name, 100);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**boot_tostring", 0);
	return mpi_errno;
    }
    /* add the shmem business card pieces */
#if 0
    mpi_errno = MPIU_Snprintf(key, key_max_sz, "P%d-shm_businesscard", pg_rank);
    if (mpi_errno < 0 || mpi_errno > key_max_sz)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**snprintf", "**snprintf %d", mpi_errno);
	return mpi_errno;
    }
    MPIU_Snprintf(val, val_max_sz, "%s:%s", pg->ch.shm_hostname, queue_name);
    /*printf("putting kvs_name: %s, key: %s, val: %s\n", pg->ch.kvs_name, key, val);fflush(stdout);*/
    mpi_errno = PMI_KVS_Put(pg->ch.kvs_name, key, val);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", mpi_errno);
	return mpi_errno;
    }
    MPIU_DBG_PRINTF(("shm_businesscard: %s\n", val));
#endif
    mpi_errno = MPIU_Str_add_string_arg(&business_card, &bc_length, MPIDI_CH3I_SHM_HOST_KEY, pg->ch.shm_hostname);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	if (mpi_errno == MPIU_STR_NOMEM)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**buscard_len", 0);
	}
	else
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**buscard", 0);
	}
	return mpi_errno;
    }

    mpi_errno = MPIU_Str_add_string_arg(&business_card, &bc_length, MPIDI_CH3I_SHM_QUEUE_KEY, queue_name);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	if (mpi_errno == MPIU_STR_NOMEM)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**buscard_len", 0);
	}
	else
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**buscard", 0);
	}
	return mpi_errno;
    }

    /* put the business card */
    mpi_errno = MPIU_Snprintf(key, key_max_sz, "P%d-businesscard", pg_rank);
    if (mpi_errno < 0 || mpi_errno > key_max_sz)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**snprintf", "**snprintf %d", mpi_errno);
	return mpi_errno;
    }
    /*printf("putting <%s> = <%s>\n", key, bc_orig);fflush(stdout);*/
    mpi_errno = PMI_KVS_Put(pg->ch.kvs_name, key, bc_orig);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", mpi_errno);
	return mpi_errno;
    }

    /* commit the puts */
    mpi_errno = PMI_KVS_Commit(pg->ch.kvs_name);
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

#if 0
/* test */
    mpi_errno = PMI_KVS_Get(pg->ch.kvs_name, key, val, val_max_sz);
    /* printf("got <%s> = <%s>\n", key, val);fflush(stdout);*/
/* end test */
#endif

#if 0
    {
	for (p = 0; p < pg_size; p++)
	{
	    mpi_errno = MPIU_Snprintf(key, key_max_sz, "P%d-businesscard", p);
	    MPIU_Assert(mpi_errno > -1 && mpi_errno < key_max_sz);
	    mpi_errno = PMI_KVS_Get(pg->ch.kvs_name, key, val, val_max_sz);
	    MPIU_Assert(mpi_errno == 0);

	    dbg_printf("[%d] businesscard=%s\n", pg_rank, val);
	}
    }
#endif
    
    *has_args = TRUE;
    *has_env = TRUE;

    *pg_pptr = pg;
    *pg_rank_ptr = pg_rank;

#if 0
    if (*has_parent)
    {
        /* This process was spawned. Create intercommunicator with parents. */

        if (pg_rank == 0)
	{
            /* get the port name of the root of the parents */
            mpi_errno = PMI_KVS_Get(pg->ch.kvs_name, "PARENT_ROOT_PORT_NAME", val, val_max_sz);
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
#endif

#endif /* brad : all above is common */
    
fn_exit:

    /* brad : these are handled (or their equivalent bc_*) in MPID_Init now */
#if 0
    if (val != NULL)
    {
	MPIU_Free(val);
    }
    if (key != NULL)
    {
	MPIU_Free(key);
    }
    if (bc_orig != NULL)
    {
	MPIU_Free(bc_orig);
    }
#endif

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_CH3_INIT);
    return mpi_errno;

fn_fail:
    if ((*pg_p) != NULL)
    {
	MPIDI_PG_Destroy(*pg_p);
    }
    goto fn_exit;
}

#if 0 /* brad : now in mpid_init.c */
static int MPIDI_CH3I_PG_Compare_ids(void * id1, void * id2)
{
    return (strcmp((char *) id1, (char *) id2) == 0) ? TRUE : FALSE;
}

static int MPIDI_CH3I_PG_Destroy(MPIDI_PG_t * pg, void * id)
{
    if (pg->ch.kvs_name != NULL)
    {
	MPIU_Free(pg->ch.kvs_name);
    }

    if (id != NULL)
    { 
	MPIU_Free(id);
    }
    
    return MPI_SUCCESS;
}
#endif
