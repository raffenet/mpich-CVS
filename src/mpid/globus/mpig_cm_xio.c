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
#define MPIG_CM_XIO_CONNACC_PROTO_VERSION 1
#define MPIG_CM_XIO_PROTO_CONNECT_MAGIC "MPIG-CM-XIO-SHAKE-MAGIC-8-BALL\n"
#define MPIG_CM_XIO_PROTO_ACCEPT_MAGIC "MPIG-CM-XIO-MAGIC-8-BALL-SAYS-YES\n"
/**********************************************************************************************************************************
							 END PARAMETERS
**********************************************************************************************************************************/


/**********************************************************************************************************************************
				      BEGIN MISCELLANEOUS MACROS, PROTOTYPES, AND VARIABLES
**********************************************************************************************************************************/
MPIG_STATIC globus_mutex_t mpig_cm_xio_mutex;
MPIG_STATIC int mpig_cm_xio_methods_active = 0;


static int mpig_cm_xio_module_init(void);

static int mpig_cm_xio_module_finalize(void);

static const char * mpig_cm_xio_msg_type_get_string(mpig_cm_xio_msg_type_t msg_type);


#define mpig_cm_xio_mutex_construct()	globus_mutex_init(&mpig_cm_xio_mutex, NULL)
#define mpig_cm_xio_mutex_destruct()	globus_mutex_destroy(&mpig_cm_xio_mutex)
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
#include "mpig_cm_xio_nets.i"
#include "mpig_cm_xio_conn.i"
#include "mpig_cm_xio_data.i"
#include "mpig_cm_xio_comm.i"
/**********************************************************************************************************************************
					 END INCLUSION OF INTERNAL MACROS AND PROTOTYPES
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					   BEGIN COMMUNICATION MODULE CORE API SECTION
**********************************************************************************************************************************/
/*
 * <mpi_errno> mpig_cm_xio_module_init(void)
 *
 * NOTE: this routine insures that the module data structures are initialized only once even if multiple methods are initialized
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_module_init
static int mpig_cm_xio_module_init(void)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    globus_result_t grc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_module_init);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_module_init);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering"));

    if(mpig_cm_xio_methods_active == 0)
    {
        mpig_cm_xio_mutex_construct();

	/* initialize the vc tracking list */
	mpig_cm_xio_vc_list_init();
    
        /* initialize the request completion queue */
        mpi_errno = mpig_cm_xio_rcq_init();
	MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|rcq_init");

	/* activate globus XIO module */
	grc = globus_module_activate(GLOBUS_XIO_MODULE);
	MPIU_ERR_CHKANDJUMP1((grc), mpi_errno, MPI_ERR_OTHER, "**globus|module_activate", "**globus|module_activate %s", "XIO");
    }

    mpig_cm_xio_methods_active += 1;

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_module_init);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}

/*
 * <mpi_errno> mpig_cm_xio_module_finalize(void)
 *
 * NOTE: this routine insures that the module data structures are destroyed only once even if multiple methods are shutdown
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_module_finalize
static int mpig_cm_xio_module_finalize(void)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    globus_result_t grc;
    int mrc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_module_finalize);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_module_finalize);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering"));

    mpig_cm_xio_methods_active -= 1;

    if(mpig_cm_xio_methods_active == 0)
    {
	/* deactivate the globus XIO module */
	grc = globus_module_deactivate(GLOBUS_XIO_MODULE);
	MPIU_ERR_CHKANDSTMT2((grc), mpi_errno, MPI_ERR_OTHER, {;}, "**globus|module_deactivate",
	    "**globus|module_deactivate %s %s", "XIO", globus_error_print_chain(globus_error_peek(grc)));

	/* shutdown the vc tracking list */
	mpig_cm_xio_vc_list_finalize();
    
	/* shutdown the request completion queue */
	mrc = mpig_cm_xio_rcq_finalize();
	MPIU_ERR_CHKANDSTMT((mrc), mrc, MPI_ERR_OTHER, {MPIU_ERR_ADD(mpi_errno, mrc);}, "**globus|cm_xio|rcq_finalize");
    
	mpig_cm_xio_mutex_destruct();

	if (mpi_errno) goto fn_fail;
    }
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_module_finalize);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/**********************************************************************************************************************************
					    END COMMUNICATION MODULE CORE API SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
				    BEGIN COMMUNICATION MODULE PROGRESS ENGINE API FUNCTIONS
**********************************************************************************************************************************/
/*
 * <mpi_errno> mpig_cm_xio_pe_wait([IN/OUT] state)
 *
 * XXX: this should be rolled together with the rcq_wait so that multiple requests can be completed in a single call.  also,
 * mpig_cm_xio_pe_start() should be used to eliminate returns everytime a message is receive just in case the application
 * might be calling MPI_Probe().  It would likely be better for progress_start() to clear the wakeup_pending flag, but it will
 * come at the cost of an extra lock/unlock on the RCQ mutex.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_pe_wait
int mpig_cm_xio_pe_wait(struct MPID_Progress_state * state)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPID_Request * req;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_pe_wait);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_pe_wait);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering"));

    mpi_errno = mpig_cm_xio_rcq_deq_wait(&req);
    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|rcq_deq_wait");

    if (req != NULL)
    {
	mpig_request_complete(req);
    }
    else
    {
	mpig_pe_notify_unexp_recv();
    }
    
  
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_pe_wait);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_pe_wait() */


/*
 * <mpi_errno> mpig_cm_xio_pe_test(void)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_pe_test
int mpig_cm_xio_pe_test(void)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPID_Request * req;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_pe_test);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_pe_test);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering"));

    mpi_errno = mpig_cm_xio_rcq_deq_test(&req);
    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|rcq_deq_test");

    if (req != NULL)
    {
	mpig_request_complete(req);
    }

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_pe_test);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
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
#include "mpig_cm_xio_nets.i"
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
static const char * mpig_cm_xio_msg_type_get_string(mpig_cm_xio_msg_type_t msg_type)
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
	case MPIG_CM_XIO_MSG_TYPE_OPEN_PROC_REQ:
	    str ="MPIG_CM_XIO_MSG_TYPE_OPEN_PORT_REQ";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_OPEN_PROC_RESP:
	    str ="MPIG_CM_XIO_MSG_TYPE_OPEN_PROC_RESP";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_OPEN_PORT_REQ:
	    str ="MPIG_CM_XIO_MSG_TYPE_OPEN_PROC_REQ";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_OPEN_PORT_RESP:
	    str ="MPIG_CM_XIO_MSG_TYPE_OPEN_RESP";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_OPEN_ERROR_RESP:
	    str ="MPIG_CM_XIO_MSG_TYPE_OPEN_RESP";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_CLOSE_PROC:
	    str ="MPIG_CM_XIO_MSG_TYPE_CLOSE_PROC";
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
