/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME xfer_scatter_replicate_op

int xfer_scatter_replicate_op(MPID_Request *request_ptr, int dest)
{
    static const char FCNAME[] = "xfer_scatter_replicate_op";

    MPID_MPI_FUNC_ENTER(MPID_STATE_XFER_SCATTER_REPLICATE_OP);

    MPID_MPI_FUNC_EXIT(MPID_STATE_XFER_SCATTER_REPLICATE_OP);
    return MPI_SUCCESS;
}
