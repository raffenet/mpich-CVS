/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   xfer_replicate_op - xfer_replicate_op

   Parameters:
+  MPID_Request *request_ptr - request
-  int dest - destination

   Notes:
@*/
int xfer_replicate_op(MPID_Request *request_ptr, int dest)
{
    MM_ENTER_FUNC(XFER_REPLICATE_OP);
    MM_EXIT_FUNC(XFER_REPLICATE_OP);
    return MPI_SUCCESS;
}
