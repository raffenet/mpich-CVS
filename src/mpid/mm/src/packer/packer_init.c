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
    MPIDI_STATE_DECL(MPID_STATE_PACKER_INIT);
    MPIDI_FUNC_ENTER(MPID_STATE_PACKER_INIT);

    MPID_Process.packer_vc_ptr = mm_vc_alloc(MM_PACKER_METHOD);

    MPIDI_FUNC_EXIT(MPID_STATE_PACKER_INIT);
    return MPI_SUCCESS;
}

/*@
   packer_finalize - finalize the packer method

   Notes:
@*/
int packer_finalize()
{
    MPIDI_STATE_DECL(MPID_STATE_PACKER_FINALIZE);
    MPIDI_FUNC_ENTER(MPID_STATE_PACKER_FINALIZE);

    if (MPID_Process.packer_vc_ptr != NULL)
    {
	mm_vc_free(MPID_Process.packer_vc_ptr);
	MPID_Process.packer_vc_ptr = NULL;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_PACKER_FINALIZE);
    return MPI_SUCCESS;
}
