/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   xfer_forward_op - xfer_forward_op

   Parameters:
+  MPID_Request *request_ptr - request
.  int size - size
.  int src - source
-  int dest - destination

   Notes:
@*/
int xfer_forward_op(MPID_Request *request_ptr, int size, int src, int dest)
{
    MPID_STATE_DECL(MPID_STATE_XFER_FORWARD_OP);
    MPID_FUNC_ENTER(MPID_STATE_XFER_FORWARD_OP);
    MPID_FUNC_EXIT(MPID_STATE_XFER_FORWARD_OP);
    return MPI_SUCCESS;
}
