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
    MPID_Process.unpacker_vc_ptr = mm_vc_alloc(MM_UNPACKER_METHOD);
    return MPI_SUCCESS;
}

/*@
   unpacker_finalize - finalize the unpacker method

   Notes:
@*/
int unpacker_finalize()
{
    mm_vc_free(MPID_Process.unpacker_vc_ptr);
    return MPI_SUCCESS;
}
