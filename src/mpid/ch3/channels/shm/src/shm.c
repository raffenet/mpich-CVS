/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include <stdio.h>

#undef USE_SHM_WRITE_FOR_SHM_WRITEV
/*#define USE_SHM_WRITE_FOR_SHM_WRITEV*/
/*#undef USE_IOV_LEN_2_SHORTCUT*/
#define USE_IOV_LEN_2_SHORTCUT

typedef int SHM_STATE;
#define SHM_READING_BIT     0x0008
#define SHM_WRITING_BIT     0x0010

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

/* shm functions */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_write
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_write(MPIDI_VC * vc, void *buf, int len, int *num_bytes_ptr)
{
    int total = 0;
    int length;
    int index;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_WRITE);
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_WRITE);
    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));

    index = vc->ch.write_shmq->tail_index;
    if (vc->ch.write_shmq->packet[index].avail == MPIDI_CH3I_PKT_USED)
    {
	*num_bytes_ptr = total;
	MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WRITE);
	return MPI_SUCCESS;
    }
    
    while (len)
    {
	length = min(len, MPIDI_CH3I_PACKET_SIZE);
	vc->ch.write_shmq->packet[index].num_bytes = length;
	MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
	memcpy(vc->ch.write_shmq->packet[index].data, buf, length);
	MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
	MPIU_DBG_PRINTF(("shm_write: %d bytes in packet %d\n", vc->ch.write_shmq->packet[index].num_bytes, index));
	MPID_WRITE_BARRIER();
	vc->ch.write_shmq->packet[index].avail = MPIDI_CH3I_PKT_USED;
	buf = (char *) buf + length;
	total += length;
	len -= length;

	index = (index + 1) % MPIDI_CH3I_NUM_PACKETS;
	if (vc->ch.write_shmq->packet[index].avail == MPIDI_CH3I_PKT_USED)
	{
	    vc->ch.write_shmq->tail_index = index;
	    *num_bytes_ptr = total;
	    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WRITE);
	    return MPI_SUCCESS;
	}
    }

    vc->ch.write_shmq->tail_index = index;
    *num_bytes_ptr = total;
    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WRITE);
    return MPI_SUCCESS;
}

#ifdef USE_SHM_WRITE_FOR_SHM_WRITEV

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_writev
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_writev(MPIDI_VC *vc, MPID_IOV *iov, int n, int *num_bytes_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    unsigned int total = 0;
    unsigned int num_bytes;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_WRITEV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_WRITEV);

    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));
    for (i=0; i<n; i++)
    {
	mpi_errno = MPIDI_CH3I_SHM_write(vc, 
					 iov[i].MPID_IOV_BUF, 
					 iov[i].MPID_IOV_LEN,
					 &num_bytes);
	if (mpi_errno != MPI_SUCCESS)
	{
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WRITEV);
	    return mpi_errno;
	}
	total += num_bytes;
	if (num_bytes < iov[i].MPID_IOV_LEN)
	{
	    *num_bytes_ptr = total;
	    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WRITEV);
	    return MPI_SUCCESS;
	}
    }

    *num_bytes_ptr = total;
    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WRITEV);
    return MPI_SUCCESS;
}

#else

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_writev
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_writev(MPIDI_VC *vc, MPID_IOV *iov, int n, int *num_bytes_ptr)
{
    int i;
    unsigned int total = 0;
    unsigned int num_bytes;
    unsigned int cur_avail, dest_avail;
    unsigned char *cur_pos, *dest_pos;
    int index;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_WRITEV);
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_WRITEV);

    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));
    index = vc->ch.write_shmq->tail_index;
    if (vc->ch.write_shmq->packet[index].avail == MPIDI_CH3I_PKT_USED)
    {
	*num_bytes_ptr = 0;
	MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WRITEV);
	return MPI_SUCCESS;
    }

#ifdef USE_IOV_LEN_2_SHORTCUT
    if (n == 2 && (iov[0].MPID_IOV_LEN + iov[1].MPID_IOV_LEN) < MPIDI_CH3I_PACKET_SIZE)
    {
	MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
	memcpy(vc->ch.write_shmq->packet[index].data, 
	       iov[0].MPID_IOV_BUF, iov[0].MPID_IOV_LEN);
	MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
	MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
	memcpy(&vc->ch.write_shmq->packet[index].data[iov[0].MPID_IOV_LEN],
	       iov[1].MPID_IOV_BUF, iov[1].MPID_IOV_LEN);
	MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
	vc->ch.write_shmq->packet[index].num_bytes = 
	    iov[0].MPID_IOV_LEN + iov[1].MPID_IOV_LEN;
	total = vc->ch.write_shmq->packet[index].num_bytes;
	MPID_WRITE_BARRIER();
	vc->ch.write_shmq->packet[index].avail = MPIDI_CH3I_PKT_USED;
	vc->ch.write_shmq->tail_index =
	    (vc->ch.write_shmq->tail_index + 1) % MPIDI_CH3I_NUM_PACKETS;
	MPIU_DBG_PRINTF(("shm_writev - %d bytes in packet %d\n", vc->ch.write_shmq->packet[index].num_bytes, index));
	*num_bytes_ptr = total;
	MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WRITEV);
	return MPI_SUCCESS;
    }
#endif

    dest_pos = (unsigned char *)(vc->ch.write_shmq->packet[index].data);
    dest_avail = MPIDI_CH3I_PACKET_SIZE;
    vc->ch.write_shmq->packet[index].num_bytes = 0;
    for (i=0; i<n; i++)
    {
	if (iov[i].MPID_IOV_LEN <= dest_avail)
	{
	    total += iov[i].MPID_IOV_LEN;
	    vc->ch.write_shmq->packet[index].num_bytes += iov[i].MPID_IOV_LEN;
	    dest_avail -= iov[i].MPID_IOV_LEN;
	    MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
	    memcpy(dest_pos, iov[i].MPID_IOV_BUF, iov[i].MPID_IOV_LEN);
	    MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
	    MPIU_DBG_PRINTF(("shm_writev: +%d=%d bytes in packet %d\n", iov[i].MPID_IOV_LEN, vc->ch.write_shmq->packet[index].num_bytes, index));
	    dest_pos += iov[i].MPID_IOV_LEN;
	}
	else
	{
	    total += dest_avail;
	    vc->ch.write_shmq->packet[index].num_bytes = MPIDI_CH3I_PACKET_SIZE;
	    MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
	    memcpy(dest_pos, iov[i].MPID_IOV_BUF, dest_avail);
	    MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
	    MPIU_DBG_PRINTF(("shm_writev: +%d=%d bytes in packet %d\n", dest_avail, vc->ch.write_shmq->packet[index].num_bytes, index));
	    MPID_WRITE_BARRIER();
	    vc->ch.write_shmq->packet[index].avail = MPIDI_CH3I_PKT_USED;
	    cur_pos = (unsigned char *)(iov[i].MPID_IOV_BUF) + dest_avail;
	    cur_avail = iov[i].MPID_IOV_LEN - dest_avail;
	    while (cur_avail)
	    {
		index = vc->ch.write_shmq->tail_index = 
		    (vc->ch.write_shmq->tail_index + 1) % MPIDI_CH3I_NUM_PACKETS;
		if (vc->ch.write_shmq->packet[index].avail == MPIDI_CH3I_PKT_USED)
		{
		    *num_bytes_ptr = total;
		    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WRITEV);
		    return MPI_SUCCESS;
		}
		num_bytes = min(cur_avail, MPIDI_CH3I_PACKET_SIZE);
		vc->ch.write_shmq->packet[index].num_bytes = num_bytes;
		MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
		memcpy(vc->ch.write_shmq->packet[index].data, cur_pos, num_bytes);
		MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
		MPIU_DBG_PRINTF(("shm_writev: +%d=%d bytes in packet %d\n", num_bytes, vc->ch.write_shmq->packet[index].num_bytes, index));
		total += num_bytes;
		cur_pos += num_bytes;
		cur_avail -= num_bytes;
		if (cur_avail)
		{
		    MPID_WRITE_BARRIER();
		    vc->ch.write_shmq->packet[index].avail = MPIDI_CH3I_PKT_USED;
		}
	    }
	    dest_pos = (unsigned char *)(vc->ch.write_shmq->packet[index].data + num_bytes);
	    dest_avail = MPIDI_CH3I_PACKET_SIZE - num_bytes;
	}
	if (dest_avail == 0)
	{
	    MPID_WRITE_BARRIER();
	    vc->ch.write_shmq->packet[index].avail = MPIDI_CH3I_PKT_USED;
	    index = vc->ch.write_shmq->tail_index = 
		(vc->ch.write_shmq->tail_index + 1) % MPIDI_CH3I_NUM_PACKETS;
	    if (vc->ch.write_shmq->packet[index].avail == MPIDI_CH3I_PKT_USED)
	    {
		*num_bytes_ptr = total;
		MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WRITEV);
		return MPI_SUCCESS;
	    }
	    dest_pos = (unsigned char *)(vc->ch.write_shmq->packet[index].data);
	    dest_avail = MPIDI_CH3I_PACKET_SIZE;
	    vc->ch.write_shmq->packet[index].num_bytes = 0;
	}
    }
    if (dest_avail < MPIDI_CH3I_PACKET_SIZE)
    {
	MPID_WRITE_BARRIER();
	vc->ch.write_shmq->packet[index].avail = MPIDI_CH3I_PKT_USED;
	vc->ch.write_shmq->tail_index = 
	    (vc->ch.write_shmq->tail_index + 1) % MPIDI_CH3I_NUM_PACKETS;
    }

    *num_bytes_ptr = total;
    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WRITEV);
    return MPI_SUCCESS;
}

#endif /* USE_SHM_WRITE_FOR_SHM_WRITEV */

#ifdef USE_SHM_UNEX

#undef FUNCNAME
#define FUNCNAME shmi_buffer_unex_read
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int shmi_buffer_unex_read(MPIDI_VC *vc_ptr, MPIDI_CH3I_SHM_Packet_t *pkt_ptr, void *mem_ptr, unsigned int offset, unsigned int num_bytes)
{
    MPIDI_CH3I_SHM_Unex_read_t *p;
    MPIDI_STATE_DECL(MPID_STATE_SHMI_BUFFER_UNEX_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_SHMI_BUFFER_UNEX_READ);

    MPIDI_DBG_PRINTF((60, FCNAME, "%d bytes\n", num_bytes));

    p = (MPIDI_CH3I_SHM_Unex_read_t *)MPIU_Malloc(sizeof(MPIDI_CH3I_SHM_Unex_read_t));
    p->pkt_ptr = pkt_ptr;
    p->buf = (unsigned char *)mem_ptr + offset;
    p->length = num_bytes;
    p->next = vc_ptr->ch.unex_list;
    vc_ptr->ch.unex_list = p;

    MPIDI_FUNC_EXIT(MPID_STATE_SHMI_BUFFER_UNEX_READ);
    return 0;
}

#undef FUNCNAME
#define FUNCNAME shmi_read_unex
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int shmi_read_unex(MPIDI_VC *vc_ptr)
{
    unsigned int len;
    MPIDI_CH3I_SHM_Unex_read_t *temp;
    MPIDI_STATE_DECL(MPID_STATE_SHMI_READ_UNEX);
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);

    MPIDI_FUNC_ENTER(MPID_STATE_SHMI_READ_UNEX);

    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));
    assert(vc_ptr->ch.unex_list);

    /* copy the received data */
    while (vc_ptr->ch.unex_list)
    {
	len = min(vc_ptr->ch.unex_list->length, vc_ptr->ch.read.bufflen);
	MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
	memcpy(vc_ptr->ch.read.buffer, vc_ptr->ch.unex_list->buf, len);
	MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
	/* advance the user pointer */
	vc_ptr->ch.read.buffer = (char*)(vc_ptr->ch.read.buffer) + len;
	vc_ptr->ch.read.bufflen -= len;
	vc_ptr->ch.read.total += len;
	if (len != vc_ptr->ch.unex_list->length)
	{
	    vc_ptr->ch.unex_list->length -= len;
	    vc_ptr->ch.unex_list->buf += len;
	}
	else
	{
	    /* put the receive packet back in the pool */
	    assert(vc_ptr->ch.unex_list->pkt_ptr != NULL);
	    vc_ptr->ch.unex_list->pkt_ptr->cur_pos = 
		vc_ptr->ch.unex_list->pkt_ptr->data;
	    vc_ptr->ch.unex_list->pkt_ptr->avail = MPIDI_CH3I_PKT_AVAILABLE;
	    /* MPIU_Free the unexpected data node */
	    temp = vc_ptr->ch.unex_list;
	    vc_ptr->ch.unex_list = vc_ptr->ch.unex_list->next;
	    MPIU_Free(temp);
	}
	/* check to see if the entire message was received */
	if (vc_ptr->ch.read.bufflen == 0)
	{
	    /* place this vc_ptr in the finished list so it will be 
	       completed by shm_wait */
	    vc_ptr->ch.shm_state &= ~SHM_READING_BIT;
	    vc_ptr->ch.unex_finished_next = MPIDI_CH3I_Process.unex_finished_list;
	    MPIDI_CH3I_Process.unex_finished_list = vc_ptr;
	    MPIDI_FUNC_EXIT(MPID_STATE_SHMI_READ_UNEX);
	    return SHM_SUCCESS;
	}
    }
    MPIDI_FUNC_EXIT(MPID_STATE_SHMI_READ_UNEX);
    return SHM_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME shmi_readv_unex
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int shmi_readv_unex(MPIDI_VC *vc_ptr)
{
    unsigned int num_bytes;
    MPIDI_CH3I_SHM_Unex_read_t *temp;
    MPIDI_STATE_DECL(MPID_STATE_SHMI_READV_UNEX);
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);

    MPIDI_FUNC_ENTER(MPID_STATE_SHMI_READV_UNEX);

    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));

    while (vc_ptr->ch.unex_list)
    {
	while (vc_ptr->ch.unex_list->length && vc_ptr->ch.read.iovlen)
	{
	    num_bytes = min(vc_ptr->ch.unex_list->length, 
			    vc_ptr->ch.read.iov[vc_ptr->ch.read.index].MPID_IOV_LEN);
	    MPIDI_DBG_PRINTF((60, FCNAME, "copying %d bytes\n", num_bytes));
	    /* copy the received data */
	    MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
	    memcpy(vc_ptr->ch.read.iov[vc_ptr->ch.read.index].MPID_IOV_BUF, vc_ptr->ch.unex_list->buf, num_bytes);
	    MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
	    vc_ptr->ch.read.total += num_bytes;
	    vc_ptr->ch.unex_list->buf += num_bytes;
	    vc_ptr->ch.unex_list->length -= num_bytes;
	    /* update the iov */
	    vc_ptr->ch.read.iov[vc_ptr->ch.read.index].MPID_IOV_LEN -= num_bytes;
	    vc_ptr->ch.read.iov[vc_ptr->ch.read.index].MPID_IOV_BUF = 
		(char*)(vc_ptr->ch.read.iov[vc_ptr->ch.read.index].MPID_IOV_BUF) + num_bytes;
	    if (vc_ptr->ch.read.iov[vc_ptr->ch.read.index].MPID_IOV_LEN == 0)
	    {
		vc_ptr->ch.read.index++;
		vc_ptr->ch.read.iovlen--;
	    }
	}

	if (vc_ptr->ch.unex_list->length == 0)
	{
	    /* put the receive packet back in the pool */
	    assert(vc_ptr->ch.unex_list->pkt_ptr != NULL);
	    vc_ptr->ch.unex_list->pkt_ptr->cur_pos = vc_ptr->ch.unex_list->pkt_ptr->data;
	    vc_ptr->ch.unex_list->pkt_ptr->avail = MPIDI_CH3I_PKT_AVAILABLE;
	    /* MPIU_Free the unexpected data node */
	    temp = vc_ptr->ch.unex_list;
	    vc_ptr->ch.unex_list = vc_ptr->ch.unex_list->next;
	    MPIU_Free(temp);
	}
	
	if (vc_ptr->ch.read.iovlen == 0)
	{
	    vc_ptr->ch.shm_state &= ~SHM_READING_BIT;
	    vc_ptr->ch.unex_finished_next = MPIDI_CH3I_Process.unex_finished_list;
	    MPIDI_CH3I_Process.unex_finished_list = vc_ptr;
	    MPIDI_DBG_PRINTF((60, FCNAME, "finished read saved in MPIDI_CH3I_Process.unex_finished_list\n"));
	    MPIDI_FUNC_EXIT(MPID_STATE_SHMI_READV_UNEX);
	    return SHM_SUCCESS;
	}
    }
    MPIDI_FUNC_EXIT(MPID_STATE_SHMI_READV_UNEX);
    return SHM_SUCCESS;
}

#endif /* USE_SHM_UNEX */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_wait
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_wait(MPIDI_VC *vc, int millisecond_timeout, MPIDI_VC **vc_pptr, int *num_bytes_ptr, shm_wait_t *shm_out)
{
    int mpi_errno;
    void *mem_ptr;
    char *iter_ptr;
    int num_bytes;
    unsigned int offset;
    MPIDI_VC *recv_vc_ptr;
    MPIDI_CH3I_SHM_Packet_t *pkt_ptr;
    int i;
    register int index, working;
#ifdef USE_SHM_UNEX
    MPIDI_VC *temp_vc_ptr;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_WAIT);
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_WAIT);

    for (;;) 
    {
#ifdef USE_SHM_UNEX
	if (MPIDI_CH3I_Process.unex_finished_list)
	{
	    MPIDI_DBG_PRINTF((60, FCNAME, "returning previously received %d bytes", MPIDI_CH3I_Process.unex_finished_list->ch.read.total));

	    *num_bytes_ptr = MPIDI_CH3I_Process.unex_finished_list->ch.read.total;
	    *vc_pptr = MPIDI_CH3I_Process.unex_finished_list;
	    /* remove this vc from the finished list */
	    temp_vc_ptr = MPIDI_CH3I_Process.unex_finished_list;
	    MPIDI_CH3I_Process.unex_finished_list = MPIDI_CH3I_Process.unex_finished_list->ch.unex_finished_next;
	    temp_vc_ptr->ch.unex_finished_next = NULL;

	    *shm_out = SHM_WAIT_READ;
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WAIT);
	    return MPI_SUCCESS;
	}
#endif /* USE_SHM_UNEX */

	working = FALSE;

	for (i=0; i<vc->ch.pg->size; i++)
	{
	    /* skip over the vc to myself */
	    if (vc->ch.pg_rank == i)
		continue;

	    index = vc->ch.shm[i].head_index;

	    /* if the packet at the head index is available, the queue is empty */
	    if (vc->ch.shm[i].packet[index].avail == MPIDI_CH3I_PKT_AVAILABLE)
		continue;
	    MPID_READ_BARRIER(); /* no loads after this line can occur before the avail flag has been read */

	    working = TRUE;

	    pkt_ptr = &vc->ch.shm[i].packet[index];
	    mem_ptr = (void*)(pkt_ptr->data + pkt_ptr->offset);
	    /*mem_ptr = (void*)vc->ch.shm[i].packet[index].cur_pos;*/
	    num_bytes = vc->ch.shm[i].packet[index].num_bytes;
	    recv_vc_ptr = &vc->ch.pg->vc_table[i]; /* This should be some GetVC function with a complete context */

	    if (recv_vc_ptr->ch.shm_reading_pkt)
	    {
		MPIU_DBG_PRINTF(("shm_read_progress: reading %d byte header from shm packet %d offset %d\n", sizeof(MPIDI_CH3_Pkt_t), index, pkt_ptr->offset));
		mpi_errno = MPIDI_CH3U_Handle_recv_pkt(recv_vc_ptr, (MPIDI_CH3_Pkt_t*)mem_ptr, &recv_vc_ptr->ch.recv_active);
		if (mpi_errno != MPI_SUCCESS)
		{
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "shared memory read progress unable to handle incoming packet");
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WAIT);
		    return mpi_errno;
		}

		if (recv_vc_ptr->ch.recv_active == NULL)
		{
		    recv_vc_ptr->ch.shm_reading_pkt = TRUE;
		}
		else
		{
		    mpi_errno = MPIDI_CH3I_SHM_post_readv(recv_vc_ptr, recv_vc_ptr->ch.recv_active->dev.iov, recv_vc_ptr->ch.recv_active->dev.iov_count, NULL);
		}

		if (num_bytes > sizeof(MPIDI_CH3_Pkt_t))
		{
		    pkt_ptr->offset += sizeof(MPIDI_CH3_Pkt_t);
		    num_bytes -= sizeof(MPIDI_CH3_Pkt_t);
		    pkt_ptr->num_bytes = num_bytes;
		    mem_ptr = (char*)mem_ptr + sizeof(MPIDI_CH3_Pkt_t);
		}
		else
		{
		    pkt_ptr->offset = 0;
		    MPID_READ_WRITE_BARRIER(); /* the writing of the flag cannot occur before the reading of the last piece of data */
		    pkt_ptr->avail = MPIDI_CH3I_PKT_AVAILABLE;
		    vc->ch.shm[i].head_index = (index + 1) % MPIDI_CH3I_NUM_PACKETS;
		    continue;
		}
		if (recv_vc_ptr->ch.recv_active == NULL)
		    continue;
	    }

	    MPIDI_DBG_PRINTF((60, FCNAME, "read %d bytes\n", num_bytes));
	    /*MPIDI_DBG_PRINTF((60, FCNAME, "shm_wait(recv finished %d bytes)", num_bytes));*/
	    if (!(recv_vc_ptr->ch.shm_state & SHM_READING_BIT))
	    {
#ifdef USE_SHM_UNEX
		/* Should we buffer unexpected messages or leave them in the shmem queue? */
		/*shmi_buffer_unex_read(recv_vc_ptr, pkt_ptr, mem_ptr, 0, num_bytes);*/
#endif
		continue;
	    }
	    MPIDI_DBG_PRINTF((60, FCNAME, "read update, total = %d + %d = %d\n", recv_vc_ptr->ch.read.total, num_bytes, recv_vc_ptr->ch.read.total + num_bytes));
	    if (recv_vc_ptr->ch.read.use_iov)
	    {
		iter_ptr = mem_ptr;
		while (num_bytes && recv_vc_ptr->ch.read.iovlen > 0)
		{
		    if ((int)recv_vc_ptr->ch.read.iov[recv_vc_ptr->ch.read.index].MPID_IOV_LEN <= num_bytes)
		    {
			/* copy the received data */
			MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
			memcpy(recv_vc_ptr->ch.read.iov[recv_vc_ptr->ch.read.index].MPID_IOV_BUF, iter_ptr,
			    recv_vc_ptr->ch.read.iov[recv_vc_ptr->ch.read.index].MPID_IOV_LEN);
			MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
			MPIU_DBG_PRINTF(("shm_read_progress: %d bytes read from packet %d offset %d\n",
			    recv_vc_ptr->ch.read.iov[recv_vc_ptr->ch.read.index].MPID_IOV_LEN, index,
			    pkt_ptr->offset + (int)((char*)iter_ptr - (char*)mem_ptr)));
			iter_ptr += recv_vc_ptr->ch.read.iov[recv_vc_ptr->ch.read.index].MPID_IOV_LEN;
			/* update the iov */
			num_bytes -= recv_vc_ptr->ch.read.iov[recv_vc_ptr->ch.read.index].MPID_IOV_LEN;
			recv_vc_ptr->ch.read.index++;
			recv_vc_ptr->ch.read.iovlen--;
		    }
		    else
		    {
			/* copy the received data */
			MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
			memcpy(recv_vc_ptr->ch.read.iov[recv_vc_ptr->ch.read.index].MPID_IOV_BUF, iter_ptr, num_bytes);
			MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
			MPIU_DBG_PRINTF(("shm_read_progress: %d bytes read from packet %d offset %d\n", num_bytes, index,
			    pkt_ptr->offset + (int)((char*)iter_ptr - (char*)mem_ptr)));
			iter_ptr += num_bytes;
			/* update the iov */
			recv_vc_ptr->ch.read.iov[recv_vc_ptr->ch.read.index].MPID_IOV_LEN -= num_bytes;
			recv_vc_ptr->ch.read.iov[recv_vc_ptr->ch.read.index].MPID_IOV_BUF = 
			    (char*)(recv_vc_ptr->ch.read.iov[recv_vc_ptr->ch.read.index].MPID_IOV_BUF) + num_bytes;
			num_bytes = 0;
		    }
		}
		offset = (unsigned char*)iter_ptr - (unsigned char*)mem_ptr;
		recv_vc_ptr->ch.read.total += offset;
		if (num_bytes == 0)
		{
		    /* put the shm buffer back in the queue */
		    vc->ch.shm[i].packet[index].offset = 0;
		    MPID_READ_WRITE_BARRIER(); /* the writing of the flag cannot occur before the reading of the last piece of data */
		    vc->ch.shm[i].packet[index].avail = MPIDI_CH3I_PKT_AVAILABLE;
		    vc->ch.shm[i].head_index = (index + 1) % MPIDI_CH3I_NUM_PACKETS;
		}
		else
		{
		    /* save the unused but received data */
		    /*shmi_buffer_unex_read(recv_vc_ptr, pkt_ptr, mem_ptr, offset, num_bytes);*/
		    /* OR */
		    /* update the head of the shmem queue */
		    pkt_ptr->offset += (pkt_ptr->num_bytes - num_bytes);
		    pkt_ptr->num_bytes = num_bytes;
		}
		if (recv_vc_ptr->ch.read.iovlen == 0)
		{
		    recv_vc_ptr->ch.shm_state &= ~SHM_READING_BIT;
		    *num_bytes_ptr = recv_vc_ptr->ch.read.total;
		    *vc_pptr = recv_vc_ptr;
		    *shm_out = SHM_WAIT_READ;
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WAIT);
		    return MPI_SUCCESS;
		}
	    }
	    else
	    {
		if ((unsigned int)num_bytes > recv_vc_ptr->ch.read.bufflen)
		{
		    /* copy the received data */
		    MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
		    memcpy(recv_vc_ptr->ch.read.buffer, mem_ptr, recv_vc_ptr->ch.read.bufflen);
		    MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
		    MPIU_DBG_PRINTF(("shm_read_progress: %d bytes read from packet %d offset %d\n", recv_vc_ptr->ch.read.bufflen, index, pkt_ptr->offset));
		    recv_vc_ptr->ch.read.total = recv_vc_ptr->ch.read.bufflen;
		    /*shmi_buffer_unex_read(recv_vc_ptr, pkt_ptr, mem_ptr, recv_vc_ptr->ch.read.bufflen, num_bytes - recv_vc_ptr->ch.read.bufflen);*/
		    pkt_ptr->offset += recv_vc_ptr->ch.read.bufflen;
		    pkt_ptr->num_bytes = num_bytes - recv_vc_ptr->ch.read.bufflen;
		    recv_vc_ptr->ch.read.bufflen = 0;
		}
		else
		{
		    /* copy the received data */
		    MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
		    memcpy(recv_vc_ptr->ch.read.buffer, mem_ptr, num_bytes);
		    MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
		    MPIU_DBG_PRINTF(("shm_read_progress: %d bytes read from packet %d offset %d\n", num_bytes, index, pkt_ptr->offset));
		    recv_vc_ptr->ch.read.total += num_bytes;
		    /* advance the user pointer */
		    recv_vc_ptr->ch.read.buffer = (char*)(recv_vc_ptr->ch.read.buffer) + num_bytes;
		    recv_vc_ptr->ch.read.bufflen -= num_bytes;
		    /* put the shm buffer back in the queue */
		    vc->ch.shm[i].packet[index].offset = 0;
		    MPID_READ_WRITE_BARRIER(); /* the writing of the flag cannot occur before the reading of the last piece of data */
		    vc->ch.shm[i].packet[index].avail = MPIDI_CH3I_PKT_AVAILABLE;
		    vc->ch.shm[i].head_index = (index + 1) % MPIDI_CH3I_NUM_PACKETS;
		}
		if (recv_vc_ptr->ch.read.bufflen == 0)
		{
		    recv_vc_ptr->ch.shm_state &= ~SHM_READING_BIT;
		    *num_bytes_ptr = recv_vc_ptr->ch.read.total;
		    *vc_pptr = recv_vc_ptr;
		    *shm_out = SHM_WAIT_READ;
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WAIT);
		    return MPI_SUCCESS;
		}
	    }
	}

	if (millisecond_timeout == 0 && !working)
	{
	    *shm_out = SHM_WAIT_TIMEOUT;
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WAIT);
	    return MPI_SUCCESS;
	}
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WAIT);
    return MPI_SUCCESS;
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
    MPIDI_DBG_PRINTF((60, FCNAME, "posting a read of %d bytes", len));
    vc->ch.read.total = 0;
    vc->ch.read.buffer = buf;
    vc->ch.read.bufflen = len;
    vc->ch.read.use_iov = FALSE;
    vc->ch.shm_state |= SHM_READING_BIT;
    vc->ch.shm_reading_pkt = FALSE;
#ifdef USE_SHM_UNEX
    if (vc->ch.unex_list)
	shmi_read_unex(vc);
#endif
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_POST_READ);
    return SHM_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_post_readv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_post_readv(MPIDI_VC *vc, MPID_IOV *iov, int n, int (*rfn)(int, void*))
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_POST_READV);
#ifdef USE_SHM_IOV_COPY
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);
#endif

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_POST_READV);
    vc->ch.read.total = 0;
#ifdef USE_SHM_IOV_COPY
    /* This isn't necessary if we require the iov to be valid for the duration of the operation */
    MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
    memcpy(vc->ch.read.iov, iov, sizeof(MPID_IOV) * n);
    MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
#else
    vc->ch.read.iov = iov;
#endif
    vc->ch.read.iovlen = n;
    vc->ch.read.index = 0;
    vc->ch.read.use_iov = TRUE;
    vc->ch.shm_state |= SHM_READING_BIT;
    vc->ch.shm_reading_pkt = FALSE;
#ifdef USE_SHM_UNEX
    if (vc->ch.unex_list)
	shmi_readv_unex(vc);
#endif
#ifdef MPICH_DBG_OUTPUT
    {
	int i, total=0;
	for (i=0; i<n; i++)
	{
	    total += iov[i].MPID_IOV_LEN;
	}
	MPIDI_DBG_PRINTF((60, FCNAME, "posting a read of %d bytes.\n", total));
    }
#endif
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_POST_READV);
    return SHM_SUCCESS;
}
