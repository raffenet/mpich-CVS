/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_free */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_free = PMPI_Win_free
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_free  MPI_Win_free
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_free as PMPI_Win_free
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Win_free PMPI_Win_free

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Win_free

/*@
   MPI_Win_free - free window

   Arguments:
.  MPI_Win *win - window

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Win_free(MPI_Win *win)
{
    static const char FCNAME[] = "MPI_Win_free";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;

    MPID_MPI_RMA_FUNC_ENTER(MPID_STATE_MPI_WIN_FREE);

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
    MPID_Win_get_ptr( *win, win_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate win_ptr */
            MPID_Win_valid_ptr( win_ptr, mpi_errno );
	    /* If win_ptr is not value, it will be reset to null */
            if (mpi_errno) {
                MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_FREE);
                return MPIR_Err_return_win( NULL, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    NMPI_Comm_free(&(win_ptr->comm));
    /* check if refcount needs to be decremented here as in group_free */
    MPIU_Handle_obj_free( &MPID_Win_mem, win_ptr );
    *win = MPI_WIN_NULL;
    
    MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_FREE);
    return MPI_SUCCESS;
}

