/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Start */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Start = PMPI_Start
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Start  MPI_Start
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Start as PMPI_Start
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Start PMPI_Start

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Start

/*@
   MPI_Start - start

   Arguments:
.  MPI_Request *request - request

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Start(MPI_Request *request)
{
    static const char FCNAME[] = "MPI_Start";
    MPID_Request * request_ptr = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_START);

    /* Verify that MPI has been initialized */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_ARGNULL(request,"request",mpi_errno);
	    if (request != NULL)
	    {
		MPIR_ERRTEST_REQUEST(*request, mpi_errno);
	    }
            if (mpi_errno) {
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
	    
    MPID_MPI_PT2PT_FUNC_ENTER(MPID_STATE_MPI_START);
    
    /* Convert MPI request handle to a request object pointer */
    MPID_Request_get_ptr( *request, request_ptr );
    
    /* Validate object pointers if error checking is enabled */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPID_Request_valid_ptr( request_ptr, mpi_errno );
	    MPIR_ERRTEST_PERSISTENT(request_ptr, mpi_errno);
	    MPIR_ERRTEST_PERSISTENT_ACTIVE(request_ptr, mpi_errno);
            if (mpi_errno) {
                MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_START);
                return MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    mpi_errno = MPID_Startall(1, &request_ptr);
    
    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_START);
    return (mpi_errno == MPI_SUCCESS) ? MPI_SUCCESS :
	MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
}
