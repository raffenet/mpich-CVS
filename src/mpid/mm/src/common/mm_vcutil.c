/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "blockallocator.h"

static BlockAllocator MM_VCTable_allocator;
static BlockAllocator MM_Connector_vc_allocator;
static BlockAllocator MM_Packer_vc_allocator;
static BlockAllocator MM_Unpacker_vc_allocator;

void mm_vcutil_init()
{
    MM_VCTable_allocator = BlockAllocInit(sizeof(MPIDI_VCRT), 100, 100, malloc, free);
    MM_Connector_vc_allocator = BlockAllocInit(sizeof(MPIDI_VC), 100, 100, malloc, free);
    MM_Packer_vc_allocator = BlockAllocInit(sizeof(MPIDI_VC), 100, 100, malloc, free);
    MM_Unpacker_vc_allocator = BlockAllocInit(sizeof(MPIDI_VC), 100, 100, malloc, free);
}

void mm_vcutil_finalize()
{
    BlockAllocFinalize(&MM_VCTable_allocator);
    BlockAllocFinalize(&MM_Connector_vc_allocator);
    BlockAllocFinalize(&MM_Packer_vc_allocator);
    BlockAllocFinalize(&MM_Unpacker_vc_allocator);
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

int mm_connector_vc_alloc(MPID_Comm *comm_ptr, int rank)
{
    MPIDI_VC *vc_ptr;
    vc_ptr = (MPIDI_VC*)BlockAlloc(MM_Connector_vc_allocator);
    vc_ptr->rank = rank;
    vc_ptr->pmi_kvsname = comm_ptr->mm.pmi_kvsname;
    vc_ptr->write = mm_connector_write;
    vc_ptr->read = mm_connector_read;
    vc_ptr->method = MM_CONNECTOR_METHOD;
    vc_ptr->ref_count = 1;
#ifdef MPICH_DEV_BUILD
    vc_ptr->recvq = NULL;
    vc_ptr->writeq_head = NULL;
    vc_ptr->writeq_tail = NULL;
#endif
    comm_ptr->vcr[rank] = vc_ptr;
    return MPI_SUCCESS;
}

int mm_connector_vc_free(MPIDI_VC *ptr)
{
    BlockFree(MM_Connector_vc_allocator, ptr);
    return MPI_SUCCESS;
}

MPIDI_VC *mm_packer_vc_alloc()
{
    MPIDI_VC *vc_ptr;
    vc_ptr = (MPIDI_VC*)BlockAlloc(MM_Packer_vc_allocator);
    vc_ptr->write = mm_packer_write;
    vc_ptr->read = mm_packer_read;
    vc_ptr->method = MM_PACKER_METHOD;
    vc_ptr->ref_count = 1;
#ifdef MPICH_DEV_BUILD
    vc_ptr->pmi_kvsname[0] = '\0';
    vc_ptr->rank = -1;
    vc_ptr->recvq = NULL;
    vc_ptr->writeq_head = NULL;
    vc_ptr->writeq_tail = NULL;
#endif
    return vc_ptr;
}

int mm_packer_vc_free(MPIDI_VC *ptr)
{
    BlockFree(MM_Packer_vc_allocator, ptr);
    return MPI_SUCCESS;
}

MPIDI_VC *mm_unpacker_vc_alloc()
{
    MPIDI_VC *vc_ptr;
    vc_ptr = (MPIDI_VC*)BlockAlloc(MM_Unpacker_vc_allocator);
    vc_ptr->write = mm_packer_write;
    vc_ptr->read = mm_packer_read;
    vc_ptr->method = MM_UNPACKER_METHOD;
    vc_ptr->ref_count = 1;
#ifdef MPICH_DEV_BUILD
    vc_ptr->pmi_kvsname[0] = '\0';
    vc_ptr->rank = -1;
    vc_ptr->recvq = NULL;
    vc_ptr->writeq_head = NULL;
    vc_ptr->writeq_tail = NULL;
#endif
    return vc_ptr;
}

int mm_unpacker_vc_free(MPIDI_VC *ptr)
{
    BlockFree(MM_Unpacker_vc_allocator, ptr);
    return MPI_SUCCESS;
}
