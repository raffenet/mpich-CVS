/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

mpig_process_t mpig_process = {NULL, "(null)", -1, -1};

/*
 * MPID_Init()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Init
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Init(int * argc, char *** argv, int requested, int * provided, int * has_args, int * has_env)
{
    mpig_bc_t bc;
    mpig_bc_t * bcs;
    mpig_pg_t * pg;
    const char * pg_id;
    int pg_rank;
    int pg_size;
    MPID_Comm * comm;
    int p;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_INIT);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_INIT);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    /*
     * Initialize the device's process information structure
     */
    mpig_process.my_pg = NULL;
    mpig_process.my_pg_rank = 0;
    mpig_process.my_pg_size = 0;
    mpig_process.lpid_counter = 0;
    mpig_process.lpid_counter = 0;
    
#   if defined(HAVE_GETHOSTNAME)
    {
	if(gethostname(mpig_process.hostname, MPIG_PROCESSOR_NAME_SIZE) != 0)
	{
	    mpig_process.hostname[0] = '\0';
	}
    }
#   else
    {
	mpig_process.hostname[0] = '\0';
    }
#   endif

    mpig_process.pid = getpid();
    
    /*
     * Initialize the receive queue
     */
    mpi_errno = mpig_recvq_init();
    MPIU_ERR_CHKANDJUMP((mpi_errno != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|recvq_init");
    
    /*
     * Create and populate the buiness card
     */
    mpi_errno = mpig_bc_create(&bc);
    MPIU_ERR_CHKANDJUMP((mpi_errno != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|bc_init");

    /*
     * Initialize the process group tracking subsystem
     */
    mpi_errno = mpig_pg_init();
    MPIU_ERR_CHKANDJUMP((mpi_errno != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**dev|pg_init");

    /*
     * Initialize Globus modules
     */
    globus_module_set_args(argc, argv);
    
    rc = globus_module_activate(GLOBUS_COMMON_MODULE);
    MPIU_ERR_CHKANDJUMP1((rc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|module_activate",
			 "**globus|module_activate %s", "common");

    /*
     * Initialize vendor MPI.  Get VMPI_COMM_WORLD, and the associated rank and size.  Add contact information to the business
     * card.
     */
#   if defined(MPIG_VMPI)
    {
	rc = mpig_vmpi_init(argc, argv);
	MPIU_ERR_CHKANDJUMP((rc != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|vmpi_init");

	/* XXX: Set the VMPI_COMM_WORLD error handler to VMPI_ERRORS_RETURN */
	
	rc = mpig_vmpi_comm_get_world(&mpig_process.vmpi_cw);
	MPIU_ERR_CHKANDJUMP((rc != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|vmpi_get_comm_world");
	
	rc = mpig_vmpi_comm_get_size(&mpig_process.vmpi_cw, &mpig_process.vmpi_cw_size);
	MPIU_ERR_CHKANDJUMP((rc != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|vmpi_get_comm_size");
	
	rc = mpig_vmpi_comm_get_rank(&mpig_process.vmpi_cw, &mpig_process.vmpi_cw_rank);
	MPIU_ERR_CHKANDJUMP((rc != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|vmpi_get_comm_rank");
    }
#   endif

    /*
     * Initialize the non-VMPI communication modules, and populate the business card with contact information.
     */
    mpi_errno = mpig_cm_self_init(argc, argv);
    MPIU_ERR_CHKANDJUMP1((mpi_errno != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|cm_init", "**globus|cm_init %s", "self");
    
    mpi_errno = mpig_cm_vmpi_init(argc, argv);
    MPIU_ERR_CHKANDJUMP1((mpi_errno != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|cm_init", "**globus|cm_init %s", "vmpi");
    
    mpi_errno = mpig_cm_xio_init(argc, argv);
    MPIU_ERR_CHKANDJUMP1((mpi_errno != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|cm_init", "**globus|cm_init %s", "xio");
    
    mpi_errno = mpig_cm_other_init(argc, argv);
    MPIU_ERR_CHKANDJUMP1((mpi_errno != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|cm_init", "**globus|cm_init %s", "other");
    
    mpi_errno = mpig_cm_self_add_contact_info(&bc);
    MPIU_ERR_CHKANDJUMP1((mpi_errno != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|cm_add_contact",
			 "**globus|cm_add_contact %s", "self");

    mpi_errno = mpig_cm_vmpi_add_contact_info(&bc);
    MPIU_ERR_CHKANDJUMP1((mpi_errno != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|cm_add_contact",
			 "**globus|cm_add_contact %s", "vmpi");

    mpi_errno = mpig_cm_xio_add_contact_info(&bc);
    MPIU_ERR_CHKANDJUMP1((mpi_errno != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|cm_add_contact",
			 "**globus|cm_add_contact %s", "xio");

    mpi_errno = mpig_cm_other_add_contact_info(&bc);
    MPIU_ERR_CHKANDJUMP1((mpi_errno != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|cm_add_contact",
			 "**globus|cm_add_contact %s", "other");

    /*
     * Initialize the process management module which interfaces with Globus.  Use it to exchange the businesses cards and obtian
     * information about the process group.
     */
    mpi_errno = mpig_pm_init();
    MPIU_ERR_CHKANDJUMP((mpi_errno != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|pm_init_failed");

    mpi_errno = mpig_pm_exchange_business_cards(&bc, &bcs);
    MPIU_ERR_CHKANDJUMP((mpi_errno != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|pm_xchg");

    mpig_pm_get_pg_id(&pg_id);
    mpig_pm_get_pg_size(&pg_size);
    mpig_pm_get_pg_rank(&pg_rank);
    
#   if defined(MPICH_DBG_OUTPUT)
    {
	MPIU_dbg_init(pg_rank);
    }
#   endif
    
    /*
     * Create a new structure to track the process group and the VCs associated with it.
     */
    mpi_errno = mpig_pg_create(pg_size, &pg);
    MPIU_ERR_CHKANDJUMP((mpi_errno != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**dev|pg_create");

    mpig_pg_id_set(pg, pg_id);

    /*
     * Stash away a pointer to my process group, and increment its reference count.  The count should be reduced again in
     * MPID_Finalize().
     */
    mpig_process.my_pg = pg;
    mpig_process.my_pg_id = pg->id;
    mpig_process.my_pg_size = pg_size;
    mpig_process.my_pg_rank = pg_rank;
    
    mpig_pg_add_ref(pg);

    /*
     * Select the protocol that will be used for each VC.
     */
    for (p = 0; p < pg_size; p++)
    {
	mpig_vc_t * vc;
	int flag;
	    
	{
	    char * bc_str;

	    mpig_bc_serialize_object(&bcs[p], &bc_str);
	    printf("[%d]: BC for process %d - %s\n", pg_rank, p, bc_str);
	    fflush(stdout);
	    mpig_bc_free_serialized_object(bc_str);
	}
	
	mpig_pg_get_vc(pg, p, &vc);
	
	mpi_errno = mpig_cm_self_select_module(&bcs[p], vc, &flag);
	MPIU_ERR_CHKANDJUMP3((mpi_errno != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|cm_select_module",
			     "**globus|cm_select_module %s %s %d", "self", pg_id, pg_rank);
	if (flag != FALSE) continue;

	mpi_errno = mpig_cm_vmpi_select_module(&bcs[p], vc, &flag);
	MPIU_ERR_CHKANDJUMP3((mpi_errno != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|cm_select_module",
			     "**globus|cm_select_module %s %s %d", "vmpi", pg_id, pg_rank);
	if (flag != FALSE) continue;

	mpi_errno = mpig_cm_xio_select_module(&bcs[p], vc, &flag);
	MPIU_ERR_CHKANDJUMP3((mpi_errno != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|cm_select_module",
			     "**globus|cm_select_module %s %s %d", "xio", pg_id, pg_rank);
	if (flag != FALSE) continue;

	MPIU_ERR_CHKANDJUMP2((flag == FALSE), mpi_errno, MPI_ERR_OTHER, "**globus|cm_no_module",
			     "**globus|cm_no_module %s %d", pg_id, pg_rank);
    }

    /*
     * Initialize the MPI_COMM_WORLD object
     */
    comm = MPIR_Process.comm_world;

    comm->rank = pg_rank;
    comm->remote_size = pg_size;
    comm->local_size = pg_size;
    
    mpi_errno = MPID_VCRT_Create(comm->remote_size, &comm->vcrt);
    MPIU_ERR_CHKANDJUMP1((mpi_errno != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**dev|vcrt_create",
			 "**dev|vcrt_create %s", "MPI_COMM_WORLD");
    
    mpi_errno = MPID_VCRT_Get_ptr(comm->vcrt, &comm->vcr);
    MPIU_ERR_CHKANDJUMP1((mpi_errno != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**dev|vcrt_get_ptr",
			 "**dev|vcrt_get_ptr %s", "MPI_COMM_WORLD");

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
    MPIU_ERR_CHKANDJUMP1((mpi_errno != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**dev|vcrt_create",
			 "**dev|vcrt_create %s", "MPI_COMM_SELF");
    
    mpi_errno = MPID_VCRT_Get_ptr(comm->vcrt, &comm->vcr);
    MPIU_ERR_CHKANDJUMP1((mpi_errno != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**dev|vcrt_get_ptr",
			 "**dev|vcrt_get_ptr %s", "MPI_COMM_SELF");
    
    MPID_VCR_Dup(&pg->vct[pg_rank], &comm->vcr[0]);
    
    /*
     * If this process group was spawned by a MPI application, then form the MPI_COMM_PARENT inter-communicator.
     */
#   if XXX
    {
	MPIU_Assert(has_parent == FALSE);
	/*
	 * - Get the (MPI) port of the parent
	 *
	 * - Perform a MPI_Comm_connect to the parent
	 *
	 * - Set MPIR_Process.comm_parent to the handle of the new inter-communicator
	 */
    }
#   endif
    
    /*
     * Set global (static) process attributes.
     */
    MPIR_Process.attrs.tag_ub = MPIG_TAG_UB;
    /* XXX: appnum should be the subjob number */
    MPIR_Process.attrs.appnum = 0;

    /*
     * Set provided thread level
     */
    if (provided != NULL)
    {
	*provided = MPICH_THREAD_LEVEL;
    }
    
  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_INIT);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPIR_Process.initialized = MPICH_PRE_INIT;

    /* XXX: track progress and clean up and allocated resources such as the process group and VCRTs */
    
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Init() */


/*
 * MPID_Finalize()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Finalize
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Finalize()
{
#if XXX    
    MPID_Progress_state progress_state;
#endif
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_FINALIZE);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_FINALIZE);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    /*
     * Wait for all posted receives to complete.
     *
     * We really should wait for all receives to complete on MPI_COMM_WORLD, but if we do then incorrectly written programs that
     * leave a receive outstanding will hang.  However, not waiting may result in posted "any source" receives erroneously
     * blowing up.  For now, we are placing a warning at the end of MPID_Finalize() to inform the user if any outstanding posted
     * receives exist.
     */
    
    /* FIXME: insert while loop here to wait for outstanding posted receives to complete */


#if XXX
    /*
     * Release MPI_COMM_WORLD and MPI_COMM_SELF
     */
    MPID_VCRT_release(MPIR_Process.comm_self->vcrt);
    MPID_VCRT_release(MPIR_Process.comm_world->vcrt);
    
    /*
     * Initiate close protocol for all active VCs
     */

    /*
     * Wait for all VCs to finish the close protocol
     */
    MPID_Progress_start(&progress_state);
    while(MPIDI_Outstanding_close_ops > 0)
    {
	mpi_errno = MPID_Progress_wait(&progress_state);
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
					     "**ch3|close_progress", 0);
	    break;
	}
    }
    MPID_Progress_end(&progress_state);

    if (mpig_process.warnings_enabled)
    {
	if (mpig_process.recvq_posted_head != NULL)
	{
	    /* XXX: insert code to print posted receive queue */
	    MPIU_Msg_printf("Warning: program exiting with outstanding receive requests\n");
	}
    }

#endif

    /*
     * Release our process group, and shutdown the process group management module.  NOTE: If an error occurs we add it to the
     * error stack, but continue on with the shutdown process.
     */
    {
	int inuse;
	
	mpig_pg_release_ref(mpig_process.my_pg, &inuse);
	if (inuse == 0)
	{
	    rc = mpig_pg_destroy(mpig_process.my_pg);
	    MPIU_ERR_CHKANDSTMT1((rc != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER,, "**dev|pg_destroy_failed",
				 "**dev|pg_destroy_failed %p", mpig_process.my_pg);
	}
    }
    mpig_process.my_pg = NULL;
    rc = mpig_pg_finalize();
    MPIU_ERR_CHKANDSTMT((rc != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER,, "**dev|pg_finalize");

    /*
     * Shutdown the process management module
     */
    mpi_errno = mpig_pm_finalize();
    
    /*
     * Shutdown communication (protocol) module(s)
     */
    rc = mpig_cm_other_finalize();
    MPIU_ERR_CHKANDSTMT1((rc != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, {;}, "**globus|cm_finalize",
			 "**globus|cm_finalize %s", "other");
    
    rc = mpig_cm_xio_finalize();
    MPIU_ERR_CHKANDSTMT1((rc != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, {;}, "**globus|cm_finalize",
			 "**globus|cm_finalize %s", "xio");
    
    rc = mpig_cm_vmpi_finalize();
    MPIU_ERR_CHKANDSTMT1((rc != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, {;}, "**globus|cm_finalize",
			 "**globus|cm_finalize %s", "vmpi");

    rc = mpig_cm_self_finalize();
    MPIU_ERR_CHKANDSTMT1((rc != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, {;}, "**globus|cm_finalize",
			 "**globus|cm_finalize %s", "self");

    /*
     * Deactivate Globus modules
     */
    rc = globus_module_deactivate(GLOBUS_COMMON_MODULE);
    MPIU_ERR_CHKANDJUMP1((rc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|module_deactivate",
			 "**globus|module_deactivate %s", "common");

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_FINALIZE);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Finalize() */


/*
 * MPID_Abort()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Abort
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Abort(MPID_Comm * comm, int mpi_errno, int exit_code, char * error_msg)
{
    int rank;
    MPIG_STATE_DECL(MPID_STATE_MPID_ABORT);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_ABORT);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    if (comm)
    {
	rank = comm->rank;
    }
    else
    {
	if (MPIR_Process.comm_world != NULL)
	{
	    rank = MPIR_Process.comm_world->rank;
	}
	else
	{
	    rank = -1;
	}
    }

    printf("[%s:%d:%d] %s\n\n", mpig_process.my_pg_id, mpig_process.my_pg_rank, 0, error_msg);
    fflush(stdout);
    exit(1);

    /*
     * TODO: contact GRAMs and cancel other subjobs, then cancel our own.  For now set an error stating that MPID_Abort() has not
     * been implemented.
     */
    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);
    
    /*
     * MPI-2: what do we do with a job that was spawned by communicator containing one or more processes in the communicator
     * being aborted.
     */

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_ABORT);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Abort() */


/*
 * MPID_Comm_spawn_multiple()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Comm_spawn_multiple
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Comm_spawn_multiple(int count, char * array_of_commands[], char ** array_of_argv[], int array_of_maxprocs[],
			     MPID_Info * array_of_info_ptrs[], int root, MPID_Comm * comm_ptr, MPID_Comm ** intercomm,
			     int array_of_errcodes[]) 
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_COMM_SPAWN_MULTIPLE);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_COMM_SPAWN_MULTIPLE);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);
    
  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_COMM_SPAWN_MULTIPLE);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Comm_spawn_multiple() */


/*
 * MPID_Get_processor_name()
 *
 * Returns the processor name.  Uses mpig_process.hostname, which is set in MPID_Init().
 */
#undef FUNCNAME
#define FUNCNAME MPID_Send
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Get_processor_name(char * name, int * resultlen)
{
    int len;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_GET_PROCESSOR_NAME);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_GET_PROCESSOR_NAME);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    len = (int) strlen(mpig_process.hostname);
    if (len > 0 && len < MPI_MAX_PROCESSOR_NAME)
    {
	MPIU_Strncpy(name, mpig_process.hostname, MPI_MAX_PROCESSOR_NAME);
	*resultlen = len;
    }
    else
    {
	mpi_errno = MPI_ERR_UNKNOWN;
	goto fn_fail;
    }

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_GET_PROCESSOR_NAME);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Get_processor_name() */


/*
 * MPID_Get_universe_size()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Get_universe_size
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Get_universe_size(int  * universe_size)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_GET_UNIVERSE_SIZE);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_GET_UNIVERSE_SIZE);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

#if XXX    
    *universe_size = MPIR_UNIVERSE_SIZE_NOT_AVAILABLE;
#endif
    
    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);
    
  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_GET_UNIVERSE_SIZE);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    *universe_size = MPIR_UNIVERSE_SIZE_NOT_AVAILABLE;
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Get_universe_size() */


/*
 * MPID_Parse_option()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Parse_option
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Parse_option(int num_args, char * args[], int * num_parsed, MPI_Info * info)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_PARSE_OPTION);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_PARSE_OPTION);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_PARSE_OPTION);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Parse_option() */
