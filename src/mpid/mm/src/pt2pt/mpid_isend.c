/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   MPID_Isend - isend

   Arguments:
+  void *buf - buffer
.  int count - count
.  MPID_Datatype *datatype_ptr - datatype
.  int dest - destination
.  int tag - tag
.  MPID_Comm *comm_ptr - communicator
.  int mode - communicator mode
-  MPID_Request **request_pptr - request

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPID_Isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPID_Comm *comm_ptr, int mode, MPID_Request **request_pptr)
{
    xfer_init(tag, comm_ptr, request_pptr);
    xfer_send_op(*request_pptr, buf, count, datatype, 0, -1, dest);
    xfer_start(*request_pptr);

    return MPI_SUCCESS;
}
