/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpir_pt2pt.h"

/* -- Begin Profiling Symbol Block for routine MPI_Send */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Send = PMPI_Send
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Send  MPI_Send
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Send as PMPI_Send
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Send PMPI_Send

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Send

/*@
   MPI_Send - send

   Arguments:
+  void *buf - buffer
.  int count - count
.  MPI_Datatype datatype - datatype
.  int dest - destination
.  int tag - tag
-  MPI_Comm comm - communicator

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	     MPI_Comm comm)
{
    static const char FCNAME[] = "MPI_Send";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_Request * request_ptr = NULL;
    MPID_MPI_STATE_DECLS;

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
	    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_SEND);
    
    /* ... body of routine ...  */

    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr( comm, comm_ptr );

    /* Validate parameters if error checking is enabled */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPID_Datatype * datatype_ptr = NULL;
	    
	    /* Validate communicator */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_SEND);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
	    
            /* Validate datatype */
	    MPID_Datatype_get_ptr(datatype, datatype_ptr);
            MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_SEND);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    mpi_errno = MPID_Send(buf, count, datatype, dest, tag, comm_ptr,
			  MPID_CONTEXT_INTRA_PT2PT, &request_ptr);
    if (mpi_errno == MPI_SUCCESS)
    {
	if (request_ptr == NULL)
	{
		MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_SEND);
		return MPI_SUCCESS;
	}
	else
	{
	    /* If a request was returned, then we need to block until the
	       request is complete */
	    MPIR_Wait(request_ptr);
	
	    mpi_errno = request_ptr->status.MPI_ERROR;
	    MPID_Request_release(request_ptr);
		
	    if (mpi_errno == MPI_SUCCESS)
	    {
		MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_SEND);
		return MPI_SUCCESS;
	    }
	}
    }
    
    /* ... end of body of routine ... */
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_SEND);
    return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
}
