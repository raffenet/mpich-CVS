/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*
 * MPIDI_VCRT - virtual connection reference table
 *
 * handle - this element is not used, but exists so that we may use the
 * MPIU_Object routines for reference counting
 *
 * ref_count - number of references to this table
 *
 * vcr_table - array of virtual connection references
 */
typedef struct MPIDI_VCRT
{
    int handle;
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
    MPIU_Object_add_ref(vcrt);
    return MPI_SUCCESS;
}

int MPID_VCRT_Release(MPID_VCRT vcrt)
{
    int count;

    MPIU_Object_release_ref(vcrt, &count);
    if (count == 0)
    {
	MPIU_Free(vcrt);
    }
    return MPI_SUCCESS;
}

int MPID_VCRT_Get_ptr(MPID_VCRT vcrt, MPID_VCR **vc_pptr)
{
    *vc_pptr = vcrt->vcr_table;
    return MPI_SUCCESS;
}

