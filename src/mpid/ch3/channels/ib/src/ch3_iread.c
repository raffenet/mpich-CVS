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
void MPIDI_CH3_iRead(MPIDI_VC * vc, MPID_Request * req)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_IREAD);
#if 0
    MPIDI_STATE_DECL(MPID_STATE_READV);
#endif

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_IREAD);
    assert(vc->tcp.state = MPIDI_CH3I_VC_STATE_CONNECTED);
    req->tcp.iov_offset = 0;
    
    MPIDI_CH3I_TCP_post_read(vc, req);

#if 0
    /* XXX: if we complete a request now, how do we convey that completion to
       the progress engine so that the progress engine knows to stop blocking?
       we could stick the completion information in a request field or possibly
       in the VC. */
    do
    {
	MPIDI_FUNC_ENTER(MPID_STATE_READV);
	nb = readv(vc->tcp.fd, req->ch3.iov + req->tcp.iov_offset,
		   req->ch3.iov_count);
	MPIDI_FUNC_EXIT(MPID_STATE_READV);
    }
    while (nb == -1 && errno == EINTR);
    
    if (nb > 0)
    {
	if (MPIDI_CH3U_Request_adjust_iov(req, nb))
	{
	    /* Read operation complete */
	    MPIDI_CA_t ca = req->ch3.ca;
			
	    if (ca == MPIDI_CH3_CA_COMPLETE)
	    {
		int cc;
		
		MPIDI_DBG_PRINTF((65, FCNAME, "received requested data, decrementing CC");
		MPIDI_CH3U_Request_decrement_cc(req, &cc);
		if (cc == 0)
		{
		    MPIDI_CH3I_Progress_notify_completion();
		    MPID_Request_release(req);
		}
	    }
	    else if (ca < MPIDI_CH3_CA_END_CH3)
	    {
		/* XXX - This code assumes that if another read is not
		   posted by the device during the callback, then the
		   device is not expecting any more data for request.  As a
		   result, the channels posts a read for another packet */
		MPIDI_DBG_PRINTF((65, FCNAME, "finished receiving iovec, calling CH3U_Handle_recv_req()"));
		if (MPIDI_CH3U_Handle_recv_req(poll_infos[elem].vc, req))
		{
		    MPIDI_CH3I_Progress_notify_completion();
		}
	    }
	    else
	    {
		assert(ca < MPIDI_CH3_CA_END_CH3);
	    }
	}
	else
	{
	    MPIDI_CH3I_TCP_post_read(vc, req);
	}
    }
#endif

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_IREAD);
}
