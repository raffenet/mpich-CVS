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


int MPID_VCRT_create(int size, MPID_VCRT *vcrt_ptr)
{
    MPIDI_VCRT * vcrt;
    
    vcrt = MPIU_Malloc(sizeof(MPIDI_VCRT) + (size - 1) * sizeof(MPIDI_VC));
    if (vcrt != NULL)
    {
	vcrt->ref_count = 1;
	*vcrt_ptr = vcrt;
	return MPI_SUCCESS;
    }
    else
    {
	return MPI_ERR_NOMEM;
    }
}

int MPID_VCRT_add_ref(MPID_VCRT vcrt)
{
    MPIU_Object_add_ref(vcrt);
    return MPI_SUCCESS;
}

int MPID_VCRT_release(MPID_VCRT vcrt)
{
    int count;

    MPIU_Object_release_ref(vcrt, &count);
    if (count == 0)
    {
	MPIU_Free(vcrt);
    }
    return MPI_SUCCESS;
}

int MPID_VCRT_get_ptr(MPID_VCRT vcrt, MPID_VCR **vc_pptr)
{
    *vc_pptr = vcrt->vcr_table;
    return MPI_SUCCESS;
}

int MPID_VCR_dup(MPID_VCR orig_vcr, MPID_VCR * new_vcr)
{
    /* MM - need to atomically increment ref_count */
    orig_vcr->ref_count++;
    *new_vcr = orig_vcr;
    return MPI_SUCCESS;
}

int MPID_VCR_release(MPID_VCR vcr)
{
    /* MM - need to atomically decrement ref_count */
    vcr->ref_count--;
    return MPI_SUCCESS;
}

int MPID_VCR_get_lpid(MPID_VCR vcr, int * lpid_ptr)
{
    *lpid_ptr = vcr->lpid;
    return MPI_SUCCESS;
}


