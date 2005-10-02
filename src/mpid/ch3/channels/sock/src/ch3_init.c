/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/*
 *  MPIDI_CH3_Init  - makes socket specific initializations.  Most of this functionality
 *                      is in the MPIDI_CH3U_Init_sock upcall because the same tasks need
 *                      to be done for the ssh (sock + shm) channel.  As a result, it
 *                      must except a lot more arguements.
 */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Init(int has_parent, MPIDI_PG_t * pg_p, int pg_rank,
                    char **publish_bc_p, char **bc_key_p, char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_CH3_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_CH3_INIT);

    /* initialize aspects specific to sockets  */
    mpi_errno = MPIDI_CH3U_Init_sock(has_parent, pg_p, pg_rank,
                               publish_bc_p, bc_key_p, bc_val_p, val_max_sz_p);

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_CH3_INIT);
    return mpi_errno;
}

/* This function simply tells the CH3 device to use the defaults for the 
   MPI Port functions */
int MPIDI_CH3_PortFnsInit( MPIDI_PortFns *a ) 
{ 
    return 0;
}
