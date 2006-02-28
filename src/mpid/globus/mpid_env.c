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

mpig_process_t mpig_process = {NULL, "{null)", -1, -1, -1, -1};

/*
 * MPID_Init()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Init
int MPID_Init(int * argc, char *** argv, int requested, int * provided, int * has_args, int * has_env)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_bc_t bc;
    mpig_bc_t * bcs = NULL;
    mpig_pg_t * pg = NULL;
    const char * pg_id = NULL;
    bool_t pg_locked = FALSE;
    int pg_rank;
    int pg_size;
    MPID_Comm * comm;
    int p;
    int rc;
    bool_t failed;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_INIT);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_INIT);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3, "entering: requested=%d", requested));
    
    /* initialize the device's process information structure */
    mpig_process_mutex_create();
    mpig_process_rc_acq(FALSE);
    {
	/* get the operating system's process id for the local process */
	mpig_process.my_pid = getpid();

	/* get the name of the machine on which the local process is running */
#       if defined(HAVE_GETHOSTNAME)
	{
	    if(gethostname(mpig_process.my_hostname, MPIG_PROCESSOR_NAME_SIZE) != 0)
	    {
		mpig_process.my_hostname[0] = '\0';
	    }
	}
#       else
	{
	    mpig_process.my_hostname[0] = '\0';
	}
#       endif
    }
    mpig_process_rc_rel(TRUE);
    

    /* activate globus modules */
    globus_module_set_args(argc, argv);
    
    rc = globus_module_activate(GLOBUS_COMMON_MODULE);
    MPIU_ERR_CHKANDJUMP1((rc), mpi_errno, MPI_ERR_OTHER, "**globus|module_activate", "**globus|module_activate %s", "common");

    /* initialize the request allocator module */
    mpig_request_alloc_init();
    
    /* initialize the receive queue */
    mpig_recvq_init(&mpi_errno, &failed);
    MPIU_ERR_CHKANDJUMP((failed), mpi_errno, MPI_ERR_OTHER, "**globus|recvq_init");
    
    /* create and populate the buiness card */
    mpig_bc_create(&bc, &mpi_errno, &failed);
    MPIU_ERR_CHKANDJUMP((failed), mpi_errno, MPI_ERR_OTHER, "**globus|bc_init");

    /* initialize the process group tracking subsystem */
    mpig_pg_init(&mpi_errno, &failed);
    MPIU_ERR_CHKANDJUMP((failed), mpi_errno, MPI_ERR_OTHER, "**dev|pg_init");

    /* initialize the communication modules, and populate the business card with contact information */
    mpi_errno = mpig_cm_self_init(argc, argv);
    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_init", "**globus|cm_init %s", "self");
    
    mpi_errno = mpig_cm_vmpi_init(argc, argv);
    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_init", "**globus|cm_init %s", "vmpi");
    
    mpi_errno = mpig_cm_xio_init(argc, argv);
    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_init", "**globus|cm_init %s", "xio");
    
    mpi_errno = mpig_cm_other_init(argc, argv);
    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_init", "**globus|cm_init %s", "other");
    
    mpi_errno = mpig_cm_self_add_contact_info(&bc);
    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_add_contact", "**globus|cm_add_contact %s", "self");

    mpi_errno = mpig_cm_vmpi_add_contact_info(&bc);
    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_add_contact", "**globus|cm_add_contact %s", "vmpi");

    mpi_errno = mpig_cm_xio_add_contact_info(&bc);
    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_add_contact", "**globus|cm_add_contact %s", "xio");

    mpi_errno = mpig_cm_other_add_contact_info(&bc);
    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_add_contact", "**globus|cm_add_contact %s", "other");

    /*initialize the process management module which interfaces with Globus.  use it to exchange the businesses cards and obtian
      information about the process group. */
    mpi_errno = mpig_pm_init();
    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|pm_init");

    mpi_errno = mpig_pm_exchange_business_cards(&bc, &bcs);
    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|pm_xchg");

    mpig_pm_get_pg_id(&pg_id);
    mpig_pm_get_pg_size(&pg_size);
    mpig_pm_get_pg_rank(&pg_rank);

    /* place a copy of the process group information in the process structure */
    mpig_process.my_pg_id = MPIU_Strdup(pg_id);
    mpig_process.my_pg_size = pg_size;
    mpig_process.my_pg_rank = pg_rank;
    
    /* now that we know the process group rank, initialize the debugging output module */
#   if defined(MPIG_DEBUG)
    {
	mpig_debug_init();
    }
#   endif

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ADI3, "pid=%lu", (unsigned long) mpig_process.my_pid));
    
    /* acquire, creating if necessary, a reference to the process group object used to manage the virtual connection objects.  if
       creation was necessary, the virtual connection objects within the PG will be initialized at this time. */
    mpig_pg_acquire_ref_locked(pg_id, pg_size, &pg, &mpi_errno, &failed);
    MPIU_ERR_CHKANDJUMP((failed), mpi_errno, MPI_ERR_OTHER, "**globus|pg_acquire_ref");
    pg_locked = TRUE;
    {    
	mpig_vc_t * vc;
	bool_t vc_was_in_use;

	/* stash away a pointer to my process group.  the PG reference count was already incremented by
	   mpig_pg_acquire_ref_and_lock().  it should be reduced again in MPID_Finalize() using mpig_pg_release(). */
	mpig_process.my_pg = pg;
	
	/* select the protocol that will be used for each VC */
	for (p = 0; p < pg_size; p++)
	{
	    bool_t selected;
	
#           if FALSE && defined(MPIG_DEBUG)
	    {
		char * bc_str;

		mpig_bc_serialize_object(&bcs[p], &bc_str, &mpi_errno, &failed);
		mpig_debug_printf(MPIG_DEBUG_LEVEL_BC, "BC for process %s:%d - %s\n", pg_id, p, bc_str);
		mpig_bc_free_serialized_object(bc_str);
	    }
#           endif

	    mpig_pg_get_vc(pg, p, &vc);

	    /*
	     * NOTE: since no barrier exists in the process management interface, some communication modules could be susceptible
	     * to receiving connection requests from remote processes before the VCs have been initialized.  for such
	     * communication modules, the cm->select_module() routine must take care not to blindly reinitialize internal fields
	     * of the VC, causing the existing connection to be corrupted or lost.
	     */
	    mpig_vc_mutex_lock(vc);
	    {
		mpi_errno = mpig_cm_self_select_module(&bcs[p], vc, &selected);
		MPIU_ERR_CHKANDSTMT3((mpi_errno), mpi_errno, MPI_ERR_OTHER, {goto vc_unlock;}, "**globus|cm_select_module",
				     "**globus|cm_select_module %s %s %d", "self", pg_id, pg_rank);
		if (selected) goto vc_unlock;
		
		mpi_errno = mpig_cm_vmpi_select_module(&bcs[p], vc, &selected);
		MPIU_ERR_CHKANDSTMT3((mpi_errno), mpi_errno, MPI_ERR_OTHER, {goto vc_unlock;}, "**globus|cm_select_module",
				     "**globus|cm_select_module %s %s %d", "vmpi", pg_id, pg_rank);
		if (selected) goto vc_unlock;
		
		mpi_errno = mpig_cm_xio_select_module(&bcs[p], vc, &selected);
		MPIU_ERR_CHKANDSTMT3((mpi_errno), mpi_errno, MPI_ERR_OTHER, {goto vc_unlock;}, "**globus|cm_select_module",
				     "**globus|cm_select_module %s %s %d", "xio", pg_id, pg_rank);
		if (selected) goto vc_unlock;
		
		MPIU_ERR_CHKANDSTMT2((!selected), mpi_errno, MPI_ERR_OTHER, {goto vc_unlock;}, "**globus|cm_no_module",
				     "**globus|cm_no_module %s %d", pg_id, pg_rank);

	      vc_unlock: ;
	    }
	    mpig_vc_mutex_unlock(vc);

	    if (mpi_errno) goto fn_fail;
	}

	/* initialize the MPI_COMM_WORLD object */
	comm = MPIR_Process.comm_world;

	mpig_comm_construct(comm);
	comm->rank = pg_rank;
	comm->remote_size = pg_size;
	comm->local_size = pg_size;
    
	mpi_errno = MPID_VCRT_Create(comm->remote_size, &comm->vcrt);
	MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**dev|vcrt_create",
			     "**dev|vcrt_create %s", "MPI_COMM_WORLD");
    
	mpi_errno = MPID_VCRT_Get_ptr(comm->vcrt, &comm->vcr);
	MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**dev|vcrt_get_ptr",
			     "**dev|vcrt_get_ptr %s", "MPI_COMM_WORLD");

	for (p = 0; p < pg_size; p++)
	{
	    mpig_pg_get_vc(pg, p, &vc);
	    mpig_comm_set_vc(comm, p, vc);
	    mpig_vc_mutex_lock(vc);
	    {
		mpig_vc_inc_ref_count(vc, &vc_was_in_use, &mpi_errno, &failed);
	    }
	    mpig_vc_mutex_unlock(vc);
	}

	mpig_comm_list_add(comm);
	
	/* initialize the MPI_COMM_SELF object */
	comm = MPIR_Process.comm_self;

	mpig_comm_construct(comm);
	comm->rank = 0;
	comm->remote_size = 1;
	comm->local_size = 1;
	
	mpi_errno = MPID_VCRT_Create(comm->remote_size, &comm->vcrt);
	MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**dev|vcrt_create",
			     "**dev|vcrt_create %s", "MPI_COMM_SELF");
    
	mpi_errno = MPID_VCRT_Get_ptr(comm->vcrt, &comm->vcr);
	MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**dev|vcrt_get_ptr",
			     "**dev|vcrt_get_ptr %s", "MPI_COMM_SELF");
    
	mpig_pg_get_vc(pg, pg_rank, &vc);
	mpig_comm_set_vc(comm, 0, vc);
	MPID_VCR_Dup(MPIR_Process.comm_world->vcr[pg_rank], &comm->vcr[0]);

	mpig_comm_list_add(comm);
    }
    mpig_pg_mutex_unlock(pg);
    pg_locked = FALSE;

    /* if this process group was spawned by a MPI job, then form the MPI_COMM_PARENT inter-communicator */
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
    
    /* set global (static) process attributes */
    MPIR_Process.attrs.tag_ub = MPIG_TAG_UB;
    /* XXX: appnum should be the subjob number */
    MPIR_Process.attrs.appnum = 0;

    /* set provided thread level */
    if (provided != NULL)
    {
	*provided = MPI_THREAD_FUNNELED;
    }

    /* indicate if the process management system has made the arguments or environment variables available */
    if (has_args != NULL)
    {
	*has_args = TRUE;
    }
    if (has_env != NULL)
    {
	*has_env = TRUE;
    }

  fn_return:
    /* mark the process group as committed, indicating that it may safely be destroyed if mpig_pg_release_ref() is called */
    if (pg != NULL)
    {
	mpig_pg_commit(pg);
    }
	
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3, "exiting: provided=%d, has_args=%s, has_env=%s, "
		       "mpi_errno=0x%08x", MPI_THREAD_FUNNELED, "true", "true", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_INIT);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	MPIR_Process.initialized = MPICH_PRE_INIT;

	/* clean up the process group structure */
	if (pg != NULL)
	{
	    mpig_pg_mutex_unlock_conditional(pg, pg_locked);
	    mpig_pg_release_ref(pg);
	}
    
	/* XXX: track progress and clean up and allocated resources such as the process group and VCRTs */

	mpig_request_alloc_finalize();
	
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* MPID_Init() */


/*
 * MPID_Finalize()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Finalize
int MPID_Finalize()
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int rc;
    bool_t failed;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_FINALIZE);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_FINALIZE);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3, "entering"));

    /* detach any buffer the application may have attach to insure that all buffered sends have completed before we proceed with
       shutdown.  MPI_Finalize() should really be doing this, but right now it does it *after* calling MPID_Finalize which is
       problematic to say the least.  it's unfortunate that we have to do this since this unnecessarily links the bsend code into
       the application even if the application doesn't use it (which most don't). */
    {
	void * prev_buf_ptr;
	int prev_buf_size;

	MPIR_Nest_incr();
	NMPI_Buffer_detach(&prev_buf_ptr, &prev_buf_size);
    }
    
    /*
     * wait for all posted operations to complete on all communications
     *
     * NOTE: applications that leave receive operation(s) unsatisfied will hang!  such an application is erroneous, and can be
     * corrected by canceling any such operations before calling MPI_Finalize().
     */
    mpig_comm_list_wait_empty(&mpi_errno, &failed);
    MPIU_ERR_CHKANDJUMP1((failed), mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %s",
			 "failed waiting for posted operations to complete");
    
    /* release the virtual connection reference tables for MPI_COMM_WORLD and MPI_COMM_SELF */
    MPID_VCRT_Release(MPIR_Process.comm_world->vcrt);
    MPID_VCRT_Release(MPIR_Process.comm_self->vcrt);
    mpig_comm_destruct(MPIR_Process.comm_world);
    mpig_comm_destruct(MPIR_Process.comm_self);
    MPIR_Process.comm_world->vcrt = NULL;
    MPIR_Process.comm_self->vcrt = NULL;
    
    /* shutdown the communication modules.  each module is responsible for insuring that any VCs managed by it are disconnected
       before returning. */
    rc = mpig_cm_other_finalize();
    MPIU_ERR_CHKANDSTMT1((rc), mpi_errno, MPI_ERR_OTHER, {;}, "**globus|cm_finalize", "**globus|cm_finalize %s", "other");
    
    rc = mpig_cm_xio_finalize();
    MPIU_ERR_CHKANDSTMT1((rc), mpi_errno, MPI_ERR_OTHER, {;}, "**globus|cm_finalize", "**globus|cm_finalize %s", "xio");
    
    rc = mpig_cm_vmpi_finalize();
    MPIU_ERR_CHKANDSTMT1((rc), mpi_errno, MPI_ERR_OTHER, {;}, "**globus|cm_finalize", "**globus|cm_finalize %s", "vmpi");

    rc = mpig_cm_self_finalize();
    MPIU_ERR_CHKANDSTMT1((rc), mpi_errno, MPI_ERR_OTHER, {;}, "**globus|cm_finalize", "**globus|cm_finalize %s", "self");

    /* release the reference to the process group associated with MPI_COMM_WORLD */
    mpig_pg_release_ref(mpig_process.my_pg);
    mpig_process.my_pg = NULL;
    
    /* shutdown the process group management module */
    mpig_pg_finalize(&mpi_errno, &failed);
    MPIU_ERR_CHKANDSTMT((failed), mpi_errno, MPI_ERR_OTHER, {;}, "**dev|pg_finalize");

    /* shutdown the process management module */
    rc = mpig_pm_finalize();
    MPIU_ERR_CHKANDSTMT((rc), mpi_errno, MPI_ERR_OTHER, {;}, "**globus|pm_finalize")
    
	/* deactivate globus modules */
	rc = globus_module_deactivate(GLOBUS_COMMON_MODULE);
    MPIU_ERR_CHKANDJUMP1((rc), mpi_errno, MPI_ERR_OTHER, "**globus|module_deactivate", "**globus|module_deactivate %s", "common");

    /* shutdown the receive queue module */
    mpig_recvq_finalize(&mpi_errno, &failed);
    MPIU_ERR_CHKANDSTMT((failed), mpi_errno, MPI_ERR_OTHER, {;}, "**globus|recvq_finalize");
    
    /* shutdown the request allocator module */
    mpig_request_alloc_finalize();
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3, "exiting: mpi_errno=0x%08x", mpi_errno));
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
int MPID_Abort(MPID_Comm * const comm, const int mpi_errno, const int exit_code, const char * const error_msg)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_MPID_ABORT);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_ABORT);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3, "entering: comm=" MPIG_PTR_FMT ",mpi_errno=0x%08x, "
		       "exit_code=%d, error_msg=%s", (MPIG_PTR_CAST) comm, mpi_errno, exit_code, MPIG_STR_VAL(error_msg)));

    if (mpi_errno)
    {
	char * str;
	
	str = (char *) MPIU_Malloc(MPIG_ERR_STRING_SIZE);
	if (str)
	{
	    MPIR_Err_print_stack_string(mpi_errno, str, MPIG_ERR_STRING_SIZE);
	    fprintf(stderr, "[%s:%d:%lu] %s", mpig_process.my_pg_id, mpig_process.my_pg_rank, mpig_thread_get_id(), str);
	    MPIU_Free(str);
	}
    }
    if (error_msg != NULL)
    {
	fflush(stdout);
	fprintf(stderr, "[%s:%d:%lu] %s\n", mpig_process.my_pg_id, mpig_process.my_pg_rank, mpig_thread_get_id(), error_msg);
    }

    /* XXX: contact GRAMs and cancel other subjobs, then cancel our own */
    
    /* XXX: MPI-2: what do we do with a job that was spawned by communicator containing one or more processes in the communicator
     * being aborted? */

    /* XXX: in an ideal universe, we would like a core file for the process initiating the abort, but there is a race condition
       between GRAM killing the process and the process reaching the call to the abort() function.  I'm not sure how to resolve
       this.  Perhaps there is a way to tell GRAM to send a SIGABRT to the job(s). */
    abort();
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_ABORT);
    return mpi_errno;
}
/* MPID_Abort() */


/*
 * MPID_Comm_spawn_multiple()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Comm_spawn_multiple
int MPID_Comm_spawn_multiple(int count, char * array_of_commands[], char ** array_of_argv[], int array_of_maxprocs[],
			     MPID_Info * array_of_info_ptrs[], int root, MPID_Comm * comm_ptr, MPID_Comm ** intercomm,
			     int array_of_errcodes[]) 
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_COMM_SPAWN_MULTIPLE);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_COMM_SPAWN_MULTIPLE);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DYNAMIC, "entering"));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", fcname);
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DYNAMIC, "exiting"));
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
 * Returns the processor name.  Uses mpig_process.my_hostname, which is set in MPID_Init().
 */
#undef FUNCNAME
#define FUNCNAME MPID_Get_processor_name
int MPID_Get_processor_name(char * name, int * resultlen)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int len;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_GET_PROCESSOR_NAME);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_GET_PROCESSOR_NAME);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3, "entering"));

    len = (int) strlen(mpig_process.my_hostname);
    if (len > 0 && len < MPI_MAX_PROCESSOR_NAME)
    {
	MPIU_Strncpy(name, mpig_process.my_hostname, MPI_MAX_PROCESSOR_NAME);
	*resultlen = len;
    }
    else
    {
	mpi_errno = MPI_ERR_UNKNOWN;
	goto fn_fail;
    }

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3, "exiting"));
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
int MPID_Get_universe_size(int  * universe_size)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_GET_UNIVERSE_SIZE);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_GET_UNIVERSE_SIZE);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3, "entering"));

    /* FIXME: someday we might want to allow the user to set an environment variable specifying the universe size, or have
       mpiexec compute it from a known hosts file, but for now set the size to "unavailable". */
    *universe_size = MPIR_UNIVERSE_SIZE_NOT_AVAILABLE;
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_GET_UNIVERSE_SIZE);
    return mpi_errno;

#if 0    
  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	*universe_size = MPIR_UNIVERSE_SIZE_NOT_AVAILABLE;
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
#endif
}
/* MPID_Get_universe_size() */


/*
 * MPID_GPID_Get([IN] comm, [IN] rank, [OUT] gpid[2])
 *
 * FIXME: THIS IS NOT RIGHT FOR MPI-2 FUNCTIONALITY!!!
 */
#undef FUNCNAME
#define FUNCNAME MPID_GPID_Get
int MPID_GPID_Get(MPID_Comm * comm, int rank, int gpid[])
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vc_t * vc;
    int mpi_errno = MPI_SUCCESS;

    MPIG_UNUSED_VAR(fcname);

    mpig_comm_get_vc(comm, rank, &vc);
    gpid[0] = 0;
    gpid[1] = vc->pg_rank;

    return mpi_errno;
}
/* MPID_GPID_Get() */
