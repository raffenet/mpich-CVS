/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_set_errhandler */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_set_errhandler = PMPI_Win_set_errhandler
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_set_errhandler  MPI_Win_set_errhandler
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_set_errhandler as PMPI_Win_set_errhandler
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Win_set_errhandler PMPI_Win_set_errhandler

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Win_set_errhandler

/*@
   MPI_Win_set_errhandler - Set window error handler

   Input Parameters:
+ win - window (handle) 
- errhandler - new error handler for window (handle) 
.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_WIN
@*/
int MPI_Win_set_errhandler(MPI_Win win, MPI_Errhandler errhandler)
{
    static const char FCNAME[] = "MPI_Win_set_errhandler";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    int  in_use;
    MPID_Errhandler *errhan_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WIN_SET_ERRHANDLER);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_WIN_SET_ERRHANDLER);
    /* Get handles to MPI objects. */
    MPID_Win_get_ptr( win, win_ptr );
    MPID_Errhandler_get_ptr( errhandler, errhan_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            /* Validate win_ptr */
            MPID_Win_valid_ptr( win_ptr, mpi_errno );
	    /* If win_ptr is not value, it will be reset to null */

	    if (HANDLE_GET_KIND(errhandler) != HANDLE_KIND_BUILTIN) {
		MPID_Errhandler_valid_ptr( errhan_ptr,mpi_errno );
		/* Also check for a valid errhandler kind */
		if (!mpi_errno) {
		    if (errhan_ptr->kind != MPID_WIN) {
			mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_ARG, "**errhandnotwin", 0 );
		    }
		}
	    }
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_WIN_SET_ERRHANDLER);
                return MPIR_Err_return_win( win_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    if (win_ptr->errhandler != NULL) {
	if (HANDLE_GET_KIND(errhandler) != HANDLE_KIND_BUILTIN) {
	    MPIU_Object_release_ref(win_ptr->errhandler,&in_use);
	    if (!in_use) {
		MPID_Errhandler_free( win_ptr->errhandler );
	    }
	}
    }
    MPIU_Object_add_ref(errhan_ptr);
    win_ptr->errhandler = errhan_ptr;
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_WIN_SET_ERRHANDLER);
    return MPI_SUCCESS;
}

