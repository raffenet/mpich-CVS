/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_get_name */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_get_name = PMPI_Win_get_name
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_get_name  MPI_Win_get_name
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_get_name as PMPI_Win_get_name
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Win_get_name PMPI_Win_get_name

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Win_get_name

/*@
   MPI_Win_get_name - get window name

   Arguments:
+  MPI_Win win - window
.  char *win_name - window name
-  int *resultlen - result length

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Win_get_name(MPI_Win win, char *win_name, int *resultlen)
{
    static const char FCNAME[] = "MPI_Win_get_name";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WIN_GET_NAME);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_WIN_GET_NAME);
    /* Get handles to MPI objects. */
    MPID_Win_get_ptr( win, win_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INITIALIZED(mpi_errno);
            /* Validate win_ptr */
            MPID_Win_valid_ptr( win_ptr, mpi_errno );
	    /* If win_ptr is not valid, it will be reset to null */

	    MPIR_ERRTEST_ARGNULL(win_name, "win_name", mpi_errno);
	    MPIR_ERRTEST_ARGNULL(resultlen, "resultlen", mpi_errno);

            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_WIN_GET_NAME);
                return MPIR_Err_return_win( win_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    MPIU_Strncpy( win_name, win_ptr->name, MPI_MAX_OBJECT_NAME );
    *resultlen = strlen( win_name );
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_WIN_GET_NAME);
    return MPI_SUCCESS;
}

