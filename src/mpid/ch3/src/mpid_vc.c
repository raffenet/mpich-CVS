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
    int size;
    MPIDI_VC * vcr_table[1];
}
MPIDI_VCRT;


int MPID_VCRT_create(int size, MPID_VCRT *vcrt_ptr)
{
    MPIDI_VCRT * vcrt;
    MPIDI_STATE_DECL(MPID_STATE_MPID_VCRT_CREATE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_VCRT_CREATE);
    
    vcrt = MPIU_Malloc(sizeof(MPIDI_VCRT) + (size - 1) * sizeof(MPIDI_VC));
    if (vcrt != NULL)
    {
	vcrt->ref_count = 1;
	vcrt->size = size;
	*vcrt_ptr = vcrt;
	MPIDI_FUNC_EXIT(MPID_STATE_MPID_VCRT_CREATE);
	return MPI_SUCCESS;
    }
    else
    {
	MPIDI_FUNC_EXIT(MPID_STATE_MPID_VCRT_CREATE);
	return MPI_ERR_NOMEM;
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_VCRT_CREATE);
}

int MPID_VCRT_add_ref(MPID_VCRT vcrt)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_VCRT_ADD_REF);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_VCRT_ADD_REF);
    MPIU_Object_add_ref(vcrt);
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_VCRT_ADD_REF);
    return MPI_SUCCESS;
}

int MPID_VCRT_release(MPID_VCRT vcrt)
{
    int count;
    MPIDI_STATE_DECL(MPID_STATE_MPID_VCRT_RELEASE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_VCRT_RELEASE);

    MPIU_Object_release_ref(vcrt, &count);
    if (count == 0)
    {
	int i;

	for (i = 0; i < vcrt->size; i++)
	{
	    MPID_VCR_release(vcrt->vcr_table[i]);
	}
	
	MPIU_Free(vcrt);
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_VCRT_RELEASE);
    return MPI_SUCCESS;
}

int MPID_VCRT_get_ptr(MPID_VCRT vcrt, MPID_VCR **vc_pptr)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_VCRT_GET_PTR);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_VCRT_GET_PTR);
    *vc_pptr = vcrt->vcr_table;
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_VCRT_GET_PTR);
    return MPI_SUCCESS;
}

int MPID_VCR_dup(MPID_VCR orig_vcr, MPID_VCR * new_vcr)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_VCR_DUP);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_VCR_DUP);
    /* MM - need to atomically increment ref_count */
    orig_vcr->ref_count++;
    *new_vcr = orig_vcr;
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_VCR_DUP);
    return MPI_SUCCESS;
}

int MPID_VCR_release(MPID_VCR vcr)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_VCR_RELEASE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_VCR_RELEASE);
    /* MM - need to atomically decrement ref_count */
    vcr->ref_count--;
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_VCR_RELEASE);
    return MPI_SUCCESS;
}

int MPID_VCR_get_lpid(MPID_VCR vcr, int * lpid_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_VCR_GET_LPID);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_VCR_GET_LPID);
    *lpid_ptr = vcr->lpid;
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_VCR_GET_LPID);
    return MPI_SUCCESS;
}


