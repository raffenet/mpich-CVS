/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Isend */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Isend = PMPI_Isend
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Isend  MPI_Isend
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Isend as PMPI_Isend
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Isend PMPI_Isend

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Isend

/*@
   MPI_Isend - isend

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
int MPI_Isend(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    static const char FCNAME[] = "MPI_Isend";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_Request *request_ptr = NULL;
    int context_offset = 0;
    MPID_MPI_STATE_DECLS;

    /* Verify that MPI has been initialized */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (MPIR_Process.initialized != MPICH_WITHIN_MPI) {
                mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER,
                            "**initialized", 0 );
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
	    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_ISEND);

    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( comm, comm_ptr );

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPID_Datatype *datatype_ptr = NULL;

            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not value, it will be reset to null */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ISEND);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }

	    /* Validate datatype */
	    MPID_Datatype_get_ptr( datatype, datatype_ptr );
	    MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ISEND);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    mpi_errno = MPID_Isend(buf, count, datatype, dest, tag, comm_ptr, context_offset, &request_ptr);

    if (!mpi_errno)
    {
	if (request_ptr == NULL)
	{
	    /* *request = MPID_STATIC_FINISHED_REQUEST; */
	    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ISEND);
	    return MPI_SUCCESS;
	}

	/* return the handle of the request to the user */
	*request = request_ptr->id;
	
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ISEND);
	return MPI_SUCCESS;
    }
    
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ISEND);
    return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
}
