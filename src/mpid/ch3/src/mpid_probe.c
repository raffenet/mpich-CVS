/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Probe
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Probe(int source, int tag, MPID_Comm * comm, int context_offset, MPI_Status * status)
{
    MPID_Request * rreq;
    const int context = comm->context_id + context_offset;
    MPIDI_STATE_DECL(MPID_STATE_MPID_PROBE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_PROBE);

    do
    {
	MPIDI_CH3_Progress_start();

	rreq = MPIDI_CH3U_Request_FU(source, tag, context);
	if (rreq != NULL)
	{
	    if (status != MPI_STATUS_IGNORE)
		*status = rreq->status;
	    
	    MPID_Request_release(rreq);
	    MPIDI_CH3_Progress_end();
	    break;
	}
	else
	{
	    MPIDI_CH3_Progress(TRUE);
	}
    }
    while(TRUE);

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_PROBE);
    return MPI_SUCCESS;
}
