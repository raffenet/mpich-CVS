/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_accept */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_accept = PMPI_Comm_accept
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_accept  MPI_Comm_accept
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_accept as PMPI_Comm_accept
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Comm_accept PMPI_Comm_accept

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_accept

/*@
   MPI_Comm_accept - Accept a request to form a new intercommunicator

 Input Parameters:
+ port_name - port name (string, used only on root) 
. info - implementation-dependent information (handle, used only on root) 
. root - rank in comm of root node (integer) 
- IN - comm intracommunicator over which call is collective (handle) 

 Output Parameter:
. newcomm - intercommunicator with client as remote group (handle) 

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_INFO
.N MPI_ERR_COMM
@*/
int MPI_Comm_accept(char *port_name, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *newcomm)
{
    static const char FCNAME[] = "MPI_Comm_accept";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_Comm *newcomm_ptr = NULL;
    MPID_Info *info_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_ACCEPT);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_ACCEPT);
    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( comm, comm_ptr );
    MPID_Info_get_ptr( info, info_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INITIALIZED(mpi_errno);
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_ACCEPT);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    mpi_errno = MPID_Comm_accept(port_name, info_ptr, root, comm_ptr, &newcomm_ptr);
    if (mpi_errno == MPI_SUCCESS)
    {
        *newcomm = newcomm_ptr->handle;

        MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_ACCEPT);
        return MPI_SUCCESS;
    }

    /* --BEGIN ERROR HANDLING-- */
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_comm_accept", "**mpi_comm_accept %s %I %d %C %p", port_name, info, root, comm, newcomm);
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_ACCEPT);
    return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    /* --END ERROR HANDLING-- */
}
