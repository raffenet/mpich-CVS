/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_disconnect */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_disconnect = PMPI_Comm_disconnect
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_disconnect  MPI_Comm_disconnect
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_disconnect as PMPI_Comm_disconnect
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Comm_disconnect PMPI_Comm_disconnect

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_disconnect

/*@
   MPI_Comm_disconnect - Disconnect from a communicator

   Input Parameter
.  comm - communicator (handle) 

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Comm_disconnect(MPI_Comm *comm)
{
    static const char FCNAME[] = "MPI_Comm_disconnect";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_DISCONNECT);

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

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_DISCONNECT);

    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( *comm, comm_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not valid, it will be reset to null */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_DISCONNECT);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    mpi_errno = MPID_Comm_disconnect(comm_ptr);

    if (mpi_errno == MPI_SUCCESS)
    {
	*comm = MPI_COMM_NULL;
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_DISCONNECT);
	return MPI_SUCCESS;
    }

    /* --BEGIN ERROR HANDLING-- */
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_comm_disconnect", "**mpi_comm_disconnect %p", comm);
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_DISCONNECT);
    return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    /* --END ERROR HANDLING-- */
}
