/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"


mpig_vc_t * mpig_cm_other_vc = NULL;


/*
 * prototypes for VC function table functions
 */
MPIG_STATIC int mpig_cm_other_adi3_send(
    const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** sreqp);

MPIG_STATIC int mpig_cm_other_adi3_isend(
    const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** sreqp);

MPIG_STATIC int mpig_cm_other_adi3_rsend(
    const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** sreqp);

MPIG_STATIC int mpig_cm_other_adi3_irsend(
    const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** sreqp);

MPIG_STATIC int mpig_cm_other_adi3_ssend(
    const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** sreqp);

MPIG_STATIC int mpig_cm_other_adi3_issend(
    const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** sreqp);

MPIG_STATIC int mpig_cm_other_adi3_recv(
    void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPI_Status * status,
    MPID_Request ** rreqp);

MPIG_STATIC int mpig_cm_other_adi3_irecv(
    void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,int ctxoff, MPID_Request ** rreqp);

MPIG_STATIC int mpig_cm_other_adi3_cancel_send(MPID_Request * sreq);


/*
 * VC communication module function table
 */
MPIG_STATIC mpig_vc_cm_funcs_t mpig_cm_other_vc_funcs =
{
    mpig_cm_other_adi3_send,
    mpig_cm_other_adi3_isend,
    mpig_cm_other_adi3_rsend,
    mpig_cm_other_adi3_irsend,
    mpig_cm_other_adi3_ssend,
    mpig_cm_other_adi3_issend,
    mpig_cm_other_adi3_recv,
    mpig_cm_other_adi3_irecv,
    mpig_adi3_cancel_recv,
    mpig_cm_other_adi3_cancel_send,
    NULL, /* vc_recv_any_source */
    NULL, /* vc_inc_ref_count */
    NULL, /* vc_dec_ref_count */
    NULL, /* vc_destruct */
    mpig_vc_null_func
};


/*
 * Prototypes for internal routines
 */
MPIG_STATIC void mpig_cm_other_recv_any_source(
    void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** rreqp,
    int * mpi_errno_p, bool_t * failed_p);

/*
 * mpig_cm_other_init([IN/OUT] argc, [IN/OUT] argv)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_other_init
int mpig_cm_other_init(int * argc, char *** argv)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIU_CHKPMEM_DECL(1);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_init);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_init);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC,
		       "entering"));

    MPIU_CHKPMEM_MALLOC(mpig_cm_other_vc, mpig_vc_t *, sizeof(mpig_vc_t), mpi_errno,
			"VC for processing MPI_ANY_SOURCE/MPI_PROC_NULL");
    mpig_vc_construct(mpig_cm_other_vc);
    mpig_vc_set_cm_type(mpig_cm_other_vc, MPIG_CM_TYPE_OTHER);
    mpig_vc_set_cm_funcs(mpig_cm_other_vc, &mpig_cm_other_vc_funcs);

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC,
		       "exiting: mpi_errno=0x%08x", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_other_init);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	MPIU_CHKPMEM_REAP();
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_other_init() */


/*
 * mpig_cm_other_finalize()
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_other_finalize
int mpig_cm_other_finalize(void)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_finalize);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_finalize);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC,
		       "entering"));

    mpig_vc_destruct(mpig_cm_other_vc);
    MPIU_Free(mpig_cm_other_vc);
    mpig_cm_other_vc = NULL;
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC,
		       "exiting: mpi_errno=0x%08x", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_other_finalize);
    return mpi_errno;
}
/* mpig_cm_other_finalize() */


/*
 * mpig_cm_other_add_contact_info([IN/MOD] business card)
 *
 * Add any and all contact information for this communication module to the supplied business card.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_other_add_contact_info
int mpig_cm_other_add_contact_info(mpig_bc_t * bc)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_add_contact_info);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_add_contact_info);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC,
		       "entering"));
    
    /* ...nothing to do... */

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC,
		       "exiting: mpi_errno=0x%08x", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_other_add_contact_info);
    return mpi_errno;
}
/* mpig_cm_other_add_contact_info() */


/*
 * int mpig_cm_other_select_module([IN] bc, [IN/MOD] vc, [OUT] selected)
 *
 * Check the business card to see if the connection module can communicate with the remote process associated with the supplied
 * VC.  If it can, then the VC will be initialized accordingly.
 *
 * Parameters:
 *
 * bc [IN] - business card containing contact information
 * vc [IN/MOD] - vc object to initialize if the communication module is capable of communicating with the associated process
 * selected [OUT] - TRUE if the communication module can communicate with the remote process; otherwise FALSE
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_other_select_module
int mpig_cm_other_select_module(mpig_bc_t * bc, mpig_vc_t * vc, bool_t * selected)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_select_module);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_select_module);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "entering"));
    
    *selected = FALSE;
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "exiting: mpi_errno=0x%08x", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_other_select_module);
    return mpi_errno;
}
/* mpig_cm_other_select_module() */


/*
 * int mpig_cm_other_adi3_send(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_other_adi3_send
MPIG_STATIC int mpig_cm_other_adi3_send(
    const void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const sreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_adi3_send);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_adi3_send);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "entering: buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT
		       ", ctx=%d", (MPIG_PTR_CAST) buf, cnt, dt, rank, tag, (MPIG_PTR_CAST) comm, comm->context_id + ctxoff));

    MPIU_Assert(rank == MPI_PROC_NULL);
    *sreqp = NULL;
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "send to MPI_PROC_NULL ignored"));
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "exiting: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT ", mpi_errno=0x%08x",
		       MPIG_HANDLE_VAL(*sreqp), (MPIG_PTR_CAST) *sreqp, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_other_adi3_send);
    return mpi_errno;
}
/* mpig_cm_other_adi3_send() */


/*
 * int mpig_cm_other_adi3_isend(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_other_adi3_isend
MPIG_STATIC int mpig_cm_other_adi3_isend(
    const void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const sreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_adi3_isend);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_adi3_isend);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "entering: buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT
		       ", ctx=%d", (MPIG_PTR_CAST) buf, cnt, dt, rank, tag, (MPIG_PTR_CAST) comm, ctx));

    MPIU_Assert(rank == MPI_PROC_NULL);
    mpig_request_create_isreq(MPIG_REQUEST_TYPE_SEND, 1, 0, (void *) buf, cnt, dt, rank, tag, ctx, comm, mpig_cm_other_vc, sreqp);
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "send to MPI_PROC_NULL; request allocated; handle=0x%08x, ptr=" MPIG_PTR_FMT,
		       (*sreqp)->handle, (MPIG_PTR_CAST) *sreqp));

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "exiting: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT ", mpi_errno=0x%08x",
		       MPIG_HANDLE_VAL(*sreqp), (MPIG_PTR_CAST) *sreqp, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_other_adi3_isend);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_other_adi3_isend(...) */


/*
 * int mpig_cm_other_adi3_rsend(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_other_adi3_rsend
MPIG_STATIC int mpig_cm_other_adi3_rsend(
    const void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const sreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_adi3_rsend);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_adi3_rsend);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "entering: buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT
		       ", ctx=%d", (MPIG_PTR_CAST) buf, cnt, dt, rank, tag, (MPIG_PTR_CAST) comm, comm->context_id + ctxoff));

    MPIU_Assert(rank == MPI_PROC_NULL);
    *sreqp = NULL;
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "send to MPI_PROC_NULL ignored"));
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "exiting: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT ", mpi_errno=0x%08x",
		       MPIG_HANDLE_VAL(*sreqp), (MPIG_PTR_CAST) *sreqp, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_other_adi3_rsend);
    return mpi_errno;
}
/* mpig_cm_other_adi3_rsend() */


/*
 * int mpig_cm_other_adi3_irsend(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_other_adi3_irsend
MPIG_STATIC int mpig_cm_other_adi3_irsend(
    const void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const sreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_adi3_irsend);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_adi3_irsend);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "entering: buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT
		       ", ctx=%d", (MPIG_PTR_CAST) buf, cnt, dt, rank, tag, (MPIG_PTR_CAST) comm, comm->context_id + ctxoff));

    MPIU_Assert(rank == MPI_PROC_NULL);
    mpig_request_create_isreq(MPIG_REQUEST_TYPE_RSEND, 1, 0, (void *) buf, cnt, dt, rank, tag, ctx, comm, mpig_cm_other_vc, sreqp);

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "send to MPI_PROC_NULL; request allocated; handle=0x%08x, ptr=" MPIG_PTR_FMT,
		       (*sreqp)->handle, (MPIG_PTR_CAST) *sreqp));

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "exiting: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT ", mpi_errno=0x%08x",
		       MPIG_HANDLE_VAL(*sreqp), (MPIG_PTR_CAST) *sreqp, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_other_adi3_irsend);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_other_adi3_irsend() */


/*
 * int mpig_cm_other_adi3_ssend(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_other_adi3_ssend
MPIG_STATIC int mpig_cm_other_adi3_ssend(
    const void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const sreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_adi3_ssend);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_adi3_ssend);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "entering: buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT
		       ", ctx=%d", (MPIG_PTR_CAST) buf, cnt, dt, rank, tag, (MPIG_PTR_CAST) comm, comm->context_id + ctxoff));

    MPIU_Assert(rank == MPI_PROC_NULL);
    *sreqp = NULL;
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "send to MPI_PROC_NULL ignored"));
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "exiting: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT ", mpi_errno=0x%08x",
		       MPIG_HANDLE_VAL(*sreqp), (MPIG_PTR_CAST) *sreqp, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_other_adi3_ssend);
    return mpi_errno;
}
/* mpig_cm_other_adi3_ssend(...) */


/*
 * int mpig_cm_other_adi3_issend(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_other_adi3_issend
MPIG_STATIC int mpig_cm_other_adi3_issend(
    const void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const sreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_adi3_issend);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_adi3_issend);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "entering: buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT
		       ", ctx=%d", (MPIG_PTR_CAST) buf, cnt, dt, rank, tag, (MPIG_PTR_CAST) comm, ctx));

    MPIU_Assert(rank == MPI_PROC_NULL);
    mpig_request_create_isreq(MPIG_REQUEST_TYPE_SSEND, 1, 0, (void *) buf, cnt, dt, rank, tag, ctx, comm, mpig_cm_other_vc, sreqp);

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "send to MPI_PROC_NULL; request allocated; handle=0x%08x, ptr=" MPIG_PTR_FMT,
		       (*sreqp)->handle, (MPIG_PTR_CAST) *sreqp));

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "exiting: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT ", mpi_errno=0x%08x",
		       MPIG_HANDLE_VAL(*sreqp), (MPIG_PTR_CAST) *sreqp, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_other_adi3_issend);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_other_adi3_issend(...) */


/*
 * int mpig_cm_other_adi3_recv(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_other_adi3_recv
MPIG_STATIC int mpig_cm_other_adi3_recv(
    void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPI_Status * const status, MPID_Request ** const rreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    bool_t failed;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_adi3_recv);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_adi3_recv);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "entering: buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT
		       ", ctx=%d", (MPIG_PTR_CAST) buf, cnt, dt, rank, tag, (MPIG_PTR_CAST) comm, comm->context_id + ctxoff));

    if (rank == MPI_PROC_NULL)
    { 
	MPIR_Status_set_procnull(status);
	*rreqp = NULL;
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "send to MPI_PROC_NULL ignored"));
    }
    else if (rank == MPI_ANY_SOURCE)
    {
	mpig_cm_other_recv_any_source(buf, cnt, dt, rank, tag, comm, ctxoff, rreqp, &mpi_errno, &failed);
	MPIU_ERR_CHKANDJUMP((failed), mpi_errno, MPI_ERR_OTHER, "**globus|cm_other|recv_any_source");
	/* the status will be extracted by MPI_Recv() once the request is complete */
    }
    else
    {
	MPIU_Assert(rank == MPI_PROC_NULL || rank == MPI_ANY_SOURCE);
    }

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "exiting: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT ", mpi_errno=0x%08x",
		       MPIG_HANDLE_VAL(*rreqp), (MPIG_PTR_CAST) *rreqp, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_other_adi3_recv);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_other_adi3_recv(...) */


/*
 * int mpig_cm_other_adi3_irecv(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_other_adi3_irecv
MPIG_STATIC int mpig_cm_other_adi3_irecv(
    void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const rreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int ctx = comm->context_id + ctxoff;
    bool_t failed;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_adi3_irecv);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_adi3_irecv);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "entering: buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT
		       ", ctx=%d", (MPIG_PTR_CAST) buf, cnt, dt, rank, tag, (MPIG_PTR_CAST) comm, ctx));
    
    if (rank == MPI_PROC_NULL)
    {
	mpig_request_create_irreq(1, 0, buf, cnt, dt, rank, tag, ctx, comm, mpig_cm_other_vc, rreqp);
	MPIR_Status_set_procnull(&(*rreqp)->status);
	
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "MPI_PROC_NULL receive request allocated, handle=0x%08x, ptr=" MPIG_PTR_FMT,
			   MPIG_HANDLE_VAL(*rreqp), (MPIG_PTR_CAST) *rreqp));
    }
    else if (rank == MPI_ANY_SOURCE)
    {
	mpig_cm_other_recv_any_source(buf, cnt, dt, rank, tag, comm, ctxoff, rreqp, &mpi_errno, &failed);
	MPIU_ERR_CHKANDJUMP((failed), mpi_errno, MPI_ERR_OTHER, "**globus|cm_other|recv_any_source");
    }
    else
    {
	MPIU_Assert(rank == MPI_PROC_NULL || rank == MPI_ANY_SOURCE);
    }
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "exiting: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT ", mpi_errno=0x%08x",
		       MPIG_HANDLE_VAL(*rreqp), (MPIG_PTR_CAST) *rreqp, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_other_adi3_irecv);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_other_adi3_irecv() */


/*
 * int mpig_cm_other_adi3_cancel_send([IN/MOD] sreq)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_other_adi3_cancel_send
MPIG_STATIC int mpig_cm_other_adi3_cancel_send(MPID_Request * const sreq)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_adi3_cancel_send);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_adi3_cancel_send);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "entering: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT, sreq->handle, (MPIG_PTR_CAST) sreq));

    /* a MPI_PROC_NULL is considered to be immediately complete, so do nothing */
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "exiting: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT "mpi_errno=0x%08x",
		       sreq->handle, (MPIG_PTR_CAST) sreq, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_other_adi3_cancel_send);
    return mpi_errno;
}
/* mpig_cm_other_adi3_cancel_send() */


/*
 * void mpig_cm_other_recv_any_source(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_other_recv_any_source
MPIG_STATIC void mpig_cm_other_recv_any_source(
    void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const rreqp, int * const mpi_errno_p, bool_t * const failed_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int ctx = comm->context_id + ctxoff;
    MPID_Request * rreq;
    bool_t found;
    bool_t failed;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_recv_any_source);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_recv_any_source);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT,
		       "entering: buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT
		       ", ctx=%d, mpi_errno=0x%08x", (MPIG_PTR_CAST) buf, cnt, dt, rank, tag, (MPIG_PTR_CAST) comm, ctx,
		       *mpi_errno_p));
    *failed_p = FALSE;
	
    rreq = mpig_recvq_deq_unexp_or_enq_posted(rank, tag, ctx, &found);
    MPIU_ERR_CHKANDJUMP1((rreq == NULL), *mpi_errno_p, MPI_ERR_OTHER, "**nomem", "**nomem %s", "receive request");

    if (found)
    {
	mpig_vc_t * vc;
	
	/* message was found in the unexepected queue */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT,
			   "request found in unexpected queue: rreq= " MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT,
			   rreq->handle, (MPIG_PTR_CAST) rreq));

	/* finish filling in the request fields */
	mpig_request_set_buffer(rreq, buf, cnt, dt);
        mpig_request_add_comm_ref(rreq, comm); /* used by MPI layer and ADI3 selection mechanism (in mpidpost.h), most notably
						  by the receive cancel routine */
	
	/* get the VC associated with this request */
	vc = mpig_request_get_vc(rreq);

	/* call VC function to process a message that has already been received */
	vc->cm_funcs->recv_any_source(vc, rreq, comm, mpi_errno_p, &failed);
	MPIU_ERR_CHKANDJUMP((failed), *mpi_errno_p, MPI_ERR_OTHER, "**globus|cm_other|vc_recv_any_source");
    }
    else
    {
	/* message has yet to arrived.  the request has been placed on the list of posted receive requests and populated with
	   information supplied in the arguments. */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT,
			   "request allocated in posted queue: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT,
			   rreq->handle, (MPIG_PTR_CAST) rreq));
	
	mpig_request_construct_irreq(rreq, 2, 1, buf, cnt, dt, rank, tag, ctx, comm, mpig_cm_other_vc);
    }

    *rreqp = rreq;
    
  fn_return:
    if (rreq != NULL)
    {
	/* the receive request is locked by the recvq routine to insure atomicity.  it must be unlocked before returning. */
	mpig_request_mutex_unlock(rreq);
    }

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT,
		       "exiting: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT ", mpi_errno=0x%08x, failed=%s",
		       MPIG_HANDLE_VAL(*rreqp), (MPIG_PTR_CAST) *rreqp, *mpi_errno_p, MPIG_BOOL_STR(*failed_p)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_other_recv_any_source);
    return;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    *failed_p = TRUE;
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_other_recv_any_source() */
