/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#if defined(HAVE_LIMITS_H)
#include <limits.h>
#endif
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#ifndef MPIDI_CH3_UNFACTORED_INIT
#include "pmi.h"
static int MPIDI_CH3I_PMI_Init(int * has_args, int * has_env, int * has_parent, MPIDI_PG_t ** pg_p, int * pg_rank_p,
                               char **publish_bc_p, char **bc_key_p, char **bc_val_p, int *val_max_sz_p);
static int MPIDI_CH3I_PG_Compare_ids(void * id1, void * id2);
static int MPIDI_CH3I_PG_Destroy(MPIDI_PG_t * pg, void * id);
#endif
#include "mpidi_ch3_impl.h"  /* for extern'd MPIDI_CH3I_Process */

MPIDI_CH3I_Process_t MPIDI_CH3I_Process = {NULL};

int MPIDI_Use_optimized_rma = 0;

MPIDI_Process_t MPIDI_Process = { NULL };

#undef FUNCNAME
#define FUNCNAME MPID_Init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Init(int * argc, char *** argv, int requested, int * provided, int * has_args, int * has_env)
{
    int mpi_errno = MPI_SUCCESS;
    int has_parent;
    MPIDI_PG_t * pg;
    int pg_rank;
    int pg_size;
    MPID_Comm * comm;
    int p;
    char * env;
    char *publish_bc_orig = NULL;
    char *bc_key = NULL;
    char *bc_val = NULL;
    int val_max_remaining;
    MPIDI_STATE_DECL(MPID_STATE_MPID_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_INIT);
    MPIDI_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_UNREFERENCED_ARG(argc);
    MPIU_UNREFERENCED_ARG(argv);
    MPIU_UNREFERENCED_ARG(requested);

    /*
     * Initialize the device's process information structure
     */
    MPIDI_Process.recvq_posted_head = NULL;
    MPIDI_Process.recvq_posted_tail = NULL;
    MPIDI_Process.recvq_unexpected_head = NULL;
    MPIDI_Process.recvq_unexpected_tail = NULL;
    MPIDI_Process.lpid_counter = 0;
    MPIDI_Process.warnings_enabled = TRUE;

    /* FIXME: This is a note that the code to find the processor name
       has been moved into the file that implements the get_processor_name
       call */

    env = getenv("MPICH_WARNINGS");
    if (env)
    {
	if (strcmp(env, "1") == 0 || strcmp(env, "on") == 0 || strcmp(env, "yes") == 0)
	{ 
	    MPIDI_Process.warnings_enabled = TRUE;
	}
	if (strcmp(env, "0") == 0 || strcmp(env, "off") == 0 || strcmp(env, "no") == 0)
	{ 
	    MPIDI_Process.warnings_enabled = FALSE;
	}
    }

    
    /*
     * Set global process attributes.  These can be overridden by the channel if necessary.
     */
    MPIR_Process.attrs.tag_ub          = MPIDI_TAG_UB;


    /*
     * Perform channel-independent PMI initialization
     */
#ifndef MPIDI_CH3_UNFACTORED_INIT
    mpi_errno = MPIDI_CH3I_PMI_Init(has_args, has_env, &has_parent, &pg, &pg_rank,
                               &publish_bc_orig, &bc_key, &bc_val, &val_max_remaining);
    if (mpi_errno != MPI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", NULL);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }    
#endif
    
    /*
     * Let the channel perform any necessary initialization
     */
    mpi_errno = MPIDI_CH3_Init(has_args, has_env, &has_parent, &pg, &pg_rank,
                              &publish_bc_orig, &bc_key, &bc_val, &val_max_remaining);
    if (mpi_errno != MPI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|ch3_init", NULL);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }


    pg_size = MPIDI_PG_Get_size(pg);
    MPIDI_Process.my_pg = pg;  /* brad : this is rework for shared memories because they need this set earlier
                                *         for getting the business card
                                */
    MPIDI_Process.my_pg_rank = pg_rank;
    MPIDI_PG_Add_ref(pg);

    /*
     * Initialize the MPI_COMM_WORLD object
     */
    comm = MPIR_Process.comm_world;

    comm->rank = pg_rank;
    comm->remote_size = pg_size;
    comm->local_size = pg_size;
    
    mpi_errno = MPID_VCRT_Create(comm->remote_size, &comm->vcrt);
    if (mpi_errno != MPI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**dev|vcrt_create", "**dev|vcrt_create %s", "MPI_COMM_WORLD");
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
    
    mpi_errno = MPID_VCRT_Get_ptr(comm->vcrt, &comm->vcr);
    if (mpi_errno != MPI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**dev|vcrt_get_ptr", "dev|vcrt_get_ptr %s", "MPI_COMM_WORLD");
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
    
    for (p = 0; p < pg_size; p++)
    {
	MPID_VCR_Dup(&pg->vct[p], &comm->vcr[p]);
    }

    
    /*
     * Initialize the MPI_COMM_SELF object
     */
    comm = MPIR_Process.comm_self;
    comm->rank = 0;
    comm->remote_size = 1;
    comm->local_size = 1;
    
    mpi_errno = MPID_VCRT_Create(comm->remote_size, &comm->vcrt);
    if (mpi_errno != MPI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**dev|vcrt_create", "**dev|vcrt_create %s", "MPI_COMM_SELF");
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
    
    mpi_errno = MPID_VCRT_Get_ptr(comm->vcrt, &comm->vcr);
    if (mpi_errno != MPI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**dev|vcrt_get_ptr", "dev|vcrt_get_ptr %s", "MPI_COMM_WORLD");
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
    
    MPID_VCR_Dup(&pg->vct[pg_rank], &comm->vcr[0]);

    
    /*
     * If this process group was spawned by a MPI application, the form the MPI_COMM_PARENT inter-communicator.
     */
    
    if (has_parent)
    {
#	if defined(MPIDI_CH3_IMPLEMENTS_COMM_GET_PARENT)
	{
	    mpi_errno = MPIDI_CH3_Comm_get_parent(&comm);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		/* --BEGIN ERROR HANDLING-- */
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
						 "**ch3|conn_parent", "**ch3|conn_parent %s", val);
		goto fn_fail;
		/* --END ERROR HANDLING-- */
	    }
	}
#       elif (defined(MPIDI_CH3_IMPLEMENTS_GET_PARENT_PORT))
	{
	    char * parent_port;

#         if 0
	    int p0_mpi_errno = MPI_SUCCESS;
  	    /*
	     * Ideally, only process zero would call MPIDI_CH3_Get_parent_port(), but MPI/R_Bcast() cannot be called until
	     * initialization of MPI is complete.  Obviously that's not the case here... (we are in MPID_Init :-)
	     */ 
	    if (pg_rank == 0)
	    { 
		p0_mpi_errno = MPIDI_CH3_Get_parent_port(&parent_port);
	    }
	    else
	    {
		parent_port = NULL;
	    }

	    mpi_errno = MPIR_Bcast(&p0_mpi_errno, 1, MPI_INT, 0, MPIR_Process.comm_world);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
						 "**ch3|get_parent_port_err_bcast", NULL);
		    goto fn_fail;
	    }
	    if (p0_mpi_errno != MPI_SUCCESS)
	    {
		if (pg_rank == 0)
		{
		    mpi_errno = p0_mpi_errno;
		}
		
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
						 "**ch3|get_parent_port", NULL);
		goto fn_fail;
	    }
	    /* --END ERROR HANDLING-- */
#	  else
	    mpi_errno = MPIDI_CH3_Get_parent_port(&parent_port);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS)
	    { 
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
						 "**ch3|get_parent_port", NULL);
		goto fn_fail;
	    }
	    /* --END ERROR HANDLING-- */
#         endif
	    
	    mpi_errno = MPID_Comm_connect(parent_port, NULL, 0, MPIR_Process.comm_world, &comm);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		/* --BEGIN ERROR HANDLING-- */
		if (pg_rank == 0)
		{ 
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
						     "**ch3|conn_parent", "**ch3|conn_parent %s", parent_port);
		}
		else
		{
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
						     "**ch3|conn_parent", NULL);
		}
		goto fn_fail;
		/* --END ERROR HANDLING-- */
	    }
	}
#	else
	{
	    /* --BEGIN ERROR HANDLING-- */
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**notimpl",
					     "**notimpl %s", "MPIDI_CH3_Comm_get_parent");
	    /* --END ERROR HANDLING-- */
	}
#	endif

	MPIR_Process.comm_parent = comm;
	MPIU_Assert(MPIR_Process.comm_parent != NULL);
	MPIU_Strncpy(comm->name, "MPI_COMM_PARENT", MPI_MAX_OBJECT_NAME);
	
	/* TODO: Check that this intercommunicator gets freed in MPI_Finalize if not already freed.  */
    }
	
    
    /*
     * Set provided thread level
     */
    if (provided != NULL)
    {
	*provided = MPICH_THREAD_LEVEL;
    }

  fn_exit:
    /* brad : free PMI business card bufs here */
    if (bc_key != NULL)
    {
        MPIU_Free(bc_key);
    }
    if (publish_bc_orig != NULL)
    {
        MPIU_Free(publish_bc_orig);
    }           
    
    MPIDI_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_INIT);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}



#ifndef MPIDI_CH3_UNFACTORED_INIT
/*
 *  MPIDI_CH3I_PMI_Init -  does channel independent initializations
 *
 */

static int MPIDI_CH3I_PMI_Init(int * has_args, int * has_env, int * has_parent, MPIDI_PG_t ** pg_p, int * pg_rank_p,
                               char **publish_bc_p, char **bc_key_p, char **bc_val_p, int *val_max_sz_p)
{
    MPIDI_PG_t * pg = NULL;
    int pg_rank;
    int pg_size;
    char * pg_id = NULL;
    int pmi_errno = PMI_SUCCESS;
    int mpi_errno = MPI_SUCCESS;
    int pg_id_sz;
    int kvs_name_sz;
    int key_max_sz;
    int appnum;

#ifdef MPIDI_CH3_IMPLEMENTS_GET_PARENT_PORT    
    MPIDI_CH3I_Process.parent_port_name = NULL;
#endif
    
#ifdef MPIDI_CH3_USES_ACCEPTQ
    MPIDI_CH3I_Process.acceptq_head = NULL;
#endif

#   if (USE_THREAD_IMPL == MPICH_THREAD_IMPL_NOT_IMPLEMENTED)
    {
	MPID_Thread_lock_init(&MPIDI_CH3I_Process.acceptq_mutex);
    }
#   endif

    /*
     * Intial the process manangement interface (PMI), and get rank and size information about our process group
     */
    pmi_errno = PMI_Init(has_parent);
    if (pmi_errno != PMI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_init",
					 "**pmi_init %d", pmi_errno);
	goto fn2_fail;
	/* --END ERROR HANDLING-- */
    }
#ifdef USE_MPIU_DBG_PRINT_VC
    MPIU_DBG_parent_str = (*has_parent) ? "+" : "";
#endif

#ifdef MPIDI_DEV_IMPLEMENTS_KVS
    /* Initialize the CH3 device KVS cache interface */
    /* Do this after PMI_Init because MPIDI_KVS uses PMI (The init funcion may or may not use PMI)*/
    MPIDI_KVS_Init();
#endif

    pmi_errno = PMI_Get_rank(&pg_rank);
    if (pmi_errno != PMI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_rank",
					 "**pmi_get_rank %d", pmi_errno);
	goto fn2_fail;
	/* --END ERROR HANDLING-- */
    }

    pmi_errno = PMI_Get_size(&pg_size);
    if (pmi_errno != 0)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_size",
					 "**pmi_get_size %d", pmi_errno);
	goto fn2_fail;
	/* --END ERROR HANDLING-- */
    }

    pmi_errno = PMI_Get_appnum(&appnum);
    if (pmi_errno != PMI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_appnum",
					 "**pmi_get_appnum %d", pmi_errno);
	goto fn2_fail;
	/* --END ERROR HANDLING-- */
    }

    if (appnum != -1)
    {
	MPIR_Process.attrs.appnum = appnum;
    }

    /*
     * Get the process group id
     */
    pmi_errno = PMI_Get_id_length_max(&pg_id_sz);
    if (pmi_errno != PMI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**pmi_get_id_length_max", "**pmi_get_id_length_max %d", pmi_errno);
	goto fn2_fail;
	/* --END ERROR HANDLING-- */
    }

    pg_id = MPIU_Malloc(pg_id_sz + 1);
    if (pg_id == NULL)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", NULL);
	goto fn2_fail;
	/* --END ERROR HANDLING-- */
    }
    
    pmi_errno = PMI_Get_id(pg_id, pg_id_sz);
    if (pmi_errno != PMI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_id",
					 "**pmi_get_id %d", pmi_errno);
	goto fn2_fail;
	/* --END ERROR HANDLING-- */
    }


    /*
     * Initialize the process group tracking subsystem
     */
    mpi_errno = MPIDI_PG_Init(MPIDI_CH3I_PG_Compare_ids, MPIDI_CH3I_PG_Destroy);
    if (mpi_errno != MPI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**dev|pg_init", NULL);
	goto fn2_fail;
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
	goto fn2_fail;
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
	goto fn2_fail;
	/* --END ERROR HANDLING-- */
    }

    pg->ch.kvs_name = MPIU_Malloc(kvs_name_sz + 1);
    if (pg->ch.kvs_name == NULL)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", NULL);
	goto fn2_fail;
	/* --END ERROR HANDLING-- */
    }
    
    pmi_errno = PMI_KVS_Get_my_name(pg->ch.kvs_name, kvs_name_sz);
    if (pmi_errno != PMI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**pmi_kvs_get_my_name", "**pmi_kvs_get_my_name %d", pmi_errno);
	goto fn2_fail;
	/* --END ERROR HANDLING-- */
    }

    /*
     *  VC initialization is now in MPIDI_CH3_Init (and some in MPIDI_CH3U_Init_* upcalls)
     */

    /*
     * Initialize Progress Engine.  This must occur before the business card is requested because part of progress engine
     * initialization is setting up the listener socket.  The port of the listener socket needs to be included in the business
     * card.
     */
    /* FIXME: This is an internal function not part of the CH3 channel interface */
    mpi_errno = MPIDI_CH3I_Progress_init();
    if (mpi_errno != MPI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**init_progress", NULL);
	goto fn2_fail;
	/* --END ERROR HANDLING-- */
    }    

    /*
     * Publish the contact information (a.k.a. business card) for this process into the PMI keyval space associated with this
     * process group.
     */
    pmi_errno = PMI_KVS_Get_key_length_max(&key_max_sz);
    if (pmi_errno != PMI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**pmi_kvs_get_key_length_max", "**pmi_kvs_get_key_length_max %d", pmi_errno);
	goto fn2_fail;
	/* --END ERROR HANDLING-- */
    }
    
    *bc_key_p = MPIU_Malloc(key_max_sz);
    if (*bc_key_p == NULL)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", NULL);
	goto fn2_fail;
	/* --END ERROR HANDLING-- */
    }

    pmi_errno = PMI_KVS_Get_value_length_max(val_max_sz_p);
    if (pmi_errno != PMI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**pmi_kvs_get_value_length_max", "**pmi_kvs_get_value_length_max %d", pmi_errno);
	goto fn2_fail;
	/* --END ERROR HANDLING-- */
    }
    
    *bc_val_p = MPIU_Malloc(*val_max_sz_p);
    if (*bc_val_p == NULL)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", NULL);
	goto fn2_fail;
	/* --END ERROR HANDLING-- */
    }
    *publish_bc_p = *bc_val_p;  /* need to keep a pointer to the front of the front of this buffer to publish */

    /* could put MPIU_Snprintf("P%d-businesscard") call here...  then take boolean publish_bc to
     *   the MPIDI_CH3U_Init_* upcalls (for sshm, it will make 2 upcalls but we don't want to publish after the
     *   first call) which will be called from within the respective MPIDI_CH3_Init channels.
     *
     * val and business_card variables are used differently from channel to channel... */
    
    mpi_errno = MPIU_Snprintf(*bc_key_p, key_max_sz, "P%d-businesscard", pg_rank);
    if (mpi_errno < 0 || mpi_errno > key_max_sz)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**snprintf",
					 "**snprintf %d", mpi_errno);
	goto fn2_fail;
	/* --END ERROR HANDLING-- */
    }
    mpi_errno = MPI_SUCCESS;
    
    
    /* FIXME: has_args and has_env need to come from PMI eventually... */
    *has_args = TRUE;
    *has_env = TRUE;

    *pg_p = pg;
    *pg_rank_p = pg_rank;
    
  fn2_exit:

    return mpi_errno;

  fn2_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (pg != NULL)
    {
	/* MPIDI_CH3I_PG_Destroy(), which is called by MPIDI_PG_Destroy(), frees pg->ch.kvs_name */
	MPIDI_PG_Destroy(pg);
    }

    goto fn2_exit;
    /* --END ERROR HANDLING-- */    
}

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
