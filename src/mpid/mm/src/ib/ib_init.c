/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "ibimpl.h"

IB_PerProcess IB_Process;
MPIDI_VC_functions g_ib_vc_functions = 
{
    ib_post_read,
    ib_enqueue_read_at_head,
    ib_merge_with_unexpected,
    ib_merge_with_posted,
    ib_merge_unexpected_data,
    ib_post_write,
    ib_enqueue_write_at_head,
    ib_reset_car,
    ib_setup_packet_car,
    ib_post_read_pkt
};

/*@
   ib_init - initialize the ib method

   Notes:
@*/
int ib_init()
{
    MPIDI_STATE_DECL(MPID_STATE_IB_INIT);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_INIT);

    /*
    ib_init_us();

    ib_init_globals();
    PMI_KVS_Put(MPID_Process.pmi_kvsname, key, value);
    PMI_Barrier();
    ib_setup_connections();
    */

    MPIDI_FUNC_EXIT(MPID_STATE_IB_INIT);
    return MPI_SUCCESS;
}

/*@
   ib_finalize - finalize the ib method

   Notes:
@*/
int ib_finalize()
{
    MPIDI_STATE_DECL(MPID_STATE_IB_FINALIZE);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_FINALIZE);

    /*
    ib_release_us();
    */

    MPIDI_FUNC_EXIT(MPID_STATE_IB_FINALIZE);
    return MPI_SUCCESS;
}
