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
    int mpi_errno;
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

    if (vc->ssm.bShm)
    {
	/* increment the number of active reads */
	MPIDI_CH3I_shm_read_active++;
	/*MPIDI_CH3I_active_flag |= MPID_CH3I_SHM_BIT;*/

	MPIDI_DBG_PRINTF((60, FCNAME, "vc.bShm == TRUE"));
	index = vc->ssm.read_shmq->head_index;
	if (vc->ssm.read_shmq->packet[index].avail == MPIDI_CH3I_PKT_EMPTY)
	{
	    rreq->ssm.iov_offset = 0;
	    vc->ssm.recv_active = rreq;
	    mpi_errno = MPIDI_CH3I_SHM_post_readv(vc, rreq->ch3.iov + rreq->ssm.iov_offset, rreq->ch3.iov_count - rreq->ssm.iov_offset, NULL);
	    MPIDI_DBG_PRINTF((60, FCNAME, "exiting after shm_post_readv"));
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_IREAD);
	    return mpi_errno;
	}

	pkt_ptr = &vc->ssm.read_shmq->packet[index];
	mem_ptr = (void*)(pkt_ptr->data + pkt_ptr->offset);
	num_bytes = pkt_ptr->num_bytes;
#ifdef MPICH_DBG_OUTPUT
	/*assert(num_bytes > 0);*/
	if (num_bytes < 1)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmq", 0);
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_IREAD);
	    return mpi_errno;
	}
#endif

	iter_ptr = mem_ptr;

	rreq->ssm.iov_offset = 0;
	while (rreq->ssm.iov_offset < rreq->ch3.iov_count)
	{
	    if ((int)rreq->ch3.iov[rreq->ssm.iov_offset].MPID_IOV_LEN <= num_bytes)
	    {
		MPIDI_DBG_PRINTF((60, FCNAME, "reading %d bytes from read_shmq %08p packet[%d]", rreq->ch3.iov[rreq->ssm.iov_offset].MPID_IOV_LEN, vc->ssm.read_shmq, index));
		MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
		memcpy(rreq->ch3.iov[rreq->ssm.iov_offset].MPID_IOV_BUF, iter_ptr, rreq->ch3.iov[rreq->ssm.iov_offset].MPID_IOV_LEN);
		MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
		iter_ptr += rreq->ch3.iov[rreq->ssm.iov_offset].MPID_IOV_LEN;
		num_bytes -= rreq->ch3.iov[rreq->ssm.iov_offset].MPID_IOV_LEN;
		rreq->ssm.iov_offset += 1;
	    }
	    else
	    {
		MPIDI_DBG_PRINTF((60, FCNAME, "reading %d bytes from read_shmq %08p packet[%d]", num_bytes, vc->ssm.read_shmq, index));
		MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
		memcpy(rreq->ch3.iov[rreq->ssm.iov_offset].MPID_IOV_BUF, iter_ptr, num_bytes);
		MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);

		pkt_ptr->offset = 0;
		MPID_READ_WRITE_BARRIER();
		pkt_ptr->avail = MPIDI_CH3I_PKT_EMPTY;
		vc->ssm.read_shmq->head_index = (index + 1) % MPIDI_CH3I_NUM_PACKETS;
		MPIDI_DBG_PRINTF((60, FCNAME, "read_shmq head = %d", vc->ssm.read_shmq->head_index));

		rreq->ch3.iov[rreq->ssm.iov_offset].MPID_IOV_BUF = (char *) rreq->ch3.iov[rreq->ssm.iov_offset].MPID_IOV_BUF + num_bytes;
		rreq->ch3.iov[rreq->ssm.iov_offset].MPID_IOV_LEN -= num_bytes;
		vc->ssm.recv_active = rreq;
		mpi_errno = MPIDI_CH3I_SHM_post_readv(vc, rreq->ch3.iov + rreq->ssm.iov_offset, rreq->ch3.iov_count - rreq->ssm.iov_offset, NULL);
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
	    vc->ssm.read_shmq->head_index = (index + 1) % MPIDI_CH3I_NUM_PACKETS;
	    MPIDI_DBG_PRINTF((60, FCNAME, "read_shmq head = %d", vc->ssm.read_shmq->head_index));
	}
	else
	{
	    pkt_ptr->offset += (pkt_ptr->num_bytes - num_bytes);
	    pkt_ptr->num_bytes = num_bytes;
	}

	if (rreq->ch3.ca == MPIDI_CH3_CA_COMPLETE)
	{
	    /* mark data transfer as complete and decrement CC */
	    rreq->ch3.iov_count = 0;
	    MPIDI_CH3U_Request_complete(rreq);
	    MPIDI_DBG_PRINTF((60, FCNAME, "called request complete"));
	}
	else
	{
	    /* FIXME: excessive recursion... */
	    /* decrement the number of active reads */
	    MPIDI_CH3I_shm_read_active--;
	    MPIDI_CH3U_Handle_recv_req(vc, rreq);
	    MPIDI_DBG_PRINTF((60, FCNAME, "called handle_recv_req"));
	}

    }
    else
    {
	int sock_errno;
	sock_size_t nb;
#ifdef MPICH_DBG_OUTPUT
	/*assert(vc->ssm.state == MPIDI_CH3I_VC_STATE_CONNECTED);*/
	if (vc->ssm.state != MPIDI_CH3I_VC_STATE_CONNECTED)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**vc_state", "**vc_state %d", vc->ssm.state);
	    goto fn_exit;
	}
#endif

	/* increment the number of active reads */
	MPIDI_CH3I_sock_read_active++;
	/*MPIDI_CH3I_active_flag |= MPID_CH3I_SOCK_BIT;*/

	sock_errno = sock_readv(vc->ssm.sock, rreq->ch3.iov, rreq->ch3.iov_count, &nb);
	if (sock_errno == SOCK_SUCCESS)
	{
	    rreq->ssm.iov_offset = 0;
	    while (rreq->ssm.iov_offset < rreq->ch3.iov_count)
	    {
		if ((sock_size_t)rreq->ch3.iov[rreq->ssm.iov_offset].MPID_IOV_LEN <= nb)
		{
		    nb -= rreq->ch3.iov[rreq->ssm.iov_offset].MPID_IOV_LEN;
		    rreq->ssm.iov_offset += 1;
		}
		else
		{
		    rreq->ch3.iov[rreq->ssm.iov_offset].MPID_IOV_BUF = (char *) rreq->ch3.iov[rreq->ssm.iov_offset].MPID_IOV_BUF + nb;
		    rreq->ch3.iov[rreq->ssm.iov_offset].MPID_IOV_LEN -= nb;
		    MPIDI_CH3I_SSM_VC_post_read(vc, rreq);
		    goto fn_exit;
		}
	    }

	    /* FIXME: excessive recursion... */
	    /* decrement the number of active reads */
	    MPIDI_CH3I_sock_read_active--;
	    MPIDI_CH3U_Handle_recv_req(vc, rreq);
	}
	else
	{
	    mpi_errno = MPIDI_CH3I_sock_errno_to_mpi_errno(sock_errno, FCNAME);
	    return mpi_errno;
	}
/*
	rreq->ssm.iov_offset = 0;

	MPIDI_CH3I_SSM_VC_post_read(vc, rreq);
	MPIDI_DBG_PRINTF((60, FCNAME, "just called vc_post_read"));
*/
    }

  fn_exit:
    MPIDI_DBG_PRINTF((60, FCNAME, "exiting from main block"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_IREAD);
    return MPI_SUCCESS;
}
