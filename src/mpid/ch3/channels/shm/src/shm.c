/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include <stdio.h>

typedef int SHM_STATE;
#define SHM_READING    0x0008
#define SHM_WRITING    0x0010

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

/* shm functions */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_write
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_write(MPIDI_VC * vc, void *buf, int len)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_WRITE);
    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));

    if (vc->shm.shm->num_free == 0)
	return 0;
    MPIDU_Process_lock(&vc->shm.shm->lock);
    if (vc->shm.shm->num_free == 0)
    {
	MPIDU_Process_unlock(&vc->shm.shm->lock);
	return 0;
    }
    len = min(len, vc->shm.shm->num_free);
    vc->shm.shm->tail->next = (MPIDI_CH3I_SHM_Packet_t*)((char*)vc->shm.shm->tail + len + sizeof(MPIDI_CH3I_SHM_Packet_t));
    vc->shm.shm->tail->num_bytes = len;
    vc->shm.shm->tail->src = vc->shm.pg_rank; //MPIDI_CH3I_Process.rank;
    memcpy(vc->shm.shm->tail + 1, buf, len);
    vc->shm.shm->tail = (MPIDI_CH3I_SHM_Packet_t*)((char*)vc->shm.shm->tail + len + sizeof(MPIDI_CH3I_SHM_Packet_t));
    vc->shm.shm->num_free -= len + sizeof(MPIDI_CH3I_SHM_Packet_t);
    if (vc->shm.shm->num_free < 0)
	vc->shm.shm->num_free = 0;
    MPIDU_Process_unlock(&vc->shm.shm->lock);
    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WRITE);
    return len;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_writev
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_writev(MPIDI_VC *vc, MPID_IOV *iov, int n)
{
    int i;
    unsigned int total = 0;
    unsigned int num_bytes;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_WRITEV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_WRITEV);

    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));
    for (i=0; i<n; i++)
    {
	num_bytes = MPIDI_CH3I_SHM_write(vc, iov[i].MPID_IOV_BUF, iov[i].MPID_IOV_LEN);
	total += num_bytes;
	if (num_bytes < iov[i].MPID_IOV_LEN)
	{
	    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WRITEV);
	    return total;
	}
    }
    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WRITEV);
    return total;
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
    MPIDI_CH3I_SHM_Queue_t * shm;
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

	shm = (MPIDI_CH3I_SHM_Queue_t *)(((shm_work_id_handle_t*)&completion_data.work_req_id)->data.ptr);
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

/* non-blocking functions */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_post_read
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_post_read(MPIDI_VC *vc, void *buf, int len, int (*rfn)(int, void*))
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_POST_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_POST_READ);
    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));
    vc->shm.read.total = 0;
    vc->shm.read.buffer = buf;
    vc->shm.read.bufflen = len;
    vc->shm.read.use_iov = FALSE;
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_POST_READ);
    return SHM_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_post_readv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_post_readv(MPIDI_VC *vc, MPID_IOV *iov, int n, int (*rfn)(int, void*))
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
    vc->shm.read.total = 0;
    /* This isn't necessary if we require the iov to be valid for the duration of the operation */
    /*vc->shm.read.iov = iov;*/
    memcpy(vc->shm.read.iov, iov, sizeof(MPID_IOV) * n);
    vc->shm.read.iovlen = n;
    vc->shm.read.index = 0;
    vc->shm.read.use_iov = TRUE;
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_POST_READV);
    return SHM_SUCCESS;
}
