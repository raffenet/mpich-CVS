/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Keyval_free */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Keyval_free = PMPI_Keyval_free
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Keyval_free  MPI_Keyval_free
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Keyval_free as PMPI_Keyval_free
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Keyval_free PMPI_Keyval_free

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Keyval_free

/*@
   MPI_Keyval_free - free keyval

   Arguments:
.  int *keyval - keyval

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Keyval_free(int *keyval)
{
    static const char FCNAME[] = "MPI_Keyval_free";
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_KEYVAL_FREE);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_KEYVAL_FREE);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_KEYVAL_FREE);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPIR_Nest_incr();
    mpi_errno = PMPI_Comm_free_keyval( keyval );
    MPIR_Nest_decr();
    if (mpi_errno) {
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_KEYVAL_FREE);
	return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    }
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_KEYVAL_FREE);
    return MPI_SUCCESS;
}
