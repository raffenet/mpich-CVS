/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpi_init.h"
#ifdef HAVE_CRTDBG_H
#include <crtdbg.h>
#endif

/* -- Begin Profiling Symbol Block for routine MPI_Init_thread */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Init_thread = PMPI_Init_thread
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Init_thread  MPI_Init_thread
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Init_thread as PMPI_Init_thread
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Init_thread PMPI_Init_thread

/* Any internal routines can go here.  Make them static if possible */
MPICH_PerProcess_t MPIR_Process = { MPICH_PRE_INIT }; /* all others are irelevant */

#ifdef HAVE_WINDOWS_H
/* User-defined abort hook function.  Exiting here will prevent the system from
 * bringing up an error dialog box.
 */
static int assert_hook( int reportType, char *message, int *returnValue )
{
    fprintf(stderr, "%s", message);
    if (returnValue != NULL)
	exit(*returnValue);
    ExitProcess(-1);
    return TRUE;
}
#endif

#ifdef MPICH_SINGLE_THREADED
/* If single threaded, we preallocate this.  Otherwise, we create it */
MPICH_PerThread_t  MPIR_Thread = { 0 };
#endif

int MPIR_Init_thread(int * argc, char ***argv, int required,
		     int * provided)
{
    int mpi_errno = MPI_SUCCESS;
    int has_args;
    int has_env;
    int thread_provided;
#if defined(HAVE_WINDOWS_H) && defined(_WIN64)
    UINT mode, old_mode;
#endif

#ifdef HAVE_WINDOWS_H
    /* prevent the process from bringing up an error message window if mpich asserts */
    _CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_FILE );
    _CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDERR );
    _CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, assert_hook);
#ifdef _WIN64
    /* FIXME: This severly degrades performance but fixes alignment issues with the datatype code. */
    /* Prevent misaligned faults on Win64 machines */
    old_mode = SetErrorMode(SEM_NOALIGNMENTFAULTEXCEPT);
    mode = old_mode | SEM_NOALIGNMENTFAULTEXCEPT;
    SetErrorMode(mode);
#endif
#endif

#   if (USE_THREAD_IMPL == MPICH_THREAD_IMPL_GLOBAL_MUTEX)
    {
	/*
	 * FIXME: this is for temporary testing purposes only and will be replaced with a suitable abstraction once initial
	 * testing is complete.
	 */
	#if (USE_THREAD_PKG == MPICH_THREAD_PKG_POSIX)
	{
	    pthread_mutex_init(&MPIR_Process.global_mutex, NULL);
	}
#	else
#	    error specified thread package is not supported
#	endif
    }
#   endif
    
#   if !defined(MPICH_SINGLE_THREADED)
    {
	MPID_Thread_key_create(&MPIR_Process.thread_key) ;
	MPIR_Process.master_thread = MPID_Thread_get_id();
	MPID_Thread_lock_init(&MPIR_Process.common_lock);
	MPID_Thread_lock_init(&MPIR_Process.allocation_lock);
    }
#   endif    

#ifdef HAVE_ERROR_CHECKING
    /* Eventually this will support commandline and environment options
     for controlling error checks.  It will use the routine 
     MPIR_Err_init, which does as little as possible (e.g., it only 
     determines the value of do_error_checks) */
    MPIR_Process.do_error_checks = 1;
#else
    MPIR_Process.do_error_checks = 0;
#endif

    /* Initialize necessary subsystems and setup the predefined attribute
       values.  Subsystems may change these values. */
    MPIR_Process.attrs.appnum          = 0;
    MPIR_Process.attrs.host            = 0;
    MPIR_Process.attrs.io              = 0;
    MPIR_Process.attrs.lastusedcode    = MPI_ERR_LASTCODE;
    MPIR_Process.attrs.tag_ub          = 0;
    MPIR_Process.attrs.universe        = -1;
    MPIR_Process.attrs.wtime_is_global = 0;

    /* Set the functions used to duplicate attributes.  These are 
       when the first corresponding keyval is created */
    MPIR_Process.attr_dup  = 0;
    MPIR_Process.attr_free = 0;

#ifdef HAVE_CXX_BINDING
    /* Set the functions used to call functions in the C++ binding 
       for reductions and attribute operations.  These are null
       until a C++ operation is defined.  This allows the C code
       that implements these operations to not invoke a C++ code
       directly, which may force the inclusion of symbols known only
       to the C++ compiler (e.g., under more non-GNU compilers, including
       Solaris and IRIX). */
    MPIR_Process.cxx_call_op_fn = 0;
    MPIR_Process.cxx_call_delfn = 0;

#endif
    /* "Allocate" from the reserved space for builtin communicators and
       (partially) initialize predefined communicators.  comm_parent is
       intially NULL and will be allocated by the device if the process group
       was started using one of the MPI_Comm_spawn functions. */
    MPIR_Process.comm_world		  = MPID_Comm_builtin + 0;
    MPIR_Process.comm_world->handle	  = MPI_COMM_WORLD;
    MPIU_Object_set_ref( MPIR_Process.comm_world, 1 );
    MPIR_Process.comm_world->context_id	  = 0; /* XXX */
    MPIR_Process.comm_world->attributes	  = NULL;
    MPIR_Process.comm_world->local_group  = NULL;
    MPIR_Process.comm_world->remote_group = NULL;
    MPIR_Process.comm_world->comm_kind	  = MPID_INTRACOMM;
    /* This initialization of the comm name could be done only when 
       comm_get_name is called */
    MPIU_Strncpy(MPIR_Process.comm_world->name, "MPI_COMM_WORLD",
		 MPI_MAX_OBJECT_NAME);
    MPIR_Process.comm_world->errhandler	  = NULL; /* XXX */
    MPIR_Process.comm_world->coll_fns	  = NULL; /* XXX */
    
    MPIR_Process.comm_self		 = MPID_Comm_builtin + 1;
    MPIR_Process.comm_self->handle	 = MPI_COMM_SELF;
    MPIU_Object_set_ref( MPIR_Process.comm_self, 1 );
    MPIR_Process.comm_self->context_id	 = 4; /* XXX */
    MPIR_Process.comm_self->attributes	 = NULL;
    MPIR_Process.comm_self->local_group	 = NULL;
    MPIR_Process.comm_self->remote_group = NULL;
    MPIR_Process.comm_self->comm_kind	 = MPID_INTRACOMM;
    MPIU_Strncpy(MPIR_Process.comm_self->name, "MPI_COMM_SELF",
		 MPI_MAX_OBJECT_NAME);
    MPIR_Process.comm_self->errhandler	 = NULL; /* XXX */
    MPIR_Process.comm_self->coll_fns	 = NULL; /* XXX */

    MPIR_Process.comm_parent = NULL;

    /* Call any and all MPID_Init type functions */
    /* FIXME: The call to err init should be within an ifdef
       HAVE_ ERROR_CHECKING block (as must all uses of Err_create_code) */
    MPIR_Err_init();
    MPID_Wtime_init();
    /* MPIU_Timer_pre_init(); */
    mpi_errno = MPID_Init(argc, argv, required, &thread_provided, 
			  &has_args, &has_env);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, "MPIR_Init_thread", __LINE__, MPI_ERR_OTHER, "**init", 0);
	/* FIXME: the default behavior for all MPI routines is to abort.  
	   This isn't always convenient, because there's no other way to 
	   get this routine to simply return.  But we should provide some
	   sort of control for that and follow the default defined 
	   by the standard */
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    /* Capture the level of thread support provided */
    MPIR_Process.thread_provided = thread_provided;
    if (provided) *provided = thread_provided;

    MPIU_dbg_init(MPIR_Process.comm_world->rank);
    MPIU_Timer_init(MPIR_Process.comm_world->rank,
		    MPIR_Process.comm_world->local_size);
#ifdef USE_MEMORY_TRACING
    MPIU_trinit( MPIR_Process.comm_world->rank );
#endif

#ifdef HAVE_FORTRAN_BINDING
    /* Initialize Fortran special names (MPI_BOTTOM and STATUS_IGNOREs)
       We must do this here because the MPI standard requires that 
       all languages be initialized by MPI_Init/MPI_Init_thread in any
       language */
#if defined(F77_NAME_LOWER_USCORE) || defined(F77_NAME_LOWER_2USCORE)
    mpirinitf_();
#elif defined(F77_NAME_UPPER)
    MPIRINITF();
#else
    mpirinitf();
#endif
#endif

    MPIR_Process.initialized = MPICH_WITHIN_MPI;

#ifdef HAVE_DEBUGGER_SUPPORT
    MPIR_WaitForDebugger();
#endif    
    return mpi_errno;
}
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Init_thread

/*@
   MPI_Init_thread - Initialize the MPI execution environment

   Input Parameters:
+  argc - Pointer to the number of arguments 
.  argv - Pointer to the argument vector
-  required - Level of desired thread support

   Output Parameter:
.  provided - Level of provided thread support

   Command line arguments:
   MPI specifies no command-line arguments but does allow an MPI 
   implementation to make use of them.  See 'MPI_INIT' for a description of 
   the command line arguments supported by 'MPI_INIT' and 'MPI_INIT_THREAD'.

   Notes:
   The valid values for the level of thread support are\:
+ MPI_THREAD_SINGLE - Only one thread will execute. 
. MPI_THREAD_FUNNELED - The process may be multi-threaded, but only the main 
  thread will make MPI calls (all MPI calls are funneled to the 
   main thread). 
. MPI_THREAD_SERIALIZED - The process may be multi-threaded, and multiple 
  threads may make MPI calls, but only one at a time: MPI calls are not 
  made concurrently from two distinct threads (all MPI calls are serialized). 
- MPI_THREAD_MULTIPLE - Multiple threads may call MPI, with no restrictions. 

Notes for Fortran:
   Note that the Fortran binding for this routine does not have the 'argc' and
   'argv' arguments. ('MPI_INIT_THREAD(required, provided, ierror)')


.N Errors
.N MPI_SUCCESS
.N MPI_ERR_OTHER

.seealso: MPI_Init, MPI_Finalize
@*/
int MPI_Init_thread( int *argc, char ***argv, int required, int *provided )
{
    static const char FCNAME[] = "MPI_Init_thread";
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_INIT_THREAD);

    MPID_MPI_INIT_FUNC_ENTER(MPID_STATE_MPI_INIT_THREAD);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (MPIR_Process.initialized != MPICH_PRE_INIT) {
                mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, "MPI_Init_thread", __LINE__, MPI_ERR_OTHER,
						  "**inittwice", 0 );
	    }
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    mpi_errno = MPIR_Init_thread( argc, argv, required, provided );

    if (mpi_errno == MPI_SUCCESS)
    {
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INIT_THREAD);
	return MPI_SUCCESS;
    }
    
fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_HANDLING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE,
				     FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_init_thread", "**mpi_init_thread %p %p %d %p", argc, argv, required, provided);
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INIT_THREAD);
    return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    /* --END ERROR HANDLING-- */
}
