/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_set_name */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_set_name = PMPI_Win_set_name
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_set_name  MPI_Win_set_name
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_set_name as PMPI_Win_set_name
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Win_set_name PMPI_Win_set_name

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Win_set_name

/*@
   MPI_Win_set_name - set the window name

   Input Parameters:
+ win - window whose identifier is to be set (handle) 
- win_name - the character string which is remembered as the name (string) 

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_WIN
.N MPI_ERR_OTHER
.N MPI_ERR_ARG
@*/
int MPI_Win_set_name(MPI_Win win, char *win_name)
{
    static const char FCNAME[] = "MPI_Win_set_name";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WIN_SET_NAME);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_WIN_SET_NAME);
    /* Get handles to MPI objects. */
    MPID_Win_get_ptr( win, win_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    if (mpi_errno) goto fn_fail;

            /* Validate win_ptr */
            MPID_Win_valid_ptr( win_ptr, mpi_errno );

	    /* If win_ptr is not valid, it will be reset to null */
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    MPIU_Strncpy( win_ptr->name, win_name, MPI_MAX_OBJECT_NAME );
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_WIN_SET_NAME);
    return MPI_SUCCESS;
fn_fail:
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_win_set_name", "**mpi_win_set_name %W %s", win, win_name);
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_WIN_SET_NAME);
    return MPIR_Err_return_win(win_ptr, FCNAME, mpi_errno);
}

