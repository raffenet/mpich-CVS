/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "attr.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_free_keyval */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_free_keyval = PMPI_Comm_free_keyval
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_free_keyval  MPI_Comm_free_keyval
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_free_keyval as PMPI_Comm_free_keyval
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Comm_free_keyval PMPI_Comm_free_keyval

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_free_keyval

/*@
   MPI_Comm_free_keyval - Frees an attribute key for communicators

Input Parameter:
. keyval - Frees the integer key value (integer) 

   Notes:
Key values are global (they can be used with any and all communicators)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
.N MPI_ERR_PERM_KEY
@*/
int MPI_Comm_free_keyval(int *comm_keyval)
{
    static const char FCNAME[] = "MPI_Comm_free_keyval";
    MPID_Keyval *keyval_ptr = NULL;
    int          in_use;
    int          mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_FREE_KEYVAL);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_FREE_KEYVAL);

    MPID_Keyval_get_ptr( *comm_keyval, keyval_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INITIALIZED(mpi_errno);

	    MPID_Keyval_valid_ptr( keyval_ptr, mpi_errno );
	    if (!mpi_errno) {
		if (keyval_ptr->kind != MPID_COMM) {
		    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, 
                                     MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
				     MPI_ERR_KEYVAL, "**keyvalnotcomm", 0 );
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
    *comm_keyval = MPI_KEYVAL_INVALID;

    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_FREE_KEYVAL);
    return MPI_SUCCESS;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, 
				     FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_comm_free_keyval", "**mpi_comm_free_keyval %p", comm_keyval);
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_FREE_KEYVAL);
    return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    /* --END ERROR HANDLING-- */
}
