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
    MM_ENTER_FUNC(PACKER_INIT);

    MPID_Process.packer_vc_ptr = mm_vc_alloc(MM_PACKER_METHOD);

    MM_EXIT_FUNC(PACKER_INIT);
    return MPI_SUCCESS;
}

/*@
   packer_finalize - finalize the packer method

   Notes:
@*/
int packer_finalize()
{
    MM_ENTER_FUNC(PACKER_FINALIZE);

    if (MPID_Process.packer_vc_ptr != NULL)
    {
	mm_vc_free(MPID_Process.packer_vc_ptr);
	MPID_Process.packer_vc_ptr = NULL;
    }

    MM_EXIT_FUNC(PACKER_FINALIZE);
    return MPI_SUCCESS;
}
