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

#ifdef USE_IB_IBAL

#include "ibuimpl.ibal.h"

int g_offset = 0;

ibuBlockAllocator ibuBlockAllocInit(unsigned int blocksize, int count, int incrementsize, void *(* alloc_fn)(size_t size), void (* free_fn)(void *p))
{
    ibuBlockAllocator p;
    void **ppVoid;
    int i;

    p = alloc_fn( sizeof(struct ibuBlockAllocator_struct) + ((blocksize + sizeof(void**)) * count) );

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

ibuQueue_t * ibuBlockAllocInitIB()
{
    ibuQueue_t *q;
    int i;
    ibuBlock_t b[2];
    ib_mr_handle_t handle;
    uint32_t lkey;
    uint32_t rkey;

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
    g_offset = (char*)&b[1].data - (char*)&b[1];
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
    ib_free_deregister(p);
}

void * ibuBlockAlloc(ibuBlockAllocator p)
{
    void *pVoid;
    
    if (p->pNextFree == NULL)
    {
	MPIU_DBG_PRINTF(("ibuBlockAlloc returning NULL\n"));
	return NULL;
    }

    pVoid = p->pNextFree + 1;
    p->pNextFree = *(p->pNextFree);

    return pVoid;
}

void * ibuBlockAllocIB(ibuQueue_t * p)
{
    void *pVoid;

    if (p->pNextFree == NULL)
    {
	MPIU_DBG_PRINTF(("ibuBlockAlloc returning NULL\n"));
	return NULL;
    }

    pVoid = p->pNextFree->data;
    p->pNextFree = p->pNextFree->next;

    return pVoid;
}

int ibuBlockFree(ibuBlockAllocator p, void *pBlock)
{
    ((void**)pBlock)--;
    *((void**)pBlock) = p->pNextFree;
    p->pNextFree = pBlock;

    return 0;
}

int ibuBlockFreeIB(ibuQueue_t * p, void *pBlock)
{
    ibuBlock_t *b;

    b = (ibuBlock_t *)((char *)pBlock - g_offset);
    b->next = p->pNextFree;
    p->pNextFree = b;
    return 0;
}

#endif /* USE_IB_VAPI */
