/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

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
MPICH_PerProcess_t MPIR_Process;
#ifdef MPICH_SINGLE_THREADED
MPICH_PerThread_t  MPIR_Thread;
#endif
int MPIR_Init_thread( int required, int *provided )
{
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
int MPI_Init_thread( int *argc, char ***argv, int required, int *provided )
{
    static const char FCNAME[] = "MPI_Init_thread";
    int mpi_errno;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_INIT_THREAD);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (MPIR_Process.initialized != MPICH_WITHIN_MPI) {
                mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER,
                            "**initialized", 0 );
	    }
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INIT_THREAD);
                return MPIR_Return( 0, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPIR_Init_thread( required, provided );
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INIT_THREAD);
    return MPI_SUCCESS;
}
