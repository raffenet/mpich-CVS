/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*
 * MPID_Isend()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Isend
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

int MPID_Isend(const void * buf, int count, MPI_Datatype datatype, int rank,
	       int tag, MPID_Comm * comm, int context_offset,
               MPID_Request ** request)
{
    MPIDI_dbg_printf(10, FCNAME, "NOT IMPLEMENTED!");
    abort();
    return MPI_ERR_INTERN;
}

