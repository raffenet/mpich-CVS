/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME xfer_scatter_recv_mop_forward_op

int xfer_scatter_recv_mop_forward_op(MPID_Request *request_ptr, void *buf, int count, MPI_Datatype dtype, int first, int last, int dest)
{
    static const char FCNAME[] = "xfer_scatter_recv_mop_forward_op";

    MPID_MPI_FUNC_ENTER(MPID_STATE_XFER_SCATTER_RECV_MOP_FORWARD_OP);

    MPID_MPI_FUNC_EXIT(MPID_STATE_XFER_SCATTER_RECV_MOP_FORWARD_OP);
    return MPI_SUCCESS;
}
