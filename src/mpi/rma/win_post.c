/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_post */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_post = PMPI_Win_post
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_post  MPI_Win_post
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_post as PMPI_Win_post
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Win_post PMPI_Win_post

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Win_post

/*@
   MPI_Win_post - post

   Arguments:
+  MPI_Group group - group
.  int assert - assert
-  MPI_Win win - window

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Win_post(MPI_Group group, int assert, MPI_Win win)
{
    static const char FCNAME[] = "MPI_Win_post";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;

    MPID_MPI_RMA_FUNC_ENTER(MPID_STATE_MPI_WIN_POST);

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
            MPID_Group *group_ptr=NULL;

            /* Validate win_ptr */
            MPID_Win_valid_ptr( win_ptr, mpi_errno );
            if (mpi_errno) {
                MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_POST);
                return MPIR_Err_return_win( NULL, FCNAME, mpi_errno );
            }

            MPID_Group_get_ptr(group, group_ptr);
            MPID_Group_valid_ptr(group_ptr, mpi_errno);
            if (mpi_errno != MPI_SUCCESS) {
                MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_POST);
                return MPIR_Err_return_win( win_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_POST);
    return MPI_SUCCESS;
}

