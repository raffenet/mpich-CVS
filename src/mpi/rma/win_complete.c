/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_complete */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_complete = PMPI_Win_complete
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_complete  MPI_Win_complete
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_complete as PMPI_Win_complete
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Win_complete PMPI_Win_complete

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Win_complete

/*@
   MPI_Win_complete - Completes an RMA access epoch

   Input parameter:
. win - window object (handle) 

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_WIN
.N MPI_ERR_OTHER
@*/
int MPI_Win_complete(MPI_Win win)
{
    static const char FCNAME[] = "MPI_Win_complete";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WIN_COMPLETE);

    MPID_MPI_RMA_FUNC_ENTER(MPID_STATE_MPI_WIN_COMPLETE);

    /* Verify that MPI has been initialized */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            if (mpi_errno != MPI_SUCCESS) {
		MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_COMPLETE);
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
                MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_COMPLETE);
                return MPIR_Err_return_win( NULL, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPID_Win_complete(win_ptr);

    MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_COMPLETE);
    return MPI_SUCCESS;
}

