/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*
 * MPID_Recv_init()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Recv_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Recv_init(void * buf, int count, MPI_Datatype datatype,
		   int rank, int tag, MPID_Comm * comm, int context_offset,
		   MPID_Request ** request)
{
    MPID_Request * rreq;
    
    rreq = MPIDI_CH3_Request_create();
    if (rreq != NULL)
    {
	rreq->ref_count = 1;
	rreq->kind = MPID_PREQUEST_RECV;
	rreq->comm = comm;
	rreq->ch3.match.rank = rank;
	rreq->ch3.match.tag = tag;
	rreq->ch3.match.context_id = comm->context_id + context_offset;
	rreq->ch3.user_buf = (void *) buf;
	rreq->ch3.user_count = count;
	rreq->ch3.datatype = datatype;
	MPIDI_Request_set_persistent_type(rreq, MPIDI_REQUEST_PERSISTENT_RECV);
	
	*request = rreq;
	
	return MPI_SUCCESS;
    }

    MPIDI_DBG_PRINTF((15, FCNAME, "request allocation failed"));
    return MPIR_ERR_MEMALLOCFAILED;
}
