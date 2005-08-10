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


/*  MPIDI_CH3U_Init_sshm - does scalable shared memory specific channel initialization
 *     publish_bc - if non-NULL, will be a pointer to the original position of the bc_val and should
 *                    do KVS Put/Commit/Barrier on business card before returning
 *     bc_key     - business card key buffer pointer.  freed if successfully published
 *     bc_val     - business card value buffer pointer, updated to the next available location or
 *                    freed if published.
 *     val_max_sz - maximum value buffer size reduced by the number of characters written
 *                               
 */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Init_sshm
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Init_sshm(int * has_args, int * has_env, int * has_parent, MPIDI_PG_t ** pg_p, int * pg_rank_p,
                         char **publish_bc_p, char **bc_key_p, char **bc_val_p, int *val_max_sz_p)
{
#ifdef MPIDI_CH3_USES_SSHM
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    int pg_size;
    int pg_rank = *pg_rank_p;
    int p;
    char * parent_bizcard = NULL;    
    char queue_name[100];
    /* brad : this key and val are different than old ones.  they are strictly local; variables
     *         who are required to be seen outside of this function are passed in
     */
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
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**pmi_kvs_get_key_length_max", "**pmi_kvs_get_key_length_max %d", pmi_errno);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
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
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**pmi_kvs_get_value_length_max", "**pmi_kvs_get_value_length_max %d", pmi_errno);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
    
    val = MPIU_Malloc(val_max_sz);
    if (val == NULL)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", NULL);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }

#ifdef MPIDI_CH3_IMPLEMENTS_COMM_CONNECT
    /* brad : added so eventually bootstrapQ_name can be incorporated into the bizcard */
    (*pg_p)->ch.bootstrapQ_name = NULL;
    (*pg_p)->ch.bootstrapQ_name = MPIU_Malloc(sizeof(char) * 100); /* brad : matches queue_name length */
    if((*pg_p)->ch.bootstrapQ_name == NULL)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", NULL);
	goto fn_fail;
	/* --END ERROR HANDLING-- */        
    }
#endif
    
    /* set the global variable defaults */
    (*pg_p)->ch.nShmEagerLimit = MPIDI_SHM_EAGER_LIMIT;
#ifdef HAVE_SHARED_PROCESS_READ
    (*pg_p)->ch.nShmRndvLimit = MPIDI_SHM_RNDV_LIMIT;
#endif
    (*pg_p)->ch.nShmWaitSpinCount = MPIDI_CH3I_SPIN_COUNT_DEFAULT;
    (*pg_p)->ch.nShmWaitYieldCount = MPIDI_CH3I_YIELD_COUNT_DEFAULT;
    /* Figure out how many processors are available and set the spin count accordingly */
    /* If there were topology information available we could calculate a multi-cpu number */
#ifdef HAVE_WINDOWS_H
    {
	/* if you know the number of processors, calculate the spin count relative to that number */
        SYSTEM_INFO info;
        GetSystemInfo(&info);
        if (info.dwNumberOfProcessors == 1)
            (*pg_p)->ch.nShmWaitSpinCount = 1;
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
	    (*pg_p)->ch.nShmWaitSpinCount = 1;
	/*
	else if (num_cpus > 0 && num_cpus < num_procs_per_node)
	    (*pg_p)->ch.nShmWaitSpinCount = ( MPIDI_CH3I_SPIN_COUNT_DEFAULT * num_cpus ) / num_procs_per_node;
	*/
	if (num_cpus > 0)
	    MPIDI_CH3I_Process.num_cpus = num_cpus;
    }
#endif
#endif
#ifndef HAVE_WINDOWS_H    /* brad - nShmWaitSpinCount is uninitialized in sshm but probably shouldn't be */
    (*pg_p)->ch.nShmWaitSpinCount = 1;
    g_nLockSpinCount = 1;
#endif

    pmi_errno = PMI_Get_size(&pg_size);
    if (pmi_errno != 0)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_size",
					 "**pmi_get_size %d", pmi_errno);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
    
    /* Initialize the VC table associated with this process group (and thus COMM_WORLD) */
    for (p = 0; p < pg_size; p++)
    {
	(*pg_p)->vct[p].ch.sendq_head = NULL;
	(*pg_p)->vct[p].ch.sendq_tail = NULL;
	(*pg_p)->vct[p].ch.recv_active = NULL;
	(*pg_p)->vct[p].ch.send_active = NULL;
	(*pg_p)->vct[p].ch.req = NULL;
	(*pg_p)->vct[p].ch.state = MPIDI_CH3I_VC_STATE_UNCONNECTED;
	(*pg_p)->vct[p].ch.read_shmq = NULL;
	(*pg_p)->vct[p].ch.write_shmq = NULL;
	(*pg_p)->vct[p].ch.shm = NULL;
	(*pg_p)->vct[p].ch.shm_state = 0;
	(*pg_p)->vct[p].ch.shm_next_reader = NULL;
	(*pg_p)->vct[p].ch.shm_next_writer = NULL;
        /* brad : sshm does not have a ch.bShm */
    }

    /* brad : do the shared memory specific setup items so we can later do the
     *         shared memory aspects of the business card
     */
    
#ifdef HAVE_WINDOWS_H
    {
	DWORD len = sizeof((*pg_p)->ch.shm_hostname);
	/*GetComputerName((*pg_p)->ch.shm_hostname, &len);*/
	GetComputerNameEx(ComputerNameDnsFullyQualified, (*pg_p)->ch.shm_hostname, &len);
    }
#else
    gethostname((*pg_p)->ch.shm_hostname, sizeof((*pg_p)->ch.shm_hostname));
/*     printf("*pg_p = %d, my_pg = %d\n",  (int) (*pg_p),  (int) MPIDI_Process.my_pg);  2nd one is NULL... */
#endif

#ifdef MPIDI_CH3_IMPLEMENTS_COMM_CONNECT
    /* brad : prepare the bootstrapQ_name field in the case where it has a parent. this is so
     *         in a SMP environment that processes on the same system but different communicators
     *         can communicate via shared memory.
     */
    MPIDI_Process.my_pg = *pg_p;  /* was later prior but internally Get_parent_port needs this */    
    if (*has_parent) /* brad : set in PMI_Init */
    {
        mpi_errno = MPIDI_CH3_Get_parent_port(&parent_bizcard);
        if (mpi_errno != MPI_SUCCESS)
        {
            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                             "**ch3|get_parent_port", NULL);
            return mpi_errno;
        }
        
        /* bootstrapQ_name already malloc'd above */
        mpi_errno = MPIU_Str_get_string_arg(parent_bizcard, MPIDI_CH3I_SHM_BOOTSTRAPQ_NAME_KEY, (*pg_p)->ch.bootstrapQ_name, 100);
        if (mpi_errno != MPIU_STR_SUCCESS)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**argstr_hostd", 0);  /* brad : TODO create new error code (used old one to compile) */
            return mpi_errno;
        }        
        
    }
#endif            
     

#ifdef USE_MQSHM

    MPIU_Strncpy(key, "bootstrapQ_name", key_max_sz );
    /* brad : distinguish pg_rank between having a parent and not (want to
     *         ensure bootstrapQ_name is identical on different processes in
     *         different process groups)
     */
    if (pg_rank == 0)
    {
          if (*has_parent == 0 )  /* brad : this guy will generate the shm string */
          {              
              mpi_errno = MPIDI_CH3I_BootstrapQ_create_unique_name(queue_name, 100);
              if (mpi_errno != MPI_SUCCESS)
              {
                  mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**boot_create", 0);
                  return mpi_errno;
              }
        
              MPIU_Strncpy(val, queue_name, val_max_sz);
              
#ifdef MPIDI_CH3_IMPLEMENTS_COMM_CONNECT
              MPIU_Strncpy((*pg_p)->ch.bootstrapQ_name, val, val_max_sz);
          }
          else      /* brad : has a parent, yet is the root of its process group so it must
                     *         publish the inherited bootstrapQ_name in THIS kvs_name. it
                     *         publishes it just for consistency (technically this information
                     *         would be available simply within the bizcard/parent_port_name).
                     */
          {
              MPIU_Strncpy(queue_name, (*pg_p)->ch.bootstrapQ_name, 100);
              MPIU_Strncpy(val, queue_name, val_max_sz);
#endif              
          }
                
          mpi_errno = MPIDI_CH3I_BootstrapQ_create_named(&(*pg_p)->ch.bootstrapQ, queue_name, 1);
          if (mpi_errno != MPI_SUCCESS)
          {
              mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**boot_create", 0);
              return mpi_errno;
          }
          /*printf("root process created bootQ: '%s'\n", queue_name);fflush(stdout);*/

	mpi_errno = PMI_KVS_Put((*pg_p)->ch.kvs_name, key, val);          
	if (mpi_errno != 0)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", mpi_errno);
	    return mpi_errno;
	}
	mpi_errno = PMI_KVS_Commit((*pg_p)->ch.kvs_name);
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
	mpi_errno = PMI_KVS_Get((*pg_p)->ch.kvs_name, key, val, val_max_sz);
	if (mpi_errno != 0)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get %d", mpi_errno);
	    return mpi_errno;
	}
	MPIU_Strncpy(queue_name, val, val_max_sz);
#ifdef MPIDID_CH3_IMPLEMENTS_COMM_CONNECT
          MPIU_Strncpy((*pg_p)->ch.bootstrapQ_name, val, val_max_sz);
#endif
	/*printf("process %d got bootQ name: '%s'\n", pg_rank, queue_name);fflush(stdout);*/
	mpi_errno = MPIDI_CH3I_BootstrapQ_create_named(&(*pg_p)->ch.bootstrapQ, queue_name, 1);
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
/*     mpi_errno = MPIDI_CH3I_BootstrapQ_unlink((*pg_p)->ch.bootstrapQ); */
/*     if (mpi_errno != MPI_SUCCESS) */
/*     { */
/* 	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**boot_unlink", 0); */
/* 	return mpi_errno; */
/*     } */

#else

    mpi_errno = MPIDI_CH3I_BootstrapQ_create(&(*pg_p)->ch.bootstrapQ);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**boot_create", 0);
	return mpi_errno;
    }

#endif

    /* brad : the pg needs to be set for sshm channels.  for all channels this is done in mpid_init.c */
    MPIDI_Process.my_pg = *pg_p;
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
    if(publish_bc_p != NULL) {
        pmi_errno = PMI_KVS_Put((*pg_p)->ch.kvs_name, *bc_key_p, *publish_bc_p);
        if (pmi_errno != PMI_SUCCESS)
            {
                /* --BEGIN ERROR HANDLING-- */
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_put",
                                                 "**pmi_kvs_put %d", pmi_errno);
                goto fn_fail;
                /* --END ERROR HANDLING-- */
            }
        pmi_errno = PMI_KVS_Commit((*pg_p)->ch.kvs_name);
        if (pmi_errno != PMI_SUCCESS)
            {
                /* --BEGIN ERROR HANDLING-- */
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_commit",
                                                 "**pmi_kvs_commit %d", pmi_errno);
                goto fn_fail;
                /* --END ERROR HANDLING-- */
            }

        pmi_errno = PMI_Barrier();
        if (pmi_errno != PMI_SUCCESS)
            {
                /* --BEGIN ERROR HANDLING-- */
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_barrier",
                                                 "**pmi_barrier %d", pmi_errno);
                goto fn_fail;
                /* --END ERROR HANDLING-- */
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
    if ((*pg_p) != NULL)
    {
	/* MPIDI_CH3I_PG_Destroy(), which is called by MPIDI_PG_Destroy(), frees pg->ch.kvs_name */
	MPIDI_PG_Destroy(*pg_p);
    }

    goto fn_exit;
    /* --END ERROR HANDLING-- */
#endif /* MPIDI_CH3_USES_SSHM #2 */    
}
