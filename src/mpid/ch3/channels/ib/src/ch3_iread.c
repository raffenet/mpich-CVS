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

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_IREAD);
    assert(vc->ib.state = MPIDI_CH3I_VC_STATE_CONNECTED);
    req->ib.iov_offset = 0;

    MPIU_dbg_printf("ch3_iread\n");
    MPIDI_CH3I_IB_post_read(vc, req);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_IREAD);
}
