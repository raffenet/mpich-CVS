/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

#define USE_NEW_WAY

/*
 * MPIDI_CH3_iRead()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iRead
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3_iRead(MPIDI_VC * vc, MPID_Request * rreq)
{
#ifdef USE_NEW_WAY
    void *mem_ptr;
    char *iter_ptr;
    int num_bytes;
    MPIDI_CH3I_SHM_Packet_t *pkt_ptr;
    register int index;
    int cur_index = 0;
#endif

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_IREAD);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_IREAD);
    MPIDI_DBG_PRINTF((60, FCNAME, "ch3_iread\n"));

#ifdef USE_NEW_WAY
    index = vc->shm.read_shmq->head_index;
    if (vc->shm.read_shmq->packet[index].avail == MPIDI_CH3I_PKT_AVAILABLE)
    {
	rreq->shm.iov_offset = 0;
	vc->shm.recv_active = rreq;
	MPIDI_CH3I_SHM_post_readv(vc, rreq->ch3.iov + rreq->shm.iov_offset, rreq->ch3.iov_count - rreq->shm.iov_offset, NULL);
	return;
    }

    mem_ptr = (void*)vc->shm.read_shmq->packet[index].cur_pos;
    pkt_ptr = &vc->shm.read_shmq->packet[index];
    num_bytes = vc->shm.read_shmq->packet[index].num_bytes;
    assert(num_bytes > 0);

    iter_ptr = mem_ptr;

    rreq->shm.iov_offset = 0;
    while (rreq->shm.iov_offset < rreq->ch3.iov_count)
    {
	if ((int)rreq->ch3.iov[rreq->shm.iov_offset].MPID_IOV_LEN <= num_bytes)
	{
	    MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
	    memcpy(rreq->ch3.iov[rreq->shm.iov_offset].MPID_IOV_BUF, iter_ptr, rreq->ch3.iov[rreq->shm.iov_offset].MPID_IOV_LEN);
	    MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
	    iter_ptr += rreq->ch3.iov[rreq->shm.iov_offset].MPID_IOV_LEN;
	    num_bytes -= rreq->ch3.iov[rreq->shm.iov_offset].MPID_IOV_LEN;
	    rreq->shm.iov_offset += 1;
	}
	else
	{
	    MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
	    memcpy(rreq->ch3.iov[rreq->shm.iov_offset].MPID_IOV_BUF, iter_ptr, num_bytes);
	    MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);

	    pkt_ptr->cur_pos = pkt_ptr->data;
	    pkt_ptr->avail = MPIDI_CH3I_PKT_AVAILABLE;
	    vc->shm.read_shmq->head_index = (index + 1) % MPIDI_CH3I_NUM_PACKETS;

	    rreq->ch3.iov[rreq->shm.iov_offset].MPID_IOV_BUF = (char *) rreq->ch3.iov[rreq->shm.iov_offset].MPID_IOV_BUF + num_bytes;
	    rreq->ch3.iov[rreq->shm.iov_offset].MPID_IOV_LEN -= num_bytes;
	    vc->shm.recv_active = rreq;
	    MPIDI_CH3I_SHM_post_readv(vc, rreq->ch3.iov + rreq->shm.iov_offset, rreq->ch3.iov_count - rreq->shm.iov_offset, NULL);
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_IREAD);
	    return;
	}
    }
    pkt_ptr->cur_pos += (pkt_ptr->num_bytes - num_bytes);
    pkt_ptr->num_bytes = num_bytes;
    if (pkt_ptr->num_bytes == 0)
    {
	pkt_ptr->cur_pos = pkt_ptr->data;
	pkt_ptr->avail = MPIDI_CH3I_PKT_AVAILABLE;
	vc->shm.read_shmq->head_index = (index + 1) % MPIDI_CH3I_NUM_PACKETS;
    }

    /* FIXME: excessive recursion... */
    MPIDI_CH3U_Handle_recv_req(vc, rreq);

#else

    rreq->shm.iov_offset = 0;
    vc->shm.recv_active = rreq;
    MPIDI_CH3I_SHM_post_readv(vc, rreq->ch3.iov + rreq->shm.iov_offset, rreq->ch3.iov_count - rreq->shm.iov_offset, NULL);

#endif

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_IREAD);
}

#if 0
/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

#define USE_NEW_WAY

int shm_readv(MPIDI_CH3I_SHM_Queue_t *shm, MPID_IOV *iov, int iovlen, int *nb)
{
    void *mem_ptr;
    char *iter_ptr;
    int num_bytes;
    MPIDI_CH3I_SHM_Packet_t *pkt_ptr;
    register int index;
    int cur_index = 0;

    index = shm->head_index;
    if (shm->packet[index].avail == MPIDI_CH3I_PKT_AVAILABLE)
    {
	*nb = 0;
	return MPI_SUCCESS;
    }

    mem_ptr = (void*)shm->packet[index].cur_pos;
    pkt_ptr = &shm->packet[index];
    num_bytes = shm->packet[index].num_bytes;
    assert(num_bytes > 0);

    iter_ptr = mem_ptr;
    while (num_bytes && iovlen > 0)
    {
	if ((int)iov[cur_index].MPID_IOV_LEN <= num_bytes)
	{
	    /* copy the received data */
	    MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
	    memcpy(iov[cur_index].MPID_IOV_BUF, iter_ptr, iov[cur_index].MPID_IOV_LEN);
	    MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
	    iter_ptr += iov[cur_index].MPID_IOV_LEN;
	    /* update the iov */
	    num_bytes -= iov[cur_index].MPID_IOV_LEN;
	    cur_index++;
	    iovlen--;
	}
	else
	{
	    /* copy the received data */
	    MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
	    memcpy(iov[cur_index].MPID_IOV_BUF, iter_ptr, num_bytes);
	    MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
	    iter_ptr += num_bytes;
	    /* update the iov */
	    iov[cur_index].MPID_IOV_LEN -= num_bytes;
	    iov[cur_index].MPID_IOV_BUF = 
		(char*)(iov[cur_index].MPID_IOV_BUF) + num_bytes;
	    num_bytes = 0;
	}
    }
    if (num_bytes == 0)
    {
	/* put the shm buffer back in the queue */
	pkt_ptr->cur_pos = pkt_ptr->data;
	pkt_ptr->avail = MPIDI_CH3I_PKT_AVAILABLE;
	shm->head_index = (index + 1) % MPIDI_CH3I_NUM_PACKETS;
    }
    else
    {
	assert(num_bytes > 0);
	/* update the head of the shmem queue */
	pkt_ptr->cur_pos += (pkt_ptr->num_bytes - num_bytes);
	pkt_ptr->num_bytes = num_bytes;
    }
    *nb = (int)((unsigned char*)iter_ptr - (unsigned char*)mem_ptr);
    return MPI_SUCCESS;
}

/*
 * MPIDI_CH3_iRead()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iRead
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3_iRead(MPIDI_VC * vc, MPID_Request * rreq)
{
#ifdef USE_NEW_WAY
    int nb;
    int error;
    MPID_IOV iov[MPID_IOV_LIMIT];
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_IREAD);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_IREAD);
    MPIDI_DBG_PRINTF((60, FCNAME, "ch3_iread\n"));

#ifdef USE_NEW_WAY
    memcpy(iov, rreq->ch3.iov, rreq->ch3.iov_count * sizeof(MPID_IOV));
    error = shm_readv(vc->shm.read_shmq, iov, rreq->ch3.iov_count, &nb);
    /*error = shm_readv(vc->shm.read_shmq, rreq->ch3.iov, rreq->ch3.iov_count, &nb);*/
    if (error == MPI_SUCCESS)
    {
	rreq->shm.iov_offset = 0;
	while (rreq->shm.iov_offset < rreq->ch3.iov_count)
	{
	    if ((int)rreq->ch3.iov[rreq->shm.iov_offset].MPID_IOV_LEN <= nb)
	    {
		nb -= rreq->ch3.iov[rreq->shm.iov_offset].MPID_IOV_LEN;
		rreq->shm.iov_offset += 1;
	    }
	    else
	    {
		rreq->ch3.iov[rreq->shm.iov_offset].MPID_IOV_BUF = (char *) rreq->ch3.iov[rreq->shm.iov_offset].MPID_IOV_BUF + nb;
		rreq->ch3.iov[rreq->shm.iov_offset].MPID_IOV_LEN -= nb;
		vc->shm.recv_active = rreq;
		MPIDI_CH3I_SHM_post_readv(vc, rreq->ch3.iov + rreq->shm.iov_offset, rreq->ch3.iov_count - rreq->shm.iov_offset, NULL);
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_IREAD);
		return;
	    }
	}

	/* FIXME: excessive recursion... */
	MPIDI_CH3U_Handle_recv_req(vc, rreq);
    }
    else
    {
	MPID_Abort(NULL, error);
    }

#else

    rreq->shm.iov_offset = 0;
    vc->shm.recv_active = rreq;
    MPIDI_CH3I_SHM_post_readv(vc, rreq->ch3.iov + rreq->shm.iov_offset, rreq->ch3.iov_count - rreq->shm.iov_offset, NULL);

#endif

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_IREAD);
}
#endif
