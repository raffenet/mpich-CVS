/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_join */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_join = PMPI_Comm_join
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_join  MPI_Comm_join
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_join as PMPI_Comm_join
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Comm_join PMPI_Comm_join

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_join

/*@
   MPI_Comm_join - join

   Input Parameter:
. fd - socket file descriptor 

   Output Parameter:
. intercomm - new intercommunicator (handle) 

 Notes:
  The socket must be quiescent before 'MPI_COMM_JOIN' is called and after 
  'MPI_COMM_JOIN' returns. More specifically, on entry to 'MPI_COMM_JOIN', a 
  read on the socket will not read any data that was written to the socket 
  before the remote process called 'MPI_COMM_JOIN'.

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
@*/
int MPI_Comm_join(int fd, MPI_Comm *intercomm)
{
    static const char FCNAME[] = "MPI_Comm_join";
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_JOIN);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_JOIN);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INITIALIZED(mpi_errno);
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_UNSUPPORTED_OPERATION, "**notimpl", 0);

    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_comm_join", "**mpi_comm_join %d %p", fd, intercomm);
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_JOIN);
    return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    /* --END ERROR HANDLING-- */
}
