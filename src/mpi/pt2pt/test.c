/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpir_pt2pt.h"

/* -- Begin Profiling Symbol Block for routine MPI_Test */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Test = PMPI_Test
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Test  MPI_Test
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Test as PMPI_Test
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Test PMPI_Test

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Test

/*@
   MPI_Test - test

   Arguments:
+  MPI_Request *request - request
.  int *flag - flag
-  MPI_Status *status - status

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status)
{
    static const char FCNAME[] = "MPI_Test";
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *request_ptr = NULL;
    MPID_MPI_STATE_DECLS;

    /* Verify that MPI has been initialized */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_ARGNULL(flag,"flag",mpi_errno);
	    if (mpi_errno) {
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TEST);

    /* ... body of routine ...  */

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (mpi_errno != MPI_SUCCESS) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TEST);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    *flag = MPIR_Test(request_ptr);
    if (*flag) {
	/* Not quite right - set only on receive or generalized requests */
	if (status != NULL) {
	    *status = request_ptr->status;
	}
    }

    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TEST);
    return MPI_SUCCESS;
}
