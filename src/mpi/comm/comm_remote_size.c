/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_remote_size */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_remote_size = PMPI_Comm_remote_size
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_remote_size  MPI_Comm_remote_size
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_remote_size as PMPI_Comm_remote_size
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Comm_remote_size PMPI_Comm_remote_size

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_remote_size

/*@
   MPI_Comm_remote_size - remote communicator size

   Arguments:
+  MPI_Comm comm - communicator
-  int *size - size

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Comm_remote_size(MPI_Comm comm, int *size)
{
    static const char FCNAME[] = "MPI_Comm_remote_size";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_REMOTE_SIZE);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_REMOTE_SIZE);
    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( comm, comm_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INITIALIZED(mpi_errno);
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not valid, it will be reset to null */
	    if (comm_ptr && comm_ptr->comm_kind != MPID_INTERCOMM) {
		mpi_errno = MPIR_Err_create_code( MPI_ERR_COMM, 
						  "**commnotinter", 0 );
	    }
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_REMOTE_SIZE);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    *size = comm_ptr->remote_size;
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_REMOTE_SIZE);
    return MPI_SUCCESS;
}

