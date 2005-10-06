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

/* FIXME:
   the PMI init function should ONLY do the PMI operations, not the process 
   group or bc operations.  These should be in a separate routine */
#include "pmi.h"
static int InitPGFromPMI( int *has_args, int *has_env, int *has_parent, 
			  int *pg_rank_p, MPIDI_PG_t **pg_p );
static int MPIDI_CH3I_PG_Compare_ids(void * id1, void * id2);
static int MPIDI_CH3I_PG_Destroy(MPIDI_PG_t * pg, void * id);

int MPIDI_CH3I_BCInit( int pg_rank, 
		       char **publish_bc_p, char **bc_key_p, 
		       char **bc_val_p, int *val_max_sz_p);

#include "mpidi_ch3_impl.h"  /* for extern'd MPIDI_CH3I_Process */

MPIDI_CH3I_Process_t MPIDI_CH3I_Process = {NULL};

/* FIXME: Use function pointers for the RMA operations instead of this flag */
int MPIDI_Use_optimized_rma = 0;

MPIDI_Process_t MPIDI_Process = { NULL };

#undef FUNCNAME
#define FUNCNAME MPID_Init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Init(int *argc, char ***argv, int requested, int *provided, 
	      int *has_args, int *has_env)
{
    int mpi_errno = MPI_SUCCESS;
    int has_parent;
    MPIDI_PG_t * pg;
    int pg_rank;
    int pg_size;
    MPID_Comm * comm;
    int p;
    char * env;
#if 0
    char *publish_bc_orig = NULL;
    char *bc_key = NULL;
    char *bc_val = NULL;
    int val_max_remaining;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPID_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_INIT);
    MPIDI_DBG_PRINTF((10, FCNAME, "entering"));

    /* FIXME: These should not be unreferenced (they should be used!) */
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
    MPIDI_Process.parent_port_name = NULL;

    /* FIXME:
       We don't know that environment variables are sent to all processes.
       In addition, we should use a common set of routines to access
       such parameters to ensure uniform handling */
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
     * Set global process attributes.  These can be overridden by the channel 
     * if necessary.
     */
    MPIR_Process.attrs.tag_ub          = MPIDI_TAG_UB;

    /*
     * Perform channel-independent PMI initialization
     */
    mpi_errno = InitPGFromPMI( has_args, has_env, &has_parent, &pg_rank, &pg );

    /*
     * Let the channel perform any necessary initialization
     * The channel init should assume that PMI_Init has been called and that
     * the basic information about the job has been extracted from PMI (e.g.,
     * the size and rank of this process, and the process group id)
     */
    /* Unresolved: some channels may want to override the PMI values for 
       size and node (gasnet is one such channel).  The initPGFromPMI routine
       could take an optional routine to call before calling PMI routines, 
       as one way to solve this */
    mpi_errno = MPIDI_CH3_Init(has_parent, pg, pg_rank);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**ch3|ch3_init");
    }

    /* FIXME: Why are pg_size and pg_rank handled differently? */
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

    comm->rank        = pg_rank;
    comm->remote_size = pg_size;
    comm->local_size  = pg_size;
    
    mpi_errno = MPID_VCRT_Create(comm->remote_size, &comm->vcrt);
    if (mpi_errno != MPI_SUCCESS)
    {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**dev|vcrt_create", 
			     "**dev|vcrt_create %s", "MPI_COMM_WORLD");
    }
    
    mpi_errno = MPID_VCRT_Get_ptr(comm->vcrt, &comm->vcr);
    if (mpi_errno != MPI_SUCCESS)
    {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**dev|vcrt_get_ptr", 
			     "dev|vcrt_get_ptr %s", "MPI_COMM_WORLD");
    }
    
    /* Initialize the connection table on COMM_WORLD from the process group's
       connection table */
    for (p = 0; p < pg_size; p++)
    {
	MPID_VCR_Dup(&pg->vct[p], &comm->vcr[p]);
    }

    
    /*
     * Initialize the MPI_COMM_SELF object
     */
    comm = MPIR_Process.comm_self;
    comm->rank        = 0;
    comm->remote_size = 1;
    comm->local_size  = 1;
    
    mpi_errno = MPID_VCRT_Create(comm->remote_size, &comm->vcrt);
    if (mpi_errno != MPI_SUCCESS)
    {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**dev|vcrt_create", 
			     "**dev|vcrt_create %s", "MPI_COMM_SELF");
    }
    
    mpi_errno = MPID_VCRT_Get_ptr(comm->vcrt, &comm->vcr);
    if (mpi_errno != MPI_SUCCESS)
    {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**dev|vcrt_get_ptr", 
			     "dev|vcrt_get_ptr %s", "MPI_COMM_WORLD");
    }
    
    MPID_VCR_Dup(&pg->vct[pg_rank], &comm->vcr[0]);

    
    /*
     * If this process group was spawned by a MPI application, then
     * form the MPI_COMM_PARENT inter-communicator.
     */

    /*
     * FIXME: The code to handle the parent case should be in a separate 
     * routine and should not rely on #ifdefs
     */
    if (has_parent)
    {
#       if (defined(MPIDI_CH3_IMPLEMENTS_GET_PARENT_PORT))
	{
	    char * parent_port;

	    /* FIXME: To allow just the "root" process to 
	       request the port and then use MPIR_Bcast to 
	       distribute it to the rest of the processes,
	       we need to perform the Bcast after MPI is
	       otherwise initialized.  We could do this
	       by adding another MPID call that the MPI_Init(_thread)
	       routine would make after the rest of MPI is 
	       initialized, but before MPI_Init returns.
	       In fact, such a routine could be used to 
	       perform various checks, including parameter
	       consistency value (e.g., all processes have the
	       same environment variable values). Alternately,
	       we could allow a few routines to operate with 
	       predefined parameter choices (e.g., bcast, allreduce)
	       for the purposes of initialization. */
	    mpi_errno = MPIDI_CH3_Get_parent_port(&parent_port);
	    if (mpi_errno != MPI_SUCCESS) {
		MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, 
				    "**ch3|get_parent_port");
	    }
	    
	    mpi_errno = MPID_Comm_connect(parent_port, NULL, 0, MPIR_Process.comm_world, &comm);
	    if (mpi_errno != MPI_SUCCESS) {
		MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,
				     "**ch3|conn_parent", 
				     "**ch3|conn_parent %s", parent_port);
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
	/* FIXME: It should be possible to select a thread-safety level 
	   lower than the configure level, avoiding the locks with a
	   simple if test */
	*provided = MPICH_THREAD_LEVEL;
    }

  fn_exit:
    MPIDI_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_INIT);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/*
 * Initialize the process group structure by using PMI calls.
 * This routine initializes PMI and uses PMI calls to setup the 
 * process group structures.
 * 
 */
static int InitPGFromPMI( int *has_args, int *has_env, int *has_parent, 
			  int *pg_rank_p, MPIDI_PG_t **pg_p )
{
    int pmi_errno;
    int mpi_errno = MPI_SUCCESS;
    int pg_rank, pg_size, appnum, pg_id_sz, kvs_name_sz;
    char *pg_id;
    MPIDI_PG_t *pg = 0;
    
    /*
     * Initialize the process manangement interface (PMI), 
     * and get rank and size information about our process group
     */
    pmi_errno = PMI_Init(has_parent);
    if (pmi_errno != PMI_SUCCESS)
    {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**pmi_init",
			     "**pmi_init %d", pmi_errno);
    }

    /* FIXME: Who is this for and where does it belong? */
#ifdef USE_MPIU_DBG_PRINT_VC
    MPIU_DBG_parent_str = (*has_parent) ? "+" : "";
#endif

    pmi_errno = PMI_Get_rank(&pg_rank);
    if (pmi_errno != PMI_SUCCESS)
    {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**pmi_get_rank",
			     "**pmi_get_rank %d", pmi_errno);
    }

    pmi_errno = PMI_Get_size(&pg_size);
    if (pmi_errno != 0)
    {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**pmi_get_size",
			     "**pmi_get_size %d", pmi_errno);
    }

    pmi_errno = PMI_Get_appnum(&appnum);
    if (pmi_errno != PMI_SUCCESS)
    {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**pmi_get_appnum",
			     "**pmi_get_appnum %d", pmi_errno);
    }

    if (appnum != -1)
    {
	MPIR_Process.attrs.appnum = appnum;
    }

    /* FIXME: Who does/does not use this? */
#ifdef MPIDI_DEV_IMPLEMENTS_KVS
    /* Initialize the CH3 device KVS cache interface */
    /* KVS is used for connection handling; thus, this should go into 
       code for that purpose, not here */
    /* Do this after PMI_Init because MPIDI_KVS uses PMI (The init funcion may or may not use PMI)*/
    MPIDI_KVS_Init();
#endif

    /* Now, initialize the process group information with PMI calls */
    /*
     * Get the process group id
     */
    pmi_errno = PMI_Get_id_length_max(&pg_id_sz);
    if (pmi_errno != PMI_SUCCESS)
    {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,
			     "**pmi_get_id_length_max", 
			     "**pmi_get_id_length_max %d", pmi_errno);
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
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**pmi_get_id",
			     "**pmi_get_id %d", pmi_errno);
    }

    /*
     * Initialize the process group tracking subsystem
     */
    mpi_errno = MPIDI_PG_Init(MPIDI_CH3I_PG_Compare_ids, MPIDI_CH3I_PG_Destroy);
    if (mpi_errno != MPI_SUCCESS)
    {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**dev|pg_init");
    }

    /*
     * Create a new structure to track the process group
     */
    mpi_errno = MPIDI_PG_Create(pg_size, pg_id, &pg);
    if (mpi_errno != MPI_SUCCESS)
    {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**dev|pg_create");
    }
    pg->ch.kvs_name = NULL;

    /*
     * Get the name of the key-value space (KVS)
     */
    pmi_errno = PMI_KVS_Get_name_length_max(&kvs_name_sz);
    if (pmi_errno != PMI_SUCCESS)
    {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, 
			     "**pmi_kvs_get_name_length_max", 
			     "**pmi_kvs_get_name_length_max %d", pmi_errno);
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
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**pmi_kvs_get_my_name", 
			     "**pmi_kvs_get_my_name %d", pmi_errno);
    }

    /* FIXME: has_args and has_env need to come from PMI eventually... */
    *has_args = TRUE;
    *has_env  = TRUE;

    *pg_p      = pg;
    *pg_rank_p = pg_rank;
    
 fn_exit:
    return mpi_errno;
 fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (pg) {
	MPIDI_PG_Destroy( pg );
    }
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}

/*
 * Initialize the business card.  This creates only the key part of the
 * business card; the value is later set by the channel.
 * 
 * FIXME: The code to set the business card should be more channel-specific, 
 * and not in this routine (and it should be performed within the channel 
 * init)
 */
int MPIDI_CH3I_BCInit( int pg_rank, 
		       char **publish_bc_p, char **bc_key_p,
		       char **bc_val_p, int *val_max_sz_p )
{
    int pmi_errno;
    int mpi_errno = MPI_SUCCESS;
    int key_max_sz;

    /*
     * Publish the contact information (a.k.a. business card) for this process into the PMI keyval space associated with this
     * process group.
     */
    pmi_errno = PMI_KVS_Get_key_length_max(&key_max_sz);
    if (pmi_errno != PMI_SUCCESS)
    {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,
			     "**pmi_kvs_get_key_length_max", 
			     "**pmi_kvs_get_key_length_max %d", pmi_errno);
    }
    
    *bc_key_p = MPIU_Malloc(key_max_sz);
    if (*bc_key_p == NULL)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", NULL);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }

    pmi_errno = PMI_KVS_Get_value_length_max(val_max_sz_p);
    if (pmi_errno != PMI_SUCCESS)
    {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, 
			     "**pmi_kvs_get_value_length_max",
			     "**pmi_kvs_get_value_length_max %d", pmi_errno);
    }
    
    *bc_val_p = MPIU_Malloc(*val_max_sz_p);
    if (*bc_val_p == NULL)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", NULL);
	goto fn_fail;
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
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**snprintf",
			     "**snprintf %d", mpi_errno);
    }
    mpi_errno = MPI_SUCCESS;
    
    
  fn_exit:

    return mpi_errno;

  fn_fail:
    goto fn_exit;
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
