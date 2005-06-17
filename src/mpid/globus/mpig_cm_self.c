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

MPIG_STATIC int mpig_cm_self_adi3_recv(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
				       int ctxoff, MPI_Status * status, MPID_Request ** rreqp);


/*
 * ADI3 function table
 */
MPIG_STATIC mpig_cm_funcs_t mpig_cm_self_funcs =
{
    mpig_cm_self_adi3_send,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    mpig_cm_self_adi3_recv,
    NULL
};


/*
 * Prototypes for internal routines
 */

MPIG_STATIC int mpig_cm_self_send(const void * buf, int cnt, MPI_Datatype datatype, int rank, int tag, int ctx, int type,
				  MPID_Request ** request);

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
int mpig_cm_self_init(int * argc, char *** argv)
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
int mpig_cm_self_add_contact_info(mpig_bc_t * bc)
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
int mpig_cm_self_select_module(mpig_bc_t * bc, mpig_vc_t * vc, int * flag)
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
MPIG_STATIC int mpig_cm_self_adi3_send(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
				       int ctxoff, MPID_Request ** sreqp)
{
    MPID_Request * sreq;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_self_adi3_send);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_self_adi3_send);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_self_send);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_self_adi3_send(...) */


/*
 * int mpig_cm_self_adi3_recv(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_adi3_recv
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_STATIC int mpig_cm_self_adi3_recv(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
				       int ctxoff, MPI_Status * status, MPID_Request ** rreqp)
{
    int found;
    MPID_Request * rreq;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_self_adi3_recv);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_self_adi3_recv);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    
    rreq = mpig_recvq_deq_unexp_or_enq_posted(rank, tag, comm->context_id + ctxoff, &found);
    MPIU_ERR_CHKANDJUMP1((rreq == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "receive request");
    
    /* set MPICH fields */
    rreq->cc_ptr = &rreq->cc;
    rreq->kind = MPID_REQUEST_RECV;
    rreq->comm = NULL;
    MPIR_Status_set_procnull(&rreq->status);
	
    rreq->comm = comm;
    MPIR_Comm_add_ref(comm);
    rreq->dev.buf = buf;
    rreq->dev.cnt = cnt;
    rreq->dev.dt = dt;

    /* set device fields */
    mpig_request_state_init(rreq);
	
    if (found)
    {
	MPID_Request * const sreq = rreq->partner_request;

	if (sreq != NULL)
	{
	    MPI_Aint data_sz;
	    
	    mpig_cm_self_buffer_copy(sreq->dev.buf, sreq->dev.cnt, sreq->dev.dt, &sreq->status.MPI_ERROR,
				     buf, cnt, dt, &data_sz, &rreq->status.MPI_ERROR);
	    rreq->status.count = (int)data_sz;
	    MPID_Request_set_completed(sreq);
	    MPID_Request_release(sreq);
	}
	else
	{
	    /* XXX: The sreq is missing which means an error occurred.  rreq->status.MPI_ERROR should have been set when the
	       error was detected. */
	}

	if (status != MPI_STATUS_IGNORE)
	{
	    *status = rreq->status;
	}

	/* no other thread can possibly be waiting on rreq, so it is safe to reset ref_count and cc */
	MPIU_Object_set_ref(rreq, 1);
	rreq->cc = 0;
	MPIG_DBG_PRINTF((15, FCNAME, "request found in unexpected queue, req=0x%08x, ptr=%p", rreq->handle, rreq));
    }
    else
    {
	/* Message has yet to arrived.  The request has been placed on the list of posted receive requests and populated with
           information supplied in the arguments. */
	MPIG_DBG_PRINTF((15, FCNAME, "request allocated in posted queue, req=0x%08x, ptr=%p", rreq->handle, rreq));
	
	MPIU_Object_set_ref(rreq, 2);
	rreq->cc = 1;
	
	if (HANDLE_GET_KIND(dt) != HANDLE_KIND_BUILTIN)
	{
	    MPID_Datatype_get_ptr(dt, rreq->dev.dtp);
	    MPID_Datatype_add_ref(rreq->dev.dtp);
	}
    }
    
  fn_return:
    if (rreq != NULL)
    {
	mpig_request_unlock(rreq);
    }

    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_self_adi3_recv);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_self_adi3_recv(...) */


/*
 * mpig_cm_self_send()
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_send
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_STATIC int mpig_cm_self_send(const void * buf, int cnt, MPI_Datatype datatype, int rank, int tag, int ctx, int type,
				  MPID_Request ** request)
{
#if XXX
    mpig_envelope_t envl;
    MPID_Request * sreq = NULL;
    MPID_Request * rreq = NULL;
    mpig_vc_t * vc;
    int found;
#endif
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_self_send);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_self_send);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
	
    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);
    
#if XXX
    envl.rank = rank;
    envl.tag = tag;
    envl.ctx = ctx;
    rreq = mpig_recvq_deq_posted_or_enq_unexpected(&envl, &found);
    MPIU_ERR_CHKANDJUMP1((rreq == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "receive request");

    rreq->status.MPI_SOURCE = rank;
    rreq->status.MPI_TAG = tag;
    
    if (found)
    {
	MPI_Aint data_sz;
	
	MPIG_DBG_PRINTF((20, FCNAME, "found posted receive request; copying data"));
	    
	mpig_cm_self_buffer_copy(buf, cnt, dt, &sreq->status.MPI_ERROR, rreq->dev.buf, rreq->dev.cnt, rreq->dev.dt,
				 &data_sz, &rreq->status.MPI_ERROR);
	rreq->status.count = (int)data_sz;
	MPID_Request_set_completed(rreq);
	MPID_Request_release(rreq);
    }
    else
    {
	if (type != MPIG_REQUEST_TYPE_RSEND)
	{
	    int dt_sz;
	
	    /* FIXME: Insert code here to buffer small sends in a temporary buffer? */

	    MPIG_DBG_PRINTF((20, FCNAME, "adding receive request to unexpected queue; attaching send request"));
	    mpig_request_create_sreq(sreq, mpi_errno, goto fn_exit);
	    mpig_request_set_type(sreq, type);
	    mpig_request_set_proto(sreq, MPIG_REQUEST_PROTO_SELF);
    
	    if (HANDLE_GET_KIND(dt) != HANDLE_KIND_BUILTIN)
	    {
		MPID_Datatype_get_ptr(dt, sreq->dev.dtp);
		MPID_Datatype_add_ref(sreq->dev.dtp);
	    }
	    rreq->partner_request = sreq;
	    rreq->dev.sreq_id = sreq->handle;
	    MPID_Datatype_get_size_macro(dt, dt_sz);
	    rreq->status.count = cnt * dt_sz;
	}
	else
	{
	    /* --BEGIN ERROR HANDLING-- */
	    MPIG_DBG_PRINTF((15, FCNAME, "ready send unable to find matching recv req"));
	    MPI_ERR_SETANDSTMT2(mpi_errno, MPI_ERR_OTHER, {;}, "**rsendnomatch", "**rsendnomatch %d %d", rank, tag);
	    rreq->status.MPI_ERROR = mpi_errno;
	    
	    rreq->partner_request = NULL;
	    rreq->dev.sreq_id = MPI_REQUEST_NULL;
	    rreq->status.count = 0;
	    goto fn_fail;
	    /* --END ERROR HANDLING-- */
	}
	    
	mpig_request_set_msg_type(rreq, MPIG_REQUEST_SELF_MSG);
	MPID_Request_initialized_set(rreq);

    }
    
  fn_exit:
	*request = sreq;
#endif

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_self_send);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
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
    const void * const sbuf, int scnt, MPI_Datatype sdt, int * smpi_errno,
    void * const rbuf, int rcnt, MPI_Datatype rdt, MPI_Aint * rsz, int * rmpi_errno)
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
