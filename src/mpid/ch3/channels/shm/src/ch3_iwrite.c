/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/*
 * MPIDI_CH3_iWrite()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iWrite
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_iWrite(MPIDI_VC * vc, MPID_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    int nb;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_IWRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_IWRITE);

    MPIDI_DBG_PRINTF((71, FCNAME, "entering"));

    req->ch.iov_offset = 0;
    vc->ch.send_active = req;
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
	    
	    vc->ch.send_active = NULL;

	    mpi_errno = MPIDI_CH3U_Handle_send_req(vc, req);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WRITE_PROGRESS);
		return mpi_errno;
	    }
	    if (req->dev.iov_count == 0 && vc->ch.sendq_head == req)
	    {
		MPIDI_CH3I_SendQ_dequeue(vc);
	    }
	    vc->ch.send_active = MPIDI_CH3I_SendQ_head(vc);
	}
	else
	{
	    /*assert(req->ch.iov_offset < req->dev.iov_count);*/
	    if (req->ch.iov_offset >= req->dev.iov_count)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**iov_offset", "**iov_offset %d %d", req->ch.iov_offset, req->dev.iov_count);
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_IWRITE);
		return mpi_errno;
	    }
	}
    }
    else if (nb == 0)
    {
	MPIDI_DBG_PRINTF((55, FCNAME, "unable to write, enqueuing"));
	if (MPIDI_CH3I_SendQ_head(vc) != req)
	{
	    MPIDI_CH3I_SendQ_enqueue(vc, req);
	}
    }
    else
    {
	/* Connection just failed.  Mark the request complete and return an error. */
	vc->ch.state = MPIDI_CH3I_VC_STATE_FAILED;
	/* TODO: Create an appropriate error message based on the value of errno */
	req->status.MPI_ERROR = MPI_ERR_INTERN;
	/* MT - CH3U_Request_complete performs write barrier */
	MPIDI_CH3U_Request_complete(req);
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_IWRITE);
    return MPI_SUCCESS;
}
