/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpidimpl.h"

/*@
   unpacker_init - initialize the unpacker method

   Notes:
@*/
int unpacker_init()
{
    MPID_STATE_DECL(MPID_STATE_UNPACKER_INIT);
    MPID_FUNC_ENTER(MPID_STATE_UNPACKER_INIT);

    MPID_Process.unpacker_vc_ptr = mm_vc_alloc(MM_UNPACKER_METHOD);

    MPID_FUNC_EXIT(MPID_STATE_UNPACKER_INIT);
    return MPI_SUCCESS;
}

/*@
   unpacker_finalize - finalize the unpacker method

   Notes:
@*/
int unpacker_finalize()
{
    MPID_STATE_DECL(MPID_STATE_UNPACKER_FINALIZE);
    MPID_FUNC_ENTER(MPID_STATE_UNPACKER_FINALIZE);

    mm_vc_free(MPID_Process.unpacker_vc_ptr);
    MPID_Process.unpacker_vc_ptr = NULL;

    MPID_FUNC_EXIT(MPID_STATE_UNPACKER_FINALIZE);
    return MPI_SUCCESS;
}
