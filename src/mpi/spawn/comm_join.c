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
   MPI_Comm_join - Create a communicator by joining two processes connected by 
     a socket.

   Input Parameter:
. fd - socket file descriptor 

   Output Parameter:
. intercomm - new intercommunicator (handle) 

 Notes:
  The socket must be quiescent before 'MPI_COMM_JOIN' is called and after 
  'MPI_COMM_JOIN' returns. More specifically, on entry to 'MPI_COMM_JOIN', a 
  read on the socket will not read any data that was written to the socket 
  before the remote process called 'MPI_COMM_JOIN'.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
@*/
int MPI_Comm_join(int fd, MPI_Comm *intercomm)
{
    static const char FCNAME[] = "MPI_Comm_join";
    int mpi_errno = MPI_SUCCESS, err;
    char *local_port, *remote_port;

    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_JOIN);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_JOIN);
    MPIR_ERRTEST_INITIALIZED_FIRSTORJUMP;

    local_port = MPIU_Malloc(MPI_MAX_PORT_NAME);
    /* --BEGIN ERROR HANDLING-- */
    if (local_port == NULL)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */
    
    mpi_errno = NMPI_Open_port(MPI_INFO_NULL, local_port);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    remote_port = MPIU_Malloc(MPI_MAX_PORT_NAME);
    /* --BEGIN ERROR HANDLING-- */
    if (remote_port == NULL)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    err = write(fd, local_port, MPI_MAX_PORT_NAME);
    if (err < 0) {
        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN, "**join_write", 0);
        goto fn_fail;
    }

    err = read(fd, remote_port, MPI_MAX_PORT_NAME);
    if (err < 0) {
        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN, "**join_read", 0);
        goto fn_fail;
    }

    if (strcmp(local_port, remote_port) == 0) {
        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN, "**join_portname", "**join_portname %s %s", local_port, remote_port);
        goto fn_fail;
    }

    if (strcmp(local_port, remote_port) < 0) {
        mpi_errno = NMPI_Comm_accept(local_port, MPI_INFO_NULL, 0, 
                                     MPI_COMM_SELF, intercomm);
    }
    else {
        mpi_errno = NMPI_Comm_connect(remote_port, MPI_INFO_NULL, 0, 
                                     MPI_COMM_SELF, intercomm);
    }

    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    mpi_errno = NMPI_Close_port(local_port);

    MPIU_Free(local_port);
    MPIU_Free(remote_port);
    
    if (mpi_errno == MPI_SUCCESS)
    {
        MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_JOIN);
        return MPI_SUCCESS;
    }

    /* --BEGIN ERROR HANDLING-- */
fn_fail:
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, 
				     FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_comm_join", "**mpi_comm_join %d %p", fd, intercomm);
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_JOIN);
    return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    /* --END ERROR HANDLING-- */
}
