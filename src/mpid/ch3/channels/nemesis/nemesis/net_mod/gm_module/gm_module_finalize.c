/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "gm_module_impl.h"
#include "gm.h"


#undef FUNCNAME
#define FUNCNAME MPID_nem_gm_module_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_gm_module_finalize()
{
    int mpi_errno = MPI_SUCCESS;
    int max_send_tokens;
    MPID_nem_gm_module_send_queue_t *e;

    max_send_tokens = gm_num_send_tokens (MPID_nem_module_gm_port);
    
    while (MPID_nem_module_gm_num_send_tokens < max_send_tokens && !MPID_nem_gm_module_queue_empty (send))
    {
	MPID_nem_gm_module_recv_poll();
    }
    
    while (MPID_nem_gm_module_send_free_queue)
    {
	e = MPID_nem_gm_module_send_free_queue;
	MPID_nem_gm_module_send_free_queue = e->next;
	MPIU_Free (e);
    }
    
    MPID_nem_gm_module_lmt_finalize();
    
    gm_finalize();

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_gm_module_ckpt_shutdown
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_gm_module_ckpt_shutdown ()
{
    /* for GM we can't touch the network because the original process is still using it */
    return MPI_SUCCESS;
}

