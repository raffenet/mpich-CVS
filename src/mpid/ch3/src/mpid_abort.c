/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Abort
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Abort(MPID_Comm * comm, int errorcode)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_ABORT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_ABORT);
    MPIDI_DBG_PRINTF((10, FCNAME, "entering"));

    fprintf(stderr, "ABORT - process %d: %s\n", MPIR_Process.comm_world->rank, MPIR_Err_get_string(errorcode));
    fflush(stderr);
    abort();
    
    MPIDI_DBG_PRINTF((10, FCNAME, "exiting"));

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_ABORT);
    return MPI_ERR_INTERN;
}

