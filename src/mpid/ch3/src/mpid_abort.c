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
    MPIDI_dbg_printf(10, FCNAME, "entering");

    MPIDI_err_printf(FCNAME, "MPID_Abort() is not properly implemented!\n");
    abort();
    
    MPIDI_dbg_printf(10, FCNAME, "exiting");

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_ABORT);
    return MPI_ERR_INTERN;
}

