/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "attr.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_free_keyval */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_free_keyval = PMPI_Win_free_keyval
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_free_keyval  MPI_Win_free_keyval
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_free_keyval as PMPI_Win_free_keyval
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Win_free_keyval PMPI_Win_free_keyval

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Win_free_keyval

/*@
   MPI_Win_free_keyval - Frees an attribute key for MPI RMA windows

   Input Parameter:
. win_keyval - key value (integer) 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_WIN
.N MPI_ERR_OTHER
.N MPI_ERR_KEYVAL
@*/
int MPI_Win_free_keyval(int *win_keyval)
{
    static const char FCNAME[] = "MPI_Win_free_keyval";
    int mpi_errno = MPI_SUCCESS;
    MPID_Keyval *keyval_ptr = NULL;
    int          in_use;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WIN_FREE_KEYVAL);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_WIN_FREE_KEYVAL);

    MPID_Keyval_get_ptr( *win_keyval, keyval_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INITIALIZED(mpi_errno);

	    MPID_Keyval_valid_ptr( keyval_ptr, mpi_errno );
	    if (!mpi_errno) {
		if (keyval_ptr->kind != MPID_WIN) {
		    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, 
						      MPIR_ERR_RECOVERABLE, 
						      FCNAME, __LINE__, 
						      MPI_ERR_KEYVAL, 
						      "**keyvalnotwin", 0 );
		}
	    }
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    MPIU_Object_release_ref( keyval_ptr, &in_use);
    if (!in_use) {
	MPIU_Handle_obj_free( &MPID_Keyval_mem, keyval_ptr );
    }
    *win_keyval = MPI_KEYVAL_INVALID;

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_WIN_FREE_KEYVAL);
    return MPI_SUCCESS;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, 
				     FCNAME, __LINE__, MPI_ERR_OTHER,
				     "**mpi_win_free_keyval", 
				     "**mpi_win_free_keyval %p", win_keyval);
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_WIN_FREE_KEYVAL);
    return MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    /* --END ERROR HANDLING-- */
}
