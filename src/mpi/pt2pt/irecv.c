/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Irecv */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Irecv = PMPI_Irecv
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Irecv  MPI_Irecv
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Irecv as PMPI_Irecv
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Irecv PMPI_Irecv

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Irecv

/*@
   MPI_Irecv - irecv

   Arguments:
+  void *buf - buffer
.  int count - count
.  MPI_Datatype datatype - datatype
.  int source - source
.  int tag - tag
.  MPI_Comm comm - communicator
-  MPI_Request *request - request

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request)
{
    static const char FCNAME[] = "MPI_Irecv";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_Request *request_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_IRECV);

    /* Verify that MPI has been initialized */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
            if (mpi_errno) {
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
	    
    MPID_MPI_PT2PT_FUNC_ENTER_BACK(MPID_STATE_MPI_IRECV);

    /* ... body of routine ...  */
    
    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr( comm, comm_ptr );

    /* Validate parameters if error checking is enabled */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPID_Datatype * datatype_ptr = NULL;
	    
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
            if (mpi_errno) {
                MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_IRECV);
                return MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
            }
	    
	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    MPIR_ERRTEST_DATATYPE(count, datatype, mpi_errno);
	    MPIR_ERRTEST_RECV_RANK(comm_ptr, source, mpi_errno);
	    MPIR_ERRTEST_RECV_TAG(tag, mpi_errno);
	    MPIR_ERRTEST_ARGNULL(request,"request",mpi_errno);
	    if (request != NULL)
	    {
		MPIR_ERRTEST_REQUEST(*request, mpi_errno);
	    }
	    if (mpi_errno) {
                MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_IRECV);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }

	    MPID_Datatype_get_ptr(datatype, datatype_ptr);
            MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
            if (mpi_errno) {
                MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_IRECV);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    if (source == MPI_PROC_NULL)
    {
	request_ptr = MPID_Request_create();
	if (request_ptr != NULL)
	{
	    request_ptr->status.MPI_SOURCE = MPI_PROC_NULL;
	    request_ptr->status.MPI_TAG = MPI_ANY_TAG;
	    request_ptr->kind = MPID_REQUEST_RECV;
	    request_ptr->cc = 0;
	    request_ptr->cc_ptr = &request_ptr->cc;
	    *request = request_ptr->handle;
	
	    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_IRECV);
	    return MPI_SUCCESS;
	}
	
	MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_IRECV);
	return MPI_ERR_NOMEM;
    }

    mpi_errno = MPID_Irecv(buf, count, datatype, source, tag, comm_ptr,
			   MPID_CONTEXT_INTRA_PT2PT, &request_ptr);

    if (mpi_errno == MPI_SUCCESS)
    {
	/* return the handle of the request to the user */
	*request = request_ptr->handle;
	
	MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_IRECV);
	return MPI_SUCCESS;
    }
    
    /* ... end of body of routine ... */
    
    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_IRECV);
    return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
}
