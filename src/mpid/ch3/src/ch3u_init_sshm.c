/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */


#include "mpidi_ch3_impl.h"
#include "pmi.h"


/*  MPIDI_CH3U_Init_sshm - does scalable shared memory specific channel initialization
 *     publish_bc - if non-NULL, will be a pointer to the original position of the bc_val and should
 *                    do KVS Put/Commit/Barrier on business card before returning
 *     bc_key     - business card key buffer pointer.  freed if successfully published
 *     bc_val     - business card value buffer pointer, updated to the next available location or
 *                    freed if published.
 *     val_max_sz - maximum value buffer size reduced by the number of characters written
 *                               
 */

/* This routine is used only by channels/{sshm,ssm}/src/ch3_init.c */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Init_sshm
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Init_sshm(int has_parent, MPIDI_PG_t *pg_p, int pg_rank,
                         char **publish_bc_p, char **bc_key_p, char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
#ifdef MPIDI_CH3_USES_SSHM
    int pmi_errno;
    int pg_size;
    int p;
    char * parent_bizcard = NULL;
#ifdef USE_MQSHM
    char queue_name[MPIDI_MAX_SHM_NAME_LENGTH];
    int initialize_queue = 0;
#endif
    int key_max_sz;
    int val_max_sz;
    char * key = NULL;
    char * val = NULL;

    srand(getpid()); /* brad : needed by generate_shm_string */

    MPIDI_CH3I_Process.shm_reading_list = NULL;
    MPIDI_CH3I_Process.shm_writing_list = NULL;
    MPIDI_CH3I_Process.num_cpus = -1;

    /* brad : need to set these locally */
    pmi_errno = PMI_KVS_Get_key_length_max(&key_max_sz);
    if (pmi_errno != PMI_SUCCESS)
    {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,
			     "**pmi_kvs_get_key_length_max", 
			     "**pmi_kvs_get_key_length_max %d", pmi_errno);
    }
    
    key = MPIU_Malloc(key_max_sz);
    if (key == NULL)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", NULL);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
    
    pmi_errno = PMI_KVS_Get_value_length_max(&val_max_sz);
    if (pmi_errno != PMI_SUCCESS)
    {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,
			     "**pmi_kvs_get_value_length_max", 
			     "**pmi_kvs_get_value_length_max %d", pmi_errno);
    }
    
    val = MPIU_Malloc(val_max_sz);
    if (val == NULL)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", NULL);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }

#ifdef MPIDI_CH3_USES_SHM_NAME
    pg_p->ch.shm_name = NULL;
    pg_p->ch.shm_name = MPIU_Malloc(sizeof(char) * MPIDI_MAX_SHM_NAME_LENGTH);
    if (pg_p->ch.shm_name == NULL)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", NULL);
	goto fn_fail;
	/* --END ERROR HANDLING-- */        
    }
#endif

    /* set the global variable defaults */
    pg_p->ch.nShmEagerLimit = MPIDI_SHM_EAGER_LIMIT;
#ifdef HAVE_SHARED_PROCESS_READ
    pg_p->ch.nShmRndvLimit = MPIDI_SHM_RNDV_LIMIT;
#endif
    pg_p->ch.nShmWaitSpinCount = MPIDI_CH3I_SPIN_COUNT_DEFAULT;
    pg_p->ch.nShmWaitYieldCount = MPIDI_CH3I_YIELD_COUNT_DEFAULT;
    /* Figure out how many processors are available and set the spin count accordingly */
    /* If there were topology information available we could calculate a multi-cpu number */
#ifdef HAVE_WINDOWS_H
    {
	/* if you know the number of processors, calculate the spin count relative to that number */
        SYSTEM_INFO info;
        GetSystemInfo(&info);
        if (info.dwNumberOfProcessors == 1)
            pg_p->ch.nShmWaitSpinCount = 1;
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
	    pg_p->ch.nShmWaitSpinCount = 1;
	/*
	else if (num_cpus > 0 && num_cpus < num_procs_per_node)
	    pg_p->ch.nShmWaitSpinCount = ( MPIDI_CH3I_SPIN_COUNT_DEFAULT * num_cpus ) / num_procs_per_node;
	*/
	if (num_cpus > 0)
	    MPIDI_CH3I_Process.num_cpus = num_cpus;
    }
#endif
#endif
#ifndef HAVE_WINDOWS_H    /* brad - nShmWaitSpinCount is uninitialized in sshm but probably shouldn't be */
    pg_p->ch.nShmWaitSpinCount = 1;
    g_nLockSpinCount = 1;
#endif

    pmi_errno = PMI_Get_size(&pg_size);
    if (pmi_errno != 0)
    {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**pmi_get_size",
			     "**pmi_get_size %d", pmi_errno);
    }
    
    /* Initialize the VC table associated with this process group (and thus COMM_WORLD) */
    for (p = 0; p < pg_size; p++)
    {
	pg_p->vct[p].ch.sendq_head = NULL;
	pg_p->vct[p].ch.sendq_tail = NULL;
	pg_p->vct[p].ch.recv_active = NULL;
	pg_p->vct[p].ch.send_active = NULL;
	pg_p->vct[p].ch.req = NULL;
	pg_p->vct[p].ch.state = MPIDI_CH3I_VC_STATE_UNCONNECTED;
	pg_p->vct[p].ch.shm_read_connected = 0;
	pg_p->vct[p].ch.read_shmq = NULL;
	pg_p->vct[p].ch.write_shmq = NULL;
	pg_p->vct[p].ch.shm = NULL;
	pg_p->vct[p].ch.shm_state = 0;
	pg_p->vct[p].ch.shm_next_reader = NULL;
	pg_p->vct[p].ch.shm_next_writer = NULL;
    }

    /* brad : do the shared memory specific setup items so we can later do the
     *         shared memory aspects of the business card
     */
    
#ifdef HAVE_WINDOWS_H
    {
	DWORD len = sizeof(pg_p->ch.shm_hostname);
	/*GetComputerName(pg_p->ch.shm_hostname, &len);*/
	GetComputerNameEx(ComputerNameDnsFullyQualified, pg_p->ch.shm_hostname, &len);
    }
#else
    gethostname(pg_p->ch.shm_hostname, sizeof(pg_p->ch.shm_hostname));
#endif

#ifdef MPIDI_CH3_USES_SHM_NAME
    MPIDI_Process.my_pg = *pg_p;  /* was later prior but internally Get_parent_port needs this */    
    if (*has_parent) /* set in PMI_Init */
    {
        mpi_errno = MPIDI_CH3_Get_parent_port(&parent_bizcard);
        if (mpi_errno != MPI_SUCCESS)
        {
            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                             "**ch3|get_parent_port", NULL);
            return mpi_errno;
        }

	/* Parse the shared memory queue name from the bizcard */
	{
	    char *orig_str, *tmp_str = MPIU_Malloc(sizeof(char) * MPIDI_MAX_SHM_NAME_LENGTH);
	    if (tmp_str == NULL)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
		return mpi_errno;
	    }
	    mpi_errno = MPIU_Str_get_string_arg(parent_bizcard, MPIDI_CH3I_SHM_QUEUE_KEY, tmp_str, MPIDI_MAX_SHM_NAME_LENGTH);
	    if (mpi_errno != MPIU_STR_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %d", mpi_errno);
		return mpi_errno;
	    }
	    orig_str = tmp_str;
	    while (*tmp_str != ':' && *tmp_str != '\0')
		tmp_str++;
	    if (*tmp_str == ':')
	    {
		tmp_str++;
		mpi_errno = MPIU_Strncpy(pg_p->ch.shm_name, tmp_str, MPIDI_MAX_SHM_NAME_LENGTH);
		MPIU_Free(orig_str);
		if (mpi_errno != 0)
		{
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %d", mpi_errno);
		    return mpi_errno;
		}
	    }
	    else
	    {
		MPIU_Free(orig_str);
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %d", mpi_errno);
		return mpi_errno;
	    }
	}
    }
#endif            

#ifdef USE_MQSHM

    MPIU_Strncpy(key, MPIDI_CH3I_SHM_QUEUE_NAME_KEY, key_max_sz );
    if (pg_rank == 0)
    {
	if (*has_parent == 0)
	{
	    /* Only the first process of the first group needs to create the bootstrap queue. */
	    /* Everyone else including spawned processes will attach to this queue. */
	    mpi_errno = MPIDI_CH3I_BootstrapQ_create_unique_name(queue_name, MPIDI_MAX_SHM_NAME_LENGTH);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**boot_create", 0);
		return mpi_errno;
	    }
	    initialize_queue = 1;
	    MPIU_Strncpy(val, queue_name, val_max_sz);

/*#ifdef MPIDI_CH3_USES_SHM_NAME*/ /* It's not possible for USE_MQSHM to be defined and MPIDI_CH3_USES_SHM_NAME not defined. */
	    MPIU_Strncpy(pg_p->ch.shm_name, val, val_max_sz);
	}
	else
	{
	    MPIU_Strncpy(queue_name, pg_p->ch.shm_name, MPIDI_MAX_SHM_NAME_LENGTH);
	    MPIU_Strncpy(val, queue_name, val_max_sz);
/*#endif*/
	}

	mpi_errno = MPIDI_CH3I_BootstrapQ_create_named(&pg_p->ch.bootstrapQ, queue_name, initialize_queue);
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**boot_create", 0);
	    return mpi_errno;
	}
	/*printf("root process created bootQ: '%s'\n", queue_name);fflush(stdout);*/

	mpi_errno = PMI_KVS_Put(pg_p->ch.kvs_name, key, val);          
	if (mpi_errno != 0)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", mpi_errno);
	    return mpi_errno;
	}
	mpi_errno = PMI_KVS_Commit(pg_p->ch.kvs_name);
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
	mpi_errno = PMI_KVS_Get(pg_p->ch.kvs_name, key, val, val_max_sz);
	if (mpi_errno != 0)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get %d", mpi_errno);
	    return mpi_errno;
	}
	MPIU_Strncpy(queue_name, val, MPIDI_MAX_SHM_NAME_LENGTH);
#ifdef MPIDI_CH3_USES_SHM_NAME
	MPIU_Strncpy(pg_p->ch.shm_name, val, MPIDI_MAX_SHM_NAME_LENGTH);
#endif
	/*printf("process %d got bootQ name: '%s'\n", pg_rank, queue_name);fflush(stdout);*/
	/* If you don't have a parent then you must initialize the queue */
	/* If you do have a parent then you must not initialize the queue since the parent already did and you could destroy valid information */
	initialize_queue = (*has_parent) ? 0 : 1;
	mpi_errno = MPIDI_CH3I_BootstrapQ_create_named(&pg_p->ch.bootstrapQ, queue_name, initialize_queue);
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
    /* The bootstrap queue cannot be unlinked because it can be used outside of this process group. */
    /* Spawned groups will use it and other MPI jobs may use it by calling MPI_Comm_connect/accept */

    	/* FIXME:
	 * By not unlinking here, if the program aborts, the 
	 * shared memory segments can be left dangling.
	 * We need to either unlink here (no dynamic process calls)
	 * for in SIGINT/FPE/SEGV abort handlers.  That isn't 
	 * fully reliable, since the handler may be replaced or the
	 * process killed with an uncatchable signal.
	 */
    /*
    mpi_errno = MPIDI_CH3I_BootstrapQ_unlink(pg_p->ch.bootstrapQ);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**boot_unlink", 0);
	return mpi_errno;
    }
    */

#else

    mpi_errno = MPIDI_CH3I_BootstrapQ_create(&pg_p->ch.bootstrapQ);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**boot_create", 0);
	return mpi_errno;
    }

#endif

    /* brad : the pg needs to be set for sshm channels.  for all channels this is done in mpid_init.c */
    MPIDI_Process.my_pg = *pg_p;

    /* brad : get the sshm part of the business card  */
    mpi_errno = MPIDI_CH3U_Get_business_card_sshm(bc_val_p, val_max_sz_p);
    if (mpi_errno != MPI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_buscard", NULL);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }

    /* see if we're meant to publish */
    if (publish_bc_p != NULL)
    {
	/*
	printf("business card:\n<%s>\npg_id:\n<%s>\n\n", *publish_bc_p, pg_p->id);
	fflush(stdout);
	*/
	pmi_errno = PMI_KVS_Put(pg_p->ch.kvs_name, *bc_key_p, *publish_bc_p);
	if (pmi_errno != PMI_SUCCESS)
	{
	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**pmi_kvs_put",
				 "**pmi_kvs_put %d", pmi_errno);
	}
	pmi_errno = PMI_KVS_Commit(pg_p->ch.kvs_name);
	if (pmi_errno != PMI_SUCCESS)
	{
	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**pmi_kvs_commit",
				 "**pmi_kvs_commit %d", pmi_errno);
	}

	pmi_errno = PMI_Barrier();
	if (pmi_errno != PMI_SUCCESS)
	{
	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**pmi_barrier",
				 "**pmi_barrier %d", pmi_errno);
	}
    }    

 fn_exit:
    if (val != NULL)
    { 
	MPIU_Free(val);
    }
    if (key != NULL)
    { 
	MPIU_Free(key);
    }
#endif /* MPIDI_CH3_USES_SSHM */    
    return mpi_errno;
#ifdef MPIDI_CH3_USES_SSHM
 fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (pg_p != NULL)
    {
	/* MPIDI_CH3I_PG_Destroy(), which is called by MPIDI_PG_Destroy(), frees pg->ch.kvs_name */
	MPIDI_PG_Destroy( pg_p );
    }

    goto fn_exit;
    /* --END ERROR HANDLING-- */
#endif /* MPIDI_CH3_USES_SSHM #2 */    
}
