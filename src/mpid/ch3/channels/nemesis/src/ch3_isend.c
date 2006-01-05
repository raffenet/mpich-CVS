/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iSend
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_iSend (MPIDI_VC_t *vc, MPID_Request *sreq, void * hdr, MPIDI_msg_sz_t hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;
    int shmem_errno;
    int complete;
    int enqueue_it;
    
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ISEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ISEND);

    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));
    assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));

    /* This channel uses a fixed length header, the size of which
     * is the maximum of all possible packet headers */
    hdr_sz = sizeof(MPIDI_CH3_Pkt_t);
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t*)hdr);

    if (MPIDI_CH3I_SendQ_empty (CH3_NORMAL_QUEUE))
        /* MT */
    {
	MPIDI_DBG_PRINTF((55, FCNAME, "  sending %d bytes\n", hdr_sz));
	shmem_errno = MPID_nem_mpich2_send_header (hdr, hdr_sz, vc->lpid);
	if (shmem_errno == MPID_NEM_MPICH2_AGAIN)
	{
	    enqueue_it = 1;
	}
	else
	{
	    enqueue_it = 0;
	    MPIDI_CH3U_Handle_send_req (vc, sreq, &complete);
	}
    }
    else
    {
	enqueue_it = 1;
    }

    if (enqueue_it)
    {
	MPIDI_DBG_PRINTF((55, FCNAME, "enqueuing"));

	sreq->ch.pkt = *(MPIDI_CH3_Pkt_t *) hdr;
	sreq->dev.iov[0].MPID_IOV_BUF = (char *) &sreq->ch.pkt;
	sreq->dev.iov[0].MPID_IOV_LEN = hdr_sz;
	sreq->dev.iov_count = 1;
	sreq->ch.iov_offset = 0;
	sreq->ch.vc = vc;
	MPIDI_CH3I_SendQ_enqueue (sreq, CH3_NORMAL_QUEUE);
    }

    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISEND);
    return mpi_errno;
}
