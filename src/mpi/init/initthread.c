/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpi_init.h"

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
#ifdef MPICH_SINGLE_THREADED
/* If single threaded, we preallocate this.  Otherwise, we create it */
MPICH_PerThread_t  MPIR_Thread;
#endif
int MPIR_Init_thread(int * argc, char ***argv, int required,
		     int * provided)
{
    int mpi_errno = MPI_SUCCESS;
    int has_args;
    int has_env;

    /* XXX - if we pass provided to MPID_Init() then the following code has
       little effect, and may in fact, allow the setting in MPIR_Process to be
       out of sync with the value returned from MPID_Init() */
    if (required > MPID_MAX_THREAD_LEVEL) {
	MPIR_Process.thread_provided = MPID_MAX_THREAD_LEVEL;
    }
    else {
	MPIR_Process.thread_provided = required;
    }
    if (provided) *provided =  MPIR_Process.thread_provided;

#ifdef MPICH_SINGLE_THREADED
#else
    MPIR_Process.thread_key = MPID_GetThreadKey();
    MPIR_Process.master_thread = MPID_GetThreadId();
    MPIR_Thread_lock_init( MPIR_Process.common_lock );
    MPIR_Thread_lock_init( MPIR_Process.allocation_lock );
#endif    
    /* Remember level of thread support promised */

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
    MPIR_Process.attrs.lastusedcode    = 0;
    MPIR_Process.attrs.tag_ub          = 0;
    MPIR_Process.attrs.universe        = 1;
    MPIR_Process.attrs.wtime_is_global = 0;

    /* Set the functions used to duplicate attributes.  These are 
       when the first corresponding keyval is created */
    MPIR_Process.comm_attr_dup = 0;
    MPIR_Process.type_attr_dup = 0;

    /* "Allocate" from the reserved space for builtin communicators and
       (partially) initialize predefined communicators.  comm_parent is
       intially NULL and will be allocated by the device if the process group
       was started using one of the MPI_Comm_spawn functions. */
    MPIR_Process.comm_world = MPID_Comm_builtin + 0;
    MPIR_Process.comm_world->handle = MPI_COMM_WORLD;
    MPIU_Object_set_ref( MPIR_Process.comm_world, 1 );
    MPIR_Process.comm_world->context_id = 0; /* XXX */
    MPIR_Process.comm_world->attributes = NULL;
    MPIR_Process.comm_world->local_group = NULL;
    MPIR_Process.comm_world->remote_group = NULL;
    MPIR_Process.comm_world->comm_kind = MPID_INTRACOMM;
    /* This initialization of the comm name could be done only when 
       comm_get_name is called */
    MPIU_Strncpy(MPIR_Process.comm_world->name, "MPI_COMM_WORLD",
		 MPI_MAX_OBJECT_NAME);
    MPIR_Process.comm_world->errhandler = NULL; /* XXX */
    MPIR_Process.comm_world->coll_fns = NULL; /* XXX */
    
    MPIR_Process.comm_self = MPID_Comm_builtin + 1;
    MPIR_Process.comm_self->handle = MPI_COMM_SELF;
    MPIU_Object_set_ref( MPIR_Process.comm_self, 1 );
    MPIR_Process.comm_self->context_id = 4; /* XXX */
    MPIR_Process.comm_self->attributes = NULL;
    MPIR_Process.comm_self->local_group = NULL;
    MPIR_Process.comm_self->remote_group = NULL;
    MPIR_Process.comm_self->comm_kind = MPID_INTRACOMM;
    MPIU_Strncpy(MPIR_Process.comm_self->name, "MPI_COMM_SELF",
		 MPI_MAX_OBJECT_NAME);
    MPIR_Process.comm_self->errhandler = NULL; /* XXX */
    MPIR_Process.comm_self->coll_fns = NULL; /* XXX */

    MPIR_Process.comm_parent = NULL;

    /* Call any and all MPID_Init type functions */
    MPID_Wtime_init();
    /*MPIU_Timer_pre_init();*/
    mpi_errno = MPID_Init(argc, argv, required, provided, &has_args, &has_env);
    MPIU_Timer_init(MPIR_Process.comm_world->rank,
		    MPIR_Process.comm_world->local_size);

    MPIR_Process.initialized = MPICH_WITHIN_MPI;

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
   Note that the Fortran binding for this routine does not have the 'argc' and
   'argv' arguments. ('MPI_INIT_THREAD(required, provided, ierror)')

.N Errors
.N MPI_SUCCESS
.N ... others
@*/
int MPI_Init_thread( int *argc, char ***argv, int required, int *provided )
{
    static const char FCNAME[] = "MPI_Init_thread";
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_INIT_THREAD);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_INIT_THREAD);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (MPIR_Process.initialized != MPICH_PRE_INIT) {
                mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER,
                            "**inittwice", 0 );
	    }
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INIT_THREAD);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    mpi_errno = MPIR_Init_thread( argc, argv, required, provided );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INIT_THREAD);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INIT_THREAD);
    return MPI_SUCCESS;
}
