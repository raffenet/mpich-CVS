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
int MPIR_Init_thread( int required, int *provided )
{
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

    /* Eventually this will support commandline and environment options
     for controlling error checks.  It will use a common routine (pre init 
     version) */
    MPIR_Process.do_error_checks = 1;

    /* Initialize necessary subsystems and setup the predefined attribute
       values.  Subsystems may change these values. */
    MPIR_Process.attrs.appnum          = 0;
    MPIR_Process.attrs.host            = 0;
    MPIR_Process.attrs.io              = 0;
    MPIR_Process.attrs.lastusedcode    = 0;
    MPIR_Process.attrs.tag_ub          = 0;
    MPIR_Process.attrs.universe        = 1;
    MPIR_Process.attrs.wtime_is_global = 0;

    /* Create the initial communicators */
    /* 
    MPI_COMM_WORLD = PREDEFINED_HANDLE(COMM,0);
    MPI_COMM_SELF  = PREDEFINED_HANDLE(COMM,1);
    */

    MPID_Init();

    MPIR_Process.initialized           = MPICH_WITHIN_MPI;
    return 0;
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
int MPI_Init_thread( int *argc, char *((*argv)[]), int required, 
		     int *provided )
{
    static const char FCNAME[] = "MPI_Init_thread";
    int mpi_errno = MPI_SUCCESS;

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

    MPIR_Init_thread( required, provided );
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INIT_THREAD);
    return MPI_SUCCESS;
}
