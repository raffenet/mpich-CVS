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
   MPI_Keyval_create - keyval create

   Arguments:
+  MPI_Copy_function *copy_fn - copy function
.  MPI_Delete_function *delete_fn - delete function
.  int *keyval - keyval
-  void *extra_state - extra state

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Keyval_create(MPI_Copy_function *copy_fn, 
		      MPI_Delete_function *delete_fn, 
		      int *keyval, void *extra_state)
{
    static const char FCNAME[] = "MPI_Keyval_create";
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECLS;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_KEYVAL_CREATE);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_KEYVAL_CREATE);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    MPIR_Nest_incr();
    mpi_errno = PMPI_Comm_create_keyval( copy_fn, delete_fn, keyval, 
					extra_state );
    MPIR_Nest_decr();
    if (mpi_errno) {
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_KEYVAL_CREATE);
	return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    }

    /* ... end of body of routine ... */
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_KEYVAL_CREATE);
    return MPI_SUCCESS;
}

