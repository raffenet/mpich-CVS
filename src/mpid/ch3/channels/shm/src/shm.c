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
    int total = 0;
    int length;
    int index;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_WRITE);
    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));

    while (len)
    {
	index = vc->shm.shm->tail_index;

	if (vc->shm.shm->packet[index].avail == MPIDI_CH3I_PKT_USED)
	    return total;
	length = min(len, MPIDI_CH3I_PACKET_SIZE);
	vc->shm.shm->packet[index].num_bytes = length;
	memcpy(vc->shm.shm->packet[index].data, buf, length);
	vc->shm.shm->packet[index].avail = MPIDI_CH3I_PKT_USED;
	total += length;
	len -= length;
    }

    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WRITE);
    return total;
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

#undef FUNCNAME
#define FUNCNAME shmi_buffer_unex_read
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int shmi_buffer_unex_read(MPIDI_VC *vc_ptr, void *mem_ptr, unsigned int offset, unsigned int num_bytes)
{
    MPIDI_CH3I_SHM_Unex_read_t *p;
    MPIDI_STATE_DECL(MPID_STATE_SHMI_BUFFER_UNEX_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_SHMI_BUFFER_UNEX_READ);

    MPIDI_DBG_PRINTF((60, FCNAME, "%d bytes\n", num_bytes));

    p = (MPIDI_CH3I_SHM_Unex_read_t *)malloc(sizeof(MPIDI_CH3I_SHM_Unex_read_t));
    p->mem_ptr = mem_ptr;
    p->buf = (unsigned char *)mem_ptr + offset;
    p->length = num_bytes;
    p->next = vc_ptr->shm.unex_list;
    vc_ptr->shm.unex_list = p;

    MPIDI_FUNC_EXIT(MPID_STATE_SHMI_BUFFER_UNEX_READ);
    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_wait
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
shm_wait_t MPIDI_CH3I_SHM_wait(MPIDI_VC *vc, int millisecond_timeout, MPIDI_VC **vc_pptr, int *num_bytes_ptr, int *error_ptr)
{
    void *mem_ptr;
    char *iter_ptr;
    int num_bytes;
    unsigned int offset;
    MPIDI_VC *recv_vc_ptr;
    int i;
    /*MPIDI_CH3I_SHM_Unex_read_t *temp_shm_ptr;*/
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_WAIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_WAIT);

    for (;;) 
    {
	if (vc->shm.unex_finished_list)
	{
	    MPIDI_DBG_PRINTF((60, FCNAME, "returning previously received %d bytes", vc->shm.unex_finished_list->read.total));

	    *num_bytes_ptr = vc->shm.unex_finished_list->length;
	    *vc_pptr = &vc->shm.pg->vc_table[vc->shm.unex_finished_list->src];
	    *error_ptr = 0;
	    /* remove this shm from the finished list */
	    /*
	    temp_shm_ptr = vc->shm.unex_finished_list;
	    vc->shm.unex_finished_list = vc->shm.unex_finished_list->unex_finished_queue;
	    temp_shm_ptr->unex_finished_queue = NULL;
	    */

	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WAIT);
	    return SHM_WAIT_READ;
	}

	for (i=0; i<vc->shm.pg->size; i++)
	{
	    if (vc->shm.shm[i].packet[vc->shm.shm[i].head_index].avail == MPIDI_CH3I_PKT_AVAILABLE)
		continue;


	    mem_ptr = (void*)vc->shm.shm[i].packet[vc->shm.shm[i].head_index].data;
	    num_bytes = vc->shm.shm[i].packet[vc->shm.shm[i].head_index].num_bytes;
	    recv_vc_ptr = &vc->shm.pg->vc_table[i]; /* This should be some GetVC function with a complete context */

	    MPIDI_DBG_PRINTF((60, FCNAME, "read %d bytes\n", num_bytes));
	    /*MPIDI_DBG_PRINTF((60, FCNAME, "shm_wait(recv finished %d bytes)", num_bytes));*/
	    if (!(recv_vc_ptr->shm.state & SHM_READING))
	    {
		shmi_buffer_unex_read(recv_vc_ptr, mem_ptr, 0, num_bytes);
		continue;
	    }
	    MPIDI_DBG_PRINTF((60, FCNAME, "read update, total = %d + %d = %d\n", recv_vc_ptr->shm.read.total, num_bytes, recv_vc_ptr->shm.read.total + num_bytes));
	    if (recv_vc_ptr->shm.read.use_iov)
	    {
		iter_ptr = mem_ptr;
		while (num_bytes && recv_vc_ptr->shm.read.iovlen > 0)
		{
		    if ((int)recv_vc_ptr->shm.read.iov[recv_vc_ptr->shm.read.index].MPID_IOV_LEN <= num_bytes)
		    {
			/* copy the received data */
			memcpy(recv_vc_ptr->shm.read.iov[recv_vc_ptr->shm.read.index].MPID_IOV_BUF, iter_ptr,
			    recv_vc_ptr->shm.read.iov[recv_vc_ptr->shm.read.index].MPID_IOV_LEN);
			iter_ptr += recv_vc_ptr->shm.read.iov[recv_vc_ptr->shm.read.index].MPID_IOV_LEN;
			/* update the iov */
			num_bytes -= recv_vc_ptr->shm.read.iov[recv_vc_ptr->shm.read.index].MPID_IOV_LEN;
			recv_vc_ptr->shm.read.index++;
			recv_vc_ptr->shm.read.iovlen--;
		    }
		    else
		    {
			/* copy the received data */
			memcpy(recv_vc_ptr->shm.read.iov[recv_vc_ptr->shm.read.index].MPID_IOV_BUF, iter_ptr, num_bytes);
			iter_ptr += num_bytes;
			/* update the iov */
			recv_vc_ptr->shm.read.iov[recv_vc_ptr->shm.read.index].MPID_IOV_LEN -= num_bytes;
			recv_vc_ptr->shm.read.iov[recv_vc_ptr->shm.read.index].MPID_IOV_BUF = 
			    (char*)(recv_vc_ptr->shm.read.iov[recv_vc_ptr->shm.read.index].MPID_IOV_BUF) + num_bytes;
			num_bytes = 0;
		    }
		}
		offset = (unsigned char*)iter_ptr - (unsigned char*)mem_ptr;
		recv_vc_ptr->shm.read.total += offset;
		if (num_bytes == 0)
		{
		    /* put the shm buffer back in the queue */
		    vc->shm.shm[i].packet[vc->shm.shm[i].head_index].avail = MPIDI_CH3I_PKT_AVAILABLE;
		    vc->shm.shm[i].head_index = (vc->shm.shm[i].head_index + 1) % MPIDI_CH3I_NUM_PACKETS;
		}
		else
		{
		    /* save the unused but received data */
		    shmi_buffer_unex_read(recv_vc_ptr, mem_ptr, offset, num_bytes);
		}
		if (recv_vc_ptr->shm.read.iovlen == 0)
		{
		    recv_vc_ptr->shm.state &= ~SHM_READING;
		    *num_bytes_ptr = recv_vc_ptr->shm.read.total;
		    *vc_pptr = recv_vc_ptr;
		    *error_ptr = 0;
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WAIT);
		    return SHM_WAIT_READ;
		}
		/* make the user upcall */
		/*
		if (recv_vc_ptr->shm.read.progress_update != NULL)
		recv_vc_ptr->shm.read.progress_update(num_bytes, recv_vc_ptr->shm.shm->user_ptr);
		*/
	    }
	    else
	    {
		if ((unsigned int)num_bytes > recv_vc_ptr->shm.read.bufflen)
		{
		    /* copy the received data */
		    memcpy(recv_vc_ptr->shm.read.buffer, mem_ptr, recv_vc_ptr->shm.read.bufflen);
		    recv_vc_ptr->shm.read.total = recv_vc_ptr->shm.read.bufflen;
		    shmi_buffer_unex_read(recv_vc_ptr, mem_ptr, recv_vc_ptr->shm.read.bufflen, num_bytes - recv_vc_ptr->shm.read.bufflen);
		    recv_vc_ptr->shm.read.bufflen = 0;
		}
		else
		{
		    /* copy the received data */
		    memcpy(recv_vc_ptr->shm.read.buffer, mem_ptr, num_bytes);
		    recv_vc_ptr->shm.read.total += num_bytes;
		    /* advance the user pointer */
		    recv_vc_ptr->shm.read.buffer = (char*)(recv_vc_ptr->shm.read.buffer) + num_bytes;
		    recv_vc_ptr->shm.read.bufflen -= num_bytes;
		    /* put the shm buffer back in the queue */
		    vc->shm.shm[i].packet[vc->shm.shm[i].head_index].avail = MPIDI_CH3I_PKT_AVAILABLE;
		    vc->shm.shm[i].head_index = (vc->shm.shm[i].head_index + 1) % MPIDI_CH3I_NUM_PACKETS;
		}
		if (recv_vc_ptr->shm.read.bufflen == 0)
		{
		    recv_vc_ptr->shm.state &= ~SHM_READING;
		    *num_bytes_ptr = recv_vc_ptr->shm.read.total;
		    *vc_pptr = recv_vc_ptr;
		    *error_ptr = 0;
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WAIT);
		    return SHM_WAIT_READ;
		}
		/* make the user upcall */
		/*
		if (recv_vc_ptr->shm.read.progress_update != NULL)
		recv_vc_ptr->shm.read.progress_update(num_bytes, recv_vc_ptr->shm.shm->user_ptr);
		*/
	    }
	}
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WAIT);
}

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
