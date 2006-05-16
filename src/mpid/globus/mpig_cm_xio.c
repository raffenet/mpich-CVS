/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"
#include "globus_dc.h"
#include "globus_xio_tcp_driver.h"


#define FCNAME fcname


/**********************************************************************************************************************************
							BEGIN PARAMETERS
**********************************************************************************************************************************/
/*
 * user tunable parameters
 */
#if !defined(MPIG_CM_XIO_EAGER_MAX_SIZE)
#define MPIG_CM_XIO_EAGER_MAX_SIZE (256*1024+3)
#endif

#if !defined(MPIG_CM_XIO_DATA_DENSITY_THRESHOLD)
#define MPIG_CM_XIO_DATA_DENSITY_THRESHOLD (8*1024)
#endif

#if !defined(MPIG_CM_XIO_DATA_SPARSE_BUFFER_SIZE)
#define MPIG_CM_XIO_DATA_SPARSE_BUFFER_SIZE (256*1024-1)
#endif

#if !defined(MPIG_CM_XIO_DATA_TRUNCATION_BUFFER_SIZE)
#define MPIG_CM_XIO_DATA_TRUNCATION_BUFFER_SIZE (15)
#endif

#if !defined(MPIG_CM_XIO_MAX_ACCEPT_ERRORS)
#define MPIG_CM_XIO_MAX_ACCEPT_ERRORS 255
#endif

/*
 *internal parameters
 */
#define MPIG_CM_XIO_PROTO_VERSION 1
#define MPIG_CM_XIO_PROTO_CONNECT_MAGIC "MPIG-CM-XIO-SHAKE-MAGIC-8-BALL\n"
#define MPIG_CM_XIO_PROTO_ACCEPT_MAGIC "MPIG-CM-XIO-MAGIC-8-BALL-SAYS-YES\n"
/**********************************************************************************************************************************
							 END PARAMETERS
**********************************************************************************************************************************/


/**********************************************************************************************************************************
				      BEGIN MISCELLANEOUS MACROS, PROTOTYPES, AND VARIABLES
**********************************************************************************************************************************/
MPIG_STATIC globus_mutex_t mpig_cm_xio_mutex;

MPIG_STATIC const char * mpig_cm_xio_msg_type_get_string(mpig_cm_xio_msg_types_t msg_type);

#define mpig_cm_xio_mutex_create()	globus_mutex_init(&mpig_cm_xio_mutex, NULL)
#define mpig_cm_xio_mutex_destroy()	globus_mutex_destroy(&mpig_cm_xio_mutex)
#define mpig_cm_xio_mutex_lock()					\
{									\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS,			\
		       "cm_xio global mutex - acquiring mutex"));	\
    globus_mutex_lock(&mpig_cm_xio_mutex);				\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS,			\
		       "cm_xio local data - mutex acquired"));		\
}
#define mpig_cm_xio_mutex_unlock()					\
{									\
    globus_mutex_unlock(&mpig_cm_xio_mutex);				\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS,			\
		       "cm_xio global mutex - mutex released"));	\
}
#define mpig_cm_xio_mutex_lock_conditional(cond_)	{if (cond_) mpig_cm_xio_mutex_lock();}
#define mpig_cm_xio_mutex_unlock_conditional(cond_)	{if (cond_) mpig_cm_xio_mutex_unlock();}
/**********************************************************************************************************************************
				       END MISCELLANEOUS MACROS, PROTOTYPES, AND VARIABLES
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					BEGIN INCLUSION OF INTERNAL MACROS AND PROTOTYPES
**********************************************************************************************************************************/
#undef MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS
#include "mpig_cm_xio_req.i"
#include "mpig_cm_xio_vc.i"
#include "mpig_cm_xio_conn.i"
#include "mpig_cm_xio_data.i"
#include "mpig_cm_xio_comm.i"
/**********************************************************************************************************************************
					 END INCLUSION OF INTERNAL MACROS AND PROTOTYPES
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					  BEGIN COMMUNICATION MODULE CORE API FUNCTIONS
**********************************************************************************************************************************/
/*
 * int mpig_cm_xio_init([IN/OUT] argc, [IN/OUT] argv)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_init
int mpig_cm_xio_init(int * argc, char *** argv)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    globus_result_t grc;
    bool_t failed;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_init);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_init);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering"));

    MPIU_Assert(sizeof(mpig_cm_xio_msghan_funcs) != MPIG_CM_XIO_MSG_TYPE_LAST + 1);

    mpig_cm_xio_mutex_create();

    /* initialize the request completion queue */
    mpig_cm_xio_rcq_init(&mpi_errno, &failed);
    MPIU_ERR_CHKANDJUMP((failed), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|rcq_init")

    /* activate globus XIO module */
    grc = globus_module_activate(GLOBUS_XIO_MODULE);
    MPIU_ERR_CHKANDJUMP1((grc), mpi_errno, MPI_ERR_OTHER, "**globus|module_activate", "**globus|module_activate %s", "XIO");

    /* initialize the connection management subsystem */
    mpig_cm_xio_conn_init(&mpi_errno, &failed);
    MPIU_ERR_CHKANDJUMP((failed), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|conn_init");
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno=0x%08x", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_init);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_init() */


/*
 * int mpig_cm_xio_finalize(void)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_finalize
int mpig_cm_xio_finalize(void)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    globus_result_t grc;
    bool_t failed;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_finalize);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_finalize);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering"));

    /* shutdown the connection management subsystem */
    mpig_cm_xio_conn_finalize(&mpi_errno, &failed);
    MPIU_ERR_CHKANDSTMT((failed), mpi_errno, MPI_ERR_OTHER, {;}, "**globus|cm_xio|conn_finalize");
    
    /* deactivate the globus XIO module */
    grc = globus_module_deactivate(GLOBUS_XIO_MODULE);
    MPIU_ERR_CHKANDSTMT2((grc), mpi_errno, MPI_ERR_OTHER, {;}, "**globus|module_deactivate",
			 "**globus|module_deactivate %s %s", "XIO", globus_error_print_chain(globus_error_peek(grc)));

    /* shutdown the request completion queue */
    mpig_cm_xio_rcq_finalize(&mpi_errno, &failed);
    MPIU_ERR_CHKANDSTMT((failed), mpi_errno, MPI_ERR_OTHER, {;}, "**globus|cm_xio|rcq_finalize");
    
    mpig_cm_xio_mutex_destroy();
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno=0x%08x", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_finalize);
    return mpi_errno;
}
/* mpig_cm_xio_finalize() */


/*
 * mpig_cm_xio_add_contact_info([IN/OUT] business card)
 *
 * Add any and all contact information for this communication module to the supplied business card.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_add_contact_info
int mpig_cm_xio_add_contact_info(mpig_bc_t * bc)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    char uint_str[10];
    bool_t failed;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_add_contact_info);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_add_contact_info);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering"));
    
    MPIU_Snprintf(uint_str, (size_t) 10, "%u", (unsigned) MPIG_CM_XIO_PROTO_VERSION);
    mpig_bc_add_contact(bc, "CM_XIO_PROTO_VERSION", uint_str, &mpi_errno, &failed);
    MPIU_ERR_CHKANDJUMP1((failed), mpi_errno, MPI_ERR_OTHER, "**globus|bc_add_contact",
			 "**globus|bc_add_contact %s", "CM_XIO_PROTO_VERSION");

    mpig_bc_add_contact(bc, "CM_XIO_CONTACT_STRING", mpig_cm_xio_server_cs, &mpi_errno, &failed);
    MPIU_ERR_CHKANDJUMP1((failed), mpi_errno, MPI_ERR_OTHER, "**globus|bc_add_contact",
			 "**globus|bc_add_contact %s", "CM_XIO_CONTACT_STRING");

    MPIU_Snprintf(uint_str, (size_t) 10, "%u", (unsigned) GLOBUS_DC_FORMAT_LOCAL);
    mpig_bc_add_contact(bc, "CM_XIO_DC_FORMAT", uint_str, &mpi_errno, &failed);
    MPIU_ERR_CHKANDJUMP1((failed), mpi_errno, MPI_ERR_OTHER, "**globus|bc_add_contact",
			 "**globus|bc_add_contact %s", "CM_XIO_DC_FORMAT");

    if (MPIG_MY_ENDIAN == MPIG_ENDIAN_LITTLE)
    {
	mpig_bc_add_contact(bc, "CM_XIO_DC_ENDIAN", "little", &mpi_errno, &failed);
	MPIU_ERR_CHKANDJUMP1((failed), mpi_errno, MPI_ERR_OTHER, "**globus|bc_add_contact",
			     "**globus|bc_add_contact %s", "CM_XIO_DC_ENDIAN");
    }
    else
    {
	mpig_bc_add_contact(bc, "CM_XIO_DC_ENDIAN", "big", &mpi_errno, &failed);
	MPIU_ERR_CHKANDJUMP1((failed), mpi_errno, MPI_ERR_OTHER, "**globus|bc_add_contact",
			     "**globus|bc_add_contact %s", "CM_XIO_DC_ENDIAN");
    }

  fn_return:
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_add_contact_info);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting (mpi_errno=%d)", mpi_errno));
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_add_contact_info() */


/*
 * int mpig_cm_xio_select_module([IN] bc, [IN/OUT] vc, [OUT] selected)
 *
 * Check the business card to see if the connection module can communicate with the remote process associated with the supplied
 * VC.  If it can, then the VC will be initialized accordingly.
 *
 * Parameters:
 *
 * bc [IN] - business card containing contact information
 * vc [IN] - vc object to initialize if the communication module is capable of performing communication with the associated process
 * selected [OUT] - TRUE if the communication module can communicate with the remote process; otherwise FALSE
 *
 * MT-NOTE: this routine assumes the VC's mutex is already locked
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_select_module
int mpig_cm_xio_select_module(mpig_bc_t * bc, mpig_vc_t * vc, bool_t * selected)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    char * version_str = NULL;
    char * contact_str = NULL;
    char * format_str = NULL;
    char * endian_str = NULL;
    int format;
    int version;
    mpig_endian_t endian;
    bool_t found;
    int rc;
    bool_t failed;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_select_module);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_select_module);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering"));

    *selected = FALSE;
    
    if (vc->cm_type == MPIG_CM_TYPE_UNDEFINED)
    {
	/* Get protocol version number and check that it is compatible with this module */
	mpig_bc_get_contact(bc, "CM_XIO_PROTO_VERSION", &version_str, &found, &mpi_errno, &failed);
	MPIU_ERR_CHKANDJUMP1((failed), mpi_errno, MPI_ERR_OTHER, "**globus|bc_get_contact",
			     "**globus|bc_get_contact %s", "CM_XIO_PROTO_VERSION");
	if (!found) goto fn_return;

	rc = sscanf(version_str, "%d", &version);
	MPIU_ERR_CHKANDJUMP((rc != 1), mpi_errno, MPI_ERR_INTERN, "**keyval");
	if (version != MPIG_CM_XIO_PROTO_VERSION) goto fn_return;

	/* Get format of basic datatypes */
	mpig_bc_get_contact(bc, "CM_XIO_DC_FORMAT", &format_str, &found, &mpi_errno, &failed);
	MPIU_ERR_CHKANDJUMP1((failed), mpi_errno, MPI_ERR_OTHER, "**globus|bc_get_contact",
			     "**globus|bc_get_contact %s", "CM_XIO_DC_FORMAT");
	if (!found) goto fn_return;
	
	rc = sscanf(format_str, "%d", &format);
	MPIU_ERR_CHKANDJUMP((rc != 1), mpi_errno, MPI_ERR_INTERN, "**keyval");

	/* Get endianess of remote system */
	mpig_bc_get_contact(bc, "CM_XIO_DC_ENDIAN", &endian_str, &found, &mpi_errno, &failed);
	MPIU_ERR_CHKANDJUMP1((failed), mpi_errno, MPI_ERR_OTHER, "**globus|bc_get_contact",
			     "**globus|bc_get_contact %s", "CM_XIO_DC_ENDIAN");
	if (!found) goto fn_return;

	endian = (strcmp(endian_str, "little") == 0) ? MPIG_ENDIAN_LITTLE : MPIG_ENDIAN_BIG;
	
	/* initialize CM XIO fields in the VC */
	mpig_cm_xio_vc_construct(vc);
	vc->cm.xio.endian = endian;
	vc->cm.xio.df = format;

	/* adjust the PG reference to account for the newly activated VC */
	mpig_pg_inc_ref_count(mpig_vc_get_pg(vc));
    }

    if (vc->cm_type == MPIG_CM_TYPE_XIO)
    {
	/* Get the contact string */
	mpig_bc_get_contact(bc, "CM_XIO_CONTACT_STRING", &contact_str, &found, &mpi_errno, &failed);
	MPIU_ERR_CHKANDJUMP1((failed), mpi_errno, MPI_ERR_OTHER, "**globus|bc_get_contact",
			     "**globus|bc_get_contact %s", "CM_XIO_CONTACT_STRING");
	if (!found) goto fn_return;

	/* add the contact string to the VC */
	vc->cm.xio.cs = MPIU_Strdup(contact_str);

	/* set the selected flag to indicate that the XIO communication module has accepted responsibility for the VC */
	*selected = TRUE;
    }

  fn_return:
    if (version_str != NULL) mpig_bc_free_contact(version_str);
    if (contact_str != NULL) mpig_bc_free_contact(contact_str);
    if (format_str != NULL) mpig_bc_free_contact(format_str);
    if (endian_str != NULL) mpig_bc_free_contact(endian_str);
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_select_module);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_select_module() */
/**********************************************************************************************************************************
					   END COMMUNICATION MODULE CORE API FUNCTIONS
**********************************************************************************************************************************/


/**********************************************************************************************************************************
				    BEGIN COMMUNICATION MODULE PROGRESS ENGINE API FUNCTIONS
**********************************************************************************************************************************/
/*
 * void mpig_cm_xio_pe_wait([IN/OUT] state, [IN/OUT] mpi_errno, [OUT] failed)
 *
 * XXX: this should be rolled together with the rcq_wait so that multiple requests can be completed in a single call.  also,
 * mpig_cm_xio_pe_start() should be used to eliminate returns everytime a message is receive just in case the application
 * might be calling MPI_Probe().  It would likely be better for progress_start() to clear the wakeup_pending flag, but it will
 * come at the cost of an extra lock/unlock on the RCQ mutex.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_pe_wait
void mpig_cm_xio_pe_wait(struct MPID_Progress_state * state, int * mpi_errno_p, bool_t * const failed_p)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPID_Request * req;
    bool_t failed;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_pe_wait);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_pe_wait);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering: mpi_errno=0x%08x", *mpi_errno_p));
    *failed_p = FALSE;

    mpig_cm_xio_rcq_deq_wait(&req, mpi_errno_p, &failed);
    MPIU_ERR_CHKANDJUMP((failed), *mpi_errno_p, MPI_ERR_OTHER, "**globus|cm_xio|rcq_deq_wait");

    if (req != NULL)
    {
	mpig_request_complete(req);
    }
    else
    {
	mpig_pe_notify_unexp_recv();
    }
    
  
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno=0x%08x", *mpi_errno_p));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_pe_wait);
    return;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	*failed_p = TRUE;
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_pe_wait() */


/*
 * void mpig_cm_xio_pe_test([IN/OUT] mpi_errno, [OUT] failed)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_pe_test
void mpig_cm_xio_pe_test(int * mpi_errno_p, bool_t * const failed_p)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPID_Request * req;
    bool_t failed;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_pe_test);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_pe_test);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering: mpi_errno=0x%08x", *mpi_errno_p));
    *failed_p = FALSE;

    mpig_cm_xio_rcq_deq_test(&req, mpi_errno_p, &failed);
    MPIU_ERR_CHKANDJUMP((failed), *mpi_errno_p, MPI_ERR_OTHER, "**globus|cm_xio|rcq_deq_test");

    if (req != NULL)
    {
	mpig_request_complete(req);
    }

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno=0x%08x", *mpi_errno_p));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_pe_test);
    return;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	*failed_p = TRUE;
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_pe_test() */
/**********************************************************************************************************************************
				     END COMMUNICATION MODULE PROGRESS ENGINE API FUNCTIONS
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					 BEGIN INCLUSION OF INTERNAL FUNCTION DEFINTIONS
**********************************************************************************************************************************/
#define MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS
#include "mpig_cm_xio_req.i"
#include "mpig_cm_xio_vc.i"
#include "mpig_cm_xio_conn.i"
#include "mpig_cm_xio_data.i"
#include "mpig_cm_xio_comm.i"
/**********************************************************************************************************************************
					  END INCLUSION OF INTERNAL FUNCTION DEFINTIONS
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						  BEGIN MISCELLANEOUS FUNCTIONS
**********************************************************************************************************************************/
/*
 * char * mpig_cm_xio_msg_type_get_string([IN/MOD] req)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_msg_type_get_string
MPIG_STATIC const char * mpig_cm_xio_msg_type_get_string(mpig_cm_xio_msg_types_t msg_type)
{
    const char * str;
    
    switch(msg_type)
    {
	case MPIG_CM_XIO_MSG_TYPE_FIRST:
	    str ="MPIG_CM_XIO_MSG_TYPE_FIRST";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_UNDEFINED:
	    str ="MPIG_CM_XIO_MSG_TYPE_UNDEFINED";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_EAGER_DATA:
	    str ="MPIG_CM_XIO_MSG_TYPE_EAGER_DATA";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_RNDV_RTS:
	    str ="MPIG_CM_XIO_MSG_TYPE_RNDV_RTS";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_RNDV_CTS:
	    str ="MPIG_CM_XIO_MSG_TYPE_RNDV_CTS";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_RNDV_DATA:
	    str ="MPIG_CM_XIO_MSG_TYPE_RNDV_DATA";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_SSEND_ACK:
	    str ="MPIG_CM_XIO_MSG_TYPE_SSEND_ACK";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_CANCEL_SEND:
	    str ="MPIG_CM_XIO_MSG_TYPE_CANCEL_SEND";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_CANCEL_SEND_RESP:
	    str ="MPIG_CM_XIO_MSG_TYPE_CANCEL_SEND_RESP";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_OPEN_REQ:
	    str ="MPIG_CM_XIO_MSG_TYPE_OPEN_REQ";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_OPEN_RESP:
	    str ="MPIG_CM_XIO_MSG_TYPE_OPEN_RESP";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_CLOSE:
	    str ="MPIG_CM_XIO_MSG_TYPE_CLOSE";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_LAST:
	    str ="MPIG_CM_XIO_MSG_TYPE_LAST";
	    break;
	default:
	    str = "(unrecognized message type)";
	    break;
    }

    return str;
}
/* mpig_cm_xio_msg_type_get_string() */
/**********************************************************************************************************************************
					       END MISCELLANEOUS HELPER FUNCTIONS
**********************************************************************************************************************************/
