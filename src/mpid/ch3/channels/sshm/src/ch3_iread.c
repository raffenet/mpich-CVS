/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
*  (C) 2001 by Argonne National Laboratory.
*      See COPYRIGHT in top-level directory.
*/

#include "mpidi_ch3_impl.h"

/*
* MPIDI_CH3_iRead()
*/
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iRead
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_iRead(MPIDI_VC * vc, MPID_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    void *mem_ptr;
    char *iter_ptr;
    int num_bytes;
    MPIDI_CH3I_SHM_Packet_t *pkt_ptr;
    register int index;
    int cur_index = 0;
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_IREAD);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_IREAD);
    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));

    index = vc->ch.read_shmq->head_index;
    if (vc->ch.read_shmq->packet[index].avail == MPIDI_CH3I_PKT_EMPTY)
    {
	rreq->ch.iov_offset = 0;
	vc->ch.recv_active = rreq;
	mpi_errno = MPIDI_CH3I_SHM_post_readv(vc, rreq->dev.iov + rreq->ch.iov_offset, rreq->dev.iov_count - rreq->ch.iov_offset, NULL);
	MPIDI_DBG_PRINTF((60, FCNAME, "exiting after shm_post_readv"));
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_IREAD);
	return mpi_errno;
    }

    pkt_ptr = &vc->ch.read_shmq->packet[index];
    mem_ptr = (void*)(pkt_ptr->data + pkt_ptr->offset);
    num_bytes = pkt_ptr->num_bytes;
#ifdef MPICH_DBG_OUTPUT
    if (num_bytes < 1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmq", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_IREAD);
	return mpi_errno;
    }
#endif

    iter_ptr = mem_ptr;

    rreq->ch.iov_offset = 0;
    while (rreq->ch.iov_offset < rreq->dev.iov_count)
    {
	if ((int)rreq->dev.iov[rreq->ch.iov_offset].MPID_IOV_LEN <= num_bytes)
	{
	    MPIDI_DBG_PRINTF((60, FCNAME, "reading %d bytes from read_shmq %08p packet[%d]", rreq->dev.iov[rreq->ch.iov_offset].MPID_IOV_LEN, vc->ch.read_shmq, index));
	    MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
	    memcpy(rreq->dev.iov[rreq->ch.iov_offset].MPID_IOV_BUF, iter_ptr, rreq->dev.iov[rreq->ch.iov_offset].MPID_IOV_LEN);
	    MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
	    iter_ptr += rreq->dev.iov[rreq->ch.iov_offset].MPID_IOV_LEN;
	    num_bytes -= rreq->dev.iov[rreq->ch.iov_offset].MPID_IOV_LEN;
	    rreq->ch.iov_offset += 1;
	}
	else
	{
	    MPIDI_DBG_PRINTF((60, FCNAME, "reading %d bytes from read_shmq %08p packet[%d]", num_bytes, vc->ch.read_shmq, index));
	    MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
	    memcpy(rreq->dev.iov[rreq->ch.iov_offset].MPID_IOV_BUF, iter_ptr, num_bytes);
	    MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);

	    pkt_ptr->offset = 0;
	    MPID_READ_WRITE_BARRIER();
	    pkt_ptr->avail = MPIDI_CH3I_PKT_EMPTY;
	    vc->ch.read_shmq->head_index = (index + 1) % MPIDI_CH3I_NUM_PACKETS;
	    MPIDI_DBG_PRINTF((60, FCNAME, "read_shmq head = %d", vc->ch.read_shmq->head_index));

	    rreq->dev.iov[rreq->ch.iov_offset].MPID_IOV_BUF = (char *) rreq->dev.iov[rreq->ch.iov_offset].MPID_IOV_BUF + num_bytes;
	    rreq->dev.iov[rreq->ch.iov_offset].MPID_IOV_LEN -= num_bytes;
	    vc->ch.recv_active = rreq;
	    mpi_errno = MPIDI_CH3I_SHM_post_readv(vc, rreq->dev.iov + rreq->ch.iov_offset, rreq->dev.iov_count - rreq->ch.iov_offset, NULL);
	    MPIDI_DBG_PRINTF((60, FCNAME, "exiting after shm_post_readv 2"));
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_IREAD);
	    return mpi_errno;
	}
    }
    if (num_bytes == 0)
    {
	pkt_ptr->num_bytes = 0;
	pkt_ptr->offset = 0;
	MPID_READ_WRITE_BARRIER();
	pkt_ptr->avail = MPIDI_CH3I_PKT_EMPTY;
	vc->ch.read_shmq->head_index = (index + 1) % MPIDI_CH3I_NUM_PACKETS;
	MPIDI_DBG_PRINTF((60, FCNAME, "read_shmq head = %d", vc->ch.read_shmq->head_index));
    }
    else
    {
	pkt_ptr->offset += (pkt_ptr->num_bytes - num_bytes);
	pkt_ptr->num_bytes = num_bytes;
    }

    mpi_errno = MPIDI_CH3U_Handle_recv_req(vc, rreq);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
    }
    if (rreq->dev.iov_count == 0)
    {
	vc->ch.shm_reading_pkt = TRUE;
    }
#if 0
    if (rreq->dev.ca == MPIDI_CH3_CA_COMPLETE)
    {
	/* mark data transfer as complete and decrement CC */
	rreq->dev.iov_count = 0;
	MPIDI_CH3U_Request_complete(rreq);
	MPIDI_DBG_PRINTF((60, FCNAME, "called request complete"));
    }
    else
    {
	/* FIXME: excessive recursion... */
	MPIDI_CH3U_Handle_recv_req(vc, rreq);
	MPIDI_DBG_PRINTF((60, FCNAME, "called handle_recv_req"));
    }
#endif

    MPIDI_DBG_PRINTF((60, FCNAME, "exiting from main block"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_IREAD);
    return mpi_errno;
}
