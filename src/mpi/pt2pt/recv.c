/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Recv */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Recv = PMPI_Recv
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Recv  MPI_Recv
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Recv as PMPI_Recv
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Recv PMPI_Recv

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Recv

/*@
    MPI_Recv - Basic receive

Output Parameters:
+ buf - initial address of receive buffer (choice) 
- status - status object (Status) 

Input Parameters:
+ count - maximum number of elements in receive buffer (integer) 
. datatype - datatype of each receive buffer element (handle) 
. source - rank of source (integer) 
. tag - message tag (integer) 
- comm - communicator (handle) 

Notes:
The 'count' argument indicates the maximum length of a message; the actual 
number can be determined with 'MPI_Get_count'.  

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_TYPE
.N MPI_ERR_COUNT
.N MPI_ERR_TAG
.N MPI_ERR_RANK

@*/
int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
	     MPI_Comm comm, MPI_Status *status)
{
    static const char FCNAME[] = "MPI_Recv";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_Request * request_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_RECV);

    /* Verify that MPI has been initialized */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
	    /* NOTE: MPI_STATUS_IGNORE != NULL */
	    MPIR_ERRTEST_ARGNULL(status, "status", mpi_errno);
            if (mpi_errno) 
	    {
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
	    
    MPID_MPI_PT2PT_FUNC_ENTER_BACK(MPID_STATE_MPI_RECV);
    
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
                MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_RECV);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
	    
	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    MPIR_ERRTEST_DATATYPE(count, datatype, mpi_errno);
	    MPIR_ERRTEST_RECV_RANK(comm_ptr, source, mpi_errno);
	    MPIR_ERRTEST_RECV_TAG(tag, mpi_errno);
            if (mpi_errno) {
                MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_RECV);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
	    
	    MPID_Datatype_get_ptr(datatype, datatype_ptr);
            MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
	    MPIR_ERRTEST_USERBUFFER(buf,count,datatype,mpi_errno);
            if (mpi_errno) {
                MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_RECV);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    mpi_errno = MPID_Recv(buf, count, datatype, source, tag, comm_ptr, MPID_CONTEXT_INTRA_PT2PT, status, &request_ptr);
    if (mpi_errno == MPI_SUCCESS)
    {
	if (request_ptr == NULL)
	{
		MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_RECV);
		return MPI_SUCCESS;
	}
	else
	{
	    /* If a request was returned, then we need to block until the request is complete */
	    MPIR_Wait(request_ptr);
	    mpi_errno = request_ptr->status.MPI_ERROR;
	    MPIR_Request_extract_status(request_ptr, status);
	    MPID_Request_release(request_ptr);

	    if (mpi_errno == MPI_SUCCESS)
	    {
		MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_RECV);
		return MPI_SUCCESS;
	    }
	}
    }

    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_RECV);
    return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
}

