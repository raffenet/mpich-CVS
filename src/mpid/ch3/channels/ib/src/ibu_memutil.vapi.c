/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "ibu.h"
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#include "mpidi_ch3_impl.h"

#ifdef USE_IB_VAPI

#include "ibuimpl.vapi.h"

#if 0
typedef struct mem_node_t
{
    void *p;
    struct mem_node_t *next;
} mem_node_t;
mem_node_t *g_pList = NULL;

void add2list(void *p)
{
    mem_node_t *node;

    node = (mem_node_t*)malloc(sizeof(mem_node_t));
    node->p = p;
    node->next = g_pList;
    g_pList = node;
}

void checklist(void *p)
{
    mem_node_t *node;
    node = g_pList;
    while (node)
    {
	if (node->p == p)
	{
	    printf("pointer %p already in global list.\n", p);fflush(stdout);
	}
	node = node->next;
    }
}

void removefromlist(void *p)
{
    mem_node_t *trailer, *iter;
    iter = trailer = g_pList;
    while (iter)
    {
	if (iter->p == p)
	{
	    if (iter == g_pList)
	    {
		g_pList = g_pList->next;
		free(iter);
		return;
	    }
	    else
	    {
		trailer->next = iter->next;
		free(iter);
		return;
	    }
	}
	if (trailer != iter)
	{
	    trailer = trailer->next;
	}
	iter = iter->next;
    }
    printf("remove error: pointer %p not in global list.\n", p);fflush(stdout);
}

void *s_base = NULL;
int s_offset = 0;
void sanity_check_recv(VAPI_rr_desc_t *work_req)
{
    VAPI_sg_lst_entry_t *data;
    int i;

    for (i=0; i<work_req->sg_lst_len; i++)
    {
	data = &work_req->sg_lst_p[i];
	if (data->len < 1 || data->len > IBU_PACKET_SIZE)
	{
	    printf("ERROR: data[%d].len = %d\n", i, data->len);fflush(stdout);
	}
	if ((void*)data->addr < s_base)
	{
	    printf("ERROR: ptr %p < %p base\n", data->addr, s_base);fflush(stdout);
	}
	if (((char*)data->addr + data->len) > ((char*)s_base + s_offset))
	{
	    printf("ERROR: ptr %p + len %d > %p base + %d offset\n",
		   data->addr, data->len, s_base, s_offset);
	    fflush(stdout);
	} 
    }
}
void sanity_check_send(VAPI_sr_desc_t *work_req)
{
    VAPI_sg_lst_entry_t *data;
    int i;

    for (i=0; i<work_req->sg_lst_len; i++)
    {
	data = &work_req->sg_lst_p[i];
	if (data->len < 1 || data->len > IBU_PACKET_SIZE)
	{
	    printf("ERROR: data[%d].len = %d\n", i, data->len);fflush(stdout);
	}
	if ((void*)data->addr < s_base)
	{
	    printf("ERROR: ptr %p < %p base\n", data->addr, s_base);fflush(stdout);
	}
	if (((char*)data->addr + data->len) > ((char*)s_base + s_offset))
	{
	    printf("ERROR: ptr %p + len %d > %p base + %d offset\n",
		   data->addr, data->len, s_base, s_offset);
	    fflush(stdout);
	} 
    }
}
#endif

ibuBlockAllocator ibuBlockAllocInit(unsigned int blocksize, int count, int incrementsize, void *(* alloc_fn)(size_t size), void (* free_fn)(void *p))
{
    ibuBlockAllocator p;
    void **ppVoid;
    int i;

    p = alloc_fn( sizeof(struct ibuBlockAllocator_struct) + ((blocksize + sizeof(void**)) * count) );
    if (p == NULL)
    {
	return NULL;
    }

    p->alloc_fn = alloc_fn;
    p->free_fn = free_fn;
    p->nIncrementSize = incrementsize;
    p->pNextAllocation = NULL;
    p->nCount = count;
    p->nBlockSize = blocksize;
    p->pNextFree = (void**)(p + 1);

    ppVoid = (void**)(p + 1);
    for (i=0; i<count-1; i++)
    {
	*ppVoid = (void*)((char*)ppVoid + sizeof(void**) + blocksize);
	ppVoid = *ppVoid;
    }
    *ppVoid = NULL;

    return p;
}

ibuQueue_t *ibuBlockAllocInitIB()
{
    ibuQueue_t *q;
    int i;
    ibuBlock_t b[2];
    VAPI_mr_hndl_t handle;
    VAPI_lkey_t lkey;
    VAPI_rkey_t rkey;

    q = (ibuQueue_t*)ib_malloc_register(sizeof(ibuQueue_t), &handle, &lkey, &rkey);
    if (q == NULL)
    {
	return NULL;
    }
    q->next_q = NULL;
    for (i=0; i<IBU_PACKET_COUNT; i++)
    {
	q->block[i].next = &q->block[i+1];
	q->block[i].handle = handle;
	q->block[i].lkey = lkey;
    }
    q->block[IBU_PACKET_COUNT-1].next = NULL;
    q->pNextFree = &q->block[0];
    IBU_Process.offset_to_lkey = (long)((char*)&b[1].data - (char*)&b[1]);
    return q;
}

int ibuBlockAllocFinalize(ibuBlockAllocator *p)
{
    if (*p == NULL)
	return 0;
    ibuBlockAllocFinalize(&(*p)->pNextAllocation);
    if ((*p)->free_fn != NULL)
	(*p)->free_fn(*p);
    *p = NULL;
    return 0;
}

int ibuBlockAllocFinalizeIB(ibuQueue_t *p)
{
    if (p == NULL)
	return 0;
    ibuBlockAllocFinalizeIB(p->next_q);
    ib_free_deregister(p/*,p->block->handle*/);
    return 0;
}

void * ibuBlockAlloc(ibuBlockAllocator p)
{
    void *pVoid;

    if (p->pNextFree == NULL)
    {
	ibuBlockAllocator q;
	void **ppVoid;
	int i;

	q = p->alloc_fn( sizeof(struct ibuBlockAllocator_struct) + ((p->nBlockSize + sizeof(void**)) * p->nCount) );
	if (q == NULL)
	{
	    MPIU_DBG_PRINTF(("ibuBlockAlloc returning NULL\n"));
	    return NULL;
	}

	q->alloc_fn = p->alloc_fn;
	q->free_fn = p->free_fn;
	q->nIncrementSize = p->nIncrementSize;
	q->pNextAllocation = NULL;
	q->nCount = p->nCount;
	q->nBlockSize = p->nBlockSize;
	q->pNextFree = (void**)(q + 1);

	ppVoid = (void**)(q + 1);
	for (i=0; i<p->nCount-1; i++)
	{
	    *ppVoid = (void*)((char*)ppVoid + sizeof(void**) + p->nBlockSize);
	    ppVoid = *ppVoid;
	}
	*ppVoid = NULL;

	p->pNextAllocation = q;
	p->pNextFree = q->pNextFree;
    }

    pVoid = p->pNextFree + 1;
    p->pNextFree = *(p->pNextFree);

    return pVoid;
}

void * ibuBlockAllocIB(ibuQueue_t *p)
{
    void *pVoid;

    if (p->pNextFree == NULL)
    {
	ibuQueue_t *q;
	int i;
	ibuBlock_t b[2];
	VAPI_mr_hndl_t handle;
	VAPI_lkey_t lkey;
	VAPI_rkey_t rkey;

	q = (ibuQueue_t*)ib_malloc_register(sizeof(ibuQueue_t), &handle, &lkey, &rkey);
	if (q == NULL)
	{
	    MPIU_DBG_PRINTF(("ibuBlockAllocIB returning NULL\n"));
	    return NULL;
	}
	q->next_q = NULL;
	for (i=0; i<IBU_PACKET_COUNT; i++)
	{
	    q->block[i].next = &q->block[i+1];
	    q->block[i].handle = handle;
	    q->block[i].lkey = lkey;
	}
	q->block[IBU_PACKET_COUNT-1].next = NULL;
	q->pNextFree = &q->block[0];
	IBU_Process.offset_to_lkey = (long)((char*)&b[1].data - (char*)&b[1]);

	p->next_q = q;
	p->pNextFree = q->pNextFree;
    } 
    pVoid = (p->pNextFree->data).alignment; /* Mellanox added by Dafna July 7th 2004*/
    p->pNextFree = p->pNextFree->next;

#if 0
    checklist(pVoid);
    add2list(pVoid);
#endif

    return pVoid;
}

int ibuBlockFree(ibuBlockAllocator p, void *pBlock)
{
    ((void**)pBlock)--;
    *((void**)pBlock) = p->pNextFree;
    p->pNextFree = pBlock;

    return 0;
}

int ibuBlockFreeIB(ibuQueue_t *p, void *pBlock)
{
    ibuBlock_t *b;
#if 0
    removefromlist(pBlock);
    checklist(pBlock);
#endif

    b = (ibuBlock_t *)((char *)pBlock - IBU_Process.offset_to_lkey);
    b->next = p->pNextFree;
    p->pNextFree = b;
    return 0;
}



ibu_rdma_buf_t* ibuRDMAAllocInitIB(ibu_mem_t *mem_handle)
{
    ibu_rdma_buf_t *buf;
    VAPI_lkey_t lkey;
    VAPI_rkey_t rkey;
    VAPI_mr_hndl_t handle;
    buf = (ibu_rdma_buf_t*)ib_malloc_register((sizeof(ibu_rdma_buf_t)*(IBU_NUM_OF_RDMA_BUFS+1)), &handle, &lkey, &rkey);
    /*buf = (ibu_rdma_buf_t*)((UINT_PTR)((unsigned char*)buf + (IBU_RDMA_BUF_SIZE -1))&(~(IBU_RDMA_BUF_SIZE-1)));*/
    buf = (ibu_rdma_buf_t*)
	MPIU_AintToPtr(
	    MPIU_PtrToAint((unsigned char*)buf + (IBU_RDMA_BUF_SIZE -1)) &
	    (MPI_Aint)(~(IBU_RDMA_BUF_SIZE-1)));
    if (buf == NULL)
    {
	return NULL;
    }    
    mem_handle->handle = handle;
    mem_handle->lkey = lkey;
    mem_handle->rkey = rkey;

    return buf;
}

int ibuRDMAAllocFinalizeIB(ibu_rdma_buf_t *buf, VAPI_mr_hndl_t mem_handle)
{
    if (buf == NULL)
	return 0;
    ib_free_deregister(buf/*,mem_handle*/);
    return 0;
}

#endif /* USE_IB_VAPI */
