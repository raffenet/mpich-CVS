/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   MPID_Comm_accept - communicator accept

   Arguments:
+  char *port_name - port name
.  MPI_Info info - info
.  int root - root
.  MPI_Comm comm - communicator
-  MPI_Comm *newcomm - new communicator

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
#undef FUNCNAME
#define FUNCNAME MPID_Comm_accept
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Comm_accept(char *port_name, MPID_Info *info_ptr, int root, MPID_Comm *comm_ptr, MPID_Comm **newcomm)
{
    int mpi_errno=MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MPID_COMM_ACCEPT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_COMM_ACCEPT);

    mpi_errno = MPIDI_CH3_Comm_accept(port_name, root, comm_ptr, newcomm);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
    }
    /* --END ERROR HANDLING-- */
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_COMM_ACCEPT);
    return mpi_errno;
}
