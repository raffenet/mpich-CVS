/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_size */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_size = PMPI_Comm_size
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_size  MPI_Comm_size
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_size as PMPI_Comm_size
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Comm_size PMPI_Comm_size
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_size

/*@

MPI_Comm_size - Determines the size of the group associated with a communictor

Input Argument:
. comm - communicator (handle) 

Output Argument:
. size - number of processes in the group of 'comm'  (integer) 

Notes:
   'MPI_COMM_NULL' is `not` a valid argument to this function.

.N fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_ARG
@*/
int MPI_Comm_size( MPI_Comm comm, int *size ) 
{
    static const char FCNAME[] = "MPI_Comm_size";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = 0;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_SIZE);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_SIZE);
    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( comm, comm_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (MPIR_Process.initialized != MPICH_WITHIN_MPI) {
                mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER,
                            "**initialized", 0 );
            }
            if (!size) {
                mpi_errno = MPIR_Err_create_code( MPI_ERR_ARG, 
                            "**nullptr", "**nullptr %s", "size" );
            } 
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not value, it will be reset to null */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_SIZE);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    *size = comm_ptr->local_size;
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_SIZE);
    return MPI_SUCCESS;
}
