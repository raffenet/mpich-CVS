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
    MPIDI_dbg_printf(10, FCNAME, "entering");

    MPIDI_err_printf(FCNAME, "MPID_Abort() is not properly implementated!\n");
    abort();
    
    MPIDI_dbg_printf(10, FCNAME, "exiting");
    return MPI_ERR_INTERN;
}

