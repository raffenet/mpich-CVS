/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Barrier */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Barrier = PMPI_Barrier
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Barrier  MPI_Barrier
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Barrier as PMPI_Barrier
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Barrier PMPI_Barrier

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Barrier

/*@
   MPI_Barrier - barrier

   Arguments:
.  MPI_Comm comm - communicator

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Barrier( MPI_Comm comm )
{
    static const char FCNAME[] = "MPI_Barrier";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECLS;

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

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_BARRIER);
    
    /* Convert handles to MPI objects. */
    MPID_Comm_get_ptr( comm, comm_ptr );
    
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    /* Validate communicator */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_BARRIER);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Barrier != NULL)
    {
	mpi_errno = comm_ptr->coll_fns->Barrier(comm_ptr);
	if (mpi_errno == MPI_SUCCESS)
	{
	    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_BARRIER);
	    return MPI_SUCCESS;
	}
	else
	{
	    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_BARRIER);
	    return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
	}
    }
    else
    {
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_BARRIER);
	mpi_errno = MPIR_Err_create_code( MPI_ERR_INTERN,
					  "**barriernotimpl", 0 );
	return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    }

}
