/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Request_free */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Request_free = PMPI_Request_free
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Request_free  MPI_Request_free
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Request_free as PMPI_Request_free
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Request_free PMPI_Request_free

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Request_free

/*@
   MPI_Request_free - free request

   Arguments:
.  MPI_Request *request - request

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
EXPORT_MPI_API int MPI_Request_free(MPI_Request *request)
{
    static const char FCNAME[] = "MPI_Request_free";
    int mpi_errno = MPI_SUCCESS;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_REQUEST_FREE);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (MPIR_Process.initialized != MPICH_WITHIN_MPI) {
                mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER,
                            "**initialized", 0 );
            }
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_REQUEST_FREE);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_REQUEST_FREE);
    return MPI_SUCCESS;
}
