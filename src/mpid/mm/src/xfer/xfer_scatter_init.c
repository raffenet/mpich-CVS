/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME xfer_scatter_init

/*@
   xfer_scatter_init - xfer_scatter_init

   Parameters:
+  int src - source
.  int tag - tag
.  MPID_Comm *comm_ptr - communicator
-  MPID_Request **request_pptr - request pointer

   Notes:
@*/
int xfer_scatter_init(int src, int tag, MPID_Comm *comm_ptr, MPID_Request **request_pptr)
{
    static const char FCNAME[] = "xfer_scatter_init";

    MPID_MPI_FUNC_ENTER(MPID_STATE_XFER_SCATTER_INIT);

    MPID_MPI_FUNC_EXIT(MPID_STATE_XFER_SCATTER_INIT);
    return MPI_SUCCESS;
}
