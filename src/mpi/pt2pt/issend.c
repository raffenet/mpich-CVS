/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Issend */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Issend = PMPI_Issend
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Issend  MPI_Issend
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Issend as PMPI_Issend
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Issend PMPI_Issend

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Issend

/*@
   MPI_Issend - issend

   Arguments:
+  void *buf - buffer
.  int count - count
.  MPI_Datatype datatype - datatype
.  int dest - destination
.  int tag - tag
.  MPI_Comm comm - communicator
-  MPI_Request *request - request

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Issend(void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	       MPI_Comm comm, MPI_Request *request)
{
    static const char FCNAME[] = "MPI_Issend";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_Request *request_ptr = NULL;
    MPID_MPI_STATE_DECLS;

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
	    
    MPID_MPI_PT2PT_FUNC_ENTER_FRONT(MPID_STATE_MPI_ISSEND);

    /* ... body of routine ...  */
    
    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr( comm, comm_ptr );

    /* Validate parameters if error checking is enabled */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPID_Datatype * datatype_ptr = NULL;
	    
	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    MPIR_ERRTEST_DATATYPE(count, datatype, mpi_errno);
	    MPIR_ERRTEST_SEND_RANK(comm_ptr, dest, mpi_errno);
	    MPIR_ERRTEST_SEND_TAG(tag, mpi_errno);
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
            if (mpi_errno) {
                MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_ISSEND);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
	    
	    MPID_Datatype_get_ptr(datatype, datatype_ptr);
            MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
            if (mpi_errno) {
                MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_ISSEND);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    mpi_errno = MPID_Issend(buf, count, datatype, dest, tag, comm_ptr,
			    MPID_CONTEXT_INTRA_PT2PT, &request_ptr);

    if (mpi_errno == MPI_SUCCESS)
    {
	if (request_ptr == NULL)
	{
	    /* *request = MPID_STATIC_FINISHED_REQUEST; */
	    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_ISSEND);
	    return MPI_SUCCESS;
	}

	/* return the handle of the request to the user */
	*request = request_ptr->handle;
	
	MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_ISSEND);
	return MPI_SUCCESS;
    }
    
    /* ... end of body of routine ... */
    
    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_ISSEND);
    return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
}
