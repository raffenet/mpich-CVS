/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME xfer_gather_start

int xfer_gather_start(MPID_Request *request_ptr)
{
    static const char FCNAME[] = "xfer_gather_start";

    MPID_MPI_FUNC_ENTER(MPID_STATE_XFER_GATHER_START);

    MPID_MPI_FUNC_EXIT(MPID_STATE_XFER_GATHER_START);
    return MPI_SUCCESS;
}
