/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "blockallocator.h"

BlockAllocator MM_VCTABLE_allocator;

void mm_vctable_init()
{
    MM_VCTABLE_allocator = BlockAllocInit(sizeof(MM_VCTABLE), 100, 100, malloc, free);
}

void mm_vctable_finalize()
{
    BlockAllocFinalize(&MM_VCTABLE_allocator);
}

MM_VCTABLE* mm_vctable_alloc()
{
    MM_VCTABLE *p;
    p = BlockAlloc(MM_VCTABLE_allocator);
    return p;
}

void mm_vctable_free(MM_VCTABLE *ptr)
{
    BlockFree(MM_VCTABLE_allocator, ptr);
}

/*@
   mm_alloc_vc_table - allocate a virtual connection table for this communicator

   Parameters:
+  MPID_Comm *comm_ptr - communicator

   Notes:
@*/
int mm_alloc_vc_table(MPID_Comm *comm_ptr)
{
    MM_VCTABLE *p;

    p = mm_vctable_alloc();

    p->ref_count = 1;
    p->table_ptr = (MPIDI_VC*)malloc(sizeof(MPIDI_VC) * comm_ptr->size);
    comm_ptr->vcrt = &p->table_ptr;
    comm_ptr->vcrt_ref_count = &p->ref_count;

    return MPI_SUCCESS;
}

/*@
   mm_release_vc_table - release virtual connection table

   Parameters:
+  MM_VCTABLE *p - vc table pointer

   Notes:
@*/
int mm_release_vc_table(MM_VCTABLE *p)
{
    p->ref_count--;
    if (p->ref_count < 1)
    {
	if (p->table_ptr != NULL)
	    free(p->table_ptr);
	mm_vctable_free(p);
    }
    return MPI_SUCCESS;
}
