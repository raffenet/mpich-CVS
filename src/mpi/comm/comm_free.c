/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpicomm.h"

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
MPI_Comm_free - Marks the communicator object for deallocation

Input Parameter:
. comm - communicator to be destroyed (handle) 

Notes:
This routine `frees` a communicator.  Because the communicator may still
be in use by other MPI routines, the actual communicator storage will not
be freed until all references to this communicator are removed.  For most
users, the effect of this routine is the same as if it was in fact freed
at this time of this call.  

Null Handles:
The MPI 1.1 specification, in the section on opaque objects, explicitly
disallows freeing a null communicator.  The text from the standard is:
.vb
 A null handle argument is an erroneous IN argument in MPI calls, unless an
 exception is explicitly stated in the text that defines the function. Such
 exception is allowed for handles to request objects in Wait and Test calls
 (sections Communication Completion and Multiple Completions ). Otherwise, a
 null handle can only be passed to a function that allocates a new object and
 returns a reference to it in the handle.
.ve

.N fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_ARG
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
	    /* If comm_ptr is not valid, it will be reset to null */
	    
	    /* Cannot free the predefined communicators */
	    if (HANDLE_GET_KIND(*comm) == HANDLE_KIND_BUILTIN) {
		mpi_errno = MPIR_Err_create_code( MPI_ERR_COMM,
					  "**commperm", "**commperm %s", 
						  comm_ptr->name );
	    }
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_FREE);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    mpi_errno = MPIR_Comm_release(comm_ptr);
    /* ... end of body of routine ... */
    if (mpi_errno == MPI_SUCCESS)
    {
	*comm = MPI_COMM_NULL;
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_FREE);
	return MPI_SUCCESS;
    }

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_FREE);
    return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
}

