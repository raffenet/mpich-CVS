/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/*
 * MPIDI_CH3_iStartMsgv() attempts to send the message immediately.
 * If the entire message is successfully sent, then NULL is returned.
 * Otherwise a request is allocated, the iovec and the first buffer
 * pointed to by the iovec (which is assumed to be a MPIDI_CH3_Pkt_t)
 * are copied into the request, and a pointer to the request is
 * returned.  An error condition also results in a request be
 * allocated and the errror being returned in the status field of the
 * request.
 */

/* NOTE - The completion action associated with a request created by CH3_iStartMsgv() is alway MPIDI_CH3_CA_COMPLETE.  This
   implies that CH3_iStartMsgv() can only be used when the entire message can be described by a single iovec of size
   MPID_IOV_LIMIT. */
    
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iStartMsgv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_iStartMsgv (MPIDI_VC_t * vc, MPID_IOV * iov, int n_iov, MPID_Request ** sreq_ptr)
{
    MPID_Request * sreq = NULL;
    int mpi_errno = MPI_SUCCESS;
    int shmem_errno;
    int j;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ISTARTMSGV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ISTARTMSGV);

    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));
    assert (n_iov <= MPID_IOV_LIMIT);
    assert (iov[0].MPID_IOV_LEN <= sizeof(MPIDI_CH3_Pkt_t));

    /* The channel uses a fixed length header, the size of which is
     * the maximum of all possible packet headers */
    iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t*)iov[0].MPID_IOV_BUF);
    
    if (MPIDI_CH3I_SendQ_empty (CH3_NORMAL_QUEUE))
        /* MT */
    {
	MPID_IOV *_iov = iov;
	int _n_iov = n_iov;

	MPIDI_DBG_PRINTF((55, FCNAME, "  sending packet with sendv_header\n"));
	shmem_errno = MPID_nem_mpich2_sendv_header (&_iov, &_n_iov, vc->lpid);
	while ((shmem_errno != MPID_NEM_MPICH2_AGAIN) && (_n_iov > 0))
	{
	    MPIDI_DBG_PRINTF((55, FCNAME, "  sending packet with sendv \n"));
	    shmem_errno = MPID_nem_mpich2_sendv (&_iov, &_n_iov, vc->lpid);
	}

	if (shmem_errno == MPID_NEM_MPICH2_AGAIN)
	{
            /* Create a new request and save remaining portions of the
	     * iov in it. */
 	    sreq = MPIDI_CH3_Request_create();
	    assert(sreq != NULL);
	    MPIU_Object_set_ref(sreq, 2);
	    sreq->kind = MPID_REQUEST_SEND;
	    sreq->dev.ca = MPIDI_CH3_CA_COMPLETE;
	    for (j = 0; j < _n_iov; ++j)
	    {
		sreq->dev.iov[j] = _iov[j];
	    }
	    sreq->ch.iov_offset = 0;
	    sreq->dev.iov_count = _n_iov;
	    sreq->ch.vc = vc;
	    if ( iov == _iov )
	    {
		/* header was not sent */
		sreq->ch.pkt = *(MPIDI_CH3_Pkt_t *) iov[0].MPID_IOV_BUF;
		sreq->dev.iov[0].MPID_IOV_BUF = (char *) &sreq->ch.pkt;
		sreq->dev.iov[0].MPID_IOV_LEN = iov[0].MPID_IOV_LEN;
	    }
	    MPIDI_CH3I_SendQ_enqueue (sreq, CH3_NORMAL_QUEUE);
	    assert (MPIDI_CH3I_active_send[CH3_NORMAL_QUEUE] == NULL);
	    MPIDI_CH3I_active_send[CH3_NORMAL_QUEUE] = sreq;
	}
    }
    else
    {
	int i;
	
	MPIDI_DBG_PRINTF((55, FCNAME, "request enqueued"));
	/* create a request */
	sreq = MPIDI_CH3_Request_create();
	assert(sreq != NULL);
	MPIU_Object_set_ref(sreq, 2);
	sreq->kind = MPID_REQUEST_SEND;

	sreq->ch.pkt = *(MPIDI_CH3_Pkt_t *) iov[0].MPID_IOV_BUF;
	sreq->dev.iov[0].MPID_IOV_BUF = (char *) &sreq->ch.pkt;
	sreq->dev.iov[0].MPID_IOV_LEN = iov[0].MPID_IOV_LEN;

	/* copy iov */
	for (i = 1; i < n_iov; ++i)
	{
	    sreq->dev.iov[i] = iov[i];
	}

	sreq->ch.iov_offset = 0;
	sreq->dev.iov_count = n_iov;
	sreq->dev.ca = MPIDI_CH3_CA_COMPLETE;
	sreq->ch.vc = vc;
	MPIDI_CH3I_SendQ_enqueue (sreq, CH3_NORMAL_QUEUE);
    }
    
    *sreq_ptr = sreq;

    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISTARTMSGV);
    return mpi_errno;
}
