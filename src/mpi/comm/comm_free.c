/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_free */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_free = PMPI_Comm_free
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_free  MPI_Comm_free
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_free as PMPI_Comm_free
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Comm_free PMPI_Comm_free

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_free

/*@
   MPI_Comm_free - free a communicator

   Arguments:
.  MPI_Comm *comm - communicator

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Comm_free(MPI_Comm *comm)
{
    static const char FCNAME[] = "MPI_Comm_free";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_FREE);

    /* Verify that MPI has been initialized */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INITIALIZED( mpi_errno );
	    if (mpi_errno) {
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_FREE);

    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( *comm, comm_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not value, it will be reset to null */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_FREE);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    /*mpi_errno = MPID_Comm_free(); */

    /* Remove the attributes, executing the attribute delete routine */
    /* FIXME: This needs to access the routine in the perprocess structure,
       to prevent comm_free from forcing linking all of the attribute code */
    mpi_errno = MPIR_Comm_attr_delete( comm_ptr, comm_ptr->attributes );

    /* Free the VCRT */
    MPID_VCRT_Release(comm_ptr->vcrt);

    /* Free the context value */
    MPIR_Free_contextid( comm_ptr->context_id );
    
    /* ... end of body of routine ... */

    if (mpi_errno == MPI_SUCCESS)
    {
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_FREE);
	return MPI_SUCCESS;
    }

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_FREE);
    return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
}

