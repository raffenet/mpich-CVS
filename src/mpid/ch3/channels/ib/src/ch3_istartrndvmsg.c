/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

#ifdef MPIDI_CH3_CHANNEL_RNDV

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iStartRndvMsg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_iStartRndvMsg(MPIDI_VC_t * vc, MPID_Request * sreq, MPIDI_CH3_Pkt_t * rts_pkt)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request * rts_sreq;
    int i;
#ifdef USE_RDMA_GET
    MPIDI_CH3_Pkt_t pkt;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ISTARTRNDVMSG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ISTARTRNDVMSG);

#ifdef USE_RDMA_GET

    MPIDI_Pkt_init(&pkt.rts_iov, MPIDI_CH3_PKT_RTS_IOV);
    pkt.rts_iov.sreq = sreq->handle;
    pkt.rts_iov.iov_len = sreq->dev.iov_count;

    sreq->dev.rdma_iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&pkt;
    sreq->dev.rdma_iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
    sreq->dev.rdma_iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)sreq->dev.iov;
    sreq->dev.rdma_iov[1].MPID_IOV_LEN = sreq->dev.iov_count * sizeof(MPID_IOV);
    sreq->dev.rdma_iov[2].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&sreq->ch.local_iov_mem[0];
    sreq->dev.rdma_iov[2].MPID_IOV_LEN = sreq->dev.iov_count * sizeof(ibu_mem_t);
    /* Save a copy of the rts packet in case it is not completely written by the iStartMsgv call */
    /*sreq->dev.rdma_iov[3].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)rts_pkt;*/
    sreq->ch.pkt = *rts_pkt;
    sreq->dev.rdma_iov[3].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&sreq->ch.pkt;
    sreq->dev.rdma_iov[3].MPID_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);

    MPIU_DBG_PRINTF(("registering the sender's iov.\n"));
    for (i=0; i<sreq->dev.iov_count; i++)
    {
	ibu_register_memory(sreq->dev.iov[i].MPID_IOV_BUF,
			    sreq->dev.iov[i].MPID_IOV_LEN,
			    &sreq->ch.local_iov_mem[i]);
    }
#ifdef MPICH_DBG_OUTPUT
    for (i=0; i<sreq->dev.iov_count; i++)
    {
	MPIU_DBG_PRINTF(("iStartRndvMsg: send buf[%d] = %p, len = %d\n",
			 i, sreq->dev.iov[i].MPID_IOV_BUF, sreq->dev.iov[i].MPID_IOV_LEN));
    }
#endif
    mpi_errno = MPIDI_CH3_iStartMsgv(vc, sreq->dev.rdma_iov, 4, &rts_sreq);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
	MPIU_Object_set_ref(sreq, 0);
	MPIDI_CH3_Request_destroy(sreq);
	sreq = NULL;
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|rtspkt", 0);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */
    if (rts_sreq != NULL)
    {
	/* The sender doesn't need to know when the message has been sent.
	   So release the request immediately */
	MPID_Request_release(rts_sreq);
    }

#else

#ifdef MPICH_DBG_OUTPUT
    for (i=0; i<sreq->dev.iov_count; i++)
    {
	MPIU_DBG_PRINTF(("iStartRndvMsg: send buf[%d] = %p, len = %d\n",
			 i, sreq->dev.iov[i].MPID_IOV_BUF, sreq->dev.iov[i].MPID_IOV_LEN));
    }
#endif
    sreq->ch.riov_offset = 0;
    mpi_errno = MPIDI_CH3_iStartMsg(vc, rts_pkt, sizeof(*rts_pkt), &rts_sreq);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
	MPIU_Object_set_ref(sreq, 0);
	MPIDI_CH3_Request_destroy(sreq);
	sreq = NULL;
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|rtspkt", 0);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */
    if (rts_sreq != NULL)
    {
	/* The sender doesn't need to know when the packet has been sent.
	   So release the request immediately */
	MPID_Request_release(rts_sreq);
    }
    MPIU_DBG_PRINTF(("registering the sender's iov.\n"));
    for (i=0; i<sreq->dev.iov_count; i++)
    {
	ibu_register_memory(sreq->dev.iov[i].MPID_IOV_BUF,
			    sreq->dev.iov[i].MPID_IOV_LEN,
			    &sreq->ch.local_iov_mem[i]);
    }

#endif

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISTARTRNDVMSG);
    return mpi_errno;
}

#endif /*MPIDI_CH3_CHANNEL_RNDV*/
