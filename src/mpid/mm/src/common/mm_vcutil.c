/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "pmi.h"

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
    vc_ptr->method = method;
    vc_ptr->ref_count = 1;
    MPID_Thread_lock_init(&vc_ptr->lock);
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
    switch (method)
    {
    case MM_NULL_METHOD:
	break;
    case MM_UNBOUND_METHOD:
	break;
#ifdef WITH_METHOD_SHM
    case MM_SHM_METHOD:
	vc_ptr->merge_post_read = shm_merge_post_read;
	vc_ptr->post_write = shm_post_write;
	break;
#endif
#ifdef WITH_METHOD_TCP
    case MM_TCP_METHOD:
	vc_ptr->data.tcp.bfd = BFD_INVALID_SOCKET;
	vc_ptr->data.tcp.connected = FALSE;
	vc_ptr->data.tcp.connecting = FALSE;
	vc_ptr->post_read = tcp_post_read;
	vc_ptr->merge_post_read = tcp_merge_post_read;
	vc_ptr->post_write = tcp_post_write;
	break;
#endif
#ifdef WITH_METHOD_VIA
    case MM_VIA_METHOD:
	vc_ptr->merge_post_read = via_merge_post_read;
	vc_ptr->post_write = via_post_write;
	break;
#endif
#ifdef WITH_METHOD_VIA_RDMA
    case MM_VIA_RDMA_METHOD:
	vc_ptr->merge_post_read = via_rdma_merge_post_read;
	vc_ptr->post_write = via_rdma_post_write;
	break;
#endif
#ifdef WITH_METHOD_NEW
    case MM_NEW_METHOD:
	vc_ptr->merge_post_read = new_merge_post_read;
	vc_ptr->post_write = new_post_write;
	break;
#endif
    default:
	break;
    }
    return vc_ptr;
}

/*@
mm_vc_connect_alloc - allocate a new vc and post a connect to its method

  Parameters:
  +  char *kvs_name - kvs name
  -  int rank - rank in the kvs database
  
    Notes:
@*/
MPIDI_VC * mm_vc_connect_alloc(char *kvs_name, int rank)
{
    MPIDI_VC *vc_ptr;
    char key[100];
    char *value;
    int value_len;
    char methods[256];
#ifdef WITH_METHOD_VIA
    char *temp;
#endif
    
    value_len = PMI_KVS_Get_value_length_max();
    value = (char*)malloc(value_len);
    
    snprintf(key, 100, "businesscard:%d", rank);
    PMI_KVS_Get(kvs_name, key, methods);
    
    /* choose method */
    
    /* match tcp first so I can test the tcp method */
    /* begin test code ****************/
    if (strstr(methods, "tcp"))
    {
	/* get the tcp method business card */
	snprintf(key, 100, "business_card_tcp:%d", rank);
	PMI_KVS_Get(kvs_name, key, value);
	
	/* check to see if we can connect with this business card */
	if (tcp_can_connect(value))
	{
	    /* allocate a vc for this method */
	    vc_ptr = mm_vc_alloc(MM_TCP_METHOD);
	    /* copy the kvs name and rank into the vc. this may not be necessary */
	    strcpy(vc_ptr->pmi_kvsname, kvs_name);
	    vc_ptr->rank = rank;
	    /* post a connection request to the method */
	    tcp_post_connect(vc_ptr, value);
	    
	    free(value);
	    return vc_ptr;
	}
    }
    /* end test code ******************/
    
#ifdef WITH_METHOD_SHM
    if (strstr(methods, "shm"))
    {
	/* get the shm method business card */
	snprintf(key, 100, "business_card_shm:%d", rank);
	PMI_KVS_Get(kvs_name, key, value);
	
	/* check to see if we can connect with this business card */
	if (shm_can_connect(value))
	{
	    /* allocate a vc for this method */
	    vc_ptr = mm_vc_alloc(MM_SHM_METHOD);
	    /* copy the kvs name and rank into the vc. this may not be necessary */
	    strcpy(vc_ptr->pmi_kvsname, kvs_name);
	    vc_ptr->rank = rank;
	    /* post a connection request to the method */
	    shm_post_connect(vc_ptr, value);
	    
	    free(value);
	    return vc_ptr;
	}
    }
#endif
    
#ifdef WITH_METHOD_VIA_RDMA
    if (strstr(methods, "via_rdma"))
    {
	/* get the via method business card */
	snprintf(key, 100, "business_card_via_rdma:%d", rank);
	PMI_KVS_Get(kvs_name, key, value);
	
	/* check to see if we can connect with this business card */
	if (via_rdma_can_connect(value))
	{
	    /* allocate a vc for this method */
	    vc_ptr = mm_vc_alloc(MM_VIA_RDMA_METHOD);
	    /* copy the kvs name and rank into the vc. this may not be necessary */
	    strcpy(vc_ptr->pmi_kvsname, kvs_name);
	    vc_ptr->rank = rank;
	    /* post a connection request to the method */
	    via_rdma_post_connect(vc_ptr, value);
	    
	    free(value);
	    return vc_ptr;
	}
    }
#endif
    
#ifdef WITH_METHOD_VIA
    /* check for a false match with the via_rdma method */
    temp = strstr(methods, "via_rdma");
    if (temp != NULL)
	*temp = 'x';
    if (strstr(methods, "via"))
    {
	/* get the via rdma method business card */
	snprintf(key, 100, "business_card_via:%d", rank);
	PMI_KVS_Get(kvs_name, key, value);
	
	/* check to see if we can connect with this business card */
	if (via_can_connect(value))
	{
	    /* allocate a vc for this method */
	    vc_ptr = mm_vc_alloc(MM_VIA_METHOD);
	    /* copy the kvs name and rank into the vc. this may not be necessary */
	    strcpy(vc_ptr->pmi_kvsname, kvs_name);
	    vc_ptr->rank = rank;
	    /* post a connection request to the method */
	    via_post_connect(vc_ptr, value);
	    
	    free(value);
	    return vc_ptr;
	}
    }
#endif
    
#ifdef WITH_METHOD_TCP
    if (strstr(methods, "tcp"))
    {
	/* get the tcp method business card */
	snprintf(key, 100, "business_card_tcp:%d", rank);
	PMI_KVS_Get(kvs_name, key, value);
	
	/* check to see if we can connect with this business card */
	if (tcp_can_connect(value))
	{
	    /* allocate a vc for this method */
	    vc_ptr = mm_vc_alloc(MM_TCP_METHOD);
	    /* copy the kvs name and rank into the vc. this may not be necessary */
	    strcpy(vc_ptr->pmi_kvsname, kvs_name);
	    vc_ptr->rank = rank;
	    /* post a connection request to the method */
	    tcp_post_connect(vc_ptr, value);
	    
	    free(value);
	    return vc_ptr;
	}
    }
#endif

    free(value);
    return NULL;
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

