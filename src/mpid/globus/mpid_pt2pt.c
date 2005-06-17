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
	    MPID_Request_release(rreq);
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
	MPID_Request_release(rreq);
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
#if defined(XXX)
    int i;
    int rc;
#endif
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_STARTALL);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_STARTALL);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);
    
#if defined(XXX)
    for (i = 0; i < count; i++)
    {
	MPID_Request * const preq = requests[i];

	switch (mpig_request_get_type(preq))
	{
	    case MPIG_REQUEST_TYPE_RECV:
	    {
		rc = MPID_Irecv(preq->dev.user_buf, preq->dev.user_count, preq->dev.datatype, preq->dev.match.rank,
				preq->dev.match.tag, preq->comm, preq->dev.match.context_id - preq->comm->context_id,
				&preq->partner_request);
		break;
	    }
	    
	    case MPIG_REQUEST_TYPE_SEND:
	    {
		rc = MPID_Isend(preq->dev.user_buf, preq->dev.user_count, preq->dev.datatype, preq->dev.match.rank,
				preq->dev.match.tag, preq->comm, preq->dev.match.context_id - preq->comm->context_id,
				&preq->partner_request);
		break;
	    }
		
	    case MPIG_REQUEST_TYPE_RSEND:
	    {
		rc = MPID_Irsend(preq->dev.user_buf, preq->dev.user_count, preq->dev.datatype, preq->dev.match.rank,
				 preq->dev.match.tag, preq->comm, preq->dev.match.context_id - preq->comm->context_id,
				 &preq->partner_request);
		break;
	    }
		
	    case MPIG_REQUEST_TYPE_SSEND:
	    {
		rc = MPID_Issend(preq->dev.user_buf, preq->dev.user_count, preq->dev.datatype, preq->dev.match.rank,
				 preq->dev.match.tag, preq->comm, preq->dev.match.context_id - preq->comm->context_id,
				 &preq->partner_request);
		break;
	    }

	    case MPIG_REQUEST_TYPE_BSEND:
	    {
		MPID_Request * sreq;
		
		sreq = MPID_Request_create();
		MPIU_ERR_CHKANDJUMP1((rreq == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "send request");

		rc = MPIR_Bsend_isend(preq->dev.user_buf, preq->dev.user_count, preq->dev.datatype, preq->dev.match.rank,
				      preq->dev.match.tag, preq->comm, BSEND_INIT, &preq->partner_request);

		MPIU_Object_set_ref(sreq, 1);
		sreq->kind = MPID_REQUEST_SEND;
		sreq->cc   = 0;
		sreq->comm = NULL;
#if 0		    
		sreq->comm = preq->comm;
		MPIR_Comm_add_ref(sreq->comm);
#endif		    
		sreq->status.MPI_ERROR = rc;
		preq->partner_request = sreq;
		rc = MPI_SUCCESS;
		
		break;
	    }

	    default:
	    {
		/* --BEGIN ERROR HANDLING-- */
		rc = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN, "**ch3|badreqtype",
					  "**ch3|badreqtype %d", mpig_request_get_type(preq));
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
    }
#endif
    
  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_STARTALL);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Startall() */


/*
 * MPID_Recv_init()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Recv_init
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Recv_init(void * buf, int count, MPI_Datatype datatype, int rank, int tag, MPID_Comm * comm, int context_offset,
		   MPID_Request ** request)
{
#if defined(XXX)
    MPID_Request * rreq;
#endif
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_RECV_INIT);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_RECV_INIT);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    
    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);
    
#if defined(XXX)
    rreq = MPID_Request_create();
    MPIU_ERR_CHKANDJUMP1((rreq == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "receive request");

    MPIU_Object_set_ref(rreq, 1);
    rreq->kind = MPID_PREQUEST_RECV;
    rreq->comm = comm;
    MPIR_Comm_add_ref(comm);
    rreq->dev.match.rank = rank;
    rreq->dev.match.tag = tag;
    rreq->dev.match.context_id = comm->context_id + context_offset;
    rreq->dev.user_buf = (void *) buf;
    rreq->dev.user_count = count;
    rreq->dev.datatype = datatype;
    rreq->partner_request = NULL;
    mpig_request_set_type(rreq, MPIG_REQUEST_TYPE_RECV);
    if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
    {
	MPID_Datatype_get_ptr(datatype, rreq->dev.datatype_ptr);
	MPID_Datatype_add_ref(rreq->dev.datatype_ptr);
    }

    *request = rreq;
    MPIG_DBG_PRINTF((15, FCNAME, "receive request allocated, handle=0x%08x, ptr=%p", rreq->handle, rreq));
#endif
    
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
int MPID_Send_init(const void * buf, int count, MPI_Datatype datatype, int rank, int tag, MPID_Comm * comm, int context_offset,
		   MPID_Request ** request)
{
#if defined(XXX)
    MPID_Request * sreq = NULL;
#endif
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_SEND_INIT);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_SEND_INIT);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);
    
#if defined(XXX)
    mpig_request_create_psreq(sreq, mpi_errno, goto fn_fail);
    mpig_request_set_type(sreq, MPIG_REQUEST_TYPE_SEND);
    if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
    {
	MPID_Datatype_get_ptr(datatype, sreq->dev.datatype_ptr);
	MPID_Datatype_add_ref(sreq->dev.datatype_ptr);
    }
    *request = sreq;
#endif
    
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
int MPID_Rsend_init(const void * buf, int count, MPI_Datatype datatype, int rank, int tag, MPID_Comm * comm, int context_offset,
		    MPID_Request ** request)
{
#if defined(XXX)
    MPID_Request * sreq = NULL;
#endif
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_RSEND_INIT);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_RSEND_INIT);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);
    
#if defined(XXX)
    mpig_request_create_psreq(sreq, mpi_errno, goto fn_fail);
    mpig_request_set_type(sreq, MPIG_REQUEST_TYPE_RSEND);
    if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
    {
	MPID_Datatype_get_ptr(datatype, sreq->dev.datatype_ptr);
	MPID_Datatype_add_ref(sreq->dev.datatype_ptr);
    }

    *request = sreq;
#endif
    
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
int MPID_Ssend_init(const void * buf, int count, MPI_Datatype datatype, int rank, int tag, MPID_Comm * comm, int context_offset,
		    MPID_Request ** request)
{
#if defined(XXX)
    MPID_Request * sreq = NULL;
#endif
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_SSEND_INIT);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_SSEND_INIT);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);
    
#if defined(XXX)
    mpig_request_create_psreq(sreq, mpi_errno, goto fn_fail);
    mpig_request_set_type(sreq, MPIG_REQUEST_TYPE_SSEND);
    if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
    {
	MPID_Datatype_get_ptr(datatype, sreq->dev.datatype_ptr);
	MPID_Datatype_add_ref(sreq->dev.datatype_ptr);
    }
    *request = sreq;
#endif
    
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
int MPID_Bsend_init(const void * buf, int count, MPI_Datatype datatype, int rank, int tag, MPID_Comm * comm, int context_offset,
		    MPID_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_BSEND_INIT);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_BSEND_INIT);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);
    
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
