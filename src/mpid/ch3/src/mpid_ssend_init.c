/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*
 * MPID_Ssend_init()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Ssend_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Ssend_init(const void * buf, int count, MPI_Datatype datatype,
		    int rank, int tag, MPID_Comm * comm, int context_offset,
		    MPID_Request ** request)
{
    MPID_Request * sreq;
    
    sreq = MPIDI_CH3_Request_create();
    if (sreq != NULL)
    {
	sreq->ref_count = 1;
	sreq->kind = MPID_PREQUEST_SEND;
	sreq->comm = comm;
	sreq->ch3.match.rank = rank;
	sreq->ch3.match.tag = tag;
	sreq->ch3.match.context_id = comm->context_id + context_offset;
	sreq->ch3.user_buf = (void *) buf;
	sreq->ch3.user_count = count;
	sreq->ch3.datatype = datatype;
	MPIDI_Request_set_persistent_type(sreq,
					  MPIDI_REQUEST_PERSISTENT_SSEND);
	
	*request = sreq;
	
	return MPI_SUCCESS;
    }

    MPIDI_DBG_PRINTF((15, FCNAME, "request allocation failed"));
    return MPIR_ERR_MEMALLOCFAILED;
}
