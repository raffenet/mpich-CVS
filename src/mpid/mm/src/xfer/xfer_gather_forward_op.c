/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME xfer_gather_forward_op

/*@
   xfer_gather_forward_op - xfer_gather_forward_op

   Parameters:
+  MPID_Request *request_ptr - request
-  int size - size

   Notes:
@*/
int xfer_gather_forward_op(MPID_Request *request_ptr, int size)
{
    static const char FCNAME[] = "xfer_gather_forward_op";

    MPID_MPI_FUNC_ENTER(MPID_STATE_XFER_GATHER_FORWARD_OP);

    MPID_MPI_FUNC_EXIT(MPID_STATE_XFER_GATHER_FORWARD_OP);
    return MPI_SUCCESS;
}
