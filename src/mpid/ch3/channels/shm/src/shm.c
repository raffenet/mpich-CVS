/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include <stdio.h>

struct shmBlockAllocator_struct
{
    void **pNextFree;
    void *(* alloc_fn)(size_t size);
    void (* free_fn)(void *p);
    struct shmBlockAllocator_struct *pNextAllocation;
    unsigned int nBlockSize;
    int nCount, nIncrementSize;
};

typedef struct shmBlockAllocator_struct * shmBlockAllocator;

typedef int SHM_STATE;
#define SHM_ACCEPTING  0x0001
#define SHM_ACCEPTED   0x0002
#define SHM_CONNECTING 0x0004
#define SHM_READING    0x0008
#define SHM_WRITING    0x0010

typedef struct shm_buffer
{
    int use_iov;
    unsigned int num_bytes;
    void *buffer;
    unsigned int bufflen;
    MPID_IOV iov[MPID_IOV_LIMIT];
    int iovlen;
    int index;
    int total;
    int (*progress_update)(int,void*);
} shm_buffer;

typedef struct shm_unex_read_t
{
    void *mem_ptr;
    unsigned char *buf;
    unsigned int length;
    struct shm_unex_read_t *next;
} shm_unex_read_t;

typedef struct shm_state_t
{
    SHM_STATE state;
    shmBlockAllocator allocator;

    int closing;
    int pending_operations;
    /* read and write structures */
    shm_buffer read;
    shm_unex_read_t *unex_list;
    shm_buffer write;
    /* user pointer */
    void *user_ptr;
    /* unexpected queue pointer */
    struct shm_state_t *unex_finished_queue;
} shm_state_t;

#define SHM_ERROR_MSG_LENGTH       255
#define SHM_PACKET_SIZE            (1024 * 64)
#define SHM_PACKET_COUNT           64
#define SHM_NUM_PREPOSTED_RECEIVES (SHM_ACK_WATER_LEVEL*2)
#define SHM_MAX_CQ_ENTRIES         255
#define SHM_MAX_POSTED_SENDS       8192
#define SHM_MAX_DATA_SEGMENTS      100
#define SHM_ACK_WATER_LEVEL        16

typedef struct SHM_Global {
         shm_state_t * unex_finished_list;
		   int error;
		  char err_msg[SHM_ERROR_MSG_LENGTH];
} SHM_Global;

SHM_Global SHM_Process;

#define DEFAULT_NUM_RETRIES 10

typedef struct shm_num_written_t
{
    void *mem_ptr;
    int length;
} shm_num_written_t;

static shm_num_written_t g_num_bytes_written_stack[SHM_MAX_POSTED_SENDS];
static int g_cur_write_stack_index = 0;

/* local prototypes */
static int shmi_post_receive(shm_t shm);
/*static int shmi_post_receive_unacked(shm_t shm);*/
static int shmi_post_write(shm_t shm, void *buf, int len, int (*write_progress_update)(int, void*));
static int shmi_post_writev(shm_t shm, MPID_IOV *iov, int n, int (*write_progress_update)(int, void*));
static int shmi_post_ack_write(shm_t shm);

/* utility allocator functions */

static shmBlockAllocator shmBlockAllocInit(unsigned int blocksize, int count, int incrementsize, void *(* alloc_fn)(unsigned int size), void (* free_fn)(void *p));
static int shmBlockAllocFinalize(shmBlockAllocator *p);
static void * shmBlockAlloc(shmBlockAllocator p);
static int shmBlockFree(shmBlockAllocator p, void *pBlock);

static shmBlockAllocator shmBlockAllocInit(unsigned int blocksize, int count, int incrementsize, void *(* alloc_fn)(unsigned int size), void (* free_fn)(void *p))
{
    shmBlockAllocator p;
    void **ppVoid;
    int i;

    p = alloc_fn( sizeof(struct shmBlockAllocator_struct) + ((blocksize + sizeof(void**)) * count) );

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

static int shmBlockAllocFinalize(shmBlockAllocator *p)
{
    if (*p == NULL)
	return 0;
    shmBlockAllocFinalize(&(*p)->pNextAllocation);
    if ((*p)->free_fn != NULL)
	(*p)->free_fn(*p);
    *p = NULL;
    return 0;
}

static void * shmBlockAlloc(shmBlockAllocator p)
{
    void *pVoid;
    
    if (p->pNextFree == NULL)
    {
	MPIU_DBG_PRINTF(("shmBlockAlloc returning NULL\n"));
	return NULL;
    }

    pVoid = p->pNextFree + 1;
    p->pNextFree = *(p->pNextFree);

    return pVoid;
}

static int shmBlockFree(shmBlockAllocator p, void *pBlock)
{
    ((void**)pBlock)--;
    *((void**)pBlock) = p->pNextFree;
    p->pNextFree = pBlock;

    return 0;
}

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

/* shm functions */

static int shmi_post_receive(shm_t shm)
{
    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_write
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_write(shm_t shm, void *buf, int len)
{
    int total = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_WRITE);

    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));
    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WRITE);
    return total;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_writev
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_writev(shm_t shm, MPID_IOV *iov, int n)
{
    int total = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_WRITEV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_WRITEV);

    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));
    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WRITEV);
    return total;
}

static shmBlockAllocator g_StateAllocator;

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_init()
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_INIT);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_INIT);
    return SHM_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_finalize()
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_FINALIZE);
    shmBlockAllocFinalize(&g_StateAllocator);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_FINALIZE);
    return SHM_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME shmi_buffer_unex_read
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int shmi_buffer_unex_read(shm_t shm, void *mem_ptr, unsigned int offset, unsigned int num_bytes)
{
    shm_unex_read_t *p;
    MPIDI_STATE_DECL(MPID_STATE_SHMI_BUFFER_UNEX_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_SHMI_BUFFER_UNEX_READ);

    MPIDI_DBG_PRINTF((60, FCNAME, "%d bytes\n", num_bytes));

    p = (shm_unex_read_t *)malloc(sizeof(shm_unex_read_t));
    p->mem_ptr = mem_ptr;
    p->buf = (unsigned char *)mem_ptr + offset;
    p->length = num_bytes;
    p->next = shm->unex_list;
    shm->unex_list = p;

    MPIDI_FUNC_EXIT(MPID_STATE_SHMI_BUFFER_UNEX_READ);
    return SHM_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME shmi_read_unex
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int shmi_read_unex(shm_t shm)
{
    unsigned int len;
    shm_unex_read_t *temp;
    MPIDI_STATE_DECL(MPID_STATE_SHMI_READ_UNEX);

    MPIDI_FUNC_ENTER(MPID_STATE_SHMI_READ_UNEX);

    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));
    assert(shm->unex_list);

    /* copy the received data */
    while (shm->unex_list)
    {
	len = min(shm->unex_list->length, shm->read.bufflen);
	memcpy(shm->read.buffer, shm->unex_list->buf, len);
	/* advance the user pointer */
	shm->read.buffer = (char*)(shm->read.buffer) + len;
	shm->read.bufflen -= len;
	shm->read.total += len;
	if (len != shm->unex_list->length)
	{
	    shm->unex_list->length -= len;
	    shm->unex_list->buf += len;
	}
	else
	{
	    /* put the receive packet back in the pool */
	    if (shm->unex_list->mem_ptr == NULL)
	    {
		err_printf("shmi_read_unex: mem_ptr == NULL\n");
	    }
	    assert(shm->unex_list->mem_ptr != NULL);
	    shmBlockFree(shm->allocator, shm->unex_list->mem_ptr);
	    /* free the unexpected data node */
	    temp = shm->unex_list;
	    shm->unex_list = shm->unex_list->next;
	    free(temp);
	    /* post another receive to replace the consumed one */
	    shmi_post_receive(shm);
	}
	/* check to see if the entire message was received */
	if (shm->read.bufflen == 0)
	{
	    /* place this shm in the finished list so it will be completed by shm_wait */
	    shm->state &= ~SHM_READING;
	    shm->unex_finished_queue = SHM_Process.unex_finished_list;
	    SHM_Process.unex_finished_list = shm;
	    /* post another receive to replace the consumed one */
	    shmi_post_receive(shm);
	    MPIDI_FUNC_EXIT(MPID_STATE_SHMI_READ_UNEX);
	    return SHM_SUCCESS;
	}
	/* make the user upcall */
	/*
	if (shm->read.progress_update != NULL)
	shm->read.progress_update(num_bytes, shm->user_ptr);
	*/
    }
    MPIDI_FUNC_EXIT(MPID_STATE_SHMI_READ_UNEX);
    return SHM_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME shmi_readv_unex
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int shmi_readv_unex(shm_t shm)
{
    unsigned int num_bytes;
    shm_unex_read_t *temp;
    MPIDI_STATE_DECL(MPID_STATE_SHMI_READV_UNEX);

    MPIDI_FUNC_ENTER(MPID_STATE_SHMI_READV_UNEX);

    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));

    while (shm->unex_list)
    {
	while (shm->unex_list->length && shm->read.iovlen)
	{
	    num_bytes = min(shm->unex_list->length, shm->read.iov[shm->read.index].MPID_IOV_LEN);
	    MPIDI_DBG_PRINTF((60, FCNAME, "copying %d bytes\n", num_bytes));
	    /* copy the received data */
	    memcpy(shm->read.iov[shm->read.index].MPID_IOV_BUF, shm->unex_list->buf, num_bytes);
	    shm->read.total += num_bytes;
	    shm->unex_list->buf += num_bytes;
	    shm->unex_list->length -= num_bytes;
	    /* update the iov */
	    shm->read.iov[shm->read.index].MPID_IOV_LEN -= num_bytes;
	    shm->read.iov[shm->read.index].MPID_IOV_BUF = 
		(char*)(shm->read.iov[shm->read.index].MPID_IOV_BUF) + num_bytes;
	    if (shm->read.iov[shm->read.index].MPID_IOV_LEN == 0)
	    {
		shm->read.index++;
		shm->read.iovlen--;
	    }
	}

	if (shm->unex_list->length == 0)
	{
	    /* put the receive packet back in the pool */
	    if (shm->unex_list->mem_ptr == NULL)
	    {
		err_printf("shmi_read_unex: mem_ptr == NULL\n");
	    }
	    assert(shm->unex_list->mem_ptr != NULL);
	    MPIDI_DBG_PRINTF((60, FCNAME, "shmBlockFree(mem_ptr)"));
	    shmBlockFree(shm->allocator, shm->unex_list->mem_ptr);
	    /* free the unexpected data node */
	    temp = shm->unex_list;
	    shm->unex_list = shm->unex_list->next;
	    free(temp);
	    /* replace the consumed read descriptor */
	    shmi_post_receive(shm);
	}
	
	if (shm->read.iovlen == 0)
	{
	    shm->state &= ~SHM_READING;
	    shm->unex_finished_queue = SHM_Process.unex_finished_list;
	    SHM_Process.unex_finished_list = shm;
	    MPIDI_DBG_PRINTF((60, FCNAME, "finished read saved in SHM_Process.unex_finished_list\n"));
	    MPIDI_FUNC_EXIT(MPID_STATE_SHMI_READV_UNEX);
	    return SHM_SUCCESS;
	}
	/* make the user upcall */
	/*
	if (shm->read.progress_update != NULL)
	shm->read.progress_update(num_bytes, shm->user_ptr);
	*/
    }
    MPIDI_FUNC_EXIT(MPID_STATE_SHMI_READV_UNEX);
    return SHM_SUCCESS;
}

#if 0
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_wait
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_wait(shm_set_t set, int millisecond_timeout, shm_wait_t *out)
{
    int i;
    ib_uint32_t status;
    ib_work_completion_t completion_data;
    void *mem_ptr;
    char *iter_ptr;
    shm_t shm;
    int num_bytes;
    unsigned int offset;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_WAIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_WAIT);
    /*MPIDI_DBG_PRINTF((60, FCNAME, "entering"));*/
    for (;;) 
    {
	if (SHM_Process.unex_finished_list)
	{
	    MPIDI_DBG_PRINTF((60, FCNAME, "returning previously received %d bytes", SHM_Process.unex_finished_list->read.total));
	    /* remove this shm from the finished list */
	    shm = SHM_Process.unex_finished_list;
	    SHM_Process.unex_finished_list = SHM_Process.unex_finished_list->unex_finished_queue;
	    shm->unex_finished_queue = NULL;

	    out->num_bytes = shm->read.total;
	    out->op_type = SHM_OP_READ;
	    out->user_ptr = shm->user_ptr;
	    shm->pending_operations--;
	    if (shm->closing && shm->pending_operations == 0)
	    {
		shm = SHM_INVALID_QP;
	    }
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WAIT);
	    return SHM_SUCCESS;
	}

	status = ib_completion_poll_us(
	    SHM_Process.hca_handle,
	    set,
	    &completion_data);
	if (status == IBA_CQ_EMPTY)
	{
	    /* shm_wait polls until there is something in the queue */
	    /* or the timeout has expired */
	    if (millisecond_timeout == 0)
	    {
		out->num_bytes = 0;
		out->error = 0;
		out->user_ptr = NULL;
		out->op_type = SHM_OP_TIMEOUT;
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WAIT);
		return SHM_SUCCESS;
	    }
	    continue;
	}
	if (status != IBA_OK)
	{
	    err_printf("%s: error: ib_completion_poll_us did not return IBA_OK\n", FCNAME);
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WAIT);
	    return SHM_FAIL;
	}
	if (completion_data.status != IB_COMP_ST_SUCCESS)
	{
	    err_printf("%s: error: status = %d != IB_COMP_ST_SUCCESS, %s\n", 
		FCNAME, completion_data.status, iba_compstr(completion_data.status));
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WAIT);
	    return SHM_FAIL;
	}

	shm = (shm_t)(((shm_work_id_handle_t*)&completion_data.work_req_id)->data.ptr);
	mem_ptr = (void*)(((shm_work_id_handle_t*)&completion_data.work_req_id)->data.mem);

	switch (completion_data.op_type)
	{
	case OP_SEND:
	    if (completion_data.immediate_data_f || (int)mem_ptr == -1)
	    {
		/* flow control ack completed, no user data so break out here */
		break;
	    }
	    g_cur_write_stack_index--;
	    num_bytes = g_num_bytes_written_stack[g_cur_write_stack_index].length;
	    MPIDI_DBG_PRINTF((60, FCNAME, "send num_bytes = %d\n", num_bytes));
	    if (num_bytes < 0)
	    {
		i = num_bytes;
		num_bytes = 0;
		for (; i<0; i++)
		{
		    g_cur_write_stack_index--;
		    MPIDI_DBG_PRINTF((60, FCNAME, "num_bytes += %d\n", g_num_bytes_written_stack[g_cur_write_stack_index].length));
		    num_bytes += g_num_bytes_written_stack[g_cur_write_stack_index].length;
		    if (g_num_bytes_written_stack[g_cur_write_stack_index].mem_ptr == NULL)
			err_printf("shm_wait: write stack has NULL mem_ptr at location %d\n", g_cur_write_stack_index);
		    assert(g_num_bytes_written_stack[g_cur_write_stack_index].mem_ptr != NULL);
		    shmBlockFree(shm->allocator, g_num_bytes_written_stack[g_cur_write_stack_index].mem_ptr);
		}
	    }
	    else
	    {
		if (mem_ptr == NULL)
		    err_printf("shm_wait: send mem_ptr == NULL\n");
		assert(mem_ptr != NULL);
		shmBlockFree(shm->allocator, mem_ptr);
	    }

	    out->num_bytes = num_bytes;
	    out->op_type = SHM_OP_WRITE;
	    out->user_ptr = shm->user_ptr;
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WAIT);
	    return SHM_SUCCESS;
	    break;
	case OP_RECEIVE:
	    if (completion_data.immediate_data_f)
	    {
		shm->nAvailRemote += completion_data.immediate_data;
		MPIDI_DBG_PRINTF((60, FCNAME, "%d packets acked, nAvailRemote now = %d", completion_data.immediate_data, shm->nAvailRemote));
		shmBlockFree(shm->allocator, mem_ptr);
		shmi_post_receive_unacked(shm);
		assert(completion_data.bytes_num == 0); /* check this after the printfs to see if the immediate data is correct */
		break;
	    }
	    num_bytes = completion_data.bytes_num;
	    MPIDI_DBG_PRINTF((60, FCNAME, "read %d bytes\n", num_bytes));
	    /*MPIDI_DBG_PRINTF((60, FCNAME, "shm_wait(recv finished %d bytes)", num_bytes));*/
	    if (!(shm->state & SHM_READING))
	    {
		shmi_buffer_unex_read(shm, mem_ptr, 0, num_bytes);
		break;
	    }
	    MPIDI_DBG_PRINTF((60, FCNAME, "read update, total = %d + %d = %d\n", shm->read.total, num_bytes, shm->read.total + num_bytes));
	    if (shm->read.use_iov)
	    {
		iter_ptr = mem_ptr;
		while (num_bytes && shm->read.iovlen > 0)
		{
		    if ((int)shm->read.iov[shm->read.index].MPID_IOV_LEN <= num_bytes)
		    {
			/* copy the received data */
			memcpy(shm->read.iov[shm->read.index].MPID_IOV_BUF, iter_ptr,
			    shm->read.iov[shm->read.index].MPID_IOV_LEN);
			iter_ptr += shm->read.iov[shm->read.index].MPID_IOV_LEN;
			/* update the iov */
			num_bytes -= shm->read.iov[shm->read.index].MPID_IOV_LEN;
			shm->read.index++;
			shm->read.iovlen--;
		    }
		    else
		    {
			/* copy the received data */
			memcpy(shm->read.iov[shm->read.index].MPID_IOV_BUF, iter_ptr, num_bytes);
			iter_ptr += num_bytes;
			/* update the iov */
			shm->read.iov[shm->read.index].MPID_IOV_LEN -= num_bytes;
			shm->read.iov[shm->read.index].MPID_IOV_BUF = 
			    (char*)(shm->read.iov[shm->read.index].MPID_IOV_BUF) + num_bytes;
			num_bytes = 0;
		    }
		}
		offset = (unsigned char*)iter_ptr - (unsigned char*)mem_ptr;
		shm->read.total += offset;
		if (num_bytes == 0)
		{
		    /* put the receive packet back in the pool */
		    if (mem_ptr == NULL)
			err_printf("shm_wait: read mem_ptr == NULL\n");
		    assert(mem_ptr != NULL);
		    shmBlockFree(shm->allocator, mem_ptr);
		    shmi_post_receive(shm);
		}
		else
		{
		    /* save the unused but received data */
		    shmi_buffer_unex_read(shm, mem_ptr, offset, num_bytes);
		}
		if (shm->read.iovlen == 0)
		{
		    shm->state &= ~SHM_READING;
		    out->num_bytes = shm->read.total;
		    out->op_type = SHM_OP_READ;
		    out->user_ptr = shm->user_ptr;
		    shm->pending_operations--;
		    if (shm->closing && shm->pending_operations == 0)
		    {
			MPIDI_DBG_PRINTF((60, FCNAME, "closing shmet after iov read completed."));
			shm = SHM_INVALID_QP;
		    }
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WAIT);
		    return SHM_SUCCESS;
		}
		/* make the user upcall */
		if (shm->read.progress_update != NULL)
		    shm->read.progress_update(num_bytes, shm->user_ptr);
	    }
	    else
	    {
		if ((unsigned int)num_bytes > shm->read.bufflen)
		{
		    /* copy the received data */
		    memcpy(shm->read.buffer, mem_ptr, shm->read.bufflen);
		    shm->read.total = shm->read.bufflen;
		    shmi_buffer_unex_read(shm, mem_ptr, shm->read.bufflen, num_bytes - shm->read.bufflen);
		    shm->read.bufflen = 0;
		}
		else
		{
		    /* copy the received data */
		    memcpy(shm->read.buffer, mem_ptr, num_bytes);
		    shm->read.total += num_bytes;
		    /* advance the user pointer */
		    shm->read.buffer = (char*)(shm->read.buffer) + num_bytes;
		    shm->read.bufflen -= num_bytes;
		    /* put the receive packet back in the pool */
		    shmBlockFree(shm->allocator, mem_ptr);
		    shmi_post_receive(shm);
		}
		if (shm->read.bufflen == 0)
		{
		    shm->state &= ~SHM_READING;
		    out->num_bytes = shm->read.total;
		    out->op_type = SHM_OP_READ;
		    out->user_ptr = shm->user_ptr;
		    shm->pending_operations--;
		    if (shm->closing && shm->pending_operations == 0)
		    {
			MPIDI_DBG_PRINTF((60, FCNAME, "closing shm after simple read completed."));
			shm = SHM_INVALID_QP;
		    }
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WAIT);
		    return SHM_SUCCESS;
		}
		/* make the user upcall */
		if (shm->read.progress_update != NULL)
		    shm->read.progress_update(num_bytes, shm->user_ptr);
	    }
	    break;
	default:
	    err_printf("%s: unknown ib op_type: %d\n", FCNAME, completion_data.op_type);
	    break;
	}
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WAIT);
}
#endif

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_set_user_ptr
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_set_user_ptr(shm_t shm, void *user_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_SET_USER_PTR);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_SET_USER_PTR);
    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));
    if (shm == SHM_INVALID_T)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_SET_USER_PTR);
	return SHM_FAIL;
    }
    shm->user_ptr = user_ptr;
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_SET_USER_PTR);
    return SHM_SUCCESS;
}

/* non-blocking functions */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_post_read
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_post_read(shm_t shm, void *buf, int len, int (*rfn)(int, void*))
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_POST_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_POST_READ);
    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));
    shm->read.total = 0;
    shm->read.buffer = buf;
    shm->read.bufflen = len;
    shm->read.use_iov = FALSE;
    shm->read.progress_update = rfn;
    shm->state |= SHM_READING;
    shm->pending_operations++;
    /* copy any pre-received data into the buffer */
    if (shm->unex_list)
	shmi_read_unex(shm);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_POST_READ);
    return SHM_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_post_readv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_post_readv(shm_t shm, MPID_IOV *iov, int n, int (*rfn)(int, void*))
{
#ifdef MPICH_DBG_OUTPUT
    char str[1024] = "MPIDI_CH3I_SHM_post_readv: ";
    char *s;
    int i;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_POST_READV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_POST_READV);
#ifdef MPICH_DBG_OUTPUT
    s = &str[16];
    for (i=0; i<n; i++)
    {
	s += sprintf(s, "%d,", iov[i].MPID_IOV_LEN);
    }
    MPIDI_DBG_PRINTF((60, FCNAME, "%s\n", str));
#endif
    shm->read.total = 0;
    /* This isn't necessary if we require the iov to be valid for the duration of the operation */
    /*shm->read.iov = iov;*/
    memcpy(shm->read.iov, iov, sizeof(MPID_IOV) * n);
    shm->read.iovlen = n;
    shm->read.index = 0;
    shm->read.use_iov = TRUE;
    shm->read.progress_update = rfn;
    shm->state |= SHM_READING;
    shm->pending_operations++;
    /* copy any pre-received data into the iov */
    if (shm->unex_list)
	shmi_readv_unex(shm);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_POST_READV);
    return SHM_SUCCESS;
}
