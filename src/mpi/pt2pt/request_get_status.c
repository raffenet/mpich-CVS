/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Request_get_status */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Request_get_status = PMPI_Request_get_status
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Request_get_status  MPI_Request_get_status
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Request_get_status as PMPI_Request_get_status
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Request_get_status PMPI_Request_get_status

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Request_get_status

/*@
   MPI_Request_get_status - Nondestructive test for the completion of a Request

Input Parameter:
.  MPI_Request request - request handle

Output Parameters:
+  int *flag - true if operation has completed (logical)
-  MPI_Status *status - status object (Status).  May be 'MPI_STATUS_NULL'.

   Notes:

   Unlike MPI_Test, MPI_Request_get_status does not deallocate or deactivate
   the request.  A call to one of the test/wait routines or MPI_Request_free
   should be made to release the request object.

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Request_get_status(MPI_Request request, int *flag, MPI_Status *status)
{
    static const char FCNAME[] = "MPI_Request_get_status";
    int mpi_errno = MPI_SUCCESS;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_REQUEST_GET_STATUS);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (MPIR_Process.initialized != MPICH_WITHIN_MPI) {
                mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER,
                            "**initialized", 0 );
            }
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_REQUEST_GET_STATUS);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_REQUEST_GET_STATUS);
    return MPI_SUCCESS;
}

