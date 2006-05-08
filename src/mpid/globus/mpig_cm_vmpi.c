/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"

#if defined(MPIG_VMPI)
/*
 * miscellaneous internal variable and function declarations
 */
globus_uuid_t mpig_vmpi_job_id;


/**********************************************************************************************************************************
					       BEGIN VC FUNCTION TABLE DEFINITION
**********************************************************************************************************************************/
/* prototypes for VC function table functions */
MPIG_STATIC int mpig_cm_vmpi_adi3_send(
    const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** sreqp);

MPIG_STATIC int mpig_cm_vmpi_adi3_isend(
    const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** sreqp);

MPIG_STATIC int mpig_cm_vmpi_adi3_rsend(
    const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** sreqp);

MPIG_STATIC int mpig_cm_vmpi_adi3_irsend(
    const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** sreqp);

MPIG_STATIC int mpig_cm_vmpi_adi3_ssend(
    const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** sreqp);

MPIG_STATIC int mpig_cm_vmpi_adi3_issend(
    const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** sreqp);

MPIG_STATIC int mpig_cm_vmpi_adi3_recv(
    void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPI_Status * status,
    MPID_Request ** rreqp);

MPIG_STATIC int mpig_cm_vmpi_adi3_irecv(
    void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,int ctxoff, MPID_Request ** rreqp);

MPIG_STATIC int mpig_cm_vmpi_adi3_cancel_send(MPID_Request * sreq);

MPIG_STATIC int mpig_cm_vmpi_adi3_cancel_recv(MPID_Request * rreq);

MPIG_STATIC void mpig_cm_vmpi_vc_recv_any_source(
    mpig_vc_t * vc, MPID_Request * rreq, MPID_Comm * comm, int * mpi_errno_p, bool_t * failed_p);


/* VC communication module function table */
MPIG_STATIC mpig_vc_cm_funcs_t mpig_cm_vmpi_vc_funcs =
{
    mpig_cm_vmpi_adi3_send,
    mpig_cm_vmpi_adi3_isend,
    mpig_cm_vmpi_adi3_rsend,
    mpig_cm_vmpi_adi3_irsend,
    mpig_cm_vmpi_adi3_ssend,
    mpig_cm_vmpi_adi3_issend,
    mpig_cm_vmpi_adi3_recv,
    mpig_cm_vmpi_adi3_irecv,
    mpig_cm_vmpi_adi3_cancel_recv,
    mpig_cm_vmpi_adi3_cancel_send,
    NULL, /* mpig_cm_vmpi_vc_recv_any_source */
    NULL, /* vc_inc_ref_count */
    NULL, /* vc_dec_ref_count */
    NULL, /* vc_destruct */
    mpig_vc_null_func
};
/**********************************************************************************************************************************
					       END VC FUNCTION TABLE DEFINITION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
				     BEGIN COMMUNICATION MODULE PROGRESS ENGINE DECLARATIONS
**********************************************************************************************************************************/
MPIG_STATIC mpig_vmpi_request_t * mpig_cm_vmpi_vreq_table;
MPIG_STATIC MPID_Request * mpig_vm_vmpi_req_table;

MPIG_STATIC void mpig_cm_vmpi_progress_init(int * mpi_errno_p, bool_t * failed_p);

MPIG_STATIC void mpig_cm_vmpi_progress_add_req(MPID_Request * req, int * mpi_errno_p, bool_t * failed_p);
/**********************************************************************************************************************************
				      END COMMUNICATION MODULE PROGRESS ENGINE DECLARATIONS
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						BEGIN UTILITY AND ACCESSOR MACROS
**********************************************************************************************************************************/
#define mpig_cm_vmpi_comm_set_vrank(comm_, mrank_, vrank_)	\
{								\
    (comm_)->cm.vmpi.remote_ranks_mtov[mrank_] = (vrank_);	\
}

#define mpig_cm_vmpi_comm_get_vrank(comm_, mrank_, vrank_p_)	\
{								\
    *(vrank_p_) = (comm_)->cm.vmpi.remote_ranks_mtov[mrank_];	\
}

#define mpig_cm_vmpi_comm_set_mrank(comm_, vrank_, mrank_)	\
{								\
    (comm_)->cm.vmpi.remote_ranks_vtom[vrank_] = (mrank_);	\
}

#define mpig_cm_vmpi_comm_get_mrank(comm_, vrank_, mrank_p_)	\
{								\
    *(mrank_p_) = (comm_)->cm.vmpi.remote_ranks_vtom[vrank_];	\
}

#define mpig_cm_vmpi_comm_set_vcomm(comm_, ctxoff_, vcomm_)	\
{								\
    MPIU_Assert((ctxoff_) < MPIG_COMM_NUM_CONTEXTS);		\
    (comm_)->cm.vmpi.comms[ctxoff_] = *(vcomm_);		\
}

#define mpig_cm_vmpi_comm_get_vcomm(comm_, ctxoff_, vcomm_p_)	\
{								\
    MPIU_Assert((ctxoff_) < MPIG_COMM_NUM_CONTEXTS);		\
    *(vcomm_p_) = &(comm_)->cm.vmpi.comms[ctxoff_];		\
}


#define mpig_cm_vmpi_request_construct(req_)		\
{							\
    (req_)->cm.vmpi.req = *MPIG_VMPI_REQUEST_NULL;	\
							\
}

#define mpig_cm_vmpi_request_construct(req_)		\
{							\
    (req_)->cm.vmpi.req = *MPIG_VMPI_REQUEST_NULL;	\
}

#define mpig_cm_vmpi_request_set_vreq(req_, vreq_)	\
{							\
    (req_)->cm.vmpi.req = *(vreq_);			\
}

#define mpig_cm_vmpi_request_get_vreq(req_, vreq_p_)	\
{							\
    *(vreq_p_) = &(req_)->cm.vmpi.req;			\
}


#define mpig_cm_vmpi_datatype_set_vdt(dt_, vdt_)	\
{							\
    MPID_Datatype * dtp__;				\
							\
    MPID_Datatype_get_ptr((dt_), dtp__);		\
    dtp__->cm.vmpi.dt = *(vdt_);			\
}

#define mpig_cm_vmpi_datatype_get_vdt(dt_, vdt_p_)			\
{									\
    if ((dt_) != MPI_DATATYPE_NULL)					\
    {									\
	MPID_Datatype * dtp__;						\
	MPID_Datatype_get_ptr((dt_), dtp__);				\
	*(vdt_p_) = &dtp__->cm.vmpi.dt;					\
    }									\
    else								\
    {									\
	*(vdt_p_) = (mpig_vmpi_datatype_t *) MPIG_VMPI_DATATYPE_NULL;	\
    }									\
}
/**********************************************************************************************************************************
						BEGIN UTILITY AND ACCESSOR MACROS
**********************************************************************************************************************************/
#endif /* defined(MPIG_VMPI) */


/**********************************************************************************************************************************
					   END COMMUNICATION MODULE CORE API FUNCTIONS
**********************************************************************************************************************************/
/*
 * mpig_cm_vmpi_init()
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_init
int mpig_cm_vmpi_init(int * argc, char *** argv)
{
#   if defined(MPIG_VMPI)
    {
	const char fcname[] = MPIG_QUOTE(FUNCNAME);
	MPID_Comm * comm;
	mpig_vmpi_comm_t vcomm;
	int rank;
	globus_result_t grc;
	int vrc;
	int mpi_errno = MPI_SUCCESS;
	MPIU_CHKPMEM_DECL(2);
	MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_init);

	MPIG_UNUSED_VAR(fcname);
	
	MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_init);
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering"));
	
	/* iniitialize the vendor MPI module.  as part of the initialization process, mpig_vmpi_init() sets the
	   MPIG_VMPI_COMM_WORLD error handler to MPI_ERRORS_RETURN. */
        vrc = mpig_vmpi_init(argc, argv);
	MPIU_ERR_CHKANDJUMP((vrc), mpi_errno, MPI_ERR_OTHER, "**globus|vmpi_init");

	/* get the size of the vendor MPI_COMM_WORLD and the rank of this process within it */
	vrc = mpig_vmpi_comm_size(MPIG_VMPI_COMM_WORLD, &mpig_process.cm.vmpi.cw_size);
	MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Comm_size", &mpi_errno);
	
	vrc = mpig_vmpi_comm_rank(MPIG_VMPI_COMM_WORLD, &mpig_process.cm.vmpi.cw_rank);
	MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Comm_rank", &mpi_errno);

	/* on process 0 of the vendor MPI_COMM_WORLD create a unique id.  then, broadcast that id to all other processes in that
	   MPI_COMM_WORLD.  this id will allow the processes to determine if they are part of the same vendor MPI job (or globus
	   subjob if you prefer). */
	if (mpig_process.cm.vmpi.cw_rank == 0)
	{
	    int job_id_len;
	    
	    grc = globus_uuid_create(&mpig_vmpi_job_id);
	    if (grc == 0)
	    {
		job_id_len = strlen(mpig_vmpi_job_id.text) + 1;
	    }
	    else
	    {
		job_id_len = 0;
	    }

	    vrc = mpig_vmpi_bcast(&job_id_len, 1, MPIG_VMPI_INT, 0, MPIG_VMPI_COMM_WORLD);
	    MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Bcast", &mpi_errno);

	    MPIU_ERR_CHKANDJUMP1((job_id_len == 0), mpi_errno, MPI_ERR_OTHER, "**globus|cm_vmpi|uuid_create",
		"**globus|cm_vmpi|uuid_create %s", globus_error_print_chain(globus_error_peek(grc)));
	    
	    vrc = mpig_vmpi_bcast(mpig_vmpi_job_id.text, job_id_len, MPIG_VMPI_CHAR, 0, MPIG_VMPI_COMM_WORLD);
	    MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Bcast", &mpi_errno);
	}
	else
	{
	    int job_id_len;
	    char job_id_str[GLOBUS_UUID_TEXTLEN + 1];
	    
	    vrc = mpig_vmpi_bcast(&job_id_len, 1, MPIG_VMPI_INT, 0, MPIG_VMPI_COMM_WORLD);
	    MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Bcast", &mpi_errno);
	    MPIU_ERR_CHKANDJUMP((job_id_len == 0), mpi_errno, MPI_ERR_OTHER, "**globus|cm_vmpi|uuid_create");

	    vrc = mpig_vmpi_bcast(job_id_str, job_id_len, MPIG_VMPI_BYTE, 0, MPIG_VMPI_COMM_WORLD);
	    MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Bcast", &mpi_errno);

	    grc = globus_uuid_import(&mpig_vmpi_job_id, job_id_str);
	    MPIU_ERR_CHKANDJUMP((job_id_len == 0), mpi_errno, MPI_ERR_OTHER, "**globus|cm_vmpi|uuid_create");
	}

	/* create a mapping from the MPICH2 intrinsic datatypes to the vendor MPI intrinsic datatypes */
	/* c basic datatypes */
	mpig_cm_vmpi_datatype_set_vdt(MPI_BYTE, MPIG_VMPI_BYTE);
	mpig_cm_vmpi_datatype_set_vdt(MPI_CHAR, MPIG_VMPI_CHAR);
	mpig_cm_vmpi_datatype_set_vdt(MPI_SIGNED_CHAR, MPIG_VMPI_SIGNED_CHAR);
	mpig_cm_vmpi_datatype_set_vdt(MPI_UNSIGNED_CHAR, MPIG_VMPI_UNSIGNED_CHAR);
	mpig_cm_vmpi_datatype_set_vdt(MPI_WCHAR, MPIG_VMPI_WCHAR);
	mpig_cm_vmpi_datatype_set_vdt(MPI_SHORT, MPIG_VMPI_SHORT);
	mpig_cm_vmpi_datatype_set_vdt(MPI_UNSIGNED_SHORT, MPIG_VMPI_UNSIGNED_SHORT);
	mpig_cm_vmpi_datatype_set_vdt(MPI_INT, MPIG_VMPI_INT);
	mpig_cm_vmpi_datatype_set_vdt(MPI_UNSIGNED, MPIG_VMPI_UNSIGNED);
	mpig_cm_vmpi_datatype_set_vdt(MPI_LONG, MPIG_VMPI_LONG);
	mpig_cm_vmpi_datatype_set_vdt(MPI_UNSIGNED_LONG, MPIG_VMPI_UNSIGNED_LONG);
	mpig_cm_vmpi_datatype_set_vdt(MPI_LONG_LONG, MPIG_VMPI_LONG_LONG);
	mpig_cm_vmpi_datatype_set_vdt(MPI_LONG_LONG_INT, MPIG_VMPI_LONG_LONG_INT);
	mpig_cm_vmpi_datatype_set_vdt(MPI_UNSIGNED_LONG_LONG, MPIG_VMPI_UNSIGNED_LONG_LONG);
	mpig_cm_vmpi_datatype_set_vdt(MPI_FLOAT, MPIG_VMPI_FLOAT);
	mpig_cm_vmpi_datatype_set_vdt(MPI_DOUBLE, MPIG_VMPI_DOUBLE);
	if (MPI_LONG_DOUBLE != MPI_DATATYPE_NULL)
	{
	    mpig_cm_vmpi_datatype_set_vdt(MPI_LONG_DOUBLE, MPIG_VMPI_LONG_DOUBLE);
	}
	/* c paired datatypes used predominantly for minloc/maxloc reduce operations */
	mpig_cm_vmpi_datatype_set_vdt(MPI_SHORT_INT, MPIG_VMPI_SHORT_INT);
	mpig_cm_vmpi_datatype_set_vdt(MPI_2INT, MPIG_VMPI_2INT);
	mpig_cm_vmpi_datatype_set_vdt(MPI_LONG_INT, MPIG_VMPI_LONG_INT);
	mpig_cm_vmpi_datatype_set_vdt(MPI_FLOAT_INT, MPIG_VMPI_FLOAT_INT);
	mpig_cm_vmpi_datatype_set_vdt(MPI_DOUBLE_INT, MPIG_VMPI_DOUBLE_INT);
	if (MPI_LONG_DOUBLE_INT != MPI_DATATYPE_NULL)
	{
	    mpig_cm_vmpi_datatype_set_vdt(MPI_LONG_DOUBLE_INT, MPIG_VMPI_LONG_DOUBLE_INT);
	}
	/* fortran basic datatypes */
	mpig_cm_vmpi_datatype_set_vdt(MPI_LOGICAL, MPIG_VMPI_LOGICAL);
	mpig_cm_vmpi_datatype_set_vdt(MPI_CHARACTER, MPIG_VMPI_CHARACTER);
	mpig_cm_vmpi_datatype_set_vdt(MPI_INTEGER, MPIG_VMPI_INTEGER);
	mpig_cm_vmpi_datatype_set_vdt(MPI_REAL, MPIG_VMPI_REAL);
	mpig_cm_vmpi_datatype_set_vdt(MPI_DOUBLE_PRECISION, MPIG_VMPI_DOUBLE_PRECISION);
	mpig_cm_vmpi_datatype_set_vdt(MPI_COMPLEX, MPIG_VMPI_COMPLEX);
	mpig_cm_vmpi_datatype_set_vdt(MPI_DOUBLE_COMPLEX, MPIG_VMPI_DOUBLE_COMPLEX);
	/* fortran paired datatypes used predominantly for minloc/maxloc reduce operations */
	mpig_cm_vmpi_datatype_set_vdt(MPI_2INTEGER, MPIG_VMPI_2INTEGER);
	mpig_cm_vmpi_datatype_set_vdt(MPI_2COMPLEX, MPIG_VMPI_2COMPLEX);
	mpig_cm_vmpi_datatype_set_vdt(MPI_2REAL, MPIG_VMPI_2REAL);
	mpig_cm_vmpi_datatype_set_vdt(MPI_2DOUBLE_COMPLEX, MPIG_VMPI_2DOUBLE_COMPLEX);
	mpig_cm_vmpi_datatype_set_vdt(MPI_2DOUBLE_PRECISION, MPIG_VMPI_2DOUBLE_PRECISION);
	/* fortran size specific datatypes */
	mpig_cm_vmpi_datatype_set_vdt(MPI_INTEGER1, MPIG_VMPI_INTEGER1);
	mpig_cm_vmpi_datatype_set_vdt(MPI_INTEGER2, MPIG_VMPI_INTEGER2);
	mpig_cm_vmpi_datatype_set_vdt(MPI_INTEGER4, MPIG_VMPI_INTEGER4);
	mpig_cm_vmpi_datatype_set_vdt(MPI_INTEGER8, MPIG_VMPI_INTEGER8);
	if (MPI_INTEGER16 != MPI_DATATYPE_NULL)
	{
	    mpig_cm_vmpi_datatype_set_vdt(MPI_INTEGER16, MPIG_VMPI_INTEGER16);
	}
	mpig_cm_vmpi_datatype_set_vdt(MPI_REAL4, MPIG_VMPI_REAL4);
	mpig_cm_vmpi_datatype_set_vdt(MPI_REAL8, MPIG_VMPI_REAL8);
	if (MPI_REAL16 != MPI_DATATYPE_NULL)
	{
	    mpig_cm_vmpi_datatype_set_vdt(MPI_REAL16, MPIG_VMPI_REAL16);
	}
	mpig_cm_vmpi_datatype_set_vdt(MPI_COMPLEX8, MPIG_VMPI_COMPLEX8);
	mpig_cm_vmpi_datatype_set_vdt(MPI_COMPLEX16, MPIG_VMPI_COMPLEX16);
	if (MPI_COMPLEX32 != MPI_DATATYPE_NULL)
	{
	    mpig_cm_vmpi_datatype_set_vdt(MPI_COMPLEX32, MPIG_VMPI_COMPLEX32);
	}
	/* type representing a packed user buffer */
	mpig_cm_vmpi_datatype_set_vdt(MPI_PACKED, MPIG_VMPI_PACKED);
	/* pseudo datatypes used to manipulate the extent */
	mpig_cm_vmpi_datatype_set_vdt(MPI_LB, MPIG_VMPI_LB);
	mpig_cm_vmpi_datatype_set_vdt(MPI_UB, MPIG_VMPI_UB);
	
	
	/* initialize the VMPI specific fields that are part of the MPI_COMM_WORLD object */
#	if (MPIG_COMM_NUM_CONTEXTS != 2)
#	    error "the code in mpig_cm_vmpi_init() assumes MPIG_COMM_NUM_CONTEXTS is 2."
#	endif

	comm = MPIR_Process.comm_world;

	vrc = mpig_vmpi_comm_dup(MPIG_VMPI_COMM_WORLD, &vcomm);
	MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Comm_dup", &mpi_errno);
	mpig_cm_vmpi_comm_set_vcomm(comm, MPID_CONTEXT_INTRA_PT2PT, &vcomm);
	
	vrc = mpig_vmpi_comm_dup(MPIG_VMPI_COMM_WORLD, &vcomm);
	MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Comm_dup", &mpi_errno);
	mpig_cm_vmpi_comm_set_vcomm(comm, MPID_CONTEXT_INTRA_COLL, &vcomm);
	
	MPIU_CHKPMEM_MALLOC(comm->cm.vmpi.remote_ranks_mtov, int *, comm->remote_size * sizeof(int), mpi_errno,
	    "MPICH2 to vendor MPI comm rank translation table");
	MPIU_CHKPMEM_MALLOC(comm->cm.vmpi.remote_ranks_vtom, int *, mpig_process.cm.vmpi.cw_size * sizeof(int), mpi_errno,
	    "vendor MPI to MPICH2 comm rank translation table");
	for (rank = 0; rank < comm->remote_size; rank++)
	{
	    mpig_cm_vmpi_comm_set_vrank(comm, rank, MPIG_VMPI_UNDEFINED);
	}
	for (rank = 0; rank <  mpig_process.cm.vmpi.cw_size; rank++)
	{
	    mpig_cm_vmpi_comm_set_mrank(comm, rank, MPI_UNDEFINED);
	}

	/* initialize the VMPI specific fields that are part of the MPI_COMM_SELF object */
	comm = MPIR_Process.comm_self;
	mpig_cm_vmpi_comm_set_vcomm(comm, MPID_CONTEXT_INTRA_PT2PT, MPIG_VMPI_COMM_NULL);
	mpig_cm_vmpi_comm_set_vcomm(comm, MPID_CONTEXT_INTRA_COLL, MPIG_VMPI_COMM_NULL);
	comm->cm.vmpi.remote_ranks_mtov = NULL;
	comm->cm.vmpi.remote_ranks_vtom = NULL;

	/* MPIU_CHKPMEM_COMMIT() is implicit */

      fn_return:
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno=0x%08x", mpi_errno));
	MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_init);
	return mpi_errno;

      fn_fail:
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIU_CHKPMEM_REAP();
	    goto fn_return;
	}   /* --END ERROR HANDLING-- */
    }
#   else
    {
	/* ...nothing to do... */
	return MPI_SUCCESS;
    }
#   endif
}
/* mpig_cm_vmpi_init() */


/*
 * mpig_cm_vmpi_finalize()
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_finalize
int mpig_cm_vmpi_finalize(void)
{
#   if defined(MPIG_VMPI)
    {
	const char fcname[] = MPIG_QUOTE(FUNCNAME);
	int vrc;
	int mpi_errno = MPI_SUCCESS;
	MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_finalize);

	MPIG_UNUSED_VAR(fcname);
	
	MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_finalize);
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering"));

	/* shutdown the vendor MPI module */
	vrc = mpig_vmpi_finalize();
	MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Finalize", &mpi_errno);

      fn_return:
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno=0x%08x", mpi_errno));
	MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_finalize);
	return mpi_errno;

      fn_fail:
	{   /* --BEGIN ERROR HANDLING-- */
	    goto fn_return;
	}   /* --END ERROR HANDLING-- */
    }
#   else
    {
	/* ...nothing to do... */
	return MPI_SUCCESS;
    }
#endif
}
/* mpig_cm_vmpi_finalize() */


/*
 * mpig_cm_vmpi_add_contact_info([IN/OUT] business card)
 *
 * Add any and all contact information for this communication module to the supplied business card.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_add_contact_info
int mpig_cm_vmpi_add_contact_info(mpig_bc_t * bc)
{
#   if defined(MPIG_VMPI)
    {
	const char fcname[] = MPIG_QUOTE(FUNCNAME);
	char uint_str[10];
	bool_t failed;
	int mpi_errno = MPI_SUCCESS;
	MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_add_contact_info);

	MPIG_UNUSED_VAR(fcname);
	
	MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_add_contact_info);
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering"));
    
	mpig_bc_add_contact(bc, "CM_VMPI_UUID", mpig_vmpi_job_id.text, &mpi_errno, &failed);
	MPIU_ERR_CHKANDJUMP1((failed), mpi_errno, MPI_ERR_OTHER, "**globus|bc_add_contact",
	    "**globus|bc_add_contact %s", "CM_VMPI_UUID");
	
	MPIU_Snprintf(uint_str, 10, "%u", (unsigned) mpig_process.cm.vmpi.cw_rank);
	mpig_bc_add_contact(bc, "CM_VMPI_CW_RANK", uint_str, &mpi_errno, &failed);
	MPIU_ERR_CHKANDJUMP1((failed), mpi_errno, MPI_ERR_OTHER, "**globus|bc_add_contact",
	    "**globus|bc_add_contact %s", "CM_VMPI_CW_RANK");
	
      fn_return:
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno=0x%08x", mpi_errno));
	MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_add_contact_info);
	return mpi_errno;

      fn_fail:
	{   /* --BEGIN ERROR HANDLING-- */
	    goto fn_return;
	}   /* --END ERROR HANDLING-- */
    }
#   else
    {
	/* ...nothing to do... */
	return MPI_SUCCESS;
    }
#   endif
}
/* mpig_cm_vmpi_add_contact_info() */


/*
 * int mpig_cm_vmpi_select_module([IN] bc, [IN/OUT] vc, [OUT] selected)
 *
 * Check the business card to see if the connection module can communicate with the remote process associated with the supplied
 * VC.  If it can, then the VC will be initialized accordingly.
 *
 * Parameters:
 *
 * bc [IN] - business card containing contact information
 * vc [IN] - vc object to initialize if the communication module is capable of performing communication with the associated process
 * selected [OUT] - TRUE if the communication module can communicate with the remote process; otherwise FALSE
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_select_module
int mpig_cm_vmpi_select_module(mpig_bc_t * bc, mpig_vc_t * vc, bool_t * selected)
{
#   if defined(MPIG_VMPI)
    {
	const char fcname[] = MPIG_QUOTE(FUNCNAME);
	char * uuid_str = NULL;
	char * cw_rank_str = NULL;
	int cw_rank;
	bool_t found;
	bool_t failed;
	int rc;
	int mpi_errno = MPI_SUCCESS;
	MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_select_module);

	MPIG_UNUSED_VAR(fcname);
	
	MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_select_module);
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering"));
	
	mpig_bc_get_contact(bc, "CM_VMPI_UUID", &uuid_str, &found, &mpi_errno, &failed);
	MPIU_ERR_CHKANDJUMP1((failed), mpi_errno, MPI_ERR_OTHER, "**globus|bc_get_contact",
	    "**globus|bc_get_contact %s", "CM_VMPI_UUID");
	if (found == FALSE) goto fn_return;
	if (strcmp(mpig_vmpi_job_id.text, uuid_str) != 0) goto fn_return;
    
	mpig_bc_get_contact(bc, "CM_VMPI_CW_RANK", &cw_rank_str, &found, &mpi_errno, &failed);
	MPIU_ERR_CHKANDJUMP1((failed), mpi_errno, MPI_ERR_OTHER, "**globus|bc_get_contact",
	    "**globus|bc_get_contact %s", "CM_VMPI_CW_RANK");
	if (found == FALSE) goto fn_return;
	
	rc = sscanf(cw_rank_str, "%d", &cw_rank);
	MPIU_ERR_CHKANDJUMP((rc != 1), mpi_errno, MPI_ERR_INTERN, "**keyval");

	/* initialize the CM VMPI fields in the VC object */
	mpig_vc_set_cm_type(vc, MPIG_CM_TYPE_VMPI);
	mpig_vc_set_cm_funcs(vc, &mpig_cm_vmpi_vc_funcs);
	vc->cm.vmpi.cw_rank = cw_rank;

	/* set the rank translations in the MPI_COMM_WORLD communicator */
	mpig_cm_vmpi_comm_set_vrank(MPIR_Process.comm_world, mpig_vc_get_pg_rank(vc), cw_rank);
	mpig_cm_vmpi_comm_set_mrank(MPIR_Process.comm_world, cw_rank, mpig_vc_get_pg_rank(vc));

	/* adjust the PG reference to account for the newly activated VC */
	mpig_pg_inc_ref_count(mpig_vc_get_pg(vc));
    
	/* set the selected flag to indicate that the "vmpi" communication module has accepted responsibility for the VC */
	*selected = TRUE;
    
      fn_return:
	if (uuid_str != NULL) mpig_bc_free_contact(uuid_str);
	if (cw_rank_str != NULL) mpig_bc_free_contact(cw_rank_str);
	
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno=0x%08x", mpi_errno));
	MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_select_module);
	return mpi_errno;

      fn_fail:
	{   /* --BEGIN ERROR HANDLING-- */
	    goto fn_return;
	}   /* --END ERROR HANDLING-- */
    }
#   else
    {
	*selected = FALSE;
	return MPI_SUCCESS;
    }
#   endif
}
/* int mpig_cm_vmpi_select_module() */
/**********************************************************************************************************************************
					   END COMMUNICATION MODULE CORE API FUNCTIONS
**********************************************************************************************************************************/


#if defined(MPIG_VMPI)
/**********************************************************************************************************************************
				    BEGIN COMMUNICATION MODULE PROGRESS ENGINE API FUNCTIONS
**********************************************************************************************************************************/
MPIG_STATIC mpig_vmpi_request_t * mpig_cm_vmpi_progress_vreq_array;
MPIG_STATIC MPID_Request * mpig_vm_vmpi_progress_mreq_array;
MPIG_STATIC mpig_vmpi_status_t * mpig_cm_vmpi_progress_vstatus_array;
MPIG_STATIC int mpig_cm_vmpi_progress_array_num_active_entries;
MPIG_STATIC int mpig_cm_vmpi_progress_array_size;
MPIG_STATIC int * mpig_cm_vmpi_progress_free_entry_stack;
MPIG_STATIC int mpig_cm_vmpi_progress_free_entry_stack_top;
volatile int mpig_cm_vmpi_progress_ops_outstanding = 0;


#define mpig_cm_vmpi_progress_start_op()	\
{						\
    mpig_cm_vmpi_progress_ops_outstanding += 1;	\
    mpig_progress_start_op();			\
}

#define mpig_cm_vmpi_progress_complete_op()	\
{						\
    mpig_cm_vmpi_progress_ops_outstanding -= 1;	\
    mpig_progress_start_complete_op();		\
}


/*
 * int mpig_cm_vmpi_progress_init([IN/OUT] mpi_errno, [OUT] failed)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_progress_init
MPIG_STATIC void mpig_cm_vmpi_progress_init(int * mpi_errno_p, bool_t * failed_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIU_CHKPMEM_DECL(4);
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_progress_init);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_progress_init);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS, "entering"));

    *failed_p = FALSE;
    
    /* the vendor MPI communication requires that it be polled.  increment the counter that tracks the number of communications
       modules that require polling to make progress. */
    mpig_progress_num_cm_requiring_polling += 1;

    /* allocate and initialize the arrays used to track active requests */
    mpig_cm_vmpi_array_size = 0;
    mpig_cm_vmpi_array_num_active_entries = 0;
    
    MPIU_CHKPMEM_MALLOC(mpig_cm_vmpi_progress_vreq_array, mpig_vmpi_request_t *, MPIG_CM_VMPI_REQUEST_TABLE_BLOCK_ALLOC_SIZE *
	sizeof(mpig_vmpi_request_t), *mpi_errno_p, "array of active vendor MPI requests");

    MPIU_CHKPMEM_MALLOC(mpig_cm_vmpi_progress_mreq_table, MPID_Request *, MPIG_CM_VMPI_REQUEST_TABLE_BLOCK_ALLOC_SIZE *
	sizeof(MPID_Request), *mpi_errno_p, "array of corresponding MPICH2 requests");

    MPIU_CHKPMEM_MALLOC(mpig_cm_vmpi_progress_status_array, mpig_vmpi_status_t *, MPIG_CM_VMPI_REQUEST_TABLE_BLOCK_ALLOC_SIZE *
	sizeof(mpig_vmpi_status_t), *mpi_errno_p, "array of vendor MPI status structures for completed requests");

    MPIU_CHKPMEM_MALLOC(mpig_cm_vmpi_progress_free_req_stack, int *, MPIG_CM_VMPI_REQUEST_TABLE_BLOCK_ALLOC_SIZE *
	sizeof(int), *mpi_errno_p, "array containing stack of free entries in request and status arrays");

    for (index = mpig_cm_vmpi_array_size; index < mpig_cm_vmpi_array_size + MPIG_CM_VMPI_REQUEST_TABLE_BLOCK_ALLOC_SIZE; index++)
    {
	mpig_cm_vmpi_progress_vreq_array[index] = MPIG_VMPI_REQUEST_NULL;
	mpig_cm_vmpi_progress_mreq_array[index] = NULL;
	mpig_cm_vmpi_progress_free_entry_stack[index - mpig_cm_vmpi_array_size] = mpig_cm_vmpi_array_size +
	    MPIG_CM_VMPI_REQUEST_TABLE_BLOCK_ALLOC_SIZE - index - 1;
    }
    
    mpig_cm_vmpi_array_size += MPIG_CM_VMPI_REQUEST_TABLE_BLOCK_ALLOC_SIZE;
    mpig_cm_vmpi_progress_free_entry_stack_top = MPIG_CM_VMPI_REQUEST_TABLE_BLOCK_ALLOC_SIZE - 1;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS,
    "exiting: mpi_errno=0x%08x, failed=%s", *mpi_errno_p, MPIG_BOOL_STR(*failed_p)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_progress_init);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_progress_init() */


/*
 * int mpig_cm_vmpi_progress_wait([IN/OUT] state, [IN/OUT] mpi_errno, [OUT] failed)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_progress_wait
void mpig_cm_vmpi_progress_wait(struct MPID_Progress_state * state, int * mpi_errno_p, bool_t * failed_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_progress_wait);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_progress_wait);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS, "entering"));

    *failed_p = FALSE;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS,
    "exiting: mpi_errno=0x%08x, failed=%s", *mpi_errno_p, MPIG_BOOL_STR(*failed_p));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_progress_wait);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_progress_wait() */

/*
 * int mpig_cm_vmpi_progress_test([IN/OUT] mpi_errno, [OUT] failed)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_progress_wait
void mpig_cm_vmpi_progress_test(int * mpi_errno_p, bool_t * failed_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_progress_wait);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_progress_wait);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS, "entering"));

    *failed_p = FALSE;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS,
    "exiting: mpi_errno=0x%08x, failed=%s", *mpi_errno_p, MPIG_BOOL_STR(*failed_p));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_progress_wait);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_progress_test() */
/**********************************************************************************************************************************
				     END COMMUNICATION MODULE PROGRESS ENGINE API FUNCTIONS
**********************************************************************************************************************************/

/**********************************************************************************************************************************
						BEGIN VC FUNCTION TABLE ROUTINES
**********************************************************************************************************************************/
/*
 * int mpig_cm_vmpi_adi3_send(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_adi3_send
MPIG_STATIC int mpig_cm_vmpi_adi3_send(
    const void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const sreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int ctx = comm->context_id + ctxoff;
    mpig_vmpi_datatype_t * vdt;
    int vrank;
    int vtag;
    mpig_vmpi_comm_t * vcomm;
    int vrc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_adi3_send);

    MPIG_UNUSED_VAR(fcname);
    MPIG_UNUSED_VAR(ctx);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_adi3_send);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "entering: buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT
		       ", ctx=%d", (MPIG_PTR_CAST) buf, cnt, dt, rank, tag, (MPIG_PTR_CAST) comm, ctx));

    mpig_cm_vmpi_datatype_get_vdt(dt, &vdt);
    mpig_cm_vmpi_comm_get_vrank(comm, rank, &vrank);
    mpig_cm_vmpi_comm_get_vtag(comm, tag, &vtag);
    mpig_cm_vmpi_comm_get_vcomm(comm, ctxoff, &vcomm);

    /* NOTE: we can only call the blocking send when no other communication module requires polling to make progress, or no
       other requests have outstanding requests. */
    if (mpig_progress_num_cm_requiring_polling == 1 || mpig_progress_ops_outstanding > mpig_cm_vmpi_ops_outstanding)
    {
	vrc = mpig_vmpi_send(buf, cnt, vdt, vrank, tag, vcomm);
	MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Send", &mpi_errno);
	*sreqp = NULL;
    }
    else
    {
	MPID_Request * sreq;
	mpig_vmpi_request_t * vsreq;

	/* create a new send request */
	mpig_request_create_isreq(MPIG_REQUEST_TYPE_SEND, 2, 1, buf, cnt, dt, rank, tag, ctx, comm, vc, &sreq);
	mpig_cm_vmpi_request_construct(sreq);

	mpig_cm_vmpi_request_get_vreq(sreq, &vsreq);
	vrc = mpig_vmpi_isend(buf, cnt, vdt, vrank, vtag, vcomm, vsreq);
	MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Irecv", &mpi_errno);

	mpig_cm_vmpi_progress_op_start();
	*sreqp = sreq;
    }

    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "exiting: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT ", mpi_errno=0x%08x",
		       MPIG_HANDLE_VAL(*sreqp), (MPIG_PTR_CAST) *sreqp, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_adi3_send);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_adi3_send(...) */


/*
 * int mpig_cm_vmpi_adi3_isend(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_adi3_isend
MPIG_STATIC int mpig_cm_vmpi_adi3_isend(
    const void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const sreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_adi3_isend);

    MPIG_UNUSED_VAR(fcname);
    MPIG_UNUSED_VAR(ctx);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_adi3_isend);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "entering: buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT
		       ", ctx=%d", (MPIG_PTR_CAST) buf, cnt, dt, rank, tag, (MPIG_PTR_CAST) comm, ctx));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", fcname);

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "exiting: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT ", mpi_errno=0x%08x",
		       MPIG_HANDLE_VAL(*sreqp), (MPIG_PTR_CAST) *sreqp, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_adi3_isend);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_adi3_isend() */


/*
 * int mpig_cm_vmpi_adi3_rsend(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_adi3_rsend
MPIG_STATIC int mpig_cm_vmpi_adi3_rsend(
    const void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const sreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_adi3_rsend);

    MPIG_UNUSED_VAR(fcname);
    MPIG_UNUSED_VAR(ctx);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_adi3_rsend);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "entering: buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT
		       ", ctx=%d", (MPIG_PTR_CAST) buf, cnt, dt, rank, tag, (MPIG_PTR_CAST) comm, ctx));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", fcname);

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "exiting: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT ", mpi_errno=0x%08x",
		       MPIG_HANDLE_VAL(*sreqp), (MPIG_PTR_CAST) *sreqp, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_adi3_rsend);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_adi3_rsend() */


/*
 * int mpig_cm_vmpi_adi3_irsend(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_adi3_irsend
MPIG_STATIC int mpig_cm_vmpi_adi3_irsend(
    const void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const sreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_adi3_irsend);

    MPIG_UNUSED_VAR(fcname);
    MPIG_UNUSED_VAR(ctx);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_adi3_irsend);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "entering: buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT
		       ", ctx=%d", (MPIG_PTR_CAST) buf, cnt, dt, rank, tag, (MPIG_PTR_CAST) comm, ctx));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", fcname);

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "exiting: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT ", mpi_errno=0x%08x",
		       MPIG_HANDLE_VAL(*sreqp), (MPIG_PTR_CAST) *sreqp, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_adi3_irsend);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_adi3_irsend() */


/*
 * int mpig_cm_vmpi_adi3_ssend(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_adi3_ssend
MPIG_STATIC int mpig_cm_vmpi_adi3_ssend(
    const void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const sreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_adi3_ssend);

    MPIG_UNUSED_VAR(fcname);
    MPIG_UNUSED_VAR(ctx);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_adi3_ssend);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "entering: buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT
		       ", ctx=%d", (MPIG_PTR_CAST) buf, cnt, dt, rank, tag, (MPIG_PTR_CAST) comm, ctx));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", fcname);

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "exiting: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT ", mpi_errno=0x%08x",
		       MPIG_HANDLE_VAL(*sreqp), (MPIG_PTR_CAST) *sreqp, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_adi3_ssend);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_adi3_ssend() */


/*
 * int mpig_cm_vmpi_adi3_issend(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_adi3_issend
MPIG_STATIC int mpig_cm_vmpi_adi3_issend(
    const void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const sreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_adi3_issend);

    MPIG_UNUSED_VAR(fcname);
    MPIG_UNUSED_VAR(ctx);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_adi3_issend);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "entering: buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT
		       ", ctx=%d", (MPIG_PTR_CAST) buf, cnt, dt, rank, tag, (MPIG_PTR_CAST) comm, ctx));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", fcname);

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "exiting: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT ", mpi_errno=0x%08x",
		       MPIG_HANDLE_VAL(*sreqp), (MPIG_PTR_CAST) *sreqp, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_adi3_issend);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_adi3_issend() */


/*
 * int mpig_cm_vmpi_adi3_recv(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_adi3_recv
MPIG_STATIC int mpig_cm_vmpi_adi3_recv(
    void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPI_Status * const status, MPID_Request ** const rreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int ctx = comm->context_id + ctxoff;
    mpig_vmpi_status_t * const vstatus = (mpig_vmpi_status_t *) status->mpig_vmpi_status;
    int vrank;
    int vtag;
    mpig_vmpi_comm_t * vcomm;
    mpig_vmpi_datatype_t * vdt;
    mpig_vmpi_request_t * vrreq;
    int vrc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_adi3_recv);

    MPIG_UNUSED_VAR(fcname);
    MPIG_UNUSED_VAR(ctx);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_adi3_recv);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "entering: buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT
		       ", ctx=%d", (MPIG_PTR_CAST) buf, cnt, dt, rank, tag, (MPIG_PTR_CAST) comm, comm->context_id + ctx));

    mpig_cm_vmpi_comm_get_vrank(comm, rank, &vrank);
    mpig_cm_vmpi_comm_get_vtag(tag, &vtag);
    mpig_cm_vmpi_comm_get_vcomm(comm, ctxoff, &vcomm);
    mpig_cm_vmpi_datatype_get_vdt(dt, &vdt);
    
    vrc = mpig_vmpi_recv(buf, cnt, vdt, vrank, tag, vcomm, vstatus);
    MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Rrecv", &mpi_errno);

    if (status != MPI_STATUS_IGNORE)
    {
	status->cancelled = FALSE;
	vrc = mpig_vmpi_get_count(vstatus, MPIG_VMPI_BYTE, &status->count);
	MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Get_count", &mpi_errno);
	mpig_cm_vmpi_comm_get_mrank(comm, mpig_vmpi_status_get_source(vstatus), &status->MPI_SOURCE);
	status->MPI_TAG = mpig_vmpi_status_get_tag(vstatus);
	status->MPI_ERROR = mpi_errno;
    }
    
    *rreqp = NULL;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "exiting: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT ", mpi_errno=0x%08x",
		       MPIG_HANDLE_VAL(*rreqp), (MPIG_PTR_CAST) *rreqp, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_adi3_recv);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* int mpig_cm_vmpi_adi3_recv() */


/*
 * int mpig_cm_vmpi_adi3_irecv(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_adi3_irecv
MPIG_STATIC int mpig_cm_vmpi_adi3_irecv(
    void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const rreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int ctx = comm->context_id + ctxoff;
    int vrank;
    int vtag;
    mpig_vmpi_comm_t * vcomm;
    mpig_vmpi_datatype_t * vdt;
    mpig_vmpi_request_t * vrreq;
    mpig_vc_t * vc;
    MPID_Request * rreq = NULL;
    int vrc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_adi3_irecv);

    MPIG_UNUSED_VAR(fcname);
    MPIG_UNUSED_VAR(ctx);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_adi3_irecv);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "entering: buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT
		       ", ctx=%d", (MPIG_PTR_CAST) buf, cnt, dt, rank, tag, (MPIG_PTR_CAST) comm, ctx));

    mpig_cm_vmpi_comm_get_vrank(comm, rank, &vrank);
    mpig_cm_vmpi_comm_get_vtag(tag, &vtag);
    mpig_cm_vmpi_comm_get_vcomm(comm, ctxoff, &vcomm);
    mpig_cm_vmpi_datatype_get_vdt(dt, &vdt);
    
    mpig_comm_get_vc(comm, rank, &vc);
    mpig_request_create_irreq(2, 1, buf, cnt, dt, rank, tag, ctx, comm, vc, &rreq);
    mpig_cm_vmpi_request_construct(rreq);
	
    mpig_cm_vmpi_request_get_vreq(rreq, &vrreq);
    vrc = mpig_vmpi_irecv(buf, cnt, vdt, vrank, tag, vcomm, vrreq);
    MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Irecv", &mpi_errno);
    
    /* mpi_errno = mpig_cm_vmpi_progress_add_req(rreq); */
    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_vmpi|progress_add_req",
	"**globus|cm_vmpi|progress_add_req %R %p", rreq->handle, rreq);
    
    *rreqp = rreq;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "exiting: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT ", mpi_errno=0x%08x",
		       MPIG_HANDLE_VAL(*rreqp), (MPIG_PTR_CAST) *rreqp, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_adi3_irecv);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	if (rreq != NULL)
	{
	    mpig_request_set_cc(rreq, 0);
	    mpig_request_set_ref_count(rreq, 0);
	    mpig_request_destroy(rreq);
	}
	
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_adi3_irecv() */


/*
 * int mpig_cm_vmpi_adi3_cancel_send([IN/MOD] sreq)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_adi3_cancel_send
MPIG_STATIC int mpig_cm_vmpi_adi3_cancel_send(MPID_Request * const sreq)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_adi3_cancel_send);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_adi3_cancel_send);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "entering: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT, sreq->handle, (MPIG_PTR_CAST) sreq));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", fcname);
    
    fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "exiting: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT "mpi_errno=0x%08x",
		       sreq->handle, (MPIG_PTR_CAST) sreq, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_adi3_cancel_send);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_adi3_cancel_send() */


/*
 * int mpig_cm_vmpi_adi3_cancel_recv([IN/MOD] sreq)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_adi3_cancel_recv
MPIG_STATIC int mpig_cm_vmpi_adi3_cancel_recv(MPID_Request * const sreq)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_adi3_cancel_recv);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_adi3_cancel_recv);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "entering: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT, sreq->handle, (MPIG_PTR_CAST) sreq));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", fcname);
    
    fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "exiting: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT "mpi_errno=0x%08x",
		       sreq->handle, (MPIG_PTR_CAST) sreq, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_adi3_cancel_recv);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_adi3_cancel_recv() */
/**********************************************************************************************************************************
						 END VC FUNCTION TABLE ROUTINES
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					       BEGIN COMMUNICATOR MANAGEMENT HOOKS
**********************************************************************************************************************************/
/*
 * void mpig_cm_vmpi_dev_comm_dup_hook([IN] orig_comm, [IN] new_comm, [OUT] mpi_errno)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_dev_comm_dup_hook
void mpig_cm_vmpi_dev_comm_dup_hook(MPID_Comm * const orig_comm, MPID_Comm * const new_comm, int * const mpi_errno_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_dev_comm_dup_hook);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_dev_comm_dup_hook);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM,
	"entering: orig_comm=" MPIG_HANDLE_FMT ", orig_commp=" MPIG_PTR_FMT ", new_comm=" MPIG_HANDLE_FMT
	", new_commp=" MPIG_PTR_FMT, orig_comm->handle, (MPIG_PTR_CAST) orig_comm, new_comm->handle, (MPIG_PTR_CAST) new_comm));

    /* NOTE: because the current implementation of MPICH2 intercommunicators use a secondary intracommunicator (local_comm), two
       additional steps are necessary.  first, MPIR_Setup_intercomm_localcomm(new_comm) must be called to create the secondary
       communicator object.  second, the communicators in orig_comm->local_comm->cm.vmpi.comms[] must duplicated. */
    MPIU_ERR_SETFATALANDSTMT1(*mpi_errno_p, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", fcname);

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "exiting: mpi_errno=0x%08x", *mpi_errno_p));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_dev_comm_dup_hook);
    return;
    
  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_dev_comm_dup_hook() */


/*
 * void mpig_cm_vmpi_dev_intercomm_create_hook([IN] orig_comm, [IN] new_comm, [OUT] mpi_errno)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_dev_intercomm_create_hook
void mpig_cm_vmpi_dev_intercomm_create_hook(MPID_Comm * const local_comm, const int local_leader, MPID_Comm * const peer_comm,
    const int remote_leader, const int tag, MPID_Comm * const new_intercomm, int * const mpi_errno_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_dev_intercomm_create_hook);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_dev_intercomm_create_hook);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM,
	"entering: local_comm=" MPIG_HANDLE_FMT ", local_commp=" MPIG_PTR_FMT ", local_leader=%d, peer_comm=" MPIG_HANDLE_FMT
	", peer_commp=" MPIG_PTR_FMT ", remote_leader=%d, tag=%d, new_intercomm=" MPIG_HANDLE_FMT ", new_intercommp=" MPIG_PTR_FMT,
	local_comm->handle, (MPIG_PTR_CAST) local_comm, local_leader, MPIG_HANDLE_VAL(peer_comm), (MPIG_PTR_CAST) peer_comm,
	remote_leader, tag, new_intercomm->handle, (MPIG_PTR_CAST) new_intercomm));

    /* NOTE: because the current implementation of MPICH2 intercommunicators use a secondary intracommunicator (local_comm), two
       additional steps are necessary.  first, MPIR_Setup_intercomm_localcomm(new_intercomm) must be called to create the
       secondary communicator object.  second, new_inter_comm->local_comm->cm.vmpi.comms[] must set with duplicated instances of
       local_comm. */
    MPIU_ERR_SETFATALANDSTMT1(*mpi_errno_p, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", fcname);

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "exiting: mpi_errno=0x%08x", *mpi_errno_p));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_dev_intercomm_create_hook);
    return;
    
  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_dev_intercomm_create_hook() */

/*
 * void mpig_cm_vmpi_dev_comm_free_hook([IN] orig_comm, [IN] new_comm, [OUT] mpi_errno)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_dev_comm_free_hook
void mpig_cm_vmpi_dev_comm_free_hook(MPID_Comm * comm, int * mpi_errno_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int context;
    mpig_vmpi_comm_t * vcomm;
    int vrc;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_dev_comm_free_hook);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_dev_comm_free_hook);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM,
	"entering: comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT, comm->handle, (MPIG_PTR_CAST) comm));

    *mpi_errno_p = MPI_SUCCESS;

    context = (comm->comm_kind == MPID_INTRACOMM) ? MPID_CONTEXT_INTRA_PT2PT : MPID_CONTEXT_INTER_PT2PT;
    mpig_cm_vmpi_comm_get_vcomm(comm, context, &vcomm);
    vrc = mpig_vmpi_comm_free(vcomm);
    MPIG_ERR_VMPI_CHKANDSTMT(vrc, "MPI_Comm_free", {;}, mpi_errno_p);

    context = (comm->comm_kind == MPID_INTRACOMM) ? MPID_CONTEXT_INTRA_COLL : MPID_CONTEXT_INTER_COLL;
    mpig_cm_vmpi_comm_get_vcomm(comm, context, &vcomm);
    vrc = mpig_vmpi_comm_free(vcomm);
    MPIG_ERR_VMPI_CHKANDSTMT(vrc, "MPI_Comm_free", {;}, mpi_errno_p);
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "exiting: mpi_errno=0x%08x", *mpi_errno_p));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_dev_comm_free_hook);
    return;
}
/* mpig_cm_vmpi_dev_comm_free_hook() */
/**********************************************************************************************************************************
						END COMMUNICATOR MANAGEMENT HOOKS
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					     BEGIN MISCELLANEOUS PUBLICLY VISIBLE ROUTINES
**********************************************************************************************************************************/
/*
 * int mpig_cm_vmpi_adi3_iprobe(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_adi3_iprobe
int mpig_cm_vmpi_iprobe(int src, int tag, MPID_Comm * comm, int ctxoff, int * flag, MPID_Status * status)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int ctx = comm->context_id + ctxoff;
    int vsrc;
    int vtag;
    mpig_vmpi_comm_t * vcomm;
    int vrc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_adi3_iprobe);

    MPIG_UNUSED_VAR(fcname);
    MPIG_UNUSED_VAR(ctx);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_adi3_iprobe);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "entering: src=%d, tag=%d, comm=" MPIG_PTR_FMT ", ctx=%d", src, tag, (MPIG_PTR_CAST) comm, ctx));

    mpig_cm_vmpi_comm_get_vrank(comm, src, &vsrc);
    mpig_cm_vmpi_comm_get_vtag(tag, &vtag);
    mpig_cm_vmpi_comm_get_vcomm(comm, ctxoff, &vcomm);

    vrc = mpig_vmpi_iprobe(vrank, tag, vcomm, flag, vstatus);
    MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Iprobe", &mpi_errno);

    if (flag && status != MPIG_VMPI_STATUS_IGNORE)
    {
	status->cancelled = FALSE;
	vrc = mpig_vmpi_get_count(vstatus, MPIG_VMPI_BYTE, &status->count);
	MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Get_count", &mpi_errno);
	mpig_cm_vmpi_comm_get_mrank(comm, mpig_vmpi_status_get_source(vstatus), &status->MPI_SOURCE);
	status->MPI_TAG = mpig_vmpi_status_get_tag(vstatus);
	status->MPI_ERROR = mpi_errno;
    }
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "exiting: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT ", mpi_errno=0x%08x",
		       MPIG_HANDLE_VAL(*sreqp), (MPIG_PTR_CAST) *sreqp, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_adi3_iprobe);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_adi3_iprobe(...) */


/*
 * int mpig_cm_vmpi_error_class_vtom([IN] vendor_class)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_error_class_vtom
int mpig_cm_vmpi_error_class_vtom(int vendor_class)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpich_class;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_error_class_vtom);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_error_class_vtom);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "entering: vendor_class=%d", vendor_class));

    /* FIXME: convert this to a hash table? */
    if (vendor_class == MPIG_VMPI_SUCCESS)
    {
	mpich_class=MPI_SUCCESS;
    }
    else if (vendor_class == MPIG_VMPI_ERR_BUFFER)
    {
	mpich_class=MPI_ERR_BUFFER;
    }
    else if (vendor_class == MPIG_VMPI_ERR_COUNT)
    {
	mpich_class = MPI_ERR_COUNT;
    }
    else if (vendor_class == MPIG_VMPI_ERR_TYPE)
    {
	mpich_class = MPI_ERR_TYPE;
    }
    else if (vendor_class == MPIG_VMPI_ERR_TAG)
    {
	mpich_class = MPI_ERR_TAG;
    }
    else if (vendor_class == MPIG_VMPI_ERR_COMM)
    {
	mpich_class = MPI_ERR_COMM;
    }
    else if (vendor_class == MPIG_VMPI_ERR_RANK)
    {
	mpich_class = MPI_ERR_RANK;
    }
    else if (vendor_class == MPIG_VMPI_ERR_ROOT)
    {
	mpich_class = MPI_ERR_ROOT;
    }
    else if (vendor_class == MPIG_VMPI_ERR_TRUNCATE)
    {
	mpich_class = MPI_ERR_TRUNCATE;
    }
    else if (vendor_class == MPIG_VMPI_ERR_GROUP)
    {
	mpich_class = MPI_ERR_GROUP;
    }
    else if (vendor_class == MPIG_VMPI_ERR_OP)
    {
	mpich_class = MPI_ERR_OP;
    }
    else if (vendor_class == MPIG_VMPI_ERR_REQUEST)
    {
	mpich_class = MPI_ERR_REQUEST;
    }
    else if (vendor_class == MPIG_VMPI_ERR_TOPOLOGY)
    {
	mpich_class = MPI_ERR_TOPOLOGY;
    }
    else if (vendor_class == MPIG_VMPI_ERR_DIMS)
    {
	mpich_class = MPI_ERR_DIMS;
    }
    else if (vendor_class == MPIG_VMPI_ERR_ARG)
    {
	mpich_class = MPI_ERR_ARG;
    }
    else if (vendor_class == MPIG_VMPI_ERR_OTHER)
    {
	mpich_class = MPI_ERR_OTHER;
    }
    else if (vendor_class == MPIG_VMPI_ERR_UNKNOWN)
    {
	mpich_class = MPI_ERR_UNKNOWN;
    }
    else if (vendor_class == MPIG_VMPI_ERR_INTERN)
    {
	mpich_class = MPI_ERR_INTERN;
    }
    else if (vendor_class == MPIG_VMPI_ERR_IN_STATUS)
    {
	mpich_class = MPI_ERR_IN_STATUS;
    }
    else if (vendor_class == MPIG_VMPI_ERR_PENDING)
    {
	mpich_class = MPI_ERR_PENDING;
    }
    else if (vendor_class == MPIG_VMPI_ERR_FILE)
    {
	mpich_class = MPI_ERR_FILE;
    }
    else if (vendor_class == MPIG_VMPI_ERR_ACCESS)
    {
	mpich_class = MPI_ERR_ACCESS;
    }
    else if (vendor_class == MPIG_VMPI_ERR_AMODE)
    {
	mpich_class = MPI_ERR_AMODE;
    }
    else if (vendor_class == MPIG_VMPI_ERR_BAD_FILE)
    {
	mpich_class = MPI_ERR_BAD_FILE;
    }
    else if (vendor_class == MPIG_VMPI_ERR_FILE_EXISTS)
    {
	mpich_class = MPI_ERR_FILE_EXISTS;
    }
    else if (vendor_class == MPIG_VMPI_ERR_FILE_IN_USE)
    {
	mpich_class = MPI_ERR_FILE_IN_USE;
    }
    else if (vendor_class == MPIG_VMPI_ERR_NO_SPACE)
    {
	mpich_class = MPI_ERR_NO_SPACE;
    }
    else if (vendor_class == MPIG_VMPI_ERR_NO_SUCH_FILE)
    {
	mpich_class = MPI_ERR_NO_SUCH_FILE;
    }
    else if (vendor_class == MPIG_VMPI_ERR_IO)
    {
	mpich_class = MPI_ERR_IO;
    }
    else if (vendor_class == MPIG_VMPI_ERR_READ_ONLY)
    {
	mpich_class = MPI_ERR_READ_ONLY;
    }
    else if (vendor_class == MPIG_VMPI_ERR_CONVERSION)
    {
	mpich_class = MPI_ERR_CONVERSION;
    }
    else if (vendor_class == MPIG_VMPI_ERR_DUP_DATAREP)
    {
	mpich_class = MPI_ERR_DUP_DATAREP;
    }
    else if (vendor_class == MPIG_VMPI_ERR_UNSUPPORTED_DATAREP)
    {
	mpich_class = MPI_ERR_UNSUPPORTED_DATAREP;
    }
    else if (vendor_class == MPIG_VMPI_ERR_INFO)
    {
	mpich_class = MPI_ERR_INFO;
    }
    else if (vendor_class == MPIG_VMPI_ERR_INFO_KEY)
    {
	mpich_class = MPI_ERR_INFO_KEY;
    }
    else if (vendor_class == MPIG_VMPI_ERR_INFO_VALUE)
    {
	mpich_class = MPI_ERR_INFO_VALUE;
    }
    else if (vendor_class == MPIG_VMPI_ERR_INFO_NOKEY)
    {
	mpich_class = MPI_ERR_INFO_NOKEY;
    }
    else if (vendor_class == MPIG_VMPI_ERR_NAME)
    {
	mpich_class = MPI_ERR_NAME;
    }
    else if (vendor_class == MPIG_VMPI_ERR_NO_MEM)
    {
	mpich_class = MPI_ERR_NO_MEM;
    }
    else if (vendor_class == MPIG_VMPI_ERR_NOT_SAME)
    {
	mpich_class = MPI_ERR_NOT_SAME;
    }
    else if (vendor_class == MPIG_VMPI_ERR_PORT)
    {
	mpich_class = MPI_ERR_PORT;
    }
    else if (vendor_class == MPIG_VMPI_ERR_QUOTA)
    {
	mpich_class = MPI_ERR_QUOTA;
    }
    else if (vendor_class == MPIG_VMPI_ERR_SERVICE)
    {
	mpich_class = MPI_ERR_SERVICE;
    }
    else if (vendor_class == MPIG_VMPI_ERR_SPAWN)
    {
	mpich_class = MPI_ERR_SPAWN;
    }
    else if (vendor_class == MPIG_VMPI_ERR_UNSUPPORTED_OPERATION)
    {
	mpich_class = MPI_ERR_UNSUPPORTED_OPERATION;
    }
    else if (vendor_class == MPIG_VMPI_ERR_WIN)
    {
	mpich_class = MPI_ERR_WIN;
    }
    else if (vendor_class == MPIG_VMPI_ERR_BASE)
    {
	mpich_class = MPI_ERR_BASE;
    }
    else if (vendor_class == MPIG_VMPI_ERR_LOCKTYPE)
    {
	mpich_class = MPI_ERR_LOCKTYPE;
    }
    else if (vendor_class == MPIG_VMPI_ERR_KEYVAL)
    {
	mpich_class = MPI_ERR_KEYVAL;
    }
    else if (vendor_class == MPIG_VMPI_ERR_RMA_CONFLICT)
    {
	mpich_class = MPI_ERR_RMA_CONFLICT;
    }
    else if (vendor_class == MPIG_VMPI_ERR_RMA_SYNC)
    {
	mpich_class = MPI_ERR_RMA_SYNC;
    }
    else if (vendor_class == MPIG_VMPI_ERR_SIZE)
    {
	mpich_class = MPI_ERR_SIZE;
    }
    else if (vendor_class == MPIG_VMPI_ERR_DISP)
    {
	mpich_class = MPI_ERR_DISP;
    }
    else if (vendor_class == MPIG_VMPI_ERR_ASSERT)
    {
	mpich_class = MPI_ERR_ASSERT;
    }
    else
    {
	mpich_class = MPI_ERR_UNKNOWN;
    }
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "exiting: mpich_class=%d", mpich_class));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_error_class_vtom);
    return mpich_class;
}
/* mpig_cm_vmpi_error_class_vtom() */
/**********************************************************************************************************************************
					   END MISCELLANEOUS PUBLICLY VISIBLE ROUTINES
**********************************************************************************************************************************/


#endif
