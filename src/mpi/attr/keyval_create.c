/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Keyval_create */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Keyval_create = PMPI_Keyval_create
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Keyval_create  MPI_Keyval_create
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Keyval_create as PMPI_Keyval_create
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Keyval_create PMPI_Keyval_create

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Keyval_create

/*@

MPI_Keyval_create - Greates a new attribute key

Input Parameters:
+ copy_fn - Copy callback function for 'keyval' 
. delete_fn - Delete callback function for 'keyval' 
- extra_state - Extra state for callback functions 

Output Parameter:
. keyval - key value for future access (integer) 

Notes:
Key values are global (available for any and all communicators).

There are subtle differences between C and Fortran that require that the
copy_fn be written in the same language that 'MPI_Keyval_create'
is called from.
This should not be a problem for most users; only programers using both 
Fortran and C in the same program need to be sure that they follow this rule.

.N ThreadSafe

.N Deprecated
The replacement for this routine is 'MPI_Comm_create_keyval'.

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_EXHAUSTED
.N MPI_ERR_ARG

.seealso  MPI_Keyval_free, MPI_Comm_create_keyval
@*/
int MPI_Keyval_create(MPI_Copy_function *copy_fn, 
		      MPI_Delete_function *delete_fn, 
		      int *keyval, void *extra_state)
{
    static const char FCNAME[] = "MPI_Keyval_create";
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_KEYVAL_CREATE);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_KEYVAL_CREATE);
    MPIR_ERRTEST_INITIALIZED_FIRSTORJUMP;

    /* ... body of routine ...  */
    MPIR_Nest_incr();
    mpi_errno = NMPI_Comm_create_keyval( copy_fn, delete_fn, keyval, 
					extra_state );
    MPIR_Nest_decr();
    if (mpi_errno == MPI_SUCCESS)
    {
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_KEYVAL_CREATE);
	return MPI_SUCCESS;
    }

    /* --BEGIN ERROR HANDLING-- */
    /* fn_fail: *//* No use of fn_fail in this routine (yet) */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, 
				     FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_keyval_create", "**mpi_keyval_create %p %p %p %p", 
				     copy_fn, delete_fn, keyval, extra_state);
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_KEYVAL_CREATE);
    return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    /* --END ERROR HANDLING-- */
}

