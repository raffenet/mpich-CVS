/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Init(int has_parent, MPIDI_PG_t * pg_p, int pg_rank,
                   char **publish_bc_p, char **bc_key_p, char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPID_CH3_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_CH3_INIT);

    mpi_errno = MPIDI_CH3U_Init_sshm(has_parent, pg_p, pg_rank,
                               publish_bc_p, bc_key_p, bc_val_p, val_max_sz_p);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER, "**fail");
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_CH3_INIT);
    return mpi_errno;
}

/* This function simply tells the CH3 device to use the defaults for the 
   MPI Port functions.  This should be ok here, since we want to 
   use the socket routines to perform all connect/accept actions
   after MPID_Init returns (see the shm_unlink discussion) */
int MPIDI_CH3_PortFnsInit( MPIDI_PortFns *a ) 
{ 
    return 0;
}

/* Perform the channel-specific vc initialization */
int MPIDI_CH3_VC_Init( MPIDI_VC_t *vc ) {
    vc->ch.sendq_head         = NULL;
    vc->ch.sendq_tail         = NULL;

    /* Which of these do we need? */
    vc->ch.recv_active        = NULL;
    vc->ch.send_active        = NULL;
    vc->ch.req                = NULL;
    vc->ch.read_shmq          = NULL;
    vc->ch.write_shmq         = NULL;
    vc->ch.shm                = NULL;
    vc->ch.shm_state          = 0;
    return 0;
}
