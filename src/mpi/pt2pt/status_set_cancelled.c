/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Status_set_cancelled */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Status_set_cancelled = PMPI_Status_set_cancelled
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Status_set_cancelled  MPI_Status_set_cancelled
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Status_set_cancelled as PMPI_Status_set_cancelled
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Status_set_cancelled PMPI_Status_set_cancelled

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Status_set_cancelled

/*@
   MPI_Status_set_cancelled - Sets the cancelled state associated with a Status object

Input Parameters:
+  MPI_Status *status - status to associate cancel flag with (Status)
-  int flag - if true indicates request was cancelled (logical)

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Status_set_cancelled(MPI_Status *status, int flag)
{
    static const char FCNAME[] = "MPI_Status_set_cancelled";
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_STATUS_SET_CANCELLED);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_STATUS_SET_CANCELLED);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_ARGNULL( status, "status", mpi_errno );
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_STATUS_SET_CANCELLED);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    status->cancelled = flag ? TRUE : FALSE;
    
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_STATUS_SET_CANCELLED);
    return MPI_SUCCESS;
}

