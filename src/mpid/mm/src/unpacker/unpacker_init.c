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
    MM_ENTER_FUNC(UNPACKER_INIT);

    MPID_Process.unpacker_vc_ptr = mm_vc_alloc(MM_UNPACKER_METHOD);

    MM_EXIT_FUNC(UNPACKER_INIT);
    return MPI_SUCCESS;
}

/*@
   unpacker_finalize - finalize the unpacker method

   Notes:
@*/
int unpacker_finalize()
{
    MM_ENTER_FUNC(UNPACKER_FINALIZE);

    mm_vc_free(MPID_Process.unpacker_vc_ptr);

    MM_EXIT_FUNC(UNPACKER_FINALIZE);
    return MPI_SUCCESS;
}
