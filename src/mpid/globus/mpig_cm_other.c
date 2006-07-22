/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"

/**********************************************************************************************************************************
				      BEGIN MISCELLANEOUS MACROS, PROTOTYPES, AND VARIABLES
**********************************************************************************************************************************/
mpig_vc_t * mpig_cm_other_vc = NULL;
/**********************************************************************************************************************************
				       END MISCELLANEOUS MACROS, PROTOTYPES, AND VARIABLES
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					      BEGIN COMMUNICATION MODULE API VTABLE
**********************************************************************************************************************************/
static int mpig_cm_other_init(mpig_cm_t * cm, int * argc, char *** argv);

static int  mpig_cm_other_finalize(mpig_cm_t * cm);

static mpig_cm_vtable_t mpig_cm_other_vtable =
{
    mpig_cm_other_init,
    mpig_cm_other_finalize,
    NULL, /* mpig_cm_other_add_contact_info */
    NULL, /* mpig_cm_other_construct_vc_contact_info */
    NULL, /* mpig_cm_other_destruct_vc_contact_info */
    NULL, /* mpig_cm_other_select_comm_method */
    NULL, /* mpig_cm_other_get_vc_compatability */
    mpig_cm_vtable_last_entry
};

mpig_cm_t mpig_cm_other =
{
    MPIG_CM_TYPE_OTHER,
    "other",
    &mpig_cm_other_vtable
};
/**********************************************************************************************************************************
					       END COMMUNICATION MODULE API VTABLE
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						    BEGIN VC CORE API VTABLE
**********************************************************************************************************************************/
static int mpig_cm_other_adi3_send(
    const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** sreqp);

static int mpig_cm_other_adi3_isend(
    const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** sreqp);

static int mpig_cm_other_adi3_rsend(
    const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** sreqp);

static int mpig_cm_other_adi3_irsend(
    const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** sreqp);

static int mpig_cm_other_adi3_ssend(
    const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** sreqp);

static int mpig_cm_other_adi3_issend(
    const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** sreqp);

static int mpig_cm_other_adi3_recv(
    void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPI_Status * status,
    MPID_Request ** rreqp);

static int mpig_cm_other_adi3_irecv(
    void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,int ctxoff, MPID_Request ** rreqp);

static int mpig_cm_other_adi3_cancel_send(MPID_Request * sreq);


MPIG_STATIC mpig_vc_vtable_t mpig_cm_other_vc_vtable =
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
    mpig_vc_vtable_last_entry
};
/**********************************************************************************************************************************
						     END VC CORE API VTABLE
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					    BEGIN COMMUNICATION MODULE API FUNCTIONS
**********************************************************************************************************************************/
/*
 * Prototypes for internal routines
 */
static int mpig_cm_other_recv_any_source(void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
    int ctxoff, MPID_Request ** rreqp);

/*
 * <mpi_errno> mpig_cm_other_init([IN/OUT] argc, [IN/OUT] argv)
 *
 * see documentation in mpidpre.h.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_other_init
static int mpig_cm_other_init(mpig_cm_t * const cm, int * const argc, char *** const argv)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIU_CHKPMEM_DECL(1);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_init);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_init);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering"));

    MPIU_CHKPMEM_MALLOC(mpig_cm_other_vc, mpig_vc_t *, sizeof(mpig_vc_t), mpi_errno,
	"VC that handles MPI_ANY_SOURCE/MPI_PROC_NULL");
    mpig_vc_construct(mpig_cm_other_vc);
    mpig_vc_set_cm(mpig_cm_other_vc, &mpig_cm_other);
    mpig_vc_set_vtable(mpig_cm_other_vc, &mpig_cm_other_vc_vtable);

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_other_init);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	MPIU_CHKPMEM_REAP();
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_other_init() */


/*
 * <mpi_errno> mpig_cm_other_finalize(void)
 *
 * see documentation in mpidpre.h.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_other_finalize
static int mpig_cm_other_finalize(mpig_cm_t * const cm)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_finalize);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_finalize);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering"));

    mpig_vc_destruct(mpig_cm_other_vc);
    MPIU_Free(mpig_cm_other_vc);
    mpig_cm_other_vc = NULL;

    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_other_finalize);
    return mpi_errno;
}
/* mpig_cm_other_finalize() */
/**********************************************************************************************************************************
					     END COMMUNICATION MODULE API FUNCTIONS
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						   BEGIN VC CORE API FUNCTIONS
**********************************************************************************************************************************/
/*
 * int mpig_cm_other_adi3_send(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_other_adi3_send
static int mpig_cm_other_adi3_send(
    const void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const sreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_adi3_send);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_adi3_send);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "entering: buf=" MPIG_PTR_FMT
	", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT ", ctx=%d", (MPIG_PTR_CAST) buf, cnt, dt,
	rank, tag, (MPIG_PTR_CAST) comm, comm->context_id + ctxoff));

    MPIU_Assert(rank == MPI_PROC_NULL);
    *sreqp = NULL;
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "send to MPI_PROC_NULL ignored"));
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "exiting: sreq=" MPIG_HANDLE_FMT
	", sreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_HANDLE_VAL(*sreqp), (MPIG_PTR_CAST) *sreqp, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_other_adi3_send);
    return mpi_errno;
}
/* mpig_cm_other_adi3_send() */


/*
 * int mpig_cm_other_adi3_isend(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_other_adi3_isend
static int mpig_cm_other_adi3_isend(
    const void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const sreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_adi3_isend);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_adi3_isend);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "entering: buf=" MPIG_PTR_FMT
	", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT ", ctx=%d", (MPIG_PTR_CAST) buf, cnt, dt,
	rank, tag, (MPIG_PTR_CAST) comm, ctx));

    MPIU_Assert(rank == MPI_PROC_NULL);
    mpig_request_create_isreq(MPIG_REQUEST_TYPE_SEND, 1, 0, (void *) buf, cnt, dt, rank, tag, ctx, comm, mpig_cm_other_vc, sreqp);
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "send to MPI_PROC_NULL; request allocated; handle=" MPIG_HANDLE_FMT ", ptr="
	MPIG_PTR_FMT, (*sreqp)->handle, (MPIG_PTR_CAST) *sreqp));

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "exiting: sreq=" MPIG_HANDLE_FMT
	", sreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_HANDLE_VAL(*sreqp), (MPIG_PTR_CAST) *sreqp, mpi_errno));
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
static int mpig_cm_other_adi3_rsend(
    const void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const sreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_adi3_rsend);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_adi3_rsend);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "entering: buf=" MPIG_PTR_FMT
	", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT ", ctx=%d", (MPIG_PTR_CAST) buf, cnt, dt,
	rank, tag, (MPIG_PTR_CAST) comm, comm->context_id + ctxoff));

    MPIU_Assert(rank == MPI_PROC_NULL);
    *sreqp = NULL;
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "send to MPI_PROC_NULL ignored"));
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "exiting: sreq=" MPIG_HANDLE_FMT
	", sreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_HANDLE_VAL(*sreqp), (MPIG_PTR_CAST) *sreqp, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_other_adi3_rsend);
    return mpi_errno;
}
/* mpig_cm_other_adi3_rsend() */


/*
 * int mpig_cm_other_adi3_irsend(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_other_adi3_irsend
static int mpig_cm_other_adi3_irsend(
    const void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const sreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_adi3_irsend);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_adi3_irsend);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "entering: buf=" MPIG_PTR_FMT
	", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT ", ctx=%d", (MPIG_PTR_CAST) buf, cnt, dt,
	rank, tag, (MPIG_PTR_CAST) comm, ctx));

    MPIU_Assert(rank == MPI_PROC_NULL);
    mpig_request_create_isreq(MPIG_REQUEST_TYPE_RSEND, 1, 0, (void *) buf, cnt, dt, rank, tag, ctx, comm, mpig_cm_other_vc, sreqp);

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "send to MPI_PROC_NULL; request allocated; handle=" MPIG_HANDLE_FMT ", ptr="
	MPIG_PTR_FMT, (*sreqp)->handle, (MPIG_PTR_CAST) *sreqp));

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "exiting: sreq=" MPIG_HANDLE_FMT
	", sreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_HANDLE_VAL(*sreqp), (MPIG_PTR_CAST) *sreqp, mpi_errno));
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
static int mpig_cm_other_adi3_ssend(
    const void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const sreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_adi3_ssend);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_adi3_ssend);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "entering: buf=" MPIG_PTR_FMT
	", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT ", ctx=%d", (MPIG_PTR_CAST) buf, cnt, dt,
	rank, tag, (MPIG_PTR_CAST) comm, comm->context_id + ctxoff));

    MPIU_Assert(rank == MPI_PROC_NULL);
    *sreqp = NULL;
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "send to MPI_PROC_NULL ignored"));
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "exiting: sreq=" MPIG_HANDLE_FMT
	", sreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_HANDLE_VAL(*sreqp), (MPIG_PTR_CAST) *sreqp, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_other_adi3_ssend);
    return mpi_errno;
}
/* mpig_cm_other_adi3_ssend(...) */


/*
 * int mpig_cm_other_adi3_issend(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_other_adi3_issend
static int mpig_cm_other_adi3_issend(
    const void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const sreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_adi3_issend);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_adi3_issend);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "entering: buf=" MPIG_PTR_FMT
	", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT ", ctx=%d", (MPIG_PTR_CAST) buf, cnt, dt,
	rank, tag, (MPIG_PTR_CAST) comm, ctx));

    MPIU_Assert(rank == MPI_PROC_NULL);
    mpig_request_create_isreq(MPIG_REQUEST_TYPE_SSEND, 1, 0, (void *) buf, cnt, dt, rank, tag, ctx, comm, mpig_cm_other_vc, sreqp);

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "send to MPI_PROC_NULL; request allocated; handle=" MPIG_HANDLE_FMT ", ptr="
	MPIG_PTR_FMT, (*sreqp)->handle, (MPIG_PTR_CAST) *sreqp));

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "exiting: sreq=" MPIG_HANDLE_FMT
	", sreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_HANDLE_VAL(*sreqp), (MPIG_PTR_CAST) *sreqp, mpi_errno));
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
static int mpig_cm_other_adi3_recv(
    void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPI_Status * const status, MPID_Request ** const rreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_adi3_recv);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_adi3_recv);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "entering: buf=" MPIG_PTR_FMT
	", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT	", ctx=%d", (MPIG_PTR_CAST) buf, cnt, dt,
	rank, tag, (MPIG_PTR_CAST) comm, comm->context_id + ctxoff));

    if (rank == MPI_PROC_NULL)
    { 
	MPIR_Status_set_procnull(status);
	*rreqp = NULL;
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "send to MPI_PROC_NULL ignored"));
    }
    else if (rank == MPI_ANY_SOURCE)
    {
	mpi_errno = mpig_cm_other_recv_any_source(buf, cnt, dt, rank, tag, comm, ctxoff, rreqp);
	MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_other|recv_any_source");
	/* the status will be extracted by MPI_Recv() once the request is complete */
    }
    else
    {
	MPIU_Assert(rank == MPI_PROC_NULL || rank == MPI_ANY_SOURCE);
    }

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "exiting: rreq=" MPIG_HANDLE_FMT
	", rreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_HANDLE_VAL(*rreqp), (MPIG_PTR_CAST) *rreqp, mpi_errno));
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
static int mpig_cm_other_adi3_irecv(
    void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const rreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_adi3_irecv);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_adi3_irecv);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "entering: buf=" MPIG_PTR_FMT
	", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT ", ctx=%d", (MPIG_PTR_CAST) buf, cnt, dt,
	rank, tag, (MPIG_PTR_CAST) comm, ctx));
    
    if (rank == MPI_PROC_NULL)
    {
	mpig_request_create_irreq(1, 0, buf, cnt, dt, rank, tag, ctx, comm, mpig_cm_other_vc, rreqp);
	MPIR_Status_set_procnull(&(*rreqp)->status);
	
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "MPI_PROC_NULL receive request allocated, handle=" MPIG_HANDLE_FMT ", ptr="
	    MPIG_PTR_FMT, MPIG_HANDLE_VAL(*rreqp), (MPIG_PTR_CAST) *rreqp));
    }
    else if (rank == MPI_ANY_SOURCE)
    {
	mpi_errno = mpig_cm_other_recv_any_source(buf, cnt, dt, rank, tag, comm, ctxoff, rreqp);
	MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_other|recv_any_source");
    }
    else
    {
	MPIU_Assert(rank == MPI_PROC_NULL || rank == MPI_ANY_SOURCE);
    }
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "exiting: rreq=" MPIG_HANDLE_FMT
	", rreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_HANDLE_VAL(*rreqp), (MPIG_PTR_CAST) *rreqp, mpi_errno));
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
static int mpig_cm_other_adi3_cancel_send(MPID_Request * const sreq)
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
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "exiting: sreq=" MPIG_HANDLE_FMT
	", sreqp=" MPIG_PTR_FMT "mpi_errno=" MPIG_ERRNO_FMT, sreq->handle, (MPIG_PTR_CAST) sreq, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_other_adi3_cancel_send);
    return mpi_errno;
}
/* mpig_cm_other_adi3_cancel_send() */


/*
 * <mpi_errno> mpig_cm_other_recv_any_source(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_other_recv_any_source
static int mpig_cm_other_recv_any_source(void * const buf, const int cnt, const MPI_Datatype dt, const int rank,
    const int tag, MPID_Comm * const comm, const int ctxoff, MPID_Request ** const rreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int ctx = comm->context_id + ctxoff;
    MPID_Request * rreq;
    bool_t found;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_recv_any_source);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_recv_any_source);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "entering: buf=" MPIG_PTR_FMT ", cnt=%d, dt="
	MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_HANDLE_FMT ", comm=" MPIG_PTR_FMT ", ctx=%d, ", (MPIG_PTR_CAST) buf, cnt,
	dt, rank, tag, comm->handle, (MPIG_PTR_CAST) comm, ctx));
	
    rreq = mpig_recvq_deq_unexp_or_enq_posted(rank, tag, ctx, &found);
    MPIU_ERR_CHKANDJUMP1((rreq == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "receive request");

    if (found)
    {
	mpig_vc_t * vc;
	
	/* message was found in the unexepected queue */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "request found in unexpected queue: rreq= " MPIG_HANDLE_FMT ", rreqp="
	    MPIG_PTR_FMT, rreq->handle, (MPIG_PTR_CAST) rreq));

	/* finish filling in the request fields */
	mpig_request_set_buffer(rreq, buf, cnt, dt);
        mpig_request_add_comm_ref(rreq, comm); /* used by MPI layer and ADI3 selection mechanism (in mpidpost.h), most notably
						  by the receive cancel routine */
	
	/* get the VC associated with this request */
	vc = mpig_request_get_vc(rreq);

	/* call VC function to process a message that has already been received */
	mpi_errno = vc->vtable->recv_any_source(vc, rreq);
	MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_other|vc_recv_any_source");
    }
    else
    {
	/* message has yet to arrived.  the request has been placed on the list of posted receive requests and populated with
	   information supplied in the arguments. */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "request allocated in posted queue: req=" MPIG_HANDLE_FMT ", reqp="
	    MPIG_PTR_FMT, rreq->handle, (MPIG_PTR_CAST) rreq));
	
	mpig_request_construct_irreq(rreq, 2, 1, buf, cnt, dt, rank, tag, ctx, comm, mpig_cm_other_vc);
    }

    *rreqp = rreq;
    
  fn_return:
    if (rreq != NULL)
    {
	/* the receive request is locked by the recvq routine to insure atomicity.  it must be unlocked before returning. */
	mpig_request_mutex_unlock(rreq);
    }

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "exiting: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT
	", mpi_errno=" MPIG_ERRNO_FMT, MPIG_HANDLE_VAL(*rreqp), (MPIG_PTR_CAST) *rreqp, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_other_recv_any_source);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_other_recv_any_source() */
/**********************************************************************************************************************************
						    END VC CORE API FUNCTIONS
**********************************************************************************************************************************/
