/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

typedef struct MPIDI_VCRT
{
    volatile int ref_count;
    MPIDI_VC * vcr_table[1];
}
MPIDI_VCRT;


int MPID_VCRT_Create(int size, MPID_VCRT *vcrt_ptr)
{
    MPIDI_VCRT * vcrt;
    
    vcrt = MPIU_Malloc(sizeof(MPIDI_VCRT) + (size - 1) * sizeof(MPIDI_VC));
    assert(vcrt != NULL);

    vcrt->ref_count = 1;
    *vcrt_ptr = vcrt;
    
    return MPI_SUCCESS;
}

int MPID_VCRT_Add_ref(MPID_VCRT vcrt)
{
    /* MT - not atomic */
    vcrt->ref_count += 1;
}

int MPID_VCRT_Release(MPID_VCRT vcrt)
{
    /* MT - not atomic */
    if (--vcrt->ref_count == 0)
    {
	MPIU_Free(vcrt);
    }
}

int MPID_VCRT_Get_ptr(MPID_VCRT vcrt, MPID_VCR **vc_pptr)
{
    *vc_pptr = vcrt->vcr_table;
    return MPI_SUCCESS;
}

