/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpidi_ch3_impl.h"

/* These routines are invoked by the ch3 connect and accept code, 
   with the MPIDI_CH3_HAS_CONN_ACCEPT_HOOK ifdef */

/* The algorithm is for the connect side to send a MPIDI_CH3I_PKT_SC_CONN_ACCEPT
 *  pkt directly to the socket (NOT via a VC) and for the accept side to ACK with its
 *  own PG and pg_rank.  VCs are determined (or malloc'd) only when receiving this
 *  packet type.  This means that we can selectively allocate tmp VCs on each side;
 *  allocating VCs isn't the (main) problem though.  A more central issue is keeping
 *  the critical path general and a large part of this is
 *  keeping the hash table consistant.  If two VCs (a tmp one and a non-tmp one) have
 *  the same SCTP association ID, they can't be distinguished easily/instantly so we
 *  attempt to reuse them whenever possible to keep the critical path simple.
 */

/* For sctp, only MPIDI_CH3_Cleanup_after_connection is needed by both sides */

/* called by MPIDI_Create_inter_root_communicator_connect */
int MPIDI_CH3_Complete_unidirectional_connection( MPIDI_VC_t *connect_vc )
{
    int mpi_errno = MPI_SUCCESS;
    return mpi_errno;
}

/* called by MPIDI_Create_inter_root_communicator_accept */
int MPIDI_CH3_Complete_unidirectional_connection2( MPIDI_VC_t *new_vc )
{
    int mpi_errno = MPI_SUCCESS;
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Cleanup_after_connection
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Cleanup_after_connection( MPIDI_VC_t *new_vc )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_CLEANUP_AFTER_CONNECTION);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_CLEANUP_AFTER_CONNECTION);

    /* remove tmp VC from hash so assocID can be reused with real VC */
    if(new_vc->pg == NULL)
    {
        hash_delete(MPIDI_CH3I_assocID_table, new_vc->ch.sinfo_assoc_id);
        MPIU_Free(new_vc);
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_CLEANUP_AFTER_CONNECTION);
    return mpi_errno;
}
