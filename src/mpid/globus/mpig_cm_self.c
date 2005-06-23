/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"

#if !defined(MPIG_COPY_BUFFER_SZ)
#define MPIG_COPY_BUFFER_SZ (256*1024)
#endif

int mpig_cm_self_vc_create(mpig_vc_t * vc);

/*
 * ADI3 function prototypes
 */
MPIG_STATIC int mpig_cm_self_adi3_send(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
				       int ctxoff, MPID_Request ** sreqp);

MPIG_STATIC int mpig_cm_self_adi3_isend(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
				       int ctxoff, MPID_Request ** sreqp);

MPIG_STATIC int mpig_cm_self_adi3_rsend(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
					int ctxoff, MPID_Request ** sreqp);

MPIG_STATIC int mpig_cm_self_adi3_irsend(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
					 int ctxoff, MPID_Request ** sreqp);

MPIG_STATIC int mpig_cm_self_adi3_ssend(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
					int ctxoff, MPID_Request ** sreqp);

MPIG_STATIC int mpig_cm_self_adi3_issend(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
				       int ctxoff, MPID_Request ** sreqp);

MPIG_STATIC int mpig_cm_self_adi3_recv(void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
				       int ctxoff, MPI_Status * status, MPID_Request ** rreqp);

MPIG_STATIC int mpig_cm_self_adi3_irecv(void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
					int ctxoff, MPID_Request ** rreqp);


/*
 * ADI3 function table
 */
MPIG_STATIC mpig_cm_funcs_t mpig_cm_self_funcs =
{
    mpig_cm_self_adi3_send,
    mpig_cm_self_adi3_isend,
    mpig_cm_self_adi3_rsend,
    mpig_cm_self_adi3_irsend,
    mpig_cm_self_adi3_ssend,
    mpig_cm_self_adi3_issend,
    mpig_cm_self_adi3_recv,
    mpig_cm_self_adi3_irecv
};


/*
 * Prototypes for internal routines
 */

MPIG_STATIC int mpig_cm_self_send(int type, const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, int ctx,
				  MPID_Comm * comm, MPID_Request ** sreqp);

MPIG_STATIC void mpig_cm_self_buffer_copy(
    const void * const sbuf, int scnt, MPI_Datatype sdt, int * smpi_errno,
    void * const rbuf, int rcnt, MPI_Datatype rdt, MPI_Aint * rsz, int * rmpi_errno);


/*
 * mpig_cm_self_init([IN/OUT] argc, [IN/OUT] argv)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_init
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_cm_self_init(int * const argc, char *** const argv)
{
    int mpi_errno = MPI_SUCCESS;

    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_ERR_CHKANDJUMP((strlen(mpig_process.hostname) == 0), mpi_errno, MPI_ERR_OTHER, "**globus|cm_self_hostname");

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_self_init() */


/*
 * mpig_cm_self_finalize()
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_finalize
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_cm_self_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    /* ...nothing to do... */

    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    return mpi_errno;
}
/* mpig_cm_self_finalize() */


/*
 * mpig_cm_self_add_contact_info([IN/OUT] business card)
 *
 * Add any and all contact information for this communication module to the supplied business card.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_add_contact_info
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_cm_self_add_contact_info(mpig_bc_t * const bc)
{
    char pid[64];
    int mpi_errno = MPI_SUCCESS;
    
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    
    mpi_errno = mpig_bc_add_contact(bc, "CM_SELF_HOSTNAME", mpig_process.hostname);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    MPIU_Snprintf(pid, 64, "%ld", (long) mpig_process.pid);
    mpi_errno = mpig_bc_add_contact(bc, "CM_SELF_PID", pid);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_self_add_contact_info() */


/*
 * int mpig_cm_self_select_module([IN] bc, [IN/OUT] vc, [OUT] flag)
 *
 * Check the business card to see if the connection module can communicate with the remote process associated with the supplied
 * VC.  If it can, then the VC will be initialized accordingly.
 *
 * Parameters:
 *
 * bc [IN] - business card containing contact information
 * vc [IN] - vc object to initialize if the communication module is capable of performing communication with the associated process
 * flag [OUT] - TRUE if the communication module can communicate with the remote process; otherwise FALSE
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_select_module
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_cm_self_select_module(mpig_bc_t * const bc, mpig_vc_t * const vc, int * const flag)
{
    char * hostname_str = NULL;
    char * pid_str = NULL;
    long pid;
    int bc_flag;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    
    mpi_errno = mpig_bc_get_contact(bc, "CM_SELF_HOSTNAME", &hostname_str, &bc_flag);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
    if (bc_flag == FALSE)
    {
	*flag = FALSE;
	goto fn_return;
    }
    
    if (strcmp(mpig_process.hostname, hostname_str) != 0)
    {
	*flag = FALSE;
	goto fn_return;
    }
    
    mpi_errno = mpig_bc_get_contact(bc, "CM_SELF_PID", &pid_str, &bc_flag);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
    if (bc_flag == FALSE)
    {
	*flag = FALSE;
	goto fn_return;
    }

    rc = sscanf(pid_str, "%ld", &pid);
    MPIU_ERR_CHKANDJUMP((rc != 1), mpi_errno, MPI_ERR_INTERN, "**keyval");
    
    if (mpig_process.pid != (pid_t) pid)
    {
	*flag = FALSE;
	goto fn_return;
    }
	
    mpig_vc_set_state(vc, MPIG_VC_STATE_CONNECTED);
    mpig_vc_set_cm_type(vc, MPIG_CM_TYPE_SELF);
    mpig_vc_set_cm_funcs(vc, mpig_cm_self_funcs);

    *flag = TRUE;
    
  fn_return:
    if (hostname_str != NULL)
    {
	mpig_bc_free_contact(hostname_str);
    }
    if (pid_str != NULL)
    {
	mpig_bc_free_contact(pid_str);
    }
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    *flag = FALSE;
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* int mpig_cm_self_select_module([IN] business card, [IN/OUT] virtual connection, [OUT] flag) */


/*
 * int mpig_cm_self_adi3_send(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_adi3_send
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_STATIC int mpig_cm_self_adi3_send(const void * const buf, const int cnt, const MPI_Datatype dt, const int rank,
				       const int tag, MPID_Comm * const comm, const int ctxoff, MPID_Request ** const sreqp)
{
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_self_adi3_send);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_self_adi3_send);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    mpi_errno = mpig_cm_self_send(MPIG_REQUEST_TYPE_SEND, buf, cnt, dt, rank, tag, ctx, comm, sreqp);
#   if (MPICH_THREAD_LEVEL < MPI_THREAD_MULTIPLE)
    {
	MPIU_ERR_CHKANDJUMP(((*sreqp)->cc != 0), mpi_errno, MPI_ERR_OTHER, "**dev|selfsenddeadlock");
    }
#   endif

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_self_adi3_send);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_self_adi3_send(...) */


/*
 * int mpig_cm_self_adi3_isend(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_adi3_isend
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_STATIC int mpig_cm_self_adi3_isend(const void * const buf, const int cnt, const MPI_Datatype dt, const int rank,
					const int tag, MPID_Comm * const comm, const int ctxoff, MPID_Request ** const sreqp)
{
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_self_adi3_isend);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_self_adi3_isend);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    mpi_errno = mpig_cm_self_send(MPIG_REQUEST_TYPE_SEND, buf, cnt, dt, rank, tag, ctx, comm, sreqp);

    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_self_adi3_isend);
    return mpi_errno;
}
/* mpig_cm_self_adi3_isend(...) */


/*
 * int mpig_cm_self_adi3_rsend(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_adi3_rsend
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_STATIC int mpig_cm_self_adi3_rsend(const void * const buf, const int cnt, const MPI_Datatype dt, const int rank,
					const int tag, MPID_Comm * const comm, const int ctxoff, MPID_Request ** const sreqp)
{
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_self_adi3_rsend);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_self_adi3_rsend);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    mpi_errno = mpig_cm_self_send(MPIG_REQUEST_TYPE_RSEND, buf, cnt, dt, rank, tag, ctx, comm, sreqp);
#   if (MPICH_THREAD_LEVEL < MPI_THREAD_MULTIPLE)
    {
	MPIU_ERR_CHKANDJUMP(((*sreqp)->cc != 0), mpi_errno, MPI_ERR_OTHER, "**dev|selfsenddeadlock");
    }
#   endif

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_self_adi3_rsend);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_self_adi3_rsend(...) */


/*
 * int mpig_cm_self_adi3_irsend(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_adi3_irsend
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_STATIC int mpig_cm_self_adi3_irsend(const void * const buf, const int cnt, const MPI_Datatype dt, const int rank,
					 const int tag, MPID_Comm * const comm, const int ctxoff, MPID_Request ** const sreqp)
{
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_self_adi3_irsend);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_self_adi3_irsend);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    mpi_errno = mpig_cm_self_send(MPIG_REQUEST_TYPE_RSEND, buf, cnt, dt, rank, tag, ctx, comm, sreqp);

    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_self_adi3_irsend);
    return mpi_errno;
}
/* mpig_cm_self_adi3_irsend(...) */


/*
 * int mpig_cm_self_adi3_ssend(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_adi3_ssend
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_STATIC int mpig_cm_self_adi3_ssend(const void * const buf, const int cnt, const MPI_Datatype dt, const int rank,
					const int tag, MPID_Comm * const comm, const int ctxoff, MPID_Request ** const sreqp)
{
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_self_adi3_ssend);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_self_adi3_ssend);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    mpi_errno = mpig_cm_self_send(MPIG_REQUEST_TYPE_SSEND, buf, cnt, dt, rank, tag, ctx, comm, sreqp);
#   if (MPICH_THREAD_LEVEL < MPI_THREAD_MULTIPLE)
    {
	MPIU_ERR_CHKANDJUMP(((*sreqp)->cc != 0), mpi_errno, MPI_ERR_OTHER, "**dev|selfsenddeadlock");
    }
#   endif

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_self_adi3_ssend);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_self_adi3_ssend(...) */


/*
 * int mpig_cm_self_adi3_issend(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_adi3_issend
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_STATIC int mpig_cm_self_adi3_issend(const void * const buf, const int cnt, const MPI_Datatype dt, const int rank,
					const int tag, MPID_Comm * const comm, const int ctxoff, MPID_Request ** const sreqp)
{
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_self_adi3_issend);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_self_adi3_issend);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    mpi_errno = mpig_cm_self_send(MPIG_REQUEST_TYPE_SSEND, buf, cnt, dt, rank, tag, ctx, comm, sreqp);

    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_self_adi3_issend);
    return mpi_errno;
}
/* mpig_cm_self_adi3_issend(...) */


/*
 * int mpig_cm_self_adi3_recv(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_adi3_recv
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_STATIC int mpig_cm_self_adi3_recv(void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
				       int ctxoff, MPI_Status * status, MPID_Request ** rreqp)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_self_adi3_recv);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_self_adi3_recv);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    mpi_errno = mpig_cm_self_adi3_irecv(buf, cnt, dt, rank, tag, comm, ctxoff, rreqp);
    /* the status will be extracted by MPI_Recv() once the request is complete */
    
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_self_adi3_recv);
    return mpi_errno;
}

/*
 * int mpig_cm_self_adi3_irecv(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_adi3_irecv
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_STATIC int mpig_cm_self_adi3_irecv(void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
					int ctxoff, MPID_Request ** rreqp)
{
    MPID_Request * sreq = NULL;
    MPID_Request * rreq;
    const int ctx = comm->context_id + ctxoff;
    int found;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_self_adi3_irecv);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_self_adi3_irecv);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    rreq = mpig_recvq_deq_unexp_or_enq_posted(rank, tag, ctx, &found);
    MPIU_ERR_CHKANDJUMP1((rreq == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "receive request");
    
    if (found)
    {
	void * sreq_buf;
	int sreq_cnt;
	MPI_Datatype sreq_dt;
	MPI_Aint data_sz;

	sreq = rreq->partner_request;
	MPIU_Assertp(sreq != NULL);
	/* XXX: MT: needed for release consistent systems??? */
	mpig_request_lock(sreq);

	MPIG_DBG_PRINTF((15, FCNAME, "request found in unexpected queue...copying data, req=0x%08x, ptr=%p", rreq->handle, rreq));

	mpig_request_get_buffer(sreq, &sreq_buf, &sreq_cnt, &sreq_dt);
	mpig_cm_self_buffer_copy(sreq_buf, sreq_cnt, sreq_dt, &sreq->status.MPI_ERROR,
				 buf, cnt, dt, &data_sz, &rreq->status.MPI_ERROR);

	mpig_request_set_buffer(rreq, buf, cnt, dt);
	rreq->status.count = (int) data_sz;
	/* no one else has a handle to the receive req, so it is safe to just set the ref count and CC before returning rreq */
	mpig_request_set_ref(rreq, 1);
	mpig_request_set_cc(rreq, 0);
	
	mpig_request_complete(&sreq);
    }
    else
    {
	/* Message has yet to arrived.  The request has been placed on the posted receive queue. */
	MPIG_DBG_PRINTF((15, FCNAME, "request allocated in posted queue, req=0x%08x, ptr=%p", rreq->handle, rreq));
	mpig_request_init_irreq(rreq, 2, 1, buf, cnt, dt, rank, tag, ctx, comm);
    }

    *rreqp = rreq;
    
  fn_return:
    if (rreq != NULL)
    {
	mpig_request_unlock(rreq);
    }
    if (sreq != NULL)
    {
	/* XXX: MT: needed for release consistent systems??? */
	mpig_request_unlock(sreq);
    }

    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_self_adi3_irecv);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_self_adi3_irecv(...) */


/*
 * mpig_cm_self_send()
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_send
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_STATIC int mpig_cm_self_send(const int type, const void * const buf, const int cnt, MPI_Datatype dt, const int rank,
				  const int tag, const int ctx, MPID_Comm * const comm, MPID_Request ** const sreqp)
{
    MPID_Request * sreq = NULL;
    MPID_Request * rreq = NULL;
    int found;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_self_send);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_self_send);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
	
    mpig_request_create_isreq(type, 2, 1, (void *) buf, cnt, dt, rank, tag, ctx, comm, &sreq);
    
    rreq = mpig_recvq_deq_posted_or_enq_unexp(rank, tag, ctx, &found);
    MPIU_ERR_CHKANDJUMP1((rreq == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "receive request");
    
    if (found)
    {
	MPI_Aint data_sz;
	    
	MPIG_DBG_PRINTF((20, FCNAME, "found posted receive request; copying data"));
	mpig_cm_self_buffer_copy(buf, cnt, dt, &rreq->status.MPI_ERROR, rreq->dev.buf, rreq->dev.cnt, rreq->dev.dt,
				 &data_sz, &rreq->status.MPI_ERROR);

	mpig_request_set_envelope(rreq, rank, tag, ctx);
	rreq->status.MPI_SOURCE = rank;
	rreq->status.MPI_TAG = tag;
	rreq->status.count = (int) data_sz;
	mpig_request_complete(&rreq);

	mpig_request_set_ref(sreq, 1);
	mpig_request_set_cc(sreq, 0);
    }
    else
    {
	MPI_Aint dt_sz;
	
	if (type != MPIG_REQUEST_TYPE_RSEND)
	{
	    MPIG_DBG_PRINTF((20, FCNAME, "adding receive request to unexpected queue; attaching send request"));
	    mpig_request_init_irreq(rreq, 2, 1, NULL, 0, MPI_DATATYPE_NULL, rank, tag, ctx, comm);
	    rreq->status.MPI_SOURCE = rank;
	    rreq->status.MPI_TAG = tag;
	    rreq->partner_request = sreq;
	    mpig_request_set_sreq_id(rreq, sreq->handle);

	    /* This is needed for MPI_Probe() and MPI_Iprobe() */
	    MPID_Datatype_get_size_macro(dt, dt_sz);
	    rreq->status.count = cnt * dt_sz;
	}
	else
	{
	    int err = MPI_SUCCESS;
	    
	    MPIG_DBG_PRINTF((15, FCNAME, "ready send unable to find matching recv req"));
	    MPIU_ERR_SETANDSTMT2(err, MPI_ERR_OTHER, {;}, "**rsendnomatch", "**rsendnomatch %d %d", rank, tag);

	    mpig_request_init_irreq(rreq, 1, 0, NULL, 0, MPI_DATATYPE_NULL, rank, tag, ctx, comm);
	    rreq->status.MPI_SOURCE = rank;
	    rreq->status.MPI_TAG = tag;
	    rreq->status.MPI_ERROR = err;

	    mpig_request_set_ref(sreq, 1);
	    mpig_request_set_cc(sreq, 0);
	    sreq->status.MPI_ERROR = err;
	}
    }

    *sreqp = sreq;
    
  fn_return:
    if (rreq != NULL)
    { 
	mpig_request_unlock();
    }
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_self_send);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (sreq != NULL)
    {
	mpig_request_destroy(sreq);
    }
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_self_send() */


/*
 * mpig_cm_self_buffer_copy()
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_buffer_copy
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_STATIC void mpig_cm_self_buffer_copy(
    const void * const sbuf, const int scnt, const MPI_Datatype sdt, int * const smpi_errno,
    void * const rbuf, const int rcnt, const MPI_Datatype rdt, MPI_Aint * const rsz, int * const rmpi_errno)
{
    int sdt_contig;
    int rdt_contig;
    MPI_Aint sdt_true_lb, rdt_true_lb;
    MPI_Aint sdata_sz;
    MPI_Aint rdata_sz;
    MPID_Datatype * sdt_ptr;
    MPID_Datatype * rdt_ptr;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_self_buffer_copy);
    MPIG_STATE_DECL(MPID_STATE_memcpy);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_self_buffer_copy);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    *smpi_errno = MPI_SUCCESS;
    *rmpi_errno = MPI_SUCCESS;

    mpig_datatype_get_info(scnt, sdt, &sdt_contig, &sdata_sz, &sdt_ptr, &sdt_true_lb);
    mpig_datatype_get_info(rcnt, rdt, &rdt_contig, &rdata_sz, &rdt_ptr, &rdt_true_lb);

    if (sdata_sz > rdata_sz)
    {
	MPIG_DBG_PRINTF((15, FCNAME, "message truncated, sdata_sz=" MPIG_AINT_FMT " rdata_sz=" MPIG_AINT_FMT,
			  sdata_sz, rdata_sz));
	sdata_sz = rdata_sz;
	MPIU_ERR_SETANDSTMT2(*rmpi_errno, MPI_ERR_TRUNCATE, {;}, "**truncate", "**truncate %d %d", sdata_sz, rdata_sz );
    }
    
    if (sdata_sz == 0)
    {
	*rsz = 0;
	goto fn_exit;
    }
    
    if (sdt_contig && rdt_contig)
    {
	MPIG_FUNC_ENTER(MPID_STATE_memcpy);
	memcpy((char *)rbuf + rdt_true_lb, (const char *) sbuf + sdt_true_lb, (size_t) sdata_sz);
	MPIG_FUNC_EXIT(MPID_STATE_memcpy);
	*rsz = sdata_sz;
    }
    else if (sdt_contig)
    {
	MPID_Segment seg;
	MPI_Aint last;

	MPID_Segment_init(rbuf, rcnt, rdt, &seg, 0);
	last = sdata_sz;
	MPIG_DBG_PRINTF((40, FCNAME, "pre-unpack last=" MPIG_AINT_FMT, last ));
	MPID_Segment_unpack(&seg, 0, &last, (const char *) sbuf + sdt_true_lb);
	MPIU_ERR_CHKANDSTMT((last != sdata_sz), *rmpi_errno, MPI_ERR_TYPE, {;}, "**dtypemismatch");
	MPIG_DBG_PRINTF((40, FCNAME, "pre-unpack last=" MPIG_AINT_FMT, last ));

	*rsz = last;
    }
    else if (rdt_contig)
    {
	MPID_Segment seg;
	MPI_Aint last;

	MPID_Segment_init(sbuf, scnt, sdt, &seg, 0);
	last = sdata_sz;
	MPIG_DBG_PRINTF((40, FCNAME, "pre-pack last=" MPIG_AINT_FMT, last ));
	MPID_Segment_pack(&seg, 0, &last, (char *) rbuf + rdt_true_lb);
	MPIU_ERR_CHKANDSTMT((last != sdata_sz), *rmpi_errno, MPI_ERR_TYPE, {;}, "**dtypemismatch");
	MPIG_DBG_PRINTF((40, FCNAME, "post-pack last=" MPIG_AINT_FMT, last ));

	*rsz = last;
    }
    else
    {
	char * buf;
	MPI_Aint buf_off;
	MPID_Segment sseg;
	MPI_Aint sfirst;
	MPID_Segment rseg;
	MPI_Aint rfirst;

	buf = MPIU_Malloc(MPIG_COPY_BUFFER_SZ);
	if (buf == NULL)
	{
	    /* --BEGIN ERROR HANDLING-- */
	    MPIG_DBG_PRINTF((40, FCNAME, "copy (pack/unpack) buffer allocation failure"));
	    MPIU_ERR_SETANDSTMT1(*smpi_errno, MPI_ERR_OTHER, {;}, "**nomem", "**nomem %s", "copy (pack/unpack) buffer");
	    *rmpi_errno = *smpi_errno;
	    *rsz = 0;
	    goto fn_exit;
	    /* --END ERROR HANDLING-- */
	}

	MPID_Segment_init(sbuf, scnt, sdt, &sseg, 0);
	MPID_Segment_init(rbuf, rcnt, rdt, &rseg, 0);

	sfirst = 0;
	rfirst = 0;
	buf_off = 0;
	
	for(;;)
	{
	    MPI_Aint last;
	    char * buf_end;

	    if (sdata_sz - sfirst > MPIG_COPY_BUFFER_SZ - buf_off)
	    {
		last = sfirst + (MPIG_COPY_BUFFER_SZ - buf_off);
	    }
	    else
	    {
		last = sdata_sz;
	    }
	    
	    MPIG_DBG_PRINTF((40, FCNAME, "pre-pack first=" MPIG_AINT_FMT ", last=" MPIG_AINT_FMT, sfirst, last ));
	    MPID_Segment_pack(&sseg, sfirst, &last, buf + buf_off);
	    MPIG_DBG_PRINTF((40, FCNAME, "post-pack first=" MPIG_AINT_FMT ", last=" MPIG_AINT_FMT, sfirst, last ));
	    MPIU_Assert(last > sfirst);
	    
	    buf_end = buf + buf_off + (last - sfirst);
	    sfirst = last;
	    
	    MPIG_DBG_PRINTF((40, FCNAME, "pre-unpack first=" MPIG_AINT_FMT ", last=" MPIG_AINT_FMT, rfirst, last ));
	    MPID_Segment_unpack(&rseg, rfirst, &last, buf);
	    MPIG_DBG_PRINTF((40, FCNAME, "post-unpack first=" MPIG_AINT_FMT ", last=" MPIG_AINT_FMT, rfirst, last ));
	    MPIU_Assert(last > rfirst);

	    rfirst = last;

	    if (rfirst == sdata_sz)
	    {
		/* successful completion */
		break;
	    }

	    /* --BEGIN ERROR HANDLING-- */
	    if (sfirst == sdata_sz)
	    {
		/* datatype mismatch -- remaining bytes could not be unpacked */
		*rmpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TYPE,
						   "**dtypemismatch", NULL);
		break;
	    }
	    /* --END ERROR HANDLING-- */

	    buf_off = sfirst - rfirst;
	    if (buf_off > 0)
	    {
		MPIG_DBG_PRINTF((40, FCNAME, "moved " MPIG_AINT_FMT " bytes to the beginning of the tmp buffer", buf_off));
		memmove(buf, buf_end - buf_off, (size_t) buf_off);
	    }
	}

	*rsz = rfirst;
	MPIU_Free(buf);
    }

  fn_exit:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_self_buffer_copy);
}
/* mpig_cm_self_buffer_copy() */
