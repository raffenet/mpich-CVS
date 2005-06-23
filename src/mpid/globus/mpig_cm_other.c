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
 * ADI3 function prototypes
 */
MPIG_STATIC int mpig_cm_other_adi3_send(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
					int ctxoff, MPID_Request ** sreqp);

MPIG_STATIC int mpig_cm_other_adi3_isend(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
					 int ctxoff, MPID_Request ** sreqp);

MPIG_STATIC int mpig_cm_other_adi3_rsend(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
					 int ctxoff, MPID_Request ** sreqp);

MPIG_STATIC int mpig_cm_other_adi3_irsend(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
					  int ctxoff, MPID_Request ** sreqp);

MPIG_STATIC int mpig_cm_other_adi3_ssend(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
					 int ctxoff, MPID_Request ** sreqp);

MPIG_STATIC int mpig_cm_other_adi3_issend(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
					  int ctxoff, MPID_Request ** sreqp);

MPIG_STATIC int mpig_cm_other_adi3_recv(void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
					int ctxoff, MPI_Status * status, MPID_Request ** rreqp);

MPIG_STATIC int mpig_cm_other_adi3_irecv(void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
					 int ctxoff, MPID_Request ** rreqp);


/*
 * ADI3 function table
 */
MPIG_STATIC mpig_cm_funcs_t mpig_cm_other_funcs =
{
    mpig_cm_other_adi3_send,
    mpig_cm_other_adi3_isend,
    mpig_cm_other_adi3_rsend,
    mpig_cm_other_adi3_irsend,
    mpig_cm_other_adi3_ssend,
    mpig_cm_other_adi3_issend,
    mpig_cm_other_adi3_recv,
    mpig_cm_other_adi3_irecv
};


/*
 * mpig_cm_other_init([IN/OUT] argc, [IN/OUT] argv)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_other_init
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_cm_other_init(int * argc, char *** argv)
{
    mpig_vc_t * vc;
    MPIU_CHKPMEM_DECL(1);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_init);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_init);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_CHKPMEM_MALLOC(vc, mpig_vc_t *, sizeof(mpig_vc_t), mpi_errno, "VC for processing MPI_ANY_SOURCE/MPI_PROC_NULL");
    mpig_vc_create(vc);
    mpig_vc_set_state(vc, MPIG_VC_STATE_CONNECTED);
    mpig_vc_set_cm_type(vc, MPIG_CM_TYPE_OTHER);
    mpig_vc_set_cm_funcs(vc, mpig_cm_other_funcs);

    mpig_cm_other_vc = vc;
    
  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_other_init);
    return mpi_errno;

  fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_return;
}
/* mpig_cm_other_init() */


/*
 * mpig_cm_other_finalize()
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_other_finalize
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_cm_other_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_finalize);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_finalize);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_Free(mpig_cm_other_vc);
    mpig_cm_other_vc = NULL;
    
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_other_finalize);
    return mpi_errno;
}
/* mpig_cm_other_finalize() */


/*
 * mpig_cm_other_add_contact_info([IN/OUT] business card)
 *
 * Add any and all contact information for this communication module to the supplied business card.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_other_add_contact_info
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_cm_other_add_contact_info(mpig_bc_t * bc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_add_contact_info);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_add_contact_info);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    
    /* ...nothing to do... */

    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_other_add_contact_info);
    return mpi_errno;
}
/* mpig_cm_other_add_contact_info() */


/*
 * int mpig_cm_other_select_module([IN] bc, [IN/OUT] vc, [OUT] flag)
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
#define FUNCNAME mpig_cm_other_select_module
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_cm_other_select_module(mpig_bc_t * bc, mpig_vc_t * vc, int * flag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_select_module);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_select_module);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    
    *flag = FALSE;
    
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_other_select_module);
    return mpi_errno;
}
/* int mpig_cm_other_select_module([IN] business card, [IN/OUT] virtual connection, [OUT] flag) */


/*
 * int mpig_cm_other_adi3_send(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_other_adi3_send
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_STATIC int mpig_cm_other_adi3_send(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
					int ctxoff, MPID_Request ** sreqp)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_adi3_send);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_adi3_send);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    if (rank == MPI_PROC_NULL)
    { 
	*sreqp = NULL;
    }
    else if (rank == MPI_ANY_SOURCE)
    {
	MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_RANK, "**globus|cm_other_send_any_source");
    }
    else
    {
	MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_RANK, "**globus|cm_other_invalid_dest", "**globus|cm_other_invalid_dest %d", rank);
    }
    
  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_other_adi3_send);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_other_adi3_send(...) */


/*
 * int mpig_cm_other_adi3_isend(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_other_adi3_isend
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_STATIC int mpig_cm_other_adi3_isend(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
					 int ctxoff, MPID_Request ** sreqp)
{
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_adi3_isend);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_adi3_isend);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    if (rank == MPI_PROC_NULL)
    {
	mpig_request_create_isreq(MPIG_REQUEST_TYPE_SEND, 1, 0, (void *) buf, cnt, dt, rank, tag, ctx, comm, sreqp);
	MPIG_DBG_PRINTF((15, FCNAME, "MPI_PROC_NULL send request allocated, handle=0x%08x, ptr=%p", (*sreqp)->handle, *sreqp));
    }
    else if (rank == MPI_ANY_SOURCE)
    {
	MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_RANK, "**globus|cm_other_send_any_source");
    }
    else
    {
	MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_RANK, "**globus|cm_other_invalid_dest", "**globus|cm_other_invalid_dest %d", rank);
    }

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_other_adi3_isend);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_other_adi3_isend(...) */


/*
 * int mpig_cm_other_adi3_rsend(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_other_adi3_rsend
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_STATIC int mpig_cm_other_adi3_rsend(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
					 int ctxoff, MPID_Request ** sreqp)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_adi3_rsend);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_adi3_rsend);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    if (rank == MPI_PROC_NULL)
    { 
	*sreqp = NULL;
    }
    else if (rank == MPI_ANY_SOURCE)
    {
	MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_RANK, "**globus|cm_other_send_any_source");
    }
    else
    {
	MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_RANK, "**globus|cm_other_invalid_dest", "**globus|cm_other_invalid_dest %d", rank);
    }
    
  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_other_adi3_rsend);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_other_adi3_rsend(...) */


/*
 * int mpig_cm_other_adi3_irsend(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_other_adi3_irsend
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_STATIC int mpig_cm_other_adi3_irsend(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
					  int ctxoff, MPID_Request ** sreqp)
{
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_adi3_irsend);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_adi3_irsend);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    if (rank == MPI_PROC_NULL)
    {
	mpig_request_create_isreq(MPIG_REQUEST_TYPE_RSEND, 1, 0, (void *) buf, cnt, dt, rank, tag, ctx, comm, sreqp);
	MPIG_DBG_PRINTF((15, FCNAME, "MPI_PROC_NULL send request allocated, handle=0x%08x, ptr=%p", (*sreqp)->handle, *sreqp));
    }
    else if (rank == MPI_ANY_SOURCE)
    {
	MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_RANK, "**globus|cm_other_send_any_source");
    }
    else
    {
	MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_RANK, "**globus|cm_other_invalid_dest", "**globus|cm_other_invalid_dest %d", rank);
    }

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_other_adi3_irsend);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_other_adi3_irsend(...) */


/*
 * int mpig_cm_other_adi3_ssend(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_other_adi3_ssend
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_STATIC int mpig_cm_other_adi3_ssend(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
					 int ctxoff, MPID_Request ** sreqp)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_adi3_ssend);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_adi3_ssend);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    if (rank == MPI_PROC_NULL)
    { 
	*sreqp = NULL;
    }
    else if (rank == MPI_ANY_SOURCE)
    {
	MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_RANK, "**globus|cm_other_send_any_source");
    }
    else
    {
	MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_RANK, "**globus|cm_other_invalid_dest", "**globus|cm_other_invalid_dest %d", rank);
    }
    
  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_other_adi3_ssend);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_other_adi3_ssend(...) */


/*
 * int mpig_cm_other_adi3_issend(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_other_adi3_issend
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_STATIC int mpig_cm_other_adi3_issend(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
					  int ctxoff, MPID_Request ** sreqp)
{
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_adi3_issend);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_adi3_issend);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    if (rank == MPI_PROC_NULL)
    {
	mpig_request_create_isreq(MPIG_REQUEST_TYPE_SSEND, 1, 0, (void *) buf, cnt, dt, rank, tag, ctx, comm, sreqp);
	MPIG_DBG_PRINTF((15, FCNAME, "MPI_PROC_NULL send request allocated, handle=0x%08x, ptr=%p", (*sreqp)->handle, *sreqp));
    }
    else if (rank == MPI_ANY_SOURCE)
    {
	MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_RANK, "**globus|cm_other_send_any_source");
    }
    else
    {
	MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_RANK, "**globus|cm_other_invalid_dest", "**globus|cm_other_invalid_dest %d", rank);
    }

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
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
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_STATIC int mpig_cm_other_adi3_recv(void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
					int ctxoff, MPI_Status * status, MPID_Request ** rreqp)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_adi3_recv);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_adi3_recv);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    if (rank == MPI_PROC_NULL)
    { 
	*rreqp = NULL;
    }
    else if (rank == MPI_ANY_SOURCE)
    {
	/* XXX: MPI ANY SOURCE not implemented!!! */
	MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);
    }
    else
    {
	MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_RANK, "**globus|cm_other_invalid_src", "**globus|cm_other_invalid_src %d", rank);
    }
    
  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
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
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_STATIC int mpig_cm_other_adi3_irecv(void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
					 int ctxoff, MPID_Request ** rreqp)
{
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_other_adi3_irecv);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_other_adi3_irecv);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    
    if (rank == MPI_PROC_NULL)
    {
	mpig_request_create_irreq(1, 0, buf, cnt, dt, rank, tag, ctx, comm, rreqp);
	MPIR_Status_set_procnull(&(*rreqp)->status);
	MPIG_DBG_PRINTF((15, FCNAME, "MPI_PROC_NULL receive request allocated, handle=0x%08x, ptr=%p", (*rreqp)->handle, *rreqp));
    }
    else if (rank == MPI_ANY_SOURCE)
    {
	/* XXX: MPI ANY SOURCE not implemented!!! */
	MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);
    }
    else
    {
	MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_RANK, "**globus|cm_other_invalid_src", "**globus|cm_other_invalid_src %d", rank);
    }
    
  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_other_adi3_irecv);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_other_adi3_irecv() */
