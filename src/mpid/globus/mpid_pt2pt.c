/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"
#include "../../../mpi/pt2pt/bsendutil.h"


/*
 * MPID_Cancel_send()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Cancel_send
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Cancel_send(MPID_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_CANCEL_SEND);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_CANCEL_SEND);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    
    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_CANCEL_SEND);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Cancel_send() */


/*
 * MPID_Cancel_recv()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Cancel_recv
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Cancel_recv(MPID_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_CANCEL_RECV);
    
    MPIG_FUNC_ENTER(MPID_STATE_MPID_CANCEL_RECV);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);
    
  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_CANCEL_RECV);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Cancel_recv() */


/*
 * MPID_Probe()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Probe
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Probe(int source, int tag, MPID_Comm * comm, int ctxoff, MPI_Status * status)
{
    MPID_Progress_state pe_state;
    MPID_Request * rreq;
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_PROBE);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_PROBE);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPID_Progress_start(&pe_state);
    do
    {
	rreq = mpig_recvq_find_unexp(source, tag, ctx);
	if (rreq != NULL)
	{
	    MPIR_Request_extract_status(rreq, status);
	    mpig_request_unlock(rreq);
	    break;
	}

	mpi_errno = MPID_Progress_wait(&pe_state);
	if (mpi_errno != MPI_SUCCESS)
	{
	    /* --BEGIN ERROR HANDLING-- */
	    MPID_Progress_end(&pe_state);
	    goto fn_fail;
	    /* --END ERROR HANDLING-- */
	}
    }
    while(mpi_errno == MPI_SUCCESS);
    MPID_Progress_end(&pe_state);

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_PROBE);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Probe() */


#undef FUNCNAME
#define FUNCNAME MPID_Iprobe
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Iprobe(int source, int tag, MPID_Comm * comm, int ctxoff, int * flag, MPI_Status * status)
{
    MPID_Request * rreq;
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_IPROBE);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_IPROBE);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    
    rreq = mpig_recvq_find_unexp(source, tag, ctx);
    if (rreq != NULL)
    {
	MPIR_Request_extract_status(rreq, status);
	mpig_request_unlock(rreq);
	*flag = TRUE;
    }
    else
    {
	mpi_errno = MPID_Progress_poke();
	if (mpi_errno != MPI_SUCCESS) goto fn_fail;
	*flag = FALSE;
    }

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_IPROBE);
    return mpi_errno;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Iprobe() */


/*
 * MPID_Startall()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Startall
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Startall(int count, MPID_Request * requests[])
{
    int i;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_STARTALL);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_STARTALL);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    for (i = 0; i < count; i++)
    {
	int rank;
	int tag;
	int ctx;
	void * buf;
	int cnt;
	MPI_Datatype dt;
	MPID_Request * const preq = requests[i];

	/* XXX: MT: mpig_request_lock(preq); -- needed for RC systems? */
	mpig_request_get_envelope(preq, &rank, &tag, &ctx);
	mpig_request_get_buffer(preq, &buf, &cnt, &dt);

	switch (mpig_request_get_type(preq))
	{
	    case MPIG_REQUEST_TYPE_RECV:
	    {
		rc = MPID_Irecv(buf, cnt, dt, rank, tag, preq->comm, ctx - preq->comm->context_id, &preq->partner_request);
		break;
	    }
	    
	    case MPIG_REQUEST_TYPE_SEND:
	    {
		rc = MPID_Isend(buf, cnt, dt, rank, tag, preq->comm, ctx - preq->comm->context_id, &preq->partner_request);
		break;
	    }
		
	    case MPIG_REQUEST_TYPE_RSEND:
	    {
		rc = MPID_Irsend(buf, cnt, dt, rank, tag, preq->comm, ctx - preq->comm->context_id, &preq->partner_request);
		break;
	    }
		
	    case MPIG_REQUEST_TYPE_SSEND:
	    {
		rc = MPID_Issend(buf, cnt, dt, rank, tag, preq->comm, ctx - preq->comm->context_id, &preq->partner_request);
		break;
	    }

	    case MPIG_REQUEST_TYPE_BSEND:
	    {
		MPID_Request * sreq;
		
		rc = MPIR_Bsend_isend(buf, cnt, dt, rank, tag, preq->comm, BSEND_INIT, &sreq); /* sreq is thrown away */
		if (rc != MPI_SUCCESS) break;

		sreq = mpig_request_create();
		MPIU_ERR_CHKANDSTMT1((sreq == NULL), rc, MPI_ERR_OTHER, {break;}, "**nomem", "**nomem %s", "bsend request");
		
		mpig_request_init(sreq, MPID_REQUEST_SEND, MPIG_REQUEST_TYPE_BSEND, 1, 0, buf, cnt, dt, rank, tag, ctx);
		mpig_request_add_comm(sreq, preq->comm);
		mpig_request_add_dt(sreq, dt);
		preq->partner_request = sreq;
		
		break;
	    }

	    default:
	    {
		/* --BEGIN ERROR HANDLING-- */
		rc = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN, "**globus|badreqtype",
					  "**globus|badreqtype %d", mpig_request_get_type(preq));
		/* --END ERROR HANDLING-- */
	    }
	}
	
	if (rc == MPI_SUCCESS)
	{
	    preq->status.MPI_ERROR = MPI_SUCCESS;
	    preq->cc_ptr = &preq->partner_request->cc;
	}
	else
	{
	    /* --BEGIN ERROR HANDLING-- */
	    /* If a failure occurs attempting to start the request, then we assume that partner request was not created, and stuff
	       the error code in the persistent request.  The wait and test routines will look at the error code in the persistent
	       request if a partner request is not present. */
	    preq->partner_request = NULL;
	    preq->status.MPI_ERROR = rc;
	    preq->cc_ptr = &preq->cc;
	    preq->cc = 0;
	    /* --END ERROR HANDLING-- */
	}
	/* XXX: MT: mpig_request_unlock(preq); -- needed for RC systems? */
    }
    
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_STARTALL);
    return mpi_errno;
}
/* MPID_Startall() */


/*
 * MPID_Recv_init()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Recv_init
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Recv_init(void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** rreqp)
{
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_RECV_INIT);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_RECV_INIT);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    mpig_request_create_prreq(1, 0, buf, cnt, dt, rank, tag, ctx, comm, rreqp);
    MPIG_DBG_PRINTF((15, FCNAME, "persistent Recv request allocated, handle=0x%08x, ptr=%p", (*rreqp)->handle, *rreqp));
    
  fn_return:    
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_RECV_INIT);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Recv_init() */


/*
 * MPID_Send_init()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Send_init
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Send_init(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff,
		   MPID_Request ** sreqp)
{
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_SEND_INIT);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_SEND_INIT);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    mpig_request_create_psreq(MPIG_REQUEST_TYPE_SEND, 1, 0, (void *) buf, cnt, dt, rank, tag, ctx, comm, sreqp);
    MPIG_DBG_PRINTF((15, FCNAME, "persistent Send request allocated, handle=0x%08x, ptr=%p", (*sreqp)->handle, *sreqp));
    
  fn_return:    
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_SEND_INIT);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Send_init() */


/*
 * MPID_Rsend_init()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Rsend_init
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Rsend_init(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff,
		   MPID_Request ** sreqp)
{
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_RSEND_INIT);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_RSEND_INIT);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    mpig_request_create_psreq(MPIG_REQUEST_TYPE_RSEND, 1, 0, (void *) buf, cnt, dt, rank, tag, ctx, comm, sreqp);
    MPIG_DBG_PRINTF((15, FCNAME, "persistent Rsend request allocated, handle=0x%08x, ptr=%p", (*sreqp)->handle, *sreqp));
    
  fn_return:    
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_RSEND_INIT);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Rsend_init() */


/*
 * MPID_Ssend_init()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Ssend_init
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Ssend_init(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff,
		   MPID_Request ** sreqp)
{
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_SSEND_INIT);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_SSEND_INIT);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    mpig_request_create_psreq(MPIG_REQUEST_TYPE_SSEND, 1, 0, (void *) buf, cnt, dt, rank, tag, ctx, comm, sreqp);
    MPIG_DBG_PRINTF((15, FCNAME, "persistent Ssend request allocated, handle=0x%08x, ptr=%p", (*sreqp)->handle, *sreqp));
    
  fn_return:    
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_SSEND_INIT);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Ssend_init() */


/*
 * MPID_Bsend_init()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Bsend_init
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Bsend_init(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff,
		   MPID_Request ** sreqp)
{
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_BSEND_INIT);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_BSEND_INIT);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    mpig_request_create_psreq(MPIG_REQUEST_TYPE_BSEND, 1, 0, (void *) buf, cnt, dt, rank, tag, ctx, comm, sreqp);
    MPIG_DBG_PRINTF((15, FCNAME, "persistent Bsend request allocated, handle=0x%08x, ptr=%p", (*sreqp)->handle, *sreqp));
    
  fn_return:    
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_BSEND_INIT);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Bsend_init() */
