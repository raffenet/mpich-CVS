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


/*
 * Miscellaneous module specific variables
 */
MPIG_STATIC globus_xio_stack_t mpig_cm_xio_stack;
MPIG_STATIC globus_xio_attr_t mpig_cm_xio_attrs;
MPIG_STATIC globus_xio_server_t mpig_cm_xio_server;
MPIG_STATIC char * mpig_cm_xio_server_cs = NULL;

MPIG_STATIC char mpig_cm_xio_connect_phrase[] = "MPICH2-GLOBUS-CM-XIO CONNECT\n";
MPIG_STATIC char mpig_cm_xio_accept_phrase[] = "MPICH2-GLOBUS-CM-XIO ACCEPT\n";

/*
 * Miscellaneous module specific macros
 */
#define mpig_cm_xio_vc_construct(vc_)		\
{						\
    (vc_)->cm.xio.cs = NULL;			\
    (vc_)->cm.xio.df = -1;			\
    (vc_)->cm.xio.handle = NULL;		\
    (vc_)->cm.xio.active_sreq = NULL;		\
    (vc_)->cm.xio.active_rreq = NULL;		\
    mpig_cm_xio_sendq_construct(vc_);		\
    (vc_)->cm.xio.rbufp = (vc_)->cm.xio.rbuf;	\
    (vc_)->cm.xio.rbufc = 0;			\
}

#define mpig_cm_xio_vc_destruct(vc_)		\
{						\
    MPIU_Free((vc_)->cm.xio.cs);		\
    (vc_)->cm.xio.df = -1;			\
    (vc_)->cm.xio.handle = NULL;		\
    (vc_)->cm.xio.active_sreq = NULL;		\
    (vc_)->cm.xio.active_rreq = NULL;		\
    mpig_cm_xio_sendq_destruct(vc_);		\
    (vc_)->cm.xio.rbufp = NULL;			\
    (vc_)->cm.xio.rbufc = 0;			\
}


/*
 * Include other CM_XIO source files
 */
#if 0
#include "mpig_cm_xio_q.i"
#include "mpig_cm_xio_data.i"
#include "mpig_cm_xio_comm.i"
#include "mpig_cm_xio_adi3.i"

#else /* XXX: REMOVE ME !!! */

#define MPIG_CM_XIO_TCP_PROTO_VERSION 1
#define mpig_cm_xio_sendq_construct(vc_)
#define mpig_cm_xio_sendq_destruct(vc_)
MPIG_STATIC void mpig_cm_xio_handle_server_accept(globus_xio_server_t server, globus_xio_handle_t handle, globus_result_t op_grc,
						  void * arg);
MPIG_STATIC void mpig_cm_xio_handle_server_accept(globus_xio_server_t server, globus_xio_handle_t handle, globus_result_t op_grc,
						  void * arg)
{
}
MPIG_STATIC mpig_cm_funcs_t mpig_cm_xio_funcs =
{
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};
#endif


/*
 * int mpig_cm_xio_init([IN/OUT] argc, [IN/OUT] argv)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_init
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_cm_xio_init(int * argc, char *** argv)
{
    globus_xio_driver_t tcp_driver;
    globus_result_t grc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_init);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_init);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    
    /*
     * Activate Globus XIO module
     */
    grc = globus_module_activate(GLOBUS_XIO_MODULE);
    MPIU_ERR_CHKANDJUMP1((grc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|module_activate",
			 "**globus|module_activate %s", "XIO");

    /*
     * Build stack of communication drivers
     */
    grc = globus_xio_stack_init(&mpig_cm_xio_stack, NULL);
    MPIU_ERR_CHKANDJUMP1((grc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|xio_server_init",
			 "**globus|xio_server_init %s", globus_error_print_chain(globus_error_peek(grc)));

    grc = globus_xio_driver_load("tcp", &tcp_driver);
    MPIU_ERR_CHKANDJUMP1((grc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|xio_server_init",
			 "**globus|xio_server_init %s", globus_error_print_chain(globus_error_peek(grc)));

    grc = globus_xio_stack_push_driver(mpig_cm_xio_stack, tcp_driver);
    MPIU_ERR_CHKANDJUMP1((grc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|xio_server_init",
			 "**globus|xio_server_init %s", globus_error_print_chain(globus_error_peek(grc)));

#   if XXX_SECURE
    {
	globus_xio_driver_t gsi_driver;
	
        grc = globus_xio_driver_load("gsi", &gsi_driver);
	MPIU_ERR_CHKANDJUMP1((grc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|xio_server_init",
			     "**globus|xio_server_init %s", globus_error_print_chain(globus_error_peek(grc)));

        grc = globus_xio_stack_push_driver(mpig_cm_xio_stack, gsi_driver);
	MPIU_ERR_CHKANDJUMP1((grc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|xio_server_init",
			     "**globus|xio_server_init %s", globus_error_print_chain(globus_error_peek(grc)));
    }
#   endif


    /*
     * Set TCP options and parameters
     */
    grc = globus_xio_attr_init(&mpig_cm_xio_attrs);
    MPIU_ERR_CHKANDJUMP1((grc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|xio_server_init",
			 "**globus|xio_server_init %s", globus_error_print_chain(globus_error_peek(grc)));

    grc = globus_xio_attr_cntl(mpig_cm_xio_attrs, tcp_driver, GLOBUS_XIO_TCP_SET_NODELAY, GLOBUS_TRUE);
    MPIU_ERR_CHKANDJUMP1((grc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|xio_server_init",
			 "**globus|xio_server_init %s", globus_error_print_chain(globus_error_peek(grc)));

#   if XXX
    {
	grc = globus_xio_attr_cntl(mpig_cm_xio_attrs, tcp_driver, GLOBUS_XIO_TCP_SET_SNDBUF, buf_size);
	MPIU_ERR_CHKANDJUMP1((grc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|xio_server_init",
			     "**globus|xio_server_init %s", globus_error_print_chain(globus_error_peek(grc)));

	grc = globus_xio_attr_cntl(mpig_cm_xio_attrs, tcp_driver, GLOBUS_XIO_TCP_SET_RCVBUF, buf_size);
	MPIU_ERR_CHKANDJUMP1((grc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|xio_server_init",
			     "**globus|xio_server_init %s", globus_error_print_chain(globus_error_peek(grc)));
    }
#    endif
    
    /*
     * Establish server to start listening for new connections
     */
    grc = globus_xio_server_create(&mpig_cm_xio_server, mpig_cm_xio_attrs, mpig_cm_xio_stack);
    MPIU_ERR_CHKANDJUMP1((grc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|xio_server_init",
			 "**globus|xio_server_init %s", globus_error_print_chain(globus_error_peek(grc)));

    grc = globus_xio_server_cntl(mpig_cm_xio_server, tcp_driver, GLOBUS_XIO_TCP_GET_LOCAL_CONTACT, &mpig_cm_xio_server_cs);
    MPIU_ERR_CHKANDJUMP1((grc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|xio_server_init",
			 "**globus|xio_server_init %s", globus_error_print_chain(globus_error_peek(grc)));

    grc = globus_xio_server_register_accept(mpig_cm_xio_server, mpig_cm_xio_handle_server_accept, NULL);
    MPIU_ERR_CHKANDJUMP1((grc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|xio_server_init",
			 "**globus|xio_server_init %s", globus_error_print_chain(globus_error_peek(grc)));

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
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
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_cm_xio_finalize(void)
{
    globus_result_t grc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_finalize);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_finalize);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    
    /*
     * Disable the connection server and free any resources used by it
     */
    if (mpig_cm_xio_server_cs != NULL)
    { 
	globus_libc_free(mpig_cm_xio_server_cs);
    }

    /*
     * XXX: Unload XIO drivers?
     */

    /*
     * Deactivate the XIO module
     */
    grc = globus_module_deactivate(GLOBUS_XIO_MODULE);
    MPIU_ERR_CHKANDJUMP1((grc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|module_deactivate",
			 "**globus|module_deactivate %s", "XIO");

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_finalize);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_finalize() */


/*
 * mpig_cm_xio_add_contact_info([IN/OUT] business card)
 *
 * Add any and all contact information for this communication module to the supplied business card.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_add_contact_info
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_cm_xio_add_contact_info(mpig_bc_t * bc)
{
    char uint_str[10];
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_add_contact_info);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_add_contact_info);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    
    MPIU_Snprintf(uint_str, 10, "%u", (unsigned) MPIG_CM_XIO_TCP_PROTO_VERSION);
    mpi_errno = mpig_bc_add_contact(bc, "CM_XIO_TCP_PROTO_VERSION", uint_str);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    mpi_errno = mpig_bc_add_contact(bc, "CM_XIO_TCP_CONTACT_STRING", mpig_cm_xio_server_cs);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    MPIU_Snprintf(uint_str, 10, "%u", (unsigned) GLOBUS_DC_FORMAT_LOCAL);
    mpi_errno = mpig_bc_add_contact(bc, "CM_XIO_DC_FORMAT", uint_str);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

  fn_return:
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_add_contact_info);
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_add_contact_info() */


/*
 * int mpig_cm_xio_select_module([IN] bc, [IN/OUT] vc, [OUT] flag)
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
#define FUNCNAME mpig_cm_xio_select_module
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_cm_xio_select_module(mpig_bc_t * bc, mpig_vc_t * vc, int * flag)
{
    char * version_str = NULL;
    char * contact_str = NULL;
    char * format_str = NULL;
    int format;
    int version;
    int bc_flag;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_select_module);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_select_module);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    /* Get protocol version number */
    mpi_errno = mpig_bc_get_contact(bc, "CM_XIO_TCP_PROTO_VERSION", &version_str, &bc_flag);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
    if (bc_flag == FALSE)
    {
	*flag = FALSE;
	goto fn_return;
    }

    rc = sscanf(version_str, "%d", &version);
    MPIU_ERR_CHKANDJUMP((rc != 1), mpi_errno, MPI_ERR_INTERN, "**keyval");
    if (version != MPIG_CM_XIO_TCP_PROTO_VERSION)
    { 
	MPIU_ERR_SETFATALANDSTMT(mpi_errno, MPI_ERR_OTHER, {goto fn_fail;}, "**globus|cm_xio_ver");
    }
	
    /* Get the contact string */
    mpi_errno = mpig_bc_get_contact(bc, "CM_XIO_TCP_CONTACT_STRING", &contact_str, &bc_flag);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
    if (bc_flag == FALSE)
    {
	*flag = FALSE;
	goto fn_return;
    }

    /* Get format of basic datatypes */
    mpi_errno = mpig_bc_get_contact(bc, "CM_XIO_DC_FORMAT", &format_str, &bc_flag);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
    if (bc_flag == FALSE)
    {
	*flag = FALSE;
	goto fn_return;
    }

    rc = sscanf(format_str, "%d", &format);
    MPIU_ERR_CHKANDJUMP((rc != 1), mpi_errno, MPI_ERR_INTERN, "**keyval");

    mpig_cm_xio_vc_construct(vc);
    mpig_vc_set_state(vc, MPIG_VC_STATE_UNCONNECTED);
    mpig_vc_set_cm_type(vc, MPIG_CM_TYPE_XIO);
    mpig_vc_set_cm_funcs(vc, mpig_cm_xio_funcs);
    vc->cm.xio.cs = MPIU_Strdup(contact_str);
    vc->cm.xio.df = format;

    *flag = TRUE;
    
  fn_return:
    if (version_str != NULL)
    {
	mpig_bc_free_contact(version_str);
    }
    if (contact_str != NULL)
    {
	mpig_bc_free_contact(contact_str);
    }
    if (format_str != NULL)
    {
	mpig_bc_free_contact(format_str);
    }
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_select_module);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    *flag = FALSE;
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_select_module() */
