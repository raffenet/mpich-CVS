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
void MPIDI_CH3_iRead(MPIDI_VC * vc, MPID_Request * rreq)
{
    int sock_errno;
    sock_size_t nb;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_IREAD);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_IREAD);
    assert(vc->sc.state == MPIDI_CH3I_VC_STATE_CONNECTED);

    sock_errno = sock_readv(vc->sc.sock, rreq->ch3.iov, rreq->ch3.iov_count, &nb);
    if (sock_errno == SOCK_SUCCESS)
    {
	rreq->sc.iov_offset = 0;
	while (rreq->sc.iov_offset < rreq->ch3.iov_count)
	{
	    if ((sock_size_t)rreq->ch3.iov[rreq->sc.iov_offset].MPID_IOV_LEN <= nb)
	    {
		nb -= rreq->ch3.iov[rreq->sc.iov_offset].MPID_IOV_LEN;
		rreq->sc.iov_offset += 1;
	    }
	    else
	    {
		rreq->ch3.iov[rreq->sc.iov_offset].MPID_IOV_BUF = (char *) rreq->ch3.iov[rreq->sc.iov_offset].MPID_IOV_BUF + nb;
		rreq->ch3.iov[rreq->sc.iov_offset].MPID_IOV_LEN -= nb;
		MPIDI_CH3I_VC_post_read(vc, rreq);
		goto fn_exit;
	    }
	}

	/* FIXME: excessive recursion... */
	MPIDI_CH3U_Handle_recv_req(vc, rreq);
    }
    else
    {
	int mpi_errno;

	mpi_errno = MPIDI_CH3I_sock_errno_to_mpi_errno(fcname, sock_errno);
	MPID_Abort(NULL, mpi_errno);
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_IREAD);
}
