/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpir_pt2pt.h"

/* -- Begin Profiling Symbol Block for routine MPI_Wait */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Wait = PMPI_Wait
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Wait  MPI_Wait
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Wait as PMPI_Wait
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Wait PMPI_Wait

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Wait

/*@
   MPI_Wait - wait

   Arguments:
+  MPI_Request  *request - request
-  MPI_Status   *status - status

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Wait(MPI_Request *request, MPI_Status *status)
{
    static const char FCNAME[] = "MPI_Wait";
    int mpi_errno = MPI_SUCCESS;
    MPID_Request * request_ptr = NULL;
    MPID_Comm * comm_ptr = NULL;
    int error = FALSE;
    MPID_MPI_STATE_DECLS;

    /* Verify that MPI has been initialized */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_REQUEST(request, mpi_errno);
            if (mpi_errno) {
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
	    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_WAIT);
    
    /* Convert MPI object handles to object pointers */
    MPID_Request_get_ptr( *request, request_ptr );
    comm_ptr = request_ptr->comm;
    
    /* Validate parameters if error checking is enabled */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    /* Validate request_ptr */
            MPID_Request_valid_ptr( request_ptr, mpi_errno );
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_WAIT);
                return MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPIR_Wait(request_ptr);
    
    if (request_ptr->kind == MPID_UREQUEST)
    {
	mpi_errno = (request_ptr->query_fn)(
	    request_ptr->grequest_extra_state, &request_ptr->status);
	request_ptr->status.MPI_ERROR = mpi_errno;
	mpi_errno = (request_ptr->free_fn)(request_ptr->grequest_extra_state);
	if (mpi_errno != MPI_SUCCESS)
	{
	    request_ptr->status.MPI_ERROR = mpi_errno;
	}
    }
    
    error = (request_ptr->status.MPI_ERROR != MPI_SUCCESS);
    if (status != NULL)
    {
	*status = request_ptr->status;
    }

    *request = MPI_REQUEST_NULL;
    MPID_Request_release(request_ptr);

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_WAIT);
    return !error ? MPI_SUCCESS :
	MPIR_Err_return_comm(comm_ptr, FCNAME, MPI_ERR_IN_STATUS);
}
