/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"


/*
 * MPIDI_CH3_iStartMsg() attempts to send the message immediately.  If
 * the entire message is successfully sent, then NULL is returned.
 * Otherwise a request is allocated, the header is copied into the
 * request, and a pointer to the request is returned.  An error
 * condition also results in a request be allocated and the errror
 * being returned in the status field of the request.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iStartMsg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_iStartMsg (MPIDI_VC_t *vc, void *hdr, MPIDI_msg_sz_t hdr_sz, MPID_Request **sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int shmem_errno;
    int enqueue_it = 0;
    
    MPID_Request * sreq = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ISTARTMSG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ISTARTMSG);

    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));
    assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));

    /* This channel uses a fixed length header, the size of which is
     * the maximum of all possible packet headers */
    hdr_sz = sizeof(MPIDI_CH3_Pkt_t);
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t*)hdr);

    if (MPIDI_CH3I_SendQ_empty (CH3_NORMAL_QUEUE))
       /* MT */
    {
	MPIDI_DBG_PRINTF((55, FCNAME, "  sending %d bytes\n", hdr_sz));
	shmem_errno = MPID_nem_mpich2_send_header (hdr, hdr_sz, vc);
	if (shmem_errno == MPID_NEM_MPICH2_AGAIN)
	{
	    enqueue_it = 1;
	}
	else
	{
	    enqueue_it = 0;
	}
    }
    else
    {
	enqueue_it = 1;
    }

    if (enqueue_it)
    {
	MPIDI_DBG_PRINTF((55, FCNAME, "enqueuing"));

	/* create a request */
	sreq = MPIDI_CH3_Request_create();
	assert (sreq != NULL);
	MPIU_Object_set_ref (sreq, 2);
	sreq->kind = MPID_REQUEST_SEND;

	sreq->ch.pkt = *(MPIDI_CH3_Pkt_t *) hdr;
	sreq->dev.iov[0].MPID_IOV_BUF = (char *) &sreq->ch.pkt;
	sreq->dev.iov[0].MPID_IOV_LEN = hdr_sz;
	sreq->dev.iov_count = 1;
	sreq->ch.iov_offset = 0;
	sreq->ch.vc = vc;
	sreq->dev.ca = MPIDI_CH3_CA_COMPLETE;
	MPIDI_CH3I_SendQ_enqueue (sreq, CH3_NORMAL_QUEUE);
    }

    *sreq_ptr = sreq;

    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISTARTMSG);
    return mpi_errno;
}


