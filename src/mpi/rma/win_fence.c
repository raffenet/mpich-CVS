/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_fence */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_fence = PMPI_Win_fence
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_fence  MPI_Win_fence
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_fence as PMPI_Win_fence
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Win_fence PMPI_Win_fence

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Win_fence

/*@
   MPI_Win_fence - Perform an MPI fence synchronization on a MPI window

   Input Parameters:
+ assert - program assertion (integer) 
- win - window object (handle) 

   Notes:
   The 'assert' argument is used to indicate special conditions for the
   fence that an implementation may use to optimize the 'MPI_Win_fence' 
   operation.  The value zero is always correct.  Other assertion values
   may be or''ed together.  Assertions that are valid for 'MPI_Win_fence' are\:

+ MPI_MODE_NOSTORE - the local window was not updated by local stores 
  (or local get or receive calls) since last synchronization. 
. MPI_MODE_NOPUT - the local window will not be updated by put or accumulate 
  calls after the fence call, until the ensuing (fence) synchronization. 
. MPI_MODE_NOPRECEDE - the fence does not complete any sequence of locally 
  issued RMA calls. If this assertion is given by any process in the window 
  group, then it must be given by all processes in the group. 
- MPI_MODE_NOSUCCEED - the fence does not start any sequence of locally 
  issued RMA calls. If the assertion is given by any process in the window 
  group, then it must be given by all processes in the group. 

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_OTHER
.N MPI_ERR_WIN
@*/
int MPI_Win_fence(int assert, MPI_Win win)
{
    static const char FCNAME[] = "MPI_Win_fence";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WIN_FENCE);

    MPID_MPI_RMA_FUNC_ENTER(MPID_STATE_MPI_WIN_FENCE);

    /* Verify that MPI has been initialized */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            if (mpi_errno != MPI_SUCCESS) {
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Get handles to MPI objects. */
    MPID_Win_get_ptr( win, win_ptr );

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate win_ptr */
            MPID_Win_valid_ptr( win_ptr, mpi_errno );
            if (mpi_errno) {
                MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_FENCE);
                return MPIR_Err_return_win( NULL, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    mpi_errno = MPID_Win_fence(assert, win_ptr);

    if (!mpi_errno)
    {
	MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_FENCE);
	return MPI_SUCCESS;
    }
    
    MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_FENCE);
    return MPIR_Err_return_win( win_ptr, FCNAME, mpi_errno );
}

