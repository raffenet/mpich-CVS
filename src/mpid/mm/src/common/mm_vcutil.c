/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "blockallocator.h"

static BlockAllocator MM_VCTable_allocator;

void mm_vctable_init()
{
    MM_VCTable_allocator = BlockAllocInit(sizeof(MPIDI_VCRT), 100, 100, malloc, free);
}

void mm_vctable_finalize()
{
    BlockAllocFinalize(&MM_VCTable_allocator);
}

int MPID_VCRT_Create(int size, MPID_VCRT *vcrt_ptr)
{
    MPID_VCRT p;

    p = BlockAlloc(MM_VCTable_allocator);
    p->ref_count = 1;
    p->table_ptr = (MPIDI_VC**)malloc(sizeof(MPIDI_VC*) * size);

    *vcrt_ptr = p;

    return MPI_SUCCESS;
}

int MPID_VCRT_Add_ref(MPID_VCRT vcrt)
{
    if (vcrt == NULL)
	return MPI_ERR_ARG;

    vcrt->ref_count++;

    return MPI_SUCCESS;
}

int MPID_VCRT_Release(MPID_VCRT vcrt)
{
    if (vcrt == NULL)
	return MPI_ERR_ARG;

    vcrt->ref_count--;
    if (vcrt->ref_count == 0)
    {
	if (vcrt->table_ptr != NULL)
	    free(vcrt->table_ptr);
	BlockFree(MM_VCTable_allocator, vcrt);
    }

    return MPI_SUCCESS;
}

int MPID_VCRT_Get_ptr(MPID_VCRT vcrt, MPID_VCR **vc_pptr)
{
    if (vcrt == NULL)
	return MPI_ERR_ARG;

    *vc_pptr = vcrt->table_ptr;

    return MPI_SUCCESS;
}
