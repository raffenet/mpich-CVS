/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   mm_vcutil_init - initialize vc stuff

   Notes:
@*/
void mm_vcutil_init()
{
    MPID_Process.VCTable_allocator = BlockAllocInit(sizeof(MPIDI_VCRT), 100, 100, malloc, free);
    MPID_Process.VC_allocator = BlockAllocInit(sizeof(MPIDI_VC), 100, 100, malloc, free);
}

/*@
   mm_vcutil_finalize - finalize vc stuff

   Notes:
@*/
void mm_vcutil_finalize()
{
    BlockAllocFinalize(&MPID_Process.VCTable_allocator);
    BlockAllocFinalize(&MPID_Process.VC_allocator);
}

/*@
   MPID_VCRT_Create - create a vc reference table

   Parameters:
+  int size - size
-  MPID_VCRT *vcrt_ptr - pointer to a reference table

   Notes:
@*/
int MPID_VCRT_Create(int size, MPID_VCRT *vcrt_ptr)
{
    MPID_VCRT p;

    p = BlockAlloc(MPID_Process.VCTable_allocator);
    p->ref_count = 1;
    p->table_ptr = (MPIDI_VC**)malloc(sizeof(MPIDI_VC*) * size);

    *vcrt_ptr = p;

    return MPI_SUCCESS;
}

/*@
   MPID_VCRT_Add_ref - add reference count

   Parameters:
+  MPID_VCRT vcrt - vc reference table

   Notes:
@*/
int MPID_VCRT_Add_ref(MPID_VCRT vcrt)
{
    if (vcrt == NULL)
	return MPI_ERR_ARG;

    vcrt->ref_count++;

    return MPI_SUCCESS;
}

/*@
   MPID_VCRT_Release - release vc reference table

   Parameters:
+  MPID_VCRT vcrt - vc reference table

   Notes:
@*/
int MPID_VCRT_Release(MPID_VCRT vcrt)
{
    if (vcrt == NULL)
	return MPI_ERR_ARG;

    vcrt->ref_count--;
    if (vcrt->ref_count == 0)
    {
	if (vcrt->table_ptr != NULL)
	    free(vcrt->table_ptr);
	BlockFree(MPID_Process.VCTable_allocator, vcrt);
    }

    return MPI_SUCCESS;
}

/*@
   MPID_VCRT_Get_ptr - get pointer to the array of vc's in the reference table

   Parameters:
+  MPID_VCRT vcrt - vc reference table
-  MPID_VCR **vc_pptr - pointer to the vc array pointer

   Notes:
@*/
int MPID_VCRT_Get_ptr(MPID_VCRT vcrt, MPID_VCR **vc_pptr)
{
    if (vcrt == NULL)
	return MPI_ERR_ARG;

    *vc_pptr = vcrt->table_ptr;

    return MPI_SUCCESS;
}

/*@
   mm_vc_alloc - allocate a virtual connection

   Parameters:
+  MM_METHOD method - method

   Notes:
@*/
MPIDI_VC * mm_vc_alloc(MM_METHOD method)
{
    MPIDI_VC *vc_ptr;

    vc_ptr = (MPIDI_VC*)BlockAlloc(MPID_Process.VC_allocator);
    switch (method)
    {
    case MM_NULL_METHOD:
	break;
    case MM_UNBOUND_METHOD:
	break;
    case MM_CONNECTOR_METHOD:
	vc_ptr->write = mm_connector_connect;
	vc_ptr->read = mm_connector_connect;
	vc_ptr->method = MM_CONNECTOR_METHOD;
	vc_ptr->ref_count = 1;
	vc_ptr->readq_head = NULL;
	vc_ptr->readq_tail = NULL;
	vc_ptr->writeq_head = NULL;
	vc_ptr->writeq_tail = NULL;
#ifdef MPICH_DEV_BUILD
	vc_ptr->read_next_ptr = NULL;
	vc_ptr->write_next_ptr = NULL;
#endif
	break;
    case MM_PACKER_METHOD:
	vc_ptr->write = mm_packer_write;
	vc_ptr->read = mm_packer_read;
	vc_ptr->method = MM_PACKER_METHOD;
	vc_ptr->ref_count = 1;
	vc_ptr->readq_head = NULL;
	vc_ptr->readq_tail = NULL;
	vc_ptr->writeq_head = NULL;
	vc_ptr->writeq_tail = NULL;
#ifdef MPICH_DEV_BUILD
	vc_ptr->pmi_kvsname[0] = '\0';
	vc_ptr->rank = -1;
	vc_ptr->read_next_ptr = NULL;
	vc_ptr->write_next_ptr = NULL;
#endif
	break;
    case MM_UNPACKER_METHOD:
	vc_ptr->write = mm_unpacker_write;
	vc_ptr->read = NULL;
	vc_ptr->method = MM_UNPACKER_METHOD;
	vc_ptr->ref_count = 1;
	vc_ptr->readq_head = NULL;
	vc_ptr->readq_tail = NULL;
	vc_ptr->writeq_head = NULL;
	vc_ptr->writeq_tail = NULL;
#ifdef MPICH_DEV_BUILD
	vc_ptr->pmi_kvsname[0] = '\0';
	vc_ptr->rank = -1;
	vc_ptr->read_next_ptr = NULL;
	vc_ptr->write_next_ptr = NULL;
#endif
	break;
#ifdef WITH_METHOD_SHM
    case MM_SHM_METHOD:
	break;
#endif
#ifdef WITH_METHOD_TCP
    case MM_TCP_METHOD:
	break;
#endif
#ifdef WITH_METHOD_VIA
    case MM_VIA_METHOD:
	break;
#endif
#ifdef WITH_METHOD_VIA_RDMA
    case MM_VIA_RDMA_METHOD:
	break;
#endif
#ifdef WITH_METHOD_NEW
    case MM_NEW_METHOD:
	break;
#endif
    default:
	break;
    }
    return vc_ptr;
}

/*@
   mm_vc_free - free a virtual connection

   Parameters:
+  MPIDI_VC *ptr - vc pointer

   Notes:
@*/
int mm_vc_free(MPIDI_VC *ptr)
{
    BlockFree(MPID_Process.VC_allocator, ptr);
    return MPI_SUCCESS;
}
