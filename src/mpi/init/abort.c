/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

/* -- Begin Profiling Symbol Block for routine MPI_Abort */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Abort = PMPI_Abort
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Abort  MPI_Abort
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Abort as PMPI_Abort
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Abort PMPI_Abort

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Abort

/*@
   MPI_Abort - abort

Input Parameters:
+ comm - communicator of tasks to abort 
- errorcode - error code to return to invoking environment 

Notes:
Terminates all MPI processes associated with the communicator 'comm'; in
most systems (all to date), terminates `all` processes.

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Abort(MPI_Comm comm, int errorcode)
{
    static const char FCNAME[] = "MPI_Abort";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    char abort_str[100], comm_name[MPI_MAX_NAME_STRING];
    int len = MPI_MAX_NAME_STRING;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_ABORT);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_ABORT);
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
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    if (!comm_ptr)
    {
	/* Use comm world if the communicator is not valid */
	comm_ptr = MPIR_Process.comm_world;
    }

    /*mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**abort", 0);*/
    NMPI_Comm_get_name(comm, comm_name, &len);
    if (len == 0)
    {
	MPIU_Snprintf(comm_name, MPI_MAX_NAME_STRING, "comm=0x%X", comm);
    }
    MPIU_Snprintf(abort_str, 100, "application called MPI_Abort(%s, %d) - process %d", comm_name, errorcode, comm_ptr->rank);
    MPID_Abort( comm_ptr, mpi_errno, errorcode, abort_str );
    /* ... end of body of routine ... */

    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_abort", "**mpi_abort %C %d", comm, errorcode);
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ABORT);
    return MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    /* --END ERROR HANDLING-- */
}
