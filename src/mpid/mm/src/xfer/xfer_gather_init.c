/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME xfer_gather_init

int xfer_gather_init(int dest, int tag, MPID_Comm *comm_ptr, MPID_Request **request_pptr)
{
    static const char FCNAME[] = "xfer_gather_init";

    MPID_MPI_FUNC_ENTER(MPID_STATE_XFER_GATHER_INIT);

    MPID_MPI_FUNC_EXIT(MPID_STATE_XFER_GATHER_INIT);
    return MPI_SUCCESS;
}
