/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "via_rdmaimpl.h"

/*@
   via_rdma_init - initialize via_rdma method

   Notes:
@*/
int via_rdma_init( void )
{
    MPIDI_STATE_DECL(MPID_STATE_VIA_RDMA_INIT);
    MPIDI_FUNC_ENTER(MPID_STATE_VIA_RDMA_INIT);
    MPIDI_FUNC_EXIT(MPID_STATE_VIA_RDMA_INIT);
    return MPI_SUCCESS;
}

/*@
   via_rdma_finalize - finalize via_rdma method

   Notes:
@*/
int via_rdma_finalize( void )
{
    MPIDI_STATE_DECL(MPID_STATE_VIA_RDMA_FINALIZE);
    MPIDI_FUNC_ENTER(MPID_STATE_VIA_RDMA_FINALIZE);
    MPIDI_FUNC_EXIT(MPID_STATE_VIA_RDMA_FINALIZE);
    return MPI_SUCCESS;
}
