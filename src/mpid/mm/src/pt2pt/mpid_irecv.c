/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Irecv

/*@
   MPID_Irecv - irecv

   Arguments:
+  void *buf - buffer
.  int count - count
.  MPID_Datatype *datatype_ptr - datatype
.  int source - source
.  int tag - tag
.  MPID_Comm *comm_ptr - communicator
.  int mode - communicator mode
-  MPID_Request **request_pptr - request

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPID_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPID_Comm *comm_ptr, int mode, MPID_Request **request_pptr)
{
    static const char FCNAME[] = "MPID_Irecv";

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPID_IRECV);

    xfer_scatter_init(source, tag, comm_ptr, request_pptr);
    xfer_scatter_recv_op(*request_pptr, buf, count, datatype, 0, -1);
    xfer_scatter_start(*request_pptr);

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPID_IRECV);
    return MPI_SUCCESS;
}
