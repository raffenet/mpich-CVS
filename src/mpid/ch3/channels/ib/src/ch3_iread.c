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
int MPIDI_CH3_iRead(MPIDI_VC * vc, MPID_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_IREAD);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_IREAD);
    assert(vc->ib.state = MPIDI_CH3I_VC_STATE_CONNECTED);
    req->ib.iov_offset = 0;

    MPIDI_DBG_PRINTF((60, FCNAME, "ch3_iread\n"));
    vc->ib.recv_active = req;
    ibu_post_readv(vc->ib.ibu, req->ch3.iov + req->ib.iov_offset, req->ch3.iov_count - req->ib.iov_offset, NULL);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_IREAD);
    return mpi_errno;
}
