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
    MPIU_Size_t nb;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_IREAD);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_IREAD);
#ifdef MPICH_DBG_OUTPUT
    /*assert(vc->sc.state == MPIDI_CH3I_VC_STATE_CONNECTED);*/
    if (vc->sc.state != MPIDI_CH3I_VC_STATE_CONNECTED)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**vc_state", "**vc_state %d", vc->sc.state);
	goto fn_exit;
    }
#endif

    mpi_errno = MPIDU_Sock_readv(vc->sc.sock, rreq->ch3.iov, rreq->ch3.iov_count, &nb);
    if (mpi_errno == MPI_SUCCESS)
    {
	rreq->sc.iov_offset = 0;
	while (rreq->sc.iov_offset < rreq->ch3.iov_count)
	{
	    if ((MPIU_Size_t)rreq->ch3.iov[rreq->sc.iov_offset].MPID_IOV_LEN <= nb)
	    {
		nb -= rreq->ch3.iov[rreq->sc.iov_offset].MPID_IOV_LEN;
		rreq->sc.iov_offset += 1;
	    }
	    else
	    {
		rreq->ch3.iov[rreq->sc.iov_offset].MPID_IOV_BUF = (char *) rreq->ch3.iov[rreq->sc.iov_offset].MPID_IOV_BUF + nb;
		rreq->ch3.iov[rreq->sc.iov_offset].MPID_IOV_LEN -= nb;
		mpi_errno = MPIDI_CH3I_VC_post_read(vc, rreq);
		goto fn_exit;
	    }
	}

	/* FIXME: excessive recursion... */
	MPIDI_CH3U_Handle_recv_req(vc, rreq);
    }
    else
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_IREAD);
    return mpi_errno;
}
