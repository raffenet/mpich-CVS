/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#if !defined(MPID_REQUEST_PTR_ARRAY_SIZE)
#define MPID_REQUEST_PTR_ARRAY_SIZE 16
#endif

/* -- Begin Profiling Symbol Block for routine MPI_Startall */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Startall = PMPI_Startall
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Startall  MPI_Startall
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Startall as PMPI_Startall
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Startall PMPI_Startall

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Startall

/*@
  MPI_Startall - Starts a collection of requests 

Input Parameters:
+ count - list length (integer) 
- array_of_requests - array of requests (array of handle) 

   Notes:

   Unlike MPI_Waitall(), MPI_Startall() does not provide a mechanism for
   returning multiple errors nor pinpointing the request(s) involved.
   Futhermore, the behavior of MPI_Startall() after an error occurs is not
   defined by the MPI standard.  If well defined error reporting and behavior
   are required, multiple calls to MPI_Start() should be used instead.

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Startall(int count, MPI_Request array_of_requests[])
{
    static const char FCNAME[] = "MPI_Startall";
    MPID_Request * request_ptr_array[MPID_REQUEST_PTR_ARRAY_SIZE];
    MPID_Request ** request_ptrs = NULL;
    int i;
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_STARTALL);

    /* Verify that MPI has been initialized */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            if (mpi_errno) {
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
	    
    MPID_MPI_PT2PT_FUNC_ENTER(MPID_STATE_MPI_STARTALL);

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    MPIR_ERRTEST_ARGNULL(array_of_requests,"array_of_requests",
				 mpi_errno);
	    if (array_of_requests != NULL && count > 0)
	    {
		for (i = 0; i < count; i++)
		{
		    MPIR_ERRTEST_REQUEST(array_of_requests[i], mpi_errno);
		}
	    }
            if (mpi_errno) {
                goto fn_exit;
            }
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    /* Convert MPI request handles to a request object pointers */
    if (count <= MPID_REQUEST_PTR_ARRAY_SIZE)
    {
	request_ptrs = request_ptr_array;
    }
    else
    {
	request_ptrs = MPIU_Malloc(count * sizeof(MPID_Request *));
	/* --BEGIN ERROR HANDLING-- */
	if (request_ptrs == NULL)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", "**nomem %d", count * sizeof(MPID_Request*));
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
		"**mpi_startall", "**mpi_startall %d %p", count, array_of_requests);
	    goto fn_exit;
	}
	/* --END ERROR HANDLING-- */
    }

    for (i = 0; i < count; i++)
    {
	MPID_Request_get_ptr(array_of_requests[i], request_ptrs[i]);
    }
    
    /* Validate object pointers if error checking is enabled */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    for (i = 0; i < count; i++)
	    {
		MPID_Request_valid_ptr( request_ptrs[i], mpi_errno );
	    }
            if (mpi_errno) {
		goto fn_exit;
            }
	    for (i = 0; i < count; i++)
	    {
		MPIR_ERRTEST_PERSISTENT(request_ptrs[i], mpi_errno);
		MPIR_ERRTEST_PERSISTENT_ACTIVE(request_ptrs[i], mpi_errno);
	    }
            if (mpi_errno) {
		goto fn_exit;
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    mpi_errno = MPID_Startall(count, request_ptrs);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	    "**mpi_startall", "**mpi_startall %d %p", count, array_of_requests);
    }

  fn_exit:
    if (request_ptrs != request_ptr_array && request_ptrs != NULL)
    {
	MPIU_Free(request_ptrs);
    }
    
    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_STARTALL);
    return (mpi_errno == MPI_SUCCESS) ? MPI_SUCCESS :
	MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
}
