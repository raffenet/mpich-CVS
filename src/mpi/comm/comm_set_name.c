/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_set_name */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_set_name = PMPI_Comm_set_name
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_set_name  MPI_Comm_set_name
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_set_name as PMPI_Comm_set_name
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Comm_set_name PMPI_Comm_set_name

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_set_name

/*@
   MPI_Comm_set_name - set the communicator name

   Input Parameters:
+  MPI_Comm comm - communicator to name (handle)
-  char *comm_name - Name for communicator

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
@*/
int MPI_Comm_set_name(MPI_Comm comm, char *comm_name)
{
    static const char FCNAME[] = "MPI_Comm_set_name";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_SET_NAME);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_SET_NAME);
    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( comm, comm_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);

            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    MPIR_ERRTEST_ARGNULL( comm_name, "comm_name", mpi_errno );
	    /* If comm_ptr is not valid, it will be reset to null */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_SET_NAME);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    MPIU_Strncpy( comm_ptr->name, comm_name, MPI_MAX_NAME_STRING );
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_SET_NAME);
    return MPI_SUCCESS;
}

