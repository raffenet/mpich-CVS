/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Iprobe
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Iprobe(int source, int tag, MPID_Comm * comm, int context_offset, int * flag, MPI_Status * status)
{
    MPID_Request * rreq;
    const int context = comm->context_id + context_offset;
    MPIDI_STATE_DECL(MPID_STATE_MPID_IPROBE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_IPROBE);
    
    rreq = MPIDI_CH3U_Request_FU(source, tag, context);
    if (rreq != NULL)
    {
	*status = rreq->status;
	MPID_Request_release(rreq);
	*flag = TRUE;
    }
    else
    {
	MPID_Progress_poke();
	*flag = FALSE;
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_IPROBE);
    return MPI_SUCCESS;
}
