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
void MPIDI_CH3_iWrite(MPIDI_VC * vc, MPID_Request * req)
{
    int nb;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_IWRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_IWRITE);
    assert(vc->ib.state = MPIDI_CH3I_VC_STATE_CONNECTED);
    req->ib.iov_offset = 0;

    MPIU_dbg_printf("ch3_iwrite\n");
    /*MPIDI_CH3I_IB_post_write(vc, req);*/
    vc->ib.send_active = req;
    nb = ibu_post_writev(vc->ib.ibu, req->ch3.iov + req->ib.iov_offset, req->ch3.iov_count - req->ib.iov_offset, NULL);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_IWRITE);
}
