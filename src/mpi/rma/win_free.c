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
   MPI_Win_free - Free an MPI RMA window

   Input Parameter:
. win - window object (handle) 

   Notes:
   If successfully freed, 'win' is set to 'MPI_WIN_NULL'.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_WIN
@*/
int MPI_Win_free(MPI_Win *win)
{
    static const char FCNAME[] = "MPI_Win_free";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WIN_FREE);

    MPID_MPI_RMA_FUNC_ENTER(MPID_STATE_MPI_WIN_FREE);

    /* Verify that MPI has been initialized */
    MPIR_ERRTEST_INITIALIZED_FIRSTORJUMP;

    /* Get handles to MPI objects. */
    MPID_Win_get_ptr( *win, win_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate win_ptr */
            MPID_Win_valid_ptr( win_ptr, mpi_errno );
	    /* If win_ptr is not valid, it will be reset to null */
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    if (MPIR_Process.attr_free && win_ptr->attributes)
    {
	mpi_errno = MPIR_Process.attr_free( win_ptr->handle, 
					    win_ptr->attributes );
    }
    if (mpi_errno == MPI_SUCCESS)
    {
	mpi_errno = MPID_Win_free(&win_ptr);
	*win = MPI_WIN_NULL;
    }
/*
    else
    {
        If the user attribute free function returns an error, 
	then do not free the window
    }
*/

    if (mpi_errno == MPI_SUCCESS)
    {
	MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_FREE);
        return MPI_SUCCESS;
    }

fn_fail:
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, 
				     FCNAME, __LINE__, MPI_ERR_OTHER,
				     "**mpi_win_free", "**mpi_win_free %p", 
				     win);
#endif
    MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_WIN_FREE);
    return MPIR_Err_return_win(win_ptr, FCNAME, mpi_errno);
}
