/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME xfer_gather_recv_mop_op

int xfer_gather_recv_mop_op(MPID_Request *request_ptr, void *buf, int count, MPI_Datatype dtype, int first, int last, int src)
{
    static const char FCNAME[] = "xfer_gather_recv_mop_op";

    MPID_MPI_FUNC_ENTER(MPID_STATE_XFER_GATHER_RECV_MOP_OP);

    MPID_MPI_FUNC_EXIT(MPID_STATE_XFER_GATHER_RECV_MOP_OP);
    return MPI_SUCCESS;
}
