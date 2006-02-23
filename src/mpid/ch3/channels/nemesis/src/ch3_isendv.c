/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

extern void *MPIDI_CH3_packet_buffer;

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iSendv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_iSendv (MPIDI_VC_t *vc, MPID_Request *sreq, MPID_IOV *iov, int n_iov)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    int j;
    int complete;
    
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ISENDV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ISENDV);
    MPIU_Assert(n_iov <= MPID_IOV_LIMIT);
    MPIU_Assert(iov[0].MPID_IOV_LEN <= sizeof(MPIDI_CH3_Pkt_t));

    /* The channel uses a fixed length header, the size of which is
     * the maximum of all possible packet headers */
    iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t *)iov[0].MPID_IOV_BUF);

    if (MPIDI_CH3I_SendQ_empty (CH3_NORMAL_QUEUE))
	/* MT */
    {
	MPID_IOV *remaining_iov = iov;
	int remaining_n_iov = n_iov;

	MPIDI_DBG_PRINTF((55, FCNAME, "  sending packet\n"));
	ret = MPID_nem_mpich2_sendv_header (&remaining_iov, &remaining_n_iov, vc);
	while (ret != MPID_NEM_MPICH2_AGAIN && remaining_n_iov > 0)
	{
	    MPIDI_DBG_PRINTF((55, FCNAME, "  sending packet\n"));
	    ret = MPID_nem_mpich2_sendv (&remaining_iov, &remaining_n_iov, vc);
	}

	if (ret == MPID_NEM_MPICH2_AGAIN)
	{
	    if (remaining_iov == iov)
	    {
		/* header was not sent */
		sreq->ch.pkt = *(MPIDI_CH3_Pkt_t *) iov[0].MPID_IOV_BUF;
		sreq->dev.iov[0].MPID_IOV_BUF = (char *) &sreq->ch.pkt;
		sreq->dev.iov[0].MPID_IOV_LEN = iov[0].MPID_IOV_LEN;
	    }
	    else
	    {
		sreq->dev.iov[0] = remaining_iov[0];
	    }
	    
	    for (j = 1; j < remaining_n_iov; ++j)
	    {
		sreq->dev.iov[j] = remaining_iov[j];
	    }
	    sreq->ch.iov_offset = 0;
	    sreq->dev.iov_count = remaining_n_iov;
	    sreq->ch.vc = vc;
	    MPIDI_CH3I_SendQ_enqueue (sreq, CH3_NORMAL_QUEUE);
	    MPIU_Assert (MPIDI_CH3I_active_send[CH3_NORMAL_QUEUE] == NULL);
	    MPIDI_CH3I_active_send[CH3_NORMAL_QUEUE] = sreq;
	}
	else
	{
	    MPIDI_CH3U_Handle_send_req (vc, sreq, &complete);

	    if (!complete)
	    {
		sreq->ch.iov_offset = 0;
		sreq->ch.vc = vc;
		MPIDI_CH3I_SendQ_enqueue (sreq, CH3_NORMAL_QUEUE);
		MPIU_Assert (MPIDI_CH3I_active_send[CH3_NORMAL_QUEUE] == NULL);
		MPIDI_CH3I_active_send[CH3_NORMAL_QUEUE] = sreq;
	    }
	}    
    }
    else
    {
	int i;
	
	MPIDI_DBG_PRINTF((55, FCNAME, "enqueuing"));
	
	sreq->ch.pkt = *(MPIDI_CH3_Pkt_t *) iov[0].MPID_IOV_BUF;
	sreq->dev.iov[0].MPID_IOV_BUF = (char *) &sreq->ch.pkt;
	sreq->dev.iov[0].MPID_IOV_LEN = iov[0].MPID_IOV_LEN;

	for (i = 1; i < n_iov; i++)
	{
	    sreq->dev.iov[i] = iov[i];
	}

	sreq->dev.iov_count = n_iov;
	sreq->ch.iov_offset = 0;
	sreq->ch.vc = vc;
	MPIDI_CH3I_SendQ_enqueue (sreq, CH3_NORMAL_QUEUE);
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISENDV);
    return mpi_errno;
}

