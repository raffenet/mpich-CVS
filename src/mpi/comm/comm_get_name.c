/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_get_name */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_get_name = PMPI_Comm_get_name
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_get_name  MPI_Comm_get_name
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_get_name as PMPI_Comm_get_name
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Comm_get_name PMPI_Comm_get_name

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_get_name

/*@
  MPI_Comm_get_name - return the print name from the communicator

  Input Parameter:
. comm - Communicator to get name of (handle)

  Output Parameters:
+ comm_name - One output, contains the name of the communicator.  It must
  be an array of size at least 'MPI_MAX_OBJECT_NAME'.
- resultlen - Number of characters in name

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
@*/
int MPI_Comm_get_name(MPI_Comm comm, char *comm_name, int *resultlen)
{
    static const char FCNAME[] = "MPI_Comm_get_name";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_GET_NAME);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_GET_NAME);
    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( comm, comm_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            /* Validate comm_ptr */
	    if (comm != MPI_COMM_NULL)
	    {
		MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    }
	    /* If comm_ptr is not valid, it will be reset to null */
	    MPIR_ERRTEST_ARGNULL( comm_name, "comm_name", mpi_errno );
	    MPIR_ERRTEST_ARGNULL( resultlen, "resultlen", mpi_errno );
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    /* The user must allocate a large enough section of memory */
    if (comm == MPI_COMM_NULL)
    {
	MPIU_Strncpy( comm_name, "MPI_COMM_NULL", MPI_MAX_OBJECT_NAME );
    }
    else
    {
	MPIU_Strncpy( comm_name, comm_ptr->name, MPI_MAX_OBJECT_NAME );
    }
    *resultlen = (int)strlen( comm_name );
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_GET_NAME);
    return MPI_SUCCESS;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_comm_get_name", "**mpi_comm_get_name %C %p %p", comm, comm_name, resultlen);
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_GET_NAME);
    return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    /* --END ERROR HANDLING-- */
}

