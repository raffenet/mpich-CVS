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
int MPID_Iprobe(int source, int tag, MPID_Comm * comm, int context_offset,
		MPI_Status * status)
{
    MPID_Request * rreq;
    
    rreq = MPIDI_CH3U_Request_FU(source, tag, 
				 comm->context_id + context_offset);
    if (rreq != NULL)
    {
	*status = rreq->status;
	MPID_Request_release(rreq);
    }
    
    return MPI_ERR_INTERN;
}
