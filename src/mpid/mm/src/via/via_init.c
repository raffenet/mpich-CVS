/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "viaimpl.h"

VIA_PerProcess VIA_Process;

/*@
   via_init - initialize via method

   Notes:
@*/
int via_init( void )
{
    MPIDI_STATE_DECL(MPID_STATE_VIA_INIT);
    MPIDI_FUNC_ENTER(MPID_STATE_VIA_INIT);
    MPIDI_FUNC_EXIT(MPID_STATE_VIA_INIT);
    return MPI_SUCCESS;
}

/*@
   via_finalize - finalize via method

   Notes:
@*/
int via_finalize( void )
{
    MPIDI_STATE_DECL(MPID_STATE_VIA_FINALIZE);
    MPIDI_FUNC_ENTER(MPID_STATE_VIA_FINALIZE);
    MPIDI_FUNC_EXIT(MPID_STATE_VIA_FINALIZE);
    return MPI_SUCCESS;
}
