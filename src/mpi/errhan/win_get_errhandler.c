/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_get_errhandler */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_get_errhandler = PMPI_Win_get_errhandler
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_get_errhandler  MPI_Win_get_errhandler
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_get_errhandler as PMPI_Win_get_errhandler
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Win_get_errhandler PMPI_Win_get_errhandler

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Win_get_errhandler

/*@
   MPI_Win_get_errhandler - Get the error handler for the MPI RMA window

   Input Parameter:
. win - window (handle) 

   Output Parameter:
. errhandler - error handler currently associated with window (handle) 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_WIN
.N MPI_ERR_OTHER
@*/
int MPI_Win_get_errhandler(MPI_Win win, MPI_Errhandler *errhandler)
{
    static const char FCNAME[] = "MPI_Win_get_errhandler";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WIN_GET_ERRHANDLER);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_WIN_GET_ERRHANDLER);
    /* Get handles to MPI objects. */
    MPID_Win_get_ptr( win, win_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_ARGNULL(errhandler,"errhandler",mpi_errno);
            /* Validate win_ptr */
            MPID_Win_valid_ptr( win_ptr, mpi_errno );
	    /* If win_ptr is not valid, it will be reset to null */
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    if (win_ptr->errhandler) {
	*errhandler = win_ptr->errhandler->handle;
	MPIU_Object_add_ref(win_ptr->errhandler);
    }
    else {
	/* Use the default */
	*errhandler = MPI_ERRORS_ARE_FATAL;
    }
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_WIN_GET_ERRHANDLER);
    return MPI_SUCCESS;
fn_fail:
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, 
				     FCNAME, __LINE__, MPI_ERR_OTHER,
				     "**mpi_win_get_errhandler",
				     "**mpi_win_get_errhandler %W %p", 
				     win, errhandler);
#endif
    return MPIR_Err_return_win(win_ptr, FCNAME, mpi_errno);
}
