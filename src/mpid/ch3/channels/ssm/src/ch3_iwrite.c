/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/*
 * MPIDI_CH3I_Request_adjust_iov()
 *
 * Adjust the iovec in the request by the supplied number of bytes.  If the iovec has been consumed, return true; otherwise return
 * false.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Request_adjust_iov
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Request_adjust_iov(MPID_Request * req, MPIDI_msg_sz_t nb)
{
    int offset = req->ssm.iov_offset;
    const int count = req->dev.iov_count;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_REQUEST_ADJUST_IOV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_REQUEST_ADJUST_IOV);
    
    while (offset < count)
    {
	if (req->dev.iov[offset].MPID_IOV_LEN <= (unsigned int)nb)
	{
	    nb -= req->dev.iov[offset].MPID_IOV_LEN;
	    offset++;
	}
	else
	{
	    req->dev.iov[offset].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)((char*)req->dev.iov[offset].MPID_IOV_BUF + nb);
	    req->dev.iov[offset].MPID_IOV_LEN -= nb;
	    req->ssm.iov_offset = offset;
	    MPIDI_DBG_PRINTF((60, FCNAME, "adjust_iov returning FALSE"));
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_REQUEST_ADJUST_IOV);
	    return FALSE;
	}
    }
    
    req->ssm.iov_offset = offset;

    MPIDI_DBG_PRINTF((60, FCNAME, "adjust_iov returning TRUE"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_REQUEST_ADJUST_IOV);
    return TRUE;
}

/*
 * MPIDI_CH3_iWrite()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iWrite
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_iWrite(MPIDI_VC * vc, MPID_Request * req)
{
    int nb, mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_IWRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_IWRITE);

    MPIDI_DBG_PRINTF((71, FCNAME, "entering"));

    if (vc->ssm.bShm)
    {
	req->ssm.iov_offset = 0;
	vc->ssm.send_active = req;
	mpi_errno = (req->dev.iov_count == 1) ?
	    MPIDI_CH3I_SHM_write(vc, req->dev.iov->MPID_IOV_BUF, req->dev.iov->MPID_IOV_LEN, &nb) :
	    MPIDI_CH3I_SHM_writev(vc, req->dev.iov, req->dev.iov_count, &nb);
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmwrite", 0);
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_IWRITE);
	    return mpi_errno;
	}

	if (nb > 0)
	{
	    if (MPIDI_CH3I_Request_adjust_iov(req, nb))
	    {
		/* Write operation complete */
		MPIDI_CA_t ca = req->dev.ca;

		vc->ssm.send_active = NULL;

		if (ca == MPIDI_CH3_CA_COMPLETE)
		{
		    /* FIXME: MT: the queuing code is not thread safe */
		    if (MPIDI_CH3I_SendQ_head(vc) == req)
		    {
			MPIDI_CH3I_SendQ_dequeue(vc);
		    }
		    vc->ssm.send_active = MPIDI_CH3I_SendQ_head(vc);
		    /* mark data transfer as complete and decrment CC */
		    req->dev.iov_count = 0;
		    MPIDI_CH3U_Request_complete(req);
		}
		else if (ca == MPIDI_CH3I_CA_HANDLE_PKT)
		{
		    MPIDI_CH3_Pkt_t * pkt = &req->ssm.pkt;

		    if (pkt->type < MPIDI_CH3_PKT_END_CH3)
		    {
			vc->ssm.send_active = MPIDI_CH3I_SendQ_head(vc);
		    }
		    else
		    {
			MPIDI_DBG_PRINTF((71, FCNAME, "unknown packet type %d", pkt->type));
		    }
		}
		else if (ca < MPIDI_CH3_CA_END_CH3)
		{
		    MPIDI_DBG_PRINTF((71, FCNAME, "finished sending iovec, calling CH3U_Handle_send_req()"));
		    MPIDI_CH3U_Handle_send_req(vc, req);
		    /* FIXME: MT: the queuing code is not thread safe */
		    if (MPIDI_CH3I_SendQ_head(vc) == req && req->dev.iov_count == 0)
		    {
			/* NOTE: This code assumes that if the iov_count is zero, then the device has completed the current
			   request.  As a result, the current request is dequeded and next request in the queue is processed. */
			MPIDI_DBG_PRINTF((71, FCNAME, "request (assumed) complete, dequeuing req and posting next send"));
			MPIDI_CH3I_SendQ_dequeue(vc);
		    }
		    vc->ssm.send_active = MPIDI_CH3I_SendQ_head(vc);
		}
		else
		{
		    /*assert(ca < MPIDI_CH3I_CA_END_SSM_CHANNEL);*/
		    if (ca >= MPIDI_CH3I_CA_END_SSM_CHANNEL)
		    {
			mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ca", "**ca %d", ca);
			MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_IWRITE);
			return mpi_errno;
		    }
		}
	    }
	    else
	    {
		/*assert(req->ssm.iov_offset < req->dev.iov_count);*/
		if (req->ssm.iov_offset >= req->dev.iov_count)
		{
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**iov_offset", "**iov_offset %d %d", req->ssm.iov_offset, req->dev.iov_count);
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_IWRITE);
		    return mpi_errno;
		}
	    }
	}
	else if (nb == 0)
	{
	    MPIDI_DBG_PRINTF((55, FCNAME, "unable to write, enqueuing"));
	    MPIDI_CH3I_SendQ_enqueue(vc, req);
	}
	else
	{
	    /* Connection just failed.  Mark the request complete and return an error. */
	    vc->ssm.state = MPIDI_CH3I_VC_STATE_FAILED;
	    /* TODO: Create an appropriate error message based on the value of errno */
	    req->status.MPI_ERROR = MPI_ERR_INTERN;
	    /* MT - CH3U_Request_complete performs write barrier */
	    MPIDI_CH3U_Request_complete(req);
	}
    }
    else
    {
	/*assert(vc->ssm.state == MPIDI_CH3I_VC_STATE_CONNECTED);*/
	if (vc->ssm.state != MPIDI_CH3I_VC_STATE_CONNECTED)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**vc_state", "**vc_state %d", vc->ssm.state);
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_IWRITE);
	    return mpi_errno;
	}
	req->ssm.iov_offset = 0;
	MPIDI_CH3I_SSM_VC_post_write(vc, req);
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_IWRITE);
    return MPI_SUCCESS;
}
