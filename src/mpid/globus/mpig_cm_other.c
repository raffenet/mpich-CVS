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

MPIG_STATIC int mpig_cm_other_adi3_recv(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
					int ctxoff, MPI_Status * status, MPID_Request ** rreqp);

MPIG_STATIC int mpig_cm_other_adi3_irecv(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
					 int ctxoff, MPID_Request ** rreqp);


/*
 * ADI3 function table
 */
MPIG_STATIC mpig_cm_funcs_t mpig_cm_other_funcs =
{
    mpig_cm_other_adi3_send,
    mpig_cm_other_adi3_isend,
    NULL,
    NULL,
    NULL,
    NULL,
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

    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_CHKPMEM_MALLOC(vc, mpig_vc_t *, sizeof(mpig_vc_t), mpi_errno, "VC for processing MPI_ANY_SOURCE/MPI_PROC_NULL");
    mpig_vc_create(vc);
    mpig_vc_set_state(vc, MPIG_VC_STATE_CONNECTED);
    mpig_vc_set_cm_type(vc, MPIG_CM_TYPE_OTHER);
    mpig_vc_set_cm_funcs(vc, mpig_cm_other_funcs);

    mpig_cm_other_vc = vc;
    
  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
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

    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_Free(mpig_cm_other_vc);
    mpig_cm_other_vc = NULL;
    
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
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
    
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    
    /* ...nothing to do... */

    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
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
    
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    
    *flag = FALSE;
    
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
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
    
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    if (rank == MPI_PROC_NULL)
    { 
	*sreqp = NULL;
    }
    else
    {
	/* XXX: error */
	mpi_errno = MPI_ERR_INTERN;
    }
    
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    return mpi_errno;
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
    MPID_Request * sreq;
    int mpi_errno = MPI_SUCCESS;
    
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    if (rank == MPI_PROC_NULL)
    { 
	/* allocate request */
	sreq = mpig_request_create();
	MPIU_ERR_CHKANDJUMP1((sreq == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "receive request");

	/* set MPICH fields */
	MPIU_Object_set_ref(sreq, 1);
	sreq->kind = MPID_REQUEST_SEND;
	sreq->cc = 0;
	sreq->cc_ptr = &sreq->cc;
	sreq->comm = NULL;
	MPIR_Status_set_empty(&sreq->status);

	/* set device fields */
	mpig_request_state_init(sreq);
	sreq->dev.dtp = NULL;
    
	*sreqp = sreq;
    }
    else
    {
	/* XXX: error */
	mpi_errno = MPI_ERR_INTERN;
    }

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_other_adi3_isend(...) */


/*
 * int mpig_cm_other_adi3_recv(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_other_adi3_recv
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_STATIC int mpig_cm_other_adi3_recv(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
					int ctxoff, MPI_Status * status, MPID_Request ** rreqp)
{
    int mpi_errno = MPI_SUCCESS;

    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    if (rank == MPI_PROC_NULL)
    { 
	*rreqp = NULL;
    }
    else if (rank == MPI_ANY_SOURCE)
    {
	/* XXX: MPI ANY SOURCE!!! */
	mpi_errno = MPI_ERR_INTERN;
    }
    else
    {
	/* XXX: error */
	mpi_errno = MPI_ERR_INTERN;
    }
    
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    return mpi_errno;
}
/* mpig_cm_other_adi3_recv(...) */


/*
 * int mpig_cm_other_adi3_irecv(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_other_adi3_recv
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_STATIC int mpig_cm_other_adi3_irecv(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
					 int ctxoff, MPID_Request ** rreqp)
{
    MPID_Request * rreq;
    int mpi_errno = MPI_SUCCESS;

    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    
    if (rank == MPI_PROC_NULL)
    { 
	/* allocate request */
	rreq = mpig_request_create();
	MPIU_ERR_CHKANDJUMP1((rreq == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "receive request");

	/* set MPICH fields */
	MPIU_Object_set_ref(rreq, 1);
	rreq->kind = MPID_REQUEST_RECV;
	rreq->cc = 0;
	rreq->cc_ptr = &rreq->cc;
	rreq->comm = NULL;
	MPIR_Status_set_procnull(&rreq->status);
	
	/* set device fields */
	mpig_request_state_init(rreq);
	rreq->dev.dtp = NULL;
	
	*rreqp = NULL;
    }
    else if (rank == MPI_ANY_SOURCE)
    {
	/* XXX: MPI ANY SOURCE!!! */
	mpi_errno = MPI_ERR_INTERN;
    }
    else
    {
	/* XXX: error */
	mpi_errno = MPI_ERR_INTERN;
    }
    
  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_other_adi3_irecv() */
