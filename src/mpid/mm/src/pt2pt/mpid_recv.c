/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   MPID_Recv - recv

   Arguments:
+  void *buf - buffer
.  int count - count
.  MPID_Datatype *datatype_ptr - datatype
.  int source - source
.  int tag - tag
.  MPID_Comm *comm_ptr - communicator
.  int mode - communicator mode
.  MPI_Status *status_ptr - status
-  MPID_Request **request_pptr - request

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPID_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPID_Comm *comm_ptr, int mode, MPI_Status *status_ptr, MPID_Request **request_pptr)
{
    int mpi_errno;

    MM_ENTER_FUNC(MPID_RECV);

    mpi_errno = MPID_Irecv(buf, count, datatype, source, tag, comm_ptr, mode, request_pptr);

    MM_EXIT_FUNC(MPID_RECV);
    return mpi_errno;
}
