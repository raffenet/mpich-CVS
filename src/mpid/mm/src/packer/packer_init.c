/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpidimpl.h"

/*@
   packer_init - initialize the packer method

   Notes:
@*/
int packer_init()
{
    MPID_Process.packer_vc_ptr = mm_vc_alloc(MM_PACKER_METHOD);
    return MPI_SUCCESS;
}

/*@
   packer_finalize - finalize the packer method

   Notes:
@*/
int packer_finalize()
{
    mm_vc_free(MPID_Process.packer_vc_ptr);
    return MPI_SUCCESS;
}
