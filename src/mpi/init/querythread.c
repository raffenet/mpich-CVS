/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Query_thread */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Query_thread = PMPI_Query_thread
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Query_thread  MPI_Query_thread
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Query_thread as PMPI_Query_thread
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Query_thread PMPI_Query_thread
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Query_thread

/*@
   MPI_Query_thread - Return the level of thread support provided

   Output Parameter:
.  provided - Level of thread support provided.  This is the same value
   that was returned in the 'provided' argument in 'MPI_Init_thread'.

   Notes:
   If 'MPI_Init' was called instead of 'MPI_Init_thread', the level of
   thread support is defined by the implementation.  This routine allows
   you to find out the provided level.  It is also useful for library 
   routines that discover that MPI has already been initialized and
   wish to determine what level of thread support is available.

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Query_thread( int *provided )
{
    static const char FCNAME[] = "MPI_Query_thread";
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_QUERY_THREAD);

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            if (mpi_errno) {
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_QUERY_THREAD);
    
    /* ... body of routine ...  */
    *provided = MPIR_Process.thread_provided;
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_QUERY_THREAD);
    return MPI_SUCCESS;
}
