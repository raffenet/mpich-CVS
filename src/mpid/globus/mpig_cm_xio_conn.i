/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */


/**********************************************************************************************************************************
			       BEGIN MISCELLANEOUS CONNECTION MANAGEMENT ROUTINES AND DEFINITIONS
**********************************************************************************************************************************/
#if !defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS)

typedef enum mpig_cm_xio_conn_open_resp
{
    MPIG_CM_XIO_CONN_OPEN_RESP_FIRST = 0,
    MPIG_CM_XIO_CONN_OPEN_RESP_UNDEFINED,
    MPIG_CM_XIO_CONN_OPEN_RESP_ACK,
    MPIG_CM_XIO_CONN_OPEN_RESP_NAK,
    MPIG_CM_XIO_CONN_OPEN_RESP_MSG_INCOMPLETE,
    MPIG_CM_XIO_CONN_OPEN_RESP_MSG_TYPE_BAD,
    MPIG_CM_XIO_CONN_OPEN_RESP_MSG_TOO_LONG,
    MPIG_CM_XIO_CONN_OPEN_RESP_VC_NOT_XIO,
    MPIG_CM_XIO_CONN_OPEN_RESP_VC_CONNECTED,
    MPIG_CM_XIO_CONN_OPEN_RESP_VC_DISCONNECTING,
    MPIG_CM_XIO_CONN_OPEN_RESP_VC_FAILED,
    MPIG_CM_XIO_CONN_OPEN_RESP_LAST
}
mpig_cm_xio_conn_open_resp_t;


MPIG_STATIC globus_xio_stack_t mpig_cm_xio_conn_stack;
MPIG_STATIC globus_xio_attr_t mpig_cm_xio_conn_attrs;
MPIG_STATIC globus_xio_server_t mpig_cm_xio_server_handle;
MPIG_STATIC char * mpig_cm_xio_server_cs = NULL;


MPIG_STATIC void mpig_cm_xio_conn_init(int * mpi_errno_p, bool_t * failed_p);

MPIG_STATIC void mpig_cm_xio_conn_finalize(int * mpi_errno_p, bool_t * failed_p);

MPIG_STATIC void mpig_cm_xio_tmp_vc_destroy(mpig_vc_t * tmp_vc, int * mpi_errno_p, bool_t * failed_p);

MPIG_STATIC void mpig_cm_xio_null_vc_handle_close(globus_xio_handle_t handle, globus_result_t op_grc, void * arg);

MPIG_STATIC const char * mpig_cm_xio_conn_open_resp_get_string(mpig_cm_xio_conn_open_resp_t open_resp);


#else /* defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS) */


/*
 * void mpig_cm_xio_conn_init([IN/MOD] tmp_vc, [IN/OUT] mpi_errno)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_conn_init
MPIG_STATIC void mpig_cm_xio_conn_init(int * mpi_errno_p, bool_t * failed_p)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    globus_xio_driver_t tcp_driver;
    globus_result_t grc;
    bool_t failed;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_conn_init);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_conn_init);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"entering: mpi_errno=0x%08x", *mpi_errno_p));
    *failed_p = FALSE;

    /* initialize the vc tracking list */
    mpig_cm_xio_vc_list_init();
    
    /* build stack of communication drivers */
    grc = globus_xio_stack_init(&mpig_cm_xio_conn_stack, NULL);
    MPIU_ERR_CHKANDJUMP1((grc), *mpi_errno_p, MPI_ERR_OTHER, "**globus|cm_xio|xio_stack_create",
	"**globus|cm_xio|xio_stack_create %s", globus_error_print_chain(globus_error_peek(grc)));

    grc = globus_xio_driver_load("tcp", &tcp_driver);
    MPIU_ERR_CHKANDJUMP1((grc), *mpi_errno_p, MPI_ERR_OTHER, "**globus|cm_xio|xio_stack_create",
	"**globus|cm_xio|xio_stack_create %s", globus_error_print_chain(globus_error_peek(grc)));

    grc = globus_xio_stack_push_driver(mpig_cm_xio_conn_stack, tcp_driver);
    MPIU_ERR_CHKANDJUMP1((grc), *mpi_errno_p, MPI_ERR_OTHER, "**globus|cm_xio|xio_stack_create",
	"**globus|cm_xio|xio_stack_create %s", globus_error_print_chain(globus_error_peek(grc)));

#   if XXX_SECURE
    {
	globus_xio_driver_t gsi_driver;
	
        grc = globus_xio_driver_load("gsi", &gsi_driver);
	MPIU_ERR_CHKANDJUMP1((grc), *mpi_errno_p, MPI_ERR_OTHER, "**globus|cm_xio|xio_stack_create",
	    "**globus|cm_xio|xio_stack_create %s", globus_error_print_chain(globus_error_peek(grc)));

        grc = globus_xio_stack_push_driver(mpig_cm_xio_conn_stack, gsi_driver);
	MPIU_ERR_CHKANDJUMP1((grc), *mpi_errno_p, MPI_ERR_OTHER, "**globus|cm_xio|xio_stack_create",
	    "**globus|cm_xio|xio_stack_create %s", globus_error_print_chain(globus_error_peek(grc)));
    }
#   endif


    /* set TCP options and parameters */
    grc = globus_xio_attr_init(&mpig_cm_xio_conn_attrs);
    MPIU_ERR_CHKANDJUMP1((grc), *mpi_errno_p, MPI_ERR_OTHER, "**globus|cm_xio|xio_stack_create",
	"**globus|cm_xio|xio_stack_create %s", globus_error_print_chain(globus_error_peek(grc)));

    grc = globus_xio_attr_cntl(mpig_cm_xio_conn_attrs, tcp_driver, GLOBUS_XIO_TCP_SET_NODELAY, GLOBUS_TRUE);
    MPIU_ERR_CHKANDJUMP1((grc), *mpi_errno_p, MPI_ERR_OTHER, "**globus|cm_xio|xio_stack_create",
	"**globus|cm_xio|xio_stack_create %s", globus_error_print_chain(globus_error_peek(grc)));

#   if XXX
    {
	grc = globus_xio_attr_cntl(mpig_cm_xio_conn_attrs, tcp_driver, GLOBUS_XIO_TCP_SET_SNDBUF, buf_size);
	MPIU_ERR_CHKANDJUMP1((grc), *mpi_errno_p, MPI_ERR_OTHER, "**globus|cm_xio|xio_stack_create",
	    "**globus|cm_xio|xio_stack_create %s", globus_error_print_chain(globus_error_peek(grc)));

	grc = globus_xio_attr_cntl(mpig_cm_xio_conn_attrs, tcp_driver, GLOBUS_XIO_TCP_SET_RCVBUF, buf_size);
	MPIU_ERR_CHKANDJUMP1((grc), *mpi_errno_p, MPI_ERR_OTHER, "**globus|cm_xio|xio_stack_create",
	    "**globus|cm_xio|xio_stack_create %s", globus_error_print_chain(globus_error_peek(grc)));
    }
#    endif
    
    /* establish server to start listening for new connections */
    grc = globus_xio_server_create(&mpig_cm_xio_server_handle, mpig_cm_xio_conn_attrs, mpig_cm_xio_conn_stack);
    MPIU_ERR_CHKANDJUMP1((grc), *mpi_errno_p, MPI_ERR_OTHER, "**globus|cm_xio|xio_server_create",
	"**globus|cm_xio|xio_server_create %s", globus_error_print_chain(globus_error_peek(grc)));

    grc = globus_xio_server_get_contact_string(mpig_cm_xio_server_handle, &mpig_cm_xio_server_cs);
    MPIU_ERR_CHKANDJUMP1((grc), *mpi_errno_p, MPI_ERR_OTHER, "**globus|cm_xio|xio_server_create",
	"**globus|cm_xio|xio_server_create %s", globus_error_print_chain(globus_error_peek(grc)));

    mpig_cm_xio_server_listen(mpi_errno_p, &failed);
    MPIU_ERR_CHKANDJUMP((failed), *mpi_errno_p, MPI_ERR_OTHER, "**globus|cm_xio|xio_server_listen");

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"exiting: mpi_errno=0x%08x, failed=%s", *mpi_errno_p, MPIG_BOOL_STR(*failed_p)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_conn_init);
    return;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	*failed_p = TRUE;
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_conn_init() */


/*
 * void mpig_cm_xio_conn_finalize([IN/MOD] tmp_vc, [IN/OUT] mpi_errno)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_conn_finalize
MPIG_STATIC void mpig_cm_xio_conn_finalize(int * mpi_errno_p, bool_t * failed_p)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    globus_result_t grc;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_conn_finalize);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_conn_finalize);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"entering: mpi_errno=0x%08x", *mpi_errno_p));
    *failed_p = FALSE;

    /* disable the connection server and free any resources used by it */
    grc = globus_xio_server_close(mpig_cm_xio_server_handle);
    MPIU_ERR_CHKANDSTMT1((grc), *mpi_errno_p, MPI_ERR_OTHER, {;}, "**globus|cm_xio|xio_close_server",
	"**globus|cm_xio|xio_close_server %s", globus_error_print_chain(globus_error_peek(grc)));
    
    if (mpig_cm_xio_server_cs != NULL)
    { 
	globus_libc_free(mpig_cm_xio_server_cs);
    }

    /* NOTE: prior to calling this routine, MPID_Finalize() dissolved all communicators, so there should not be any VC references
       outstanding.  that is not to say that the VCs are completely inactive.  some may be going through the close protocol,
       while others may still be receiving messages from the remote process.  at this point message reception from a remote
       process is only valid, meaning the application is a valid MPI application, if the remote process intends to cancel those
       messages; otherwise, the messages will go unmatched, possibly causing the remote process to hang (i.e., the message is
       being sent using the rendezvous protocol). */
    
    /* wait for all of the connections to close */
    mpig_cm_xio_vc_list_wait_empty();
    
    /* shutdown the vc tracking list */
    mpig_cm_xio_vc_list_finalize();
    
    /* XXX: unload globus XIO drivers? */

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"exiting: mpi_errno=0x%08x, failed=%s", *mpi_errno_p, MPIG_BOOL_STR(*failed_p)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_conn_finalize);
    return;
}
/* mpig_cm_xio_conn_finalize() */


/*
 * void mpig_cm_xio_tmp_vc_destroy([IN/MOD] tmp_vc, [IN/OUT] mpi_errno)
 *
 * MT-NOTE: this routine asssumes that the temp VC's mutex and the associated PG's mutex are _NOT_ held by the current context.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_tmp_vc_destroy
MPIG_STATIC void mpig_cm_xio_tmp_vc_destroy(mpig_vc_t * const tmp_vc, int * const mpi_errno_p, bool_t * const failed_p)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_pg_t * pg;
    globus_xio_handle_t tmp_vc_handle;
    globus_result_t grc;
    int err = 0;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_tmp_vc_destroy);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_tmp_vc_destroy);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"entering: tmp_vc=" MPIG_PTR_FMT ", mpi_errno=0x%08x", (MPIG_PTR_CAST) tmp_vc, *mpi_errno_p));
    *failed_p = FALSE;
    
    /* get infromation about the PG and the XIO handle from the temp VC */
    mpig_vc_mutex_lock(tmp_vc);
    {
	pg = mpig_vc_get_pg(tmp_vc);
	tmp_vc_handle = tmp_vc->cm.xio.handle;
	tmp_vc->cm.xio.handle = NULL;
	mpig_cm_xio_vc_list_remove(tmp_vc);
    }
    mpig_vc_mutex_unlock(tmp_vc);

    /* if the temp VC holds a reference to the the PG, then release the PG */
    if (pg != NULL) mpig_pg_release_ref(pg);

    /* destroy the temp VC and release the memory used by it */
    mpig_vc_destruct(tmp_vc);
    MPIU_Free(tmp_vc);
    
    /* close the handle associated with the temp VC */
    if (tmp_vc_handle)
    {
	grc = globus_xio_register_close(tmp_vc_handle, NULL, mpig_cm_xio_null_vc_handle_close, NULL);
	MPIU_ERR_CHKANDSTMT1((grc), *mpi_errno_p, MPI_ERR_INTERN, {err++; goto fn_fail;}, "**globus|cm_xio|xio_reg_close",
	    "**globus|cm_xio|xio_reg_close %s", globus_error_print_chain(globus_error_peek(grc)));
    }

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"exiting: tmp_vc=" MPIG_PTR_FMT ", mpi_errno=0x%08x, failed=%s", (MPIG_PTR_CAST) tmp_vc, *mpi_errno_p,
	MPIG_BOOL_STR(*failed_p)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_tmp_vc_destroy);
    return;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	if (err > 0) *failed_p = TRUE;
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_tmp_vc_destroy() */


/*
 * mpig_cm_xio_null_vc_handle_close([IN] handle, [IN] result, [IN] NULL)
 *
 * This function is called by globus_xio when a registered close operation has completed for a handle that has no VC associated
 * with it.
 *
 * Parameters:
 *
 * handle [IN] - globus_xio handle that has been closed, and thus is no longer valid
 * result [IN] - globus result indicating any error
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_null_vc_handle_close
MPIG_STATIC void mpig_cm_xio_null_vc_handle_close(globus_xio_handle_t handle, globus_result_t op_grc, void * arg)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_null_vc_handle_close);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_null_vc_handle_close);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"entering: op_grc=%d", op_grc));

    MPIU_ERR_CHKANDJUMP1((op_grc), mpi_errno, MPI_ERR_INTERN, "**globus|cm_xio|null_vc_handle_close",
	"**globus|cm_xio|null_vc_handle_close %s", globus_error_print_chain(globus_error_peek(op_grc)));

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_null_vc_handle_close);
    return;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	/* report the error through MPID_Progress_{poke,test,wait}() */
	mpig_cm_xio_fault_handle_async_error(&mpi_errno);

	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_null_vc_handle_close() */

/*
 * char * mpig_cm_xio_conn_open_resp_get_string([IN] open_resp)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_conn_open_resp_get_string
MPIG_STATIC const char * mpig_cm_xio_conn_open_resp_get_string(const mpig_cm_xio_conn_open_resp_t open_resp)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const char * str;
    
    MPIG_UNUSED_VAR(fcname);

    switch(open_resp)
    {
	case MPIG_CM_XIO_CONN_OPEN_RESP_FIRST:
	    str = "MPIG_CM_XIO_CONN_OPEN_RESP_FIRST";
	    break;
	case MPIG_CM_XIO_CONN_OPEN_RESP_UNDEFINED:
	    str = "MPIG_CM_XIO_CONN_OPEN_RESP_UNDEFINED";
	    break;
	case MPIG_CM_XIO_CONN_OPEN_RESP_ACK:
	    str = "MPIG_CM_XIO_CONN_OPEN_RESP_ACK";
	    break;
	case MPIG_CM_XIO_CONN_OPEN_RESP_NAK:
	    str = "MPIG_CM_XIO_CONN_OPEN_RESP_NAK";
	    break;
	case MPIG_CM_XIO_CONN_OPEN_RESP_MSG_INCOMPLETE:
	    str = "MPIG_CM_XIO_CONN_OPEN_RESP_MSG_INCOMPLETE";
	    break;
	case MPIG_CM_XIO_CONN_OPEN_RESP_MSG_TYPE_BAD:
	    str = "MPIG_CM_XIO_CONN_OPEN_RESP_MSG_TYPE_BAD";
	    break;
	case MPIG_CM_XIO_CONN_OPEN_RESP_MSG_TOO_LONG:
	    str = "MPIG_CM_XIO_CONN_OPEN_RESP_MSG_TOO_LONG";
	    break;
	case MPIG_CM_XIO_CONN_OPEN_RESP_VC_NOT_XIO:
	    str = "MPIG_CM_XIO_CONN_OPEN_RESP_VC_NOT_XIO";
	    break;
	case MPIG_CM_XIO_CONN_OPEN_RESP_VC_CONNECTED:
	    str = "MPIG_CM_XIO_CONN_OPEN_RESP_VC_CONNECTED";
	    break;
	case MPIG_CM_XIO_CONN_OPEN_RESP_VC_DISCONNECTING:
	    str = "MPIG_CM_XIO_CONN_OPEN_RESP_VC_DISCONNECTING";
	    break;
	case MPIG_CM_XIO_CONN_OPEN_RESP_VC_FAILED:
	    str = "MPIG_CM_XIO_CONN_OPEN_RESP_VC_FAILED";
	    break;
	case MPIG_CM_XIO_CONN_OPEN_RESP_LAST:
	    str = "MPIG_CM_XIO_CONN_OPEN_RESP_LAST";
	    break;
	default:
	    str = "(unrecognized open response)";
	    break;
    }

    return str;
}
/* mpig_cm_xio_conn_open_resp_get_string() */

#endif /* MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS */
/**********************************************************************************************************************************
				 END MISCELLANEOUS CONNECTION MANAGEMENT ROUTINES AND DEFINTIONS
**********************************************************************************************************************************/


/**********************************************************************************************************************************
				      BEGIN SERVER SIDE (LISTENER) CONNECTION ESTABLISHMENT
**********************************************************************************************************************************/
#if !defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS)
MPIG_STATIC int mpig_cm_xio_server_accept_errcnt;

MPIG_STATIC void mpig_cm_xio_server_listen(int * mpi_errno_p, bool_t * failed_p);

MPIG_STATIC void mpig_cm_xio_server_handle_connection(
    globus_xio_server_t server, globus_xio_handle_t handle, globus_result_t op_grc, void * arg);

MPIG_STATIC void mpig_cm_xio_server_handle_open(globus_xio_handle_t handle, globus_result_t op_grc, void * arg);

MPIG_STATIC void mpig_cm_xio_server_handle_recv_magic(
    globus_xio_handle_t handle, globus_result_t op_grc, globus_byte_t * rbuf, globus_size_t rbuf_len, globus_size_t nbytes,
    globus_xio_data_descriptor_t dd, void * arg);

MPIG_STATIC void mpig_cm_xio_server_handle_send_magic(
    globus_xio_handle_t handle, globus_result_t op_grc, globus_byte_t * rbuf, globus_size_t rbuf_len, globus_size_t nbytes,
    globus_xio_data_descriptor_t dd, void * arg);

MPIG_STATIC void mpig_cm_xio_server_handle_recv_open_req(
    globus_xio_handle_t handle, globus_result_t op_grc, globus_byte_t * rbuf, globus_size_t rbuf_len, globus_size_t nbytes,
    globus_xio_data_descriptor_t dd, void * arg);

MPIG_STATIC void mpig_cm_xio_server_handle_send_open_resp_ack(
    globus_xio_handle_t handle, globus_result_t op_grc, globus_byte_t * rbuf, globus_size_t rbuf_len, globus_size_t nbytes,
    globus_xio_data_descriptor_t dd, void * arg);

MPIG_STATIC void mpig_cm_xio_server_handle_send_open_resp_nak(
    globus_xio_handle_t handle, globus_result_t op_grc, globus_byte_t * rbuf, globus_size_t rbuf_len, globus_size_t nbytes,
    globus_xio_data_descriptor_t dd, void * arg);

MPIG_STATIC void mpig_cm_xio_server_handle_send_open_resp_error(
    globus_xio_handle_t handle, globus_result_t op_grc, globus_byte_t * rbuf, globus_size_t rbuf_len, globus_size_t nbytes,
    globus_xio_data_descriptor_t dd, void * arg);


#define MPIG_CM_XIO_CONN_VOTE(pg_id_, pg_rank_)									\
    (mpig_pg_compare_ids(mpig_process.my_pg->id, (pg_id_)) < 0 ||						\
	(mpig_pg_compare_ids(mpig_process.my_pg->id, (pg_id_)) == 0 && mpig_process.my_pg_rank < (pg_rank_)))


#else /* defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS) */


/*
 * void mpig_cm_xio_server_listen([IN/MOD] mpi_errno)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_server_listen
MPIG_STATIC void mpig_cm_xio_server_listen(int * const mpi_errno_p, bool_t * const failed_p)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    globus_result_t grc;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_server_listen);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_server_listen);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"entering: mpi_errno=0x%08x", *mpi_errno_p));
    *failed_p = FALSE;

    MPIG_UNUSED_VAR(fcname);

    grc = globus_xio_server_register_accept(mpig_cm_xio_server_handle, mpig_cm_xio_server_handle_connection, NULL);
    if (grc)
    {
	/* --BEGIN ERROR HANDLING-- */
	MPIU_ERR_SETFATALANDSTMT1(*mpi_errno_p, MPI_ERR_INTERN, {goto fn_fail;}, "**globus|cm_xio|xio_server_reg_accept",
	    "**globus|cm_xio|xio_server_reg_accept %s", globus_error_print_chain(globus_error_peek(grc)));
	/* --END ERROR HANDLING-- */
    }

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"exiting: mpi_errno=0x%08x", *mpi_errno_p));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_server_listen);
    return;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	*failed_p = TRUE;
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
    
}
/* mpig_cm_xio_server_listen() */


/*
 * void mpig_cm_xio_server_handle_connection([IN] server, [IN] handle, [IN] op_grc, [IN] NULL)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_server_handle_connection
MPIG_STATIC void mpig_cm_xio_server_handle_connection(
    const globus_xio_server_t server, const globus_xio_handle_t handle, const globus_result_t op_grc, void * const arg)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    globus_result_t grc;
    bool_t mutex_locked = FALSE;
    int warn = 0;
    int err = 0;
    bool_t failed;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_server_handle_connection);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_server_handle_connection);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"entering: handle=" MPIG_PTR_FMT ", op_grc=%d", (MPIG_PTR_CAST) handle, op_grc));

    /* if the accept was cancelled, then we are likely shutting down.  XXX: add code to verify this? */
    if (globus_xio_error_is_canceled(op_grc))
    {
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
	    "register accept cancelled.  the XIO communication module must be shutting down."));
	goto fn_return;
    }
    
    mpig_cm_xio_mutex_lock();
    mutex_locked = TRUE;
    {
	if (op_grc == GLOBUS_SUCCESS)
	{    
	    /* complete the formation of the new connection */
	    grc = globus_xio_register_open(handle, NULL, mpig_cm_xio_conn_attrs, mpig_cm_xio_server_handle_open, NULL);
	    if (grc)
	    {
		/* --END ERROR HANDLING-- */
		MPIU_ERR_SETANDSTMT1(mpi_errno, MPI_ERR_INTERN, {err++; goto fn_fail;}, "**globus|cm_xio|server_handle_connection",
		    "**globus|cm_xio|server_handle_connection %s", globus_error_print_chain(globus_error_peek(grc)));

		mpig_cm_xio_server_accept_errcnt += 1;
		
		grc = globus_xio_register_close(handle, NULL, mpig_cm_xio_null_vc_handle_close, NULL);
		MPIU_ERR_CHKANDSTMT1((grc), mpi_errno, MPI_ERR_INTERN, {err++; goto fn_fail;}, "**globus|cm_xio|xio_reg_close",
		    "**globus|cm_xio|xio_reg_close %s", globus_error_print_chain(globus_error_peek(grc)));

		goto fn_fail;
		/* --END ERROR HANDLING-- */
	    }
	    else
	    {
		mpig_cm_xio_server_accept_errcnt = 0;
	    }
	}
	else
	{ 
	    /* --BEGIN ERROR HANDLING-- */
	    mpig_cm_xio_server_accept_errcnt += 1;
	    
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		"server failed to accept incoming connection: errcnt=%d", mpig_cm_xio_server_accept_errcnt));
	    MPIU_ERR_SETANDSTMT1(mpi_errno, MPI_ERR_OTHER, {warn++;}, "**globus|cm_xio|server_handle_connection",
		"**globus|cm_xio|server_handle_connection %s", globus_error_print_chain(globus_error_peek(op_grc)));
	    /* --END ERROR HANDLING-- */
	}
	
	/* check for repeated connection attempt errors */
	if (mpig_cm_xio_server_accept_errcnt > MPIG_CM_XIO_MAX_ACCEPT_ERRORS)
	{
	    /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR,
		"FATAL ERROR: server has repeatedly failed to establish new connections; reporting error: errcnt=%d",
		mpig_cm_xio_server_accept_errcnt));
	    MPIU_ERR_SETFATALANDSTMT(mpi_errno, MPI_ERR_OTHER, {err++; goto fn_fail;}, "**globus|cm_xio|server_multiple_failures");

	    /* --END ERROR HANDLING-- */
	}
    }
    mpig_cm_xio_mutex_unlock();
    mutex_locked = FALSE;
    
    /* tell the server to continue listening for new connections */
    mpig_cm_xio_server_listen(&mpi_errno, &failed);
    if (failed)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR,
	    "FATAL ERROR: the server failed to register a new accept operation"));
	MPIU_ERR_SETFATALANDSTMT(mpi_errno, MPI_ERR_OTHER, {err++; goto fn_fail;}, "**globus|cm_xio|listen");
    }	/* --END ERROR HANDLING-- */

    if (warn > 0) goto fn_fail;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"exiting: mpi_errno=0x%08x", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_server_handle_connection);
    return;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	/* release the errcnt mutex if it is being held by the current context */
	mpig_cm_xio_mutex_unlock_conditional((mutex_locked));
    
	/* report the error through MPID_Progress_{poke,test,wait}() */
	if (err > 0) mpig_cm_xio_fault_handle_async_error(&mpi_errno);

	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_server_handle_connection() */


/*
 * void mpig_cm_xio_server_handle_open([IN] handle, [IN] result, [IN/MOD] vc)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_server_handle_open
MPIG_STATIC void mpig_cm_xio_server_handle_open(const globus_xio_handle_t handle, const globus_result_t op_grc, void * const arg)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vc_t * tmp_vc = NULL;
    struct mpig_cm_xio_vc * tmp_vc_cm = NULL;
    bool_t tmp_vc_locked = FALSE;
    int warn = 0;
    int err = 0;
    bool_t failed;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_server_handle_open);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_server_handle_open);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"entering: handle=" MPIG_PTR_FMT ", op_grc=%d", (MPIG_PTR_CAST) handle, op_grc));

    MPIU_ERR_CHKANDSTMT1((op_grc), mpi_errno, MPI_ERR_OTHER, {warn++; goto fn_fail;}, "**globus|server_handle_open",
	"**globus|server_handle_open %s", globus_error_print_chain(globus_error_peek(op_grc)));
    
    /* allocate a temporary VC for use during connection establishment */
    tmp_vc = (mpig_vc_t *) MPIU_Malloc(sizeof(mpig_vc_t));
    MPIU_ERR_CHKANDSTMT1((tmp_vc == NULL), mpi_errno, MPI_ERR_OTHER, {err++; goto fn_fail;}, "**nomem",
	"**nomem %s", "temp VC for an incoming connection");

    /* initialize the temp VC object */
    tmp_vc_cm = &tmp_vc->cm.xio;
    mpig_vc_construct(tmp_vc);
    mpig_cm_xio_vc_construct(tmp_vc);
    
    mpig_vc_mutex_lock(tmp_vc);
    tmp_vc_locked = TRUE;
    {
	const size_t magic_size = strlen(MPIG_CM_XIO_PROTO_CONNECT_MAGIC);
	
	mpig_cm_xio_vc_set_state(tmp_vc, MPIG_CM_XIO_VC_STATE_ACCEPT_OPEN);
	mpig_cm_xio_vc_list_add(tmp_vc);
	tmp_vc_cm->handle = handle;
	
	/* Register a receive for the magic string.  The magic string exchange is used just to verify that an incoming connection
	   is most probably a MPIG process, and not a port scanner.  The exchange does not prevent malicious attacks. */
	mpig_cm_xio_register_read_vc_msgbuf(tmp_vc, magic_size, magic_size, mpig_cm_xio_server_handle_recv_magic,
	    &mpi_errno, &failed);
	MPIU_ERR_CHKANDSTMT((failed), mpi_errno, MPI_ERR_INTERN, {err++; goto fn_fail;}, "**globus|cm_xio|server_reg_recv_magic");
	
	mpig_cm_xio_vc_set_state(tmp_vc, MPIG_CM_XIO_VC_STATE_ACCEPT_RECEIVING_MAGIC);
    }
    mpig_vc_mutex_unlock(tmp_vc);
    tmp_vc_locked = FALSE;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"exiting: tmp_vc=" MPIG_PTR_FMT ", mpi_errno=0x%08x", (MPIG_PTR_CAST) tmp_vc, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_server_handle_open);
    return;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	globus_result_t grc;

	if (tmp_vc == NULL)
	{
	    /* close the accepted connection */
	    grc = globus_xio_register_close(handle, NULL, mpig_cm_xio_null_vc_handle_close, NULL);
	    MPIU_ERR_CHKANDSTMT1((grc), mpi_errno, MPI_ERR_INTERN, {err++;}, "**globus|cm_xio|xio_reg_close",
		"**globus|cm_xio|xio_reg_close %s", globus_error_print_chain(globus_error_peek(grc)));
	}
	else
	{
	    /* release the temp VC mutex if it is being held by the current context */
	    mpig_vc_mutex_unlock_conditional(tmp_vc, (tmp_vc_locked));
	    
	    /* close the connection and destroy the temp VC object */
	    mpig_cm_xio_tmp_vc_destroy(tmp_vc, &mpi_errno, &failed);
	    MPIU_ERR_CHKANDSTMT1((failed), mpi_errno, MPI_ERR_INTERN, {err++;}, "**globus|cm_xio|tmp_vc_destroy",
		"**globus|cm_xio|tmp_vc_destroy %p", tmp_vc);
	}
	
	/* report any errors through MPID_Progress_{poke,test,wait}() */
	if (err > 0) mpig_cm_xio_fault_handle_async_error(&mpi_errno);

	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_server_handle_open() */


/*
 * void mpig_cm_xio_server_handle_recv_magic(
 *          [IN] handle, [IN] result, [IN] rbuf, [IN] rbuf_len, [IN] nbytes, [IN] NULL, [IN/MOD] vc)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_server_handle_recv_magic
MPIG_STATIC void mpig_cm_xio_server_handle_recv_magic(
    const globus_xio_handle_t handle, const globus_result_t op_grc, globus_byte_t * const rbuf, const globus_size_t rbuf_len,
    const globus_size_t nbytes, const globus_xio_data_descriptor_t dd, void * const arg)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vc_t * const tmp_vc = (mpig_vc_t *) arg;
    struct mpig_cm_xio_vc * const tmp_vc_cm = &tmp_vc->cm.xio;
    bool_t tmp_vc_locked = FALSE;
    int warn = 0;
    int err = 0;
    bool_t failed;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_server_handle_recv_magic);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_server_handle_recv_magic);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"entering: tmp_vc=" MPIG_PTR_FMT ", handle=" MPIG_PTR_FMT ", op_grc=%d, rbuf=" MPIG_PTR_FMT ", rbuf_len=" MPIG_SIZE_FMT
	", nbytes=" MPIG_SIZE_FMT, (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) handle, op_grc, (MPIG_PTR_CAST) rbuf, rbuf_len,
	nbytes));

    mpig_vc_mutex_lock(tmp_vc);
    tmp_vc_locked = TRUE;
    {
	int rc;
	
	MPIU_Assert(mpig_cm_xio_vc_get_state(tmp_vc) == MPIG_CM_XIO_VC_STATE_ACCEPT_RECEIVING_MAGIC);
	MPIU_ERR_CHKANDSTMT2((op_grc), mpi_errno, MPI_ERR_OTHER, {warn++; goto fn_fail;},
	    "**globus|cm_xio|server_handle_recv_magic", "**globus|cm_xio|server_handle_recv_magic %p %s",
	    tmp_vc, globus_error_print_chain(globus_error_peek(op_grc)));
	MPIU_Assert(nbytes == strlen(MPIG_CM_XIO_PROTO_CONNECT_MAGIC));

	/* adjust end of message buffer to account for the number of bytes received */
	mpig_databuf_inc_eod(tmp_vc_cm->msgbuf, nbytes);

	/* compare the string to make sure the expect value was received. */
	rc = strncmp(mpig_databuf_get_ptr(tmp_vc_cm->msgbuf), MPIG_CM_XIO_PROTO_CONNECT_MAGIC,
	    strlen(MPIG_CM_XIO_PROTO_CONNECT_MAGIC));
	MPIU_ERR_CHKANDSTMT1((rc != 0), mpi_errno, MPI_ERR_OTHER, {warn++; goto fn_fail;}, "**globus|cm_xio|server_magic_mismatch",
	    "**globus|cm_xio|server_magic_mismatch %p", tmp_vc);

	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
	    "magic string matched.  sending magic string to the connector."));
	
	/* copy magic string into the VC message buffer */
	mpig_databuf_reset(tmp_vc_cm->msgbuf);
	rc = MPIU_Strncpy(mpig_databuf_get_ptr(tmp_vc_cm->msgbuf), MPIG_CM_XIO_PROTO_ACCEPT_MAGIC,
	    mpig_databuf_get_size(tmp_vc_cm->msgbuf));
	MPIU_Assert(rc == 0);
	mpig_databuf_inc_eod(tmp_vc_cm->msgbuf, strlen(MPIG_CM_XIO_PROTO_ACCEPT_MAGIC));

	/* Send the magic string */
	mpig_cm_xio_register_write_vc_msgbuf(tmp_vc, mpig_cm_xio_server_handle_send_magic, &mpi_errno, &failed);
	MPIU_ERR_CHKANDSTMT1((failed), mpi_errno, MPI_ERR_INTERN, {err++; goto fn_fail;}, "**globus|cm_xio|server_reg_send_magic",
	    "**globus|cm_xio|server_reg_send_magic %p", tmp_vc);

	mpig_cm_xio_vc_set_state(tmp_vc, MPIG_CM_XIO_VC_STATE_ACCEPT_SENDING_MAGIC);
    }
    mpig_vc_mutex_unlock(tmp_vc);
    tmp_vc_locked = FALSE;

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"exiting: tmp_vc=" MPIG_PTR_FMT ", mpi_errno=0x%08x", (MPIG_PTR_CAST) tmp_vc, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_server_handle_recv_magic);
    return;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	/* release the temp VC mutex if it is being held by the current context */
	mpig_vc_mutex_unlock_conditional(tmp_vc, (tmp_vc_locked));
	
	/* close the connection and destroy the temporary VC object */
	mpig_cm_xio_tmp_vc_destroy(tmp_vc, &mpi_errno, &failed);
	MPIU_ERR_CHKANDSTMT1((failed), mpi_errno, MPI_ERR_INTERN, {err++;}, "**globus|cm_xio|tmp_vc_destroy",
	    "**globus|cm_xio|tmp_vc_destroy %p", tmp_vc);
    
	/* report the error through MPID_Progress_{poke,test,wait}() */
	if (err > 0) mpig_cm_xio_fault_handle_async_error(&mpi_errno);

	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_server_handle_recv_magic() */


/*
 * void mpig_cm_xio_server_handle_send_magic(
 *          [IN] handle, [IN] result, [IN] sbuf, [IN] sbuf_len, [IN] nbytes, [IN] NULL, [IN/MOD] vc)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_server_handle_send_magic
MPIG_STATIC void mpig_cm_xio_server_handle_send_magic(
    const globus_xio_handle_t handle, const globus_result_t op_grc, globus_byte_t * const sbuf, globus_size_t const sbuf_len,
    const globus_size_t nbytes, const globus_xio_data_descriptor_t dd, void * const arg)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vc_t * const tmp_vc = (mpig_vc_t *) arg;
    struct mpig_cm_xio_vc * const tmp_vc_cm = &tmp_vc->cm.xio;
    bool_t tmp_vc_locked = FALSE;
    int warn = 0;
    int err = 0;
    bool_t failed;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_server_handle_send_magic);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_server_handle_send_magic);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"entering: tmp_vc=" MPIG_PTR_FMT ", handle=" MPIG_PTR_FMT ", op_grc=%d, sbuf=" MPIG_PTR_FMT ", sbuf_len=" MPIG_SIZE_FMT
	", nbytes=" MPIG_SIZE_FMT, (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) handle, op_grc, (MPIG_PTR_CAST) sbuf, sbuf_len,
	nbytes));

    MPIU_ERR_CHKANDSTMT2((op_grc), mpi_errno, MPI_ERR_OTHER, {warn++; goto fn_fail;}, "**globus|cm_xio|server_handle_send_magic",
	"**globus|cm_xio|server_handle_send_magic %p %s", tmp_vc, globus_error_print_chain(globus_error_peek(op_grc)));
    
    mpig_vc_mutex_lock(tmp_vc);
    tmp_vc_locked = TRUE;
    {
	MPIU_Assert(mpig_cm_xio_vc_get_state(tmp_vc) == MPIG_CM_XIO_VC_STATE_ACCEPT_SENDING_MAGIC);
	MPIU_Assert(nbytes == strlen(MPIG_CM_XIO_PROTO_ACCEPT_MAGIC));
	
	/* register a read operation to receive the open request control message */
	mpig_databuf_reset(tmp_vc_cm->msgbuf);
	tmp_vc_cm->msg_hdr_size = 0;
	mpig_cm_xio_register_read_vc_msgbuf(tmp_vc, mpig_cm_xio_msg_hdr_remote_sizeof_msg_size(tmp_vc),
	    mpig_databuf_get_size(tmp_vc_cm->msgbuf), mpig_cm_xio_server_handle_recv_open_req, &mpi_errno, &failed);
	MPIU_ERR_CHKANDSTMT1((failed), mpi_errno, MPI_ERR_INTERN, {err++; goto fn_fail;},
	    "**globus|cm_xio|server_reg_recv_open_req", "**globus|cm_xio|server_reg_recv_open_req %p", tmp_vc);
	
	mpig_cm_xio_vc_set_state(tmp_vc, MPIG_CM_XIO_VC_STATE_ACCEPT_RECEIVING_OPEN_REQ);
    }
    mpig_vc_mutex_unlock(tmp_vc);
    tmp_vc_locked = FALSE;

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"exiting: tmp_vc=" MPIG_PTR_FMT ", mpi_errno=0x%08x", (MPIG_PTR_CAST) tmp_vc, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_server_handle_send_magic);
    return;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	/* release the temp VC mutex if it is being held by the current context */
	mpig_vc_mutex_unlock_conditional(tmp_vc, (tmp_vc_locked));
    
	/* close the connection and destroy the temp VC object */
	mpig_cm_xio_tmp_vc_destroy(tmp_vc, &mpi_errno, &failed);
	MPIU_ERR_CHKANDSTMT1((failed), mpi_errno, MPI_ERR_INTERN, {err++;}, "**globus|cm_xio|tmp_vc_destroy",
	    "**globus|cm_xio|tmp_vc_destroy %p", tmp_vc);

	/* report the error through MPID_Progress_{poke,test,wait}() */
	if (err > 0) mpig_cm_xio_fault_handle_async_error(&mpi_errno);

	goto fn_return;
    }
}
/* mpig_cm_xio_server_handle_send_magic() */


/*
 * void mpig_cm_xio_server_handle_recv_open_req(
 *          [IN] handle, [IN] result, [IN] rbuf, [IN] rbuf_len, [IN] nbytes, [IN] NULL, [IN/MOD] vc)
 *
 * MT-NOTE: this function assumes that the VC mutex has already been acquired by this thread.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_server_handle_recv_open_req
MPIG_STATIC void mpig_cm_xio_server_handle_recv_open_req(
    const globus_xio_handle_t handle, const globus_result_t op_grc, globus_byte_t * const rbuf, const globus_size_t rbuf_len,
    const globus_size_t nbytes, const globus_xio_data_descriptor_t dd, void * const arg)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vc_t * const tmp_vc = (mpig_vc_t *) arg;
    struct mpig_cm_xio_vc * const tmp_vc_cm = &tmp_vc->cm.xio;
    bool_t tmp_vc_locked = FALSE;
    mpig_vc_t * real_vc = NULL;
    struct mpig_cm_xio_vc * real_vc_cm = NULL;
    bool_t real_vc_locked = FALSE;
    mpig_pg_t * pg = NULL;
    int warn = 0;
    int err = 0;
    bool_t failed;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_server_handle_recv_open_req);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_server_handle_recv_open_req);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"entering: tmp_vc=" MPIG_PTR_FMT ", handle=" MPIG_PTR_FMT ", op_grc=%d, rbuf=" MPIG_PTR_FMT ", rbuf_len=" MPIG_SIZE_FMT
	", nbytes=" MPIG_SIZE_FMT, (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) handle, op_grc, (MPIG_PTR_CAST) rbuf, rbuf_len,
	nbytes));

    MPIG_UNUSED_VAR(fcname);

    /* if the receive failed, then the connection is probably toast.  close the handle and free the temp VC. */
    MPIU_ERR_CHKANDSTMT2((op_grc), mpi_errno, MPI_ERR_OTHER, {warn++; goto fn_fail;},
	"**globus|cm_xio|server_handle_recv_open_req", "**globus|cm_xio|server_handle_recv_open_req %p %s",
	tmp_vc, globus_error_print_chain(globus_error_peek(op_grc)));
	
    mpig_vc_mutex_lock(tmp_vc);
    tmp_vc_locked = TRUE;
    {
	mpig_cm_xio_msg_types_t req_msg_type;
	mpig_cm_xio_conn_open_resp_t open_resp = MPIG_CM_XIO_CONN_OPEN_RESP_UNDEFINED;
	globus_xio_data_callback_t resp_callback_fn = NULL;
	char * pg_id = NULL;
	size_t pg_id_size = 0;
	int pg_size = -1;
	int pg_rank = -1;
	mpig_endian_t endian;
	int df;

	MPIU_Assert(mpig_cm_xio_vc_get_state(tmp_vc) == MPIG_CM_XIO_VC_STATE_ACCEPT_RECEIVING_OPEN_REQ);
	
	/* adjust the end of the message buffer to account for the number of bytes received */
	mpig_databuf_inc_eod(tmp_vc_cm->msgbuf, nbytes);

	/* if the message header size has not been determined, then extract the header size.  if the message buffer does not
	   contain the complete header, the register a read for the the remaining bytes.  assuming an error does not occur, the
	   entire header will be in the VC message buffer when the next callback occurs. */
	if (tmp_vc_cm->msg_hdr_size == 0)
	{
	    MPIU_Assert(nbytes >= mpig_cm_xio_msg_hdr_remote_sizeof_msg_size(tmp_vc));
	    mpig_cm_xio_msg_hdr_get_msg_size(tmp_vc, tmp_vc_cm->msgbuf, &tmp_vc_cm->msg_hdr_size);
	    tmp_vc_cm->msg_hdr_size -= mpig_cm_xio_msg_hdr_remote_sizeof_msg_size(tmp_vc);

	    if (mpig_databuf_get_remaining_bytes(tmp_vc_cm->msgbuf) < tmp_vc_cm->msg_hdr_size)
	    {
		MPIU_Size_t bytes_needed = tmp_vc_cm->msg_hdr_size - mpig_databuf_get_remaining_bytes(tmp_vc_cm->msgbuf);
		
		/* not all of the open request header was received.  try to get the remainder of it. */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		    "not all of the open request message was received; posting a read for the remaining bytes, tmp_vc="
		    MPIG_PTR_FMT  ", nbytes=%u", (MPIG_PTR_CAST) tmp_vc, (unsigned) bytes_needed));
	    
		mpig_cm_xio_register_read_vc_msgbuf(tmp_vc, bytes_needed, bytes_needed, mpig_cm_xio_server_handle_recv_open_req,
		    &mpi_errno, &failed);
		MPIU_ERR_CHKANDSTMT((failed), mpi_errno, MPI_ERR_INTERN, {err++; goto fn_fail;},
		    "**globus|cm_xio|server_reg_recv_open_req");

		mpig_vc_mutex_unlock(tmp_vc);
		tmp_vc_locked = FALSE;
		goto fn_return;
	    }
	}
	
	/* if all of the message header is not available, then report an incomplete message eror to the remote side */
	if (mpig_databuf_get_remaining_bytes(tmp_vc_cm->msgbuf) != tmp_vc_cm->msg_hdr_size)
	{
	    /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		"message received is incomplete: tmp_vc=" MPIG_PTR_FMT ", expected=%d, received=" MPIG_SIZE_FMT,
		(MPIG_PTR_CAST) tmp_vc, tmp_vc_cm->msg_hdr_size, mpig_databuf_get_remaining_bytes(tmp_vc_cm->msgbuf)));
	    
	    mpig_cm_xio_vc_set_state(tmp_vc, MPIG_CM_XIO_VC_STATE_ACCEPT_SENDING_OPEN_RESP_ERROR);
	    open_resp = MPIG_CM_XIO_CONN_OPEN_RESP_MSG_INCOMPLETE;
	    resp_callback_fn = mpig_cm_xio_server_handle_send_open_resp_error;
	    goto send_msg;
	    /* --END ERROR HANDLING-- */
	}
	
	/* get message type and verify it is an open request */
	mpig_cm_xio_msg_hdr_get_msg_type(tmp_vc, tmp_vc_cm->msgbuf, &req_msg_type);
	if (req_msg_type != MPIG_CM_XIO_MSG_TYPE_OPEN_REQ)
	{
	    /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		"message received is not an open request: tmp_vc=" MPIG_PTR_FMT ", msg_type=%d",
		(MPIG_PTR_CAST) tmp_vc, (int) req_msg_type));
	    
	    mpig_cm_xio_vc_set_state(tmp_vc, MPIG_CM_XIO_VC_STATE_ACCEPT_SENDING_OPEN_RESP_ERROR);
	    open_resp = MPIG_CM_XIO_CONN_OPEN_RESP_MSG_TYPE_BAD;
	    resp_callback_fn = mpig_cm_xio_server_handle_send_open_resp_error;
	    goto send_msg;
	    /* --END ERROR HANDLING-- */
	}

	/* unpack information from message header */
	mpig_cm_xio_msg_hdr_get_endian(tmp_vc, tmp_vc_cm->msgbuf, &endian);
	mpig_cm_xio_msg_hdr_get_df(tmp_vc, tmp_vc_cm->msgbuf, &df);
	mpig_cm_xio_vc_set_endian(tmp_vc, endian);
	mpig_cm_xio_vc_set_data_format(tmp_vc, df);
	mpig_cm_xio_msg_hdr_get_rank(tmp_vc, tmp_vc_cm->msgbuf, &pg_size);
	mpig_cm_xio_msg_hdr_get_rank(tmp_vc, tmp_vc_cm->msgbuf, &pg_rank);
	pg_id = (char *) mpig_databuf_get_pos_ptr(tmp_vc_cm->msgbuf);
	pg_id_size = strlen(pg_id) + 1;
	mpig_databuf_inc_pos(tmp_vc_cm->msgbuf, pg_id_size);
	
	/* verify that the message buffer is now empty.  extra data should never be received. */
	if (mpig_databuf_get_remaining_bytes(tmp_vc_cm->msgbuf) != 0)
	{
	    /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		"message received contains extra data: tmp_vc=" MPIG_PTR_FMT ", bytes=%d",
		(MPIG_PTR_CAST) tmp_vc, (int) mpig_databuf_get_remaining_bytes(tmp_vc_cm->msgbuf)));
	    
	    mpig_cm_xio_vc_set_state(tmp_vc, MPIG_CM_XIO_VC_STATE_ACCEPT_SENDING_OPEN_RESP_ERROR);
	    open_resp = MPIG_CM_XIO_CONN_OPEN_RESP_MSG_TOO_LONG;
	    resp_callback_fn = mpig_cm_xio_server_handle_send_open_resp_error;
	    goto send_msg;
	    /* --END ERROR HANDLING-- */
	}

	/* given the PG ID and rank, acquire a reference to the the process group object (creating it if necessary).  the
	   reference is associated with the temp VC. */
	mpig_pg_acquire_ref_locked(pg_id, pg_size, &pg, &mpi_errno, &failed);
	MPIU_ERR_CHKANDSTMT2((failed), mpi_errno, MPI_ERR_OTHER, {warn++; goto fn_fail;}, "**globus|pg_acquire_ref",
	    "**globus|pg_acquire_ref %s %d", pg_id, pg_size);
	{
	    /* set the PG information fields in the temp VC */
	    mpig_vc_set_pg_info(tmp_vc, pg, pg_rank);

	    /* get the real VC using the process group and the rank */
	    mpig_pg_get_vc(pg, pg_rank, &real_vc);
	    real_vc_cm = &real_vc->cm.xio;
	    
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		"open request received: pg_id=%s, pg_rank=%d, tmp_vc=" MPIG_PTR_FMT ", real_vc=" MPIG_PTR_FMT,
		pg_id, pg_rank, (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) real_vc));
	    
	    mpig_vc_mutex_lock(real_vc);
	    real_vc_locked = TRUE;
	    {
		bool_t was_inuse;
		
		/* verify that the real VC has been initialized by the XIO CM.  if it hasn't been initialized, then do so; also,
		   increment the PG reference count to indicate that a new VC is active. */
		if (real_vc->cm_type == MPIG_CM_TYPE_UNDEFINED)
		{
		    mpig_cm_xio_vc_construct(real_vc);
		    real_vc_cm->df = tmp_vc_cm->df;
		    mpig_cm_xio_vc_set_endian(real_vc, mpig_cm_xio_vc_get_endian(tmp_vc));
		}

		/* adjust the real VC reference count to compensate for the internal reference; the CM XIO increment routine is
		   used to avoid a potential feedback loop should a routine for the VC function table ever be implemented */
		mpig_cm_xio_vc_inc_ref_count(real_vc, &was_inuse);
		if (was_inuse == FALSE)
		{
		    mpig_pg_inc_ref_count(pg);
		}
	    }
	    /* mpig_vc_mutex_unlock(real_vc); real_vc_locked = FALSE; -- real VC mutex is left locked for next block of code */
	}
	mpig_pg_mutex_unlock(pg);

	/* mpig_vc_mutex_lock(real_vc); -- real VC mutex is locked in previous block of code */
	{
	    if (real_vc->cm_type != MPIG_CM_TYPE_XIO)
	    {
		/* --BEGIN ERROR HANDLING-- */
		/* if the real VC is managed by another CM then send an error to the remote process */
		mpig_cm_xio_vc_set_state(tmp_vc, MPIG_CM_XIO_VC_STATE_ACCEPT_SENDING_OPEN_RESP_ERROR);
		open_resp = MPIG_CM_XIO_CONN_OPEN_RESP_VC_NOT_XIO;
		resp_callback_fn = mpig_cm_xio_server_handle_send_open_resp_error;
		/* --END ERROR HANDLING-- */
	    }
	    else if (mpig_cm_xio_vc_is_unconnected(real_vc))
	    {
		/*
		 * no head-to-head condition.  send an ACK message to solidify the connection.  temporary vc will be destroyed
		 * once the ack message has been sent and any state in temporary VC has been transfer to the real VC.  if any
		 * sends are enqueued in the real VC, they will be started at that time.
		 */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		    "real VC unconnected; ACKing open req: tmp_vc=" MPIG_PTR_FMT ", real_vc=" MPIG_PTR_FMT,
		    (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) real_vc));
		
		mpig_cm_xio_vc_set_state(real_vc, MPIG_CM_XIO_VC_STATE_ACCEPTING);
		
		mpig_cm_xio_vc_set_state(tmp_vc, MPIG_CM_XIO_VC_STATE_ACCEPT_SENDING_OPEN_RESP_ACK);
		open_resp = MPIG_CM_XIO_CONN_OPEN_RESP_ACK;
		resp_callback_fn = mpig_cm_xio_server_handle_send_open_resp_ack;
	    }
	    else if (mpig_cm_xio_vc_is_connecting(real_vc))
	    {
		/*
		 * head-to-head in progress.  big guy wins.  send NAK to lesser process.
		 *
		 * MT-RC-NOTE: the ID in the progress group is initialized at startup at is static from that point forward.
		 * therefore it is unnecessary to acquire updates to process group just to get the ID.  any updates would have
		 * been acquired when the PG or real VC mutexes were locked above.
		 */
		/* XXX: macroize voting algorithm */
		if (MPIG_CM_XIO_CONN_VOTE(pg_id, pg_rank))
		{
		    /* see the unconnected case above for the process */
		    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
			"real VC connecting; remote process wins; ACKing open req: tmp_vc=" MPIG_PTR_FMT ", real_vc=" MPIG_PTR_FMT,
			(MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) real_vc));
		    
		    mpig_cm_xio_vc_set_state(real_vc, MPIG_CM_XIO_VC_STATE_ACCEPTING);
		    
		    mpig_cm_xio_vc_set_state(tmp_vc, MPIG_CM_XIO_VC_STATE_ACCEPT_SENDING_OPEN_RESP_ACK);
		    open_resp = MPIG_CM_XIO_CONN_OPEN_RESP_ACK;
		    resp_callback_fn = mpig_cm_xio_server_handle_send_open_resp_ack;
		}
		else
		{
		    /* the local process wins the connection race.  the temp VC associated with the local process' outgoing
		       connection is the valid one.  tell the remote process to teardown the VC associated associate with his
		       outgoing connection.  the callback will close specified below will close the local process' side of the
		       connection and dstroy the temp VC associated with it. */
		    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
			"real VC connecting; local process wins; NAKing open req: tmp_vc=" MPIG_PTR_FMT ", real_vc=" MPIG_PTR_FMT,
			(MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) real_vc));
		    mpig_cm_xio_vc_set_state(tmp_vc, MPIG_CM_XIO_VC_STATE_ACCEPT_SENDING_OPEN_RESP_NAK);
		    open_resp = MPIG_CM_XIO_CONN_OPEN_RESP_NAK;
		    resp_callback_fn = mpig_cm_xio_server_handle_send_open_resp_nak;
		}
	    }
	    else if (mpig_cm_xio_vc_is_connected(real_vc))
	    {
		if (!MPIG_CM_XIO_CONN_VOTE(pg_id, pg_rank))
		{
		    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
			"real VC connected; local process won; NAKing open req: tmp_vc=" MPIG_PTR_FMT ", real_vc=" MPIG_PTR_FMT,
			(MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) real_vc));
		    mpig_cm_xio_vc_set_state(tmp_vc, MPIG_CM_XIO_VC_STATE_ACCEPT_SENDING_OPEN_RESP_NAK);
		    open_resp = MPIG_CM_XIO_CONN_OPEN_RESP_NAK;
		    resp_callback_fn = mpig_cm_xio_server_handle_send_open_resp_nak;
		}
		else
		{   /* --BEGIN ERROR HANDLING-- */
		    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
			"ERROR: real VC connected; winner connecting again: tmp_vc=" MPIG_PTR_FMT ", real_vc=" MPIG_PTR_FMT,
			(MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) real_vc));
		    mpig_cm_xio_vc_set_state(tmp_vc, MPIG_CM_XIO_VC_STATE_ACCEPT_SENDING_OPEN_RESP_ERROR);
		    open_resp = MPIG_CM_XIO_CONN_OPEN_RESP_VC_CONNECTED;
		    resp_callback_fn = mpig_cm_xio_server_handle_send_open_resp_error;
		}   /* --END ERROR HANDLING-- */
	    }
	    else if (mpig_cm_xio_vc_is_disconnecting(real_vc))
	    {
		/* --BEGIN ERROR HANDLING-- */
		/*
		 * MPI-2-FIXME: it may be possible for a disconnect followed by a reconnect to cause a new connection to be
		 * requested from the remote process while the current connection is executing the close protocol.  this require
		 * further thought.  should it prove to be a problem, it may be solvable by using a temp VC during the close.
		 */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		    "ERROR: real VC is disconnecting: tmp_vc=" MPIG_PTR_FMT ", real_vc=" MPIG_PTR_FMT,
		    (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) real_vc));
		mpig_cm_xio_vc_set_state(tmp_vc, MPIG_CM_XIO_VC_STATE_ACCEPT_SENDING_OPEN_RESP_ERROR);
		open_resp = MPIG_CM_XIO_CONN_OPEN_RESP_VC_DISCONNECTING;
		resp_callback_fn = mpig_cm_xio_server_handle_send_open_resp_error;
		/* --END ERROR HANDLING-- */
	    }
	    else if (mpig_cm_xio_vc_has_failed(real_vc))
	    {
		/* --BEGIN ERROR HANDLING-- */
		/*
		 * the real VC has already failed.  however, the remote process may not be aware of this.  for example, if a
		 * connection request from the local process never made it to the remote process, then the local process would
		 * have marked the local VC object as failed, but the VC object at the remote would still be in the unconnected
		 * state.  therefore, we reply with a failure response to make the state of the two objects consistent.
		 */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		    "real VC has already failed; sending failure message: tmp_vc=" MPIG_PTR_FMT ", real_vc=" MPIG_PTR_FMT,
		    (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) real_vc));
		mpig_cm_xio_vc_set_state(tmp_vc, MPIG_CM_XIO_VC_STATE_ACCEPT_SENDING_OPEN_RESP_ERROR);
		open_resp = MPIG_CM_XIO_CONN_OPEN_RESP_VC_FAILED;
		resp_callback_fn = mpig_cm_xio_server_handle_send_open_resp_error;
		/* --END ERROR HANDLING-- */
	    }
	    else
	    {
		/* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		    "ERROR: real VC is in an unknown state: tmp_vc=" MPIG_PTR_FMT ", real_vc=" MPIG_PTR_FMT "state=%d",
		    (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) real_vc, (int) mpig_cm_xio_vc_get_state(real_vc)));
		MPIU_Assertp(FALSE && "VC is in state is corrupt!")
		    /* --END ERROR HANDLING-- */
		    }
	}
	mpig_vc_mutex_unlock(real_vc);
	real_vc_locked = FALSE;

      send_msg:
	if (open_resp != MPIG_CM_XIO_MSG_TYPE_UNDEFINED)
	{
	    /* construct the open response message */
	    mpig_databuf_reset(tmp_vc_cm->msgbuf);
	    mpig_cm_xio_msg_hdr_put_init(tmp_vc, tmp_vc_cm->msgbuf);
	    mpig_cm_xio_msg_hdr_put_msg_type(tmp_vc, tmp_vc_cm->msgbuf, MPIG_CM_XIO_MSG_TYPE_OPEN_RESP);
	    mpig_cm_xio_msg_hdr_put_conn_open_resp(tmp_vc, tmp_vc_cm->msgbuf, open_resp);
	    mpig_cm_xio_msg_hdr_put_msg_size(tmp_vc, tmp_vc_cm->msgbuf);

	    /* send the open respoinse message */
	    mpig_cm_xio_register_write_vc_msgbuf(tmp_vc, resp_callback_fn, &mpi_errno, &failed);
	    MPIU_ERR_CHKANDSTMT3((failed), mpi_errno, MPI_ERR_INTERN, {err++; goto fn_fail;}, "**globus|cm_xio|reg_send_open_resp",
		"**globus|cm_xio|reg_send_open_resp %p %s %d", tmp_vc, (pg_id != NULL) ? pg_id : "(unknown)", pg_rank);
	}
    }
    mpig_vc_mutex_unlock(tmp_vc);
    tmp_vc_locked = FALSE;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"exiting: tmp_vc=" MPIG_PTR_FMT "real_vc=" MPIG_PTR_FMT ", mpi_errno=0x%08x",
	(MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) real_vc, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_server_handle_recv_open_req);
    return;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	/* release the temp VC mutex if it is being held by the current context */
	mpig_vc_mutex_unlock_conditional(tmp_vc, (tmp_vc_locked));
    
	/* close the connection and destroy the temp VC object, releasing the associated PG reference */
	mpig_cm_xio_tmp_vc_destroy(tmp_vc, &mpi_errno, &failed);
	MPIU_ERR_CHKANDSTMT1((failed), mpi_errno, MPI_ERR_INTERN, {err++;}, "**globus|cm_xio|tmp_vc_destroy",
	    "**globus|cm_xio|tmp_vc_destroy %p", tmp_vc);

	/* set the state of the real VC back to unconnected; release the reference to the VC (and the reference to the PG if the
	   VC reference count reaches zero) */
	if (real_vc != NULL)
	{
	    mpig_vc_mutex_lock_conditional(real_vc, (!real_vc_locked));
	    {
		if (mpig_cm_xio_vc_get_state(real_vc) == MPIG_CM_XIO_VC_STATE_ACCEPTING && !mpig_cm_xio_sendq_empty(real_vc))
		{
		    mpig_cm_xio_fault_handle_async_vc_error(real_vc, &mpi_errno);
		    err = 0;
		}
		else
		{
		    mpig_cm_xio_vc_set_state(real_vc, MPIG_CM_XIO_VC_STATE_UNCONNECTED);
		}
	    }
	    mpig_vc_mutex_unlock(real_vc);

	    mpig_vc_release_ref(real_vc, &mpi_errno, &failed);
	    MPIU_ERR_CHKANDSTMT1((failed), mpi_errno, MPI_ERR_OTHER, {err++;}, "**globus|vc_release_ref",
		"**globus|vc_release_ref %p", real_vc);
	}

	/* report the error(s) through MPID_Progress_{poke,test,wait}() */
	if (err > 0) mpig_cm_xio_fault_handle_async_error(&mpi_errno);

	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_server_handle_recv_open_req() */


/*
 * void mpig_cm_xio_server_handle_send_open_resp_ack(
 *          [IN] handle, [IN] result, [IN] sbuf, [IN] sbuf_len, [IN] nbytes, [IN] NULL, [IN/MOD] vc)
 *
 * this handler gets invoked after sending an OPEN_RESP_ACK message using the temporary vc.  this message is confirmation that
 * the process pair is to use the connection associated with temp VC as the connection for the real VC.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_server_handle_send_open_resp_ack
MPIG_STATIC void mpig_cm_xio_server_handle_send_open_resp_ack(
    const globus_xio_handle_t handle, const globus_result_t op_grc, globus_byte_t * const sbuf, const globus_size_t sbuf_len,
    const globus_size_t nbytes, const globus_xio_data_descriptor_t dd, void * const arg)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vc_t * const tmp_vc = (mpig_vc_t *) arg;
    struct mpig_cm_xio_vc * const tmp_vc_cm = &tmp_vc->cm.xio;
    mpig_vc_t * real_vc = NULL;
    struct mpig_cm_xio_vc * real_vc_cm = NULL;
    bool_t real_vc_locked = FALSE;
    mpig_pg_t * pg;
    int pg_rank;
    int warn = 0;
    int err = 0;
    bool_t failed;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_server_handle_send_open_resp_ack);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_server_handle_send_open_resp_ack);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"entering: tmp_vc=" MPIG_PTR_FMT ", handle=" MPIG_PTR_FMT ", op_grc=%d, sbuf=" MPIG_PTR_FMT ", sbuf_len=" MPIG_SIZE_FMT
	", nbytes=" MPIG_SIZE_FMT, (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) handle, op_grc, (MPIG_PTR_CAST) sbuf, sbuf_len,
	nbytes));

    /* if send operation completed successfully, clear the handle field in temp VC so that the handle is not closed when the temp
       VC is destroyed.  in addition, extract the the the pg and pg_rank fields from the temp VC so that we can get a pointer to
       the VC. */
    mpig_vc_mutex_lock(tmp_vc);
    {
	MPIU_Assert(mpig_cm_xio_vc_get_state(tmp_vc) == MPIG_CM_XIO_VC_STATE_ACCEPT_SENDING_OPEN_RESP_ACK);

	if (op_grc == GLOBUS_SUCCESS) tmp_vc_cm->handle = NULL;
	pg = tmp_vc->pg;
	pg_rank = tmp_vc->pg_rank;
    }
    mpig_vc_mutex_unlock(tmp_vc);

    /* now that all useful information has been extracted from the temp VC, close the connection and destroy the temp VC object,
       releasing the reference to the PG */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
	"destroying the temp VC: tmp_vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) tmp_vc));
    mpig_cm_xio_tmp_vc_destroy(tmp_vc, &mpi_errno, &failed);
    MPIU_ERR_CHKANDSTMT1((failed), mpi_errno, MPI_ERR_INTERN, {err++;}, "**globus|cm_xio|tmp_vc_destroy",
	"**globus|cm_xio|tmp_vc_destroy %p", tmp_vc);

    /* using the pg and pg rank acquired above, get the real VC */
    mpig_pg_rc_acq(pg, TRUE);
    {
	mpig_pg_get_vc(pg, pg_rank, &real_vc);
	real_vc_cm = &real_vc->cm.xio;
    }
    mpig_pg_rc_rel(pg, FALSE);

    mpig_vc_mutex_lock(real_vc);
    real_vc_locked = TRUE;
    {
	/* check if an error occurred while sending the OPEN_RESP_ACK message.  if the send failed, and we have pending messages,
	   then the connection should be marked as failed. */
	if (op_grc)
	{
	    MPIU_ERR_SETANDSTMT2(mpi_errno, MPI_ERR_OTHER, {warn++;}, "**globus|cm_xio|send_handle_open_resp_ack_msg",
		"**globus|cm_xio|send_handle_open_resp_ack_msg %p %s",
		tmp_vc, globus_error_print_chain(globus_error_peek(op_grc)));

	    if (!mpig_cm_xio_sendq_empty(real_vc))
	    {
		/* [BRT] FT: should we attempt to reconnect??? */
		mpig_cm_xio_vc_set_state(real_vc, MPIG_CM_XIO_VC_STATE_FAILED_CONNECTING);
		mpig_cm_xio_client_connect(real_vc, &mpi_errno, &failed);
		MPIU_ERR_CHKANDSTMT((failed), mpi_errno, MPI_ERR_OTHER, {err++;}, "**globus|cm_xio|connect");
	    }
	    else
	    {
		mpig_cm_xio_vc_set_state(real_vc, MPIG_CM_XIO_VC_STATE_UNCONNECTED);
	    }

	    goto fn_fail;
	}

	/* set the handle on the real VC, and change the state to connected */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
	    "change state of real VC to connected; starting communication: real_vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) real_vc));
	real_vc_cm->handle = handle;
	mpig_cm_xio_vc_set_state(real_vc, MPIG_CM_XIO_VC_STATE_CONNECTED);
	mpig_cm_xio_vc_list_add(real_vc);
	
	/* start receiving messages on the real VC.  note: it was not necesary to transfer the receive buffer from the temp VC to
	   the real VC since we know that the buffer was depleted by mpig_cm_xio_server_handle_recv_open_req(). */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VC | MPIG_DEBUG_LEVEL_VCCM,
	    "connection established; starting communication engines: vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) real_vc));
		
	mpig_cm_xio_recv_next_msg(real_vc, &mpi_errno, &failed);
	MPIU_ERR_CHKANDSTMT((failed), mpi_errno, MPI_ERR_INTERN, {err++; goto fn_fail;}, "**globus|xio_cm|recv_next_msg");

	/* if the real VC has any sends queued up, start send them */
	mpig_cm_xio_send_next_sreq(real_vc, &mpi_errno, &failed);
	MPIU_ERR_CHKANDSTMT((failed), mpi_errno, MPI_ERR_INTERN, {err++; goto fn_fail;}, "**globus|xio_cm|send_next_sreq");

	/* we must release the internal reference to the VC; however, because the connection may not a local communicator
	   referencing it yet, we do not want to release the VC's reference to the PG or cause the connection to close before the
	   remote process can perform whatever actions it intends to perform.  therefore, we use mpig_cm_xio_vc_dec_ref_count()
	   instead of mpig_vc_release_ref() or mpig_vc_dec_ref_count().  a correct MPI will eventually result in a communicator
	   referencing the VC, so it is not necessary to detect this situation in the disconnect code. FT-FIXME: however, an
	   incorrect program, or a failure during the formation of a communicator, will cause the program to hang in finalize. */
	{
	    bool_t inuse;
	    
	    mpig_cm_xio_vc_dec_ref_count(real_vc, &inuse);
	}
    }
    mpig_vc_mutex_unlock(real_vc);
    real_vc_locked = FALSE;

    if (warn > 0) goto fn_fail;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"exiting: tmp_vc=" MPIG_PTR_FMT ", real_vc=" MPIG_PTR_FMT ", mpi_errno=0x%08x",
	(MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) real_vc, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_server_handle_send_open_resp_ack);
    return;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	MPIU_Assertp(FALSE && "XXX: FIX ERROR HANDLING CODE"); /* [BRT] */

#if 0
	if (err > 0)
	{
	    mpig_vc_mutex_lock_conditional(real_vc, (!real_vc_locked));
	    {
		if (mpig_cm_xio_vc_get_state(real_vc) == MPIG_CM_XIO_VC_STATE_CONNECTED)
		{
		}
	    }
	    mpig_vc_mutex_unlock(real_vc);
	    
	    /* release the internal reference to the VC, and the PG if the VC reference count reaches zero */
	    mpig_vc_release_ref(real_vc, &mpi_errno, &failed);
	    MPIU_ERR_CHKANDSTMT1((failed), mpi_errno, MPI_ERR_OTHER, {err++;}, "**globus|vc_release_ref",
		"**globus|vc_release_ref %p", real_vc);
    
	    /* report the error through MPID_Progress_{poke,test,wait}() */
	    mpig_cm_xio_fault_handle_async_vc_error(real_vc, &mpi_errno);
	}
	else
#endif
	{
	    /* release the real VC mutex if it is being held by the current context */
	    mpig_vc_mutex_unlock_conditional(real_vc, (real_vc_locked));
	}
	
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_server_handle_send_open_resp_ack() */


/*
 * void mpig_cm_xio_server_handle_send_open_resp_nak(
 *          [IN] handle, [IN] result, [IN] sbuf, [IN] sbuf_len, [IN] nbytes, [IN] NULL, [IN/MOD] vc)
 *
 * this handler gets invoked after sending an OPEN_RESP_NAK message using the temporary vc
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_server_handle_send_open_resp_nak
MPIG_STATIC void mpig_cm_xio_server_handle_send_open_resp_nak(
    const globus_xio_handle_t handle, const globus_result_t op_grc, globus_byte_t * const sbuf, const globus_size_t sbuf_len,
    const globus_size_t nbytes, const globus_xio_data_descriptor_t dd, void * const arg)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vc_t * const tmp_vc = (mpig_vc_t *) arg;
    mpig_vc_t * real_vc = NULL;
    mpig_pg_t * pg = NULL;
    int pg_rank;
    bool_t failed;
    int err = 0;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_handle_send_open_resp_nak);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_handle_send_open_resp_nak);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"entering: tmp_vc=" MPIG_PTR_FMT ", handle=" MPIG_PTR_FMT ", op_grc=%d, sbuf=" MPIG_PTR_FMT ", sbuf_len=" MPIG_SIZE_FMT
	", nbytes=" MPIG_SIZE_FMT, (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) handle, op_grc, (MPIG_PTR_CAST) sbuf, sbuf_len,
	nbytes));

    MPIU_ERR_CHKANDSTMT1((op_grc), mpi_errno, MPI_ERR_OTHER, {err++;}, "**globus|cm_xio|server_handle_send_open_resp_nak",
	"**globus|cm_xio|server_handle_send_open_resp_nak %s", globus_error_print_chain(globus_error_peek(op_grc)));
    
    /* extract the the the pg and pg_rank fields from the temp VC so that we can get a pointer to the VC */
    mpig_vc_mutex_lock(tmp_vc);
    {	 
	MPIU_Assert(mpig_cm_xio_vc_get_state(tmp_vc) == MPIG_CM_XIO_VC_STATE_ACCEPT_SENDING_OPEN_RESP_NAK);
	pg = tmp_vc->pg;
	pg_rank = tmp_vc->pg_rank;
    }
    mpig_vc_mutex_unlock(tmp_vc);

    /* close the connection and destroy the temp VC object, releasing the reference to the PG */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
	"closing and destroying the temp VC: tmp_vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) tmp_vc));
    mpig_cm_xio_tmp_vc_destroy(tmp_vc, &mpi_errno, &failed);
    MPIU_ERR_CHKANDSTMT1((failed), mpi_errno, MPI_ERR_OTHER, {err++;}, "**globus|cm_xio|tmp_vc_destroy",
	"**globus|cm_xio|tmp_vc_destroy %p", tmp_vc);
    
    /* using the pg and pg rank acquired above, get the real VC */
    mpig_pg_rc_acq(pg, TRUE);
    {
	mpig_pg_get_vc(pg, pg_rank, &real_vc);
    }
    mpig_pg_rc_rel(pg, FALSE);

    /* release the internal reference to the real VC, and the PG if the VC reference count reaches zero */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
	"releasing the internal reference to the real VC: tmp_vc=" MPIG_PTR_FMT ",real_vc=" MPIG_PTR_FMT,
	(MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) real_vc));
    mpig_vc_release_ref(real_vc, &mpi_errno, &failed);
    MPIU_ERR_CHKANDSTMT1((failed), mpi_errno, MPI_ERR_OTHER, {err++;}, "**globus|vc_release_ref",
	"**globus|vc_release_ref %p", real_vc);

    if (err > 0) goto fn_fail;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"exiting: tmp_vc=" MPIG_PTR_FMT ", real_vc=" MPIG_PTR_FMT ", mpi_errno=0x%08x",
	(MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) real_vc, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_handle_send_open_resp_nak);
    return;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	MPIU_Assertp(FALSE && "XXX: FIX ERROR HANDLING CODE"); /* [BRT] */
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_handle_send_open_resp_nak() */


/*
 * void mpig_cm_xio_server_handle_send_open_resp_error(
 *          [IN] handle, [IN] result, [IN] sbuf, [IN] sbuf_len, [IN] nbytes, [IN] NULL, [IN/MOD] vc)
 *
 * this handler gets invoked after sending an error message to the remote (connecting) process using the temporary vc.
 * regardless of the outcome, success or failure, this routine has no affect on the real VC.  it's only purpose is to destroy the
 * temp VC.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_server_handle_send_open_resp_error
MPIG_STATIC void mpig_cm_xio_server_handle_send_open_resp_error(
    const globus_xio_handle_t handle, const globus_result_t op_grc, globus_byte_t * const sbuf, const globus_size_t sbuf_len,
    const globus_size_t nbytes, const globus_xio_data_descriptor_t dd, void * const arg)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vc_t * const tmp_vc = (mpig_vc_t *) arg;
    mpig_vc_t * real_vc = NULL;
    mpig_pg_t * pg = NULL;
    int pg_rank;
    bool_t failed;
    int err = 0;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_handle_send_open_resp_error);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_handle_send_open_resp_error);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"entering: tmp_vc=" MPIG_PTR_FMT ", handle=" MPIG_PTR_FMT ", op_grc=%d, sbuf=" MPIG_PTR_FMT ", sbuf_len=" MPIG_SIZE_FMT
	", nbytes=" MPIG_SIZE_FMT, (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) handle, op_grc, (MPIG_PTR_CAST) sbuf, sbuf_len,
	nbytes));

    MPIU_ERR_CHKANDSTMT1((op_grc), mpi_errno, MPI_ERR_OTHER, {err++;}, "**globus|cm_xio|server_handle_send_open_resp_error",
	"**globus|cm_xio|server_handle_send_open_resp_error %s", globus_error_print_chain(globus_error_peek(op_grc)));
    
    /* extract the the the pg and pg_rank fields from the temp VC so that we can get a pointer to the VC */
    mpig_vc_mutex_lock(tmp_vc);
    {	 
	MPIU_Assert(mpig_cm_xio_vc_get_state(tmp_vc) == MPIG_CM_XIO_VC_STATE_ACCEPT_SENDING_OPEN_RESP_ERROR);
	pg = tmp_vc->pg;
	pg_rank = tmp_vc->pg_rank;
    }
    mpig_vc_mutex_unlock(tmp_vc);

    /* close the connection and destroy the temp VC object, releasing the reference to the PG */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
	"closing and destroying the temp VC: tmp_vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) tmp_vc));
    mpig_cm_xio_tmp_vc_destroy(tmp_vc, &mpi_errno, &failed);
    MPIU_ERR_CHKANDSTMT1((failed), mpi_errno, MPI_ERR_OTHER, {err++;}, "**globus|cm_xio|tmp_vc_destroy",
	"**globus|cm_xio|tmp_vc_destroy %p", tmp_vc);

    if (pg != NULL)
    {
	/* using the pg and pg rank acquired above, get the real VC */
	mpig_pg_rc_acq(pg, TRUE);
	{
	    mpig_pg_get_vc(pg, pg_rank, &real_vc);
	}
	mpig_pg_rc_rel(pg, FALSE);

	/* release the internal reference to the real VC, and the PG if the VC reference count reaches zero */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
	    "releasing the internal reference to the real VC: tmp_vc=" MPIG_PTR_FMT ",real_vc=" MPIG_PTR_FMT,
	    (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) real_vc));
	mpig_vc_release_ref(real_vc, &mpi_errno, &failed);
	MPIU_ERR_CHKANDSTMT1((failed), mpi_errno, MPI_ERR_OTHER, {err++;}, "**globus|vc_release_ref",
	    "**globus|vc_release_ref %p", real_vc);
    }
    
    if (err > 0) goto fn_fail;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"exiting: tmp_vc=" MPIG_PTR_FMT ", real_vc=" MPIG_PTR_FMT ", mpi_errno=0x%08x",
	(MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) real_vc, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_handle_send_open_resp_error);
    return;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	MPIU_Assertp(FALSE && "XXX: FIX ERROR HANDLING CODE"); /* [BRT] */
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_server_handle_send_open_resp_error */

#endif /* MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS */
/**********************************************************************************************************************************
				       END SERVER SIDE (LISTENER) CONNECTION ESTABLISHMENT
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					   BEGIN CLIENT SIDE CONNECTION ESTABLISHMENT
**********************************************************************************************************************************/
#if !defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS)

MPIG_STATIC void mpig_cm_xio_client_connect(mpig_vc_t * vc, int * mpi_errno_p, bool_t * failed_p);

MPIG_STATIC void mpig_cm_xio_client_handle_open(globus_xio_handle_t handle, globus_result_t op_grc, void * arg);

MPIG_STATIC void mpig_cm_xio_client_handle_send_magic(
    globus_xio_handle_t handle, globus_result_t op_grc, globus_byte_t * rbuf, globus_size_t rbuf_len, globus_size_t nbytes,
    globus_xio_data_descriptor_t dd, void * arg);

MPIG_STATIC void mpig_cm_xio_client_handle_recv_magic(
    globus_xio_handle_t handle, globus_result_t op_grc, globus_byte_t * rbuf, globus_size_t rbuf_len, globus_size_t nbytes,
    globus_xio_data_descriptor_t dd, void * arg);

MPIG_STATIC void mpig_cm_xio_client_handle_send_open_req(
    globus_xio_handle_t handle, globus_result_t op_grc, globus_byte_t * rbuf, globus_size_t rbuf_len, globus_size_t nbytes,
    globus_xio_data_descriptor_t dd, void * arg);

MPIG_STATIC void mpig_cm_xio_client_handle_recv_open_resp(
    globus_xio_handle_t handle, globus_result_t op_grc, globus_byte_t * rbuf, globus_size_t rbuf_len, globus_size_t nbytes,
    globus_xio_data_descriptor_t dd, void * arg);


#else /* defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS) */


/*
 * void mpig_cm_xio_client_connect([IN/MOD] vc, [IN/OUT] mpi_errno, [OUT] failed)
 *
 * MT-NOTE: this function assumes that the VC mutex is held when the function is called.  furthermore, the function may only be
 * called when a connection is not established or knowingly in the process of being established.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_client_connect
MPIG_STATIC void mpig_cm_xio_client_connect(mpig_vc_t * const real_vc, int * const mpi_errno_p, bool_t * const failed_p)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    struct mpig_cm_xio_vc * const real_vc_cm = &real_vc->cm.xio;
    mpig_vc_t * tmp_vc = NULL;
    struct mpig_cm_xio_vc * tmp_vc_cm = NULL;
    bool_t tmp_vc_locked = FALSE;
    mpig_pg_t * const pg = mpig_vc_get_pg(real_vc);
    const int pg_rank = mpig_vc_get_pg_rank(real_vc);
    globus_result_t grc;
    bool_t failed;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_client_connect);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_client_connect);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"entering: real_vc=" MPIG_PTR_FMT ", mpi_errno=0x%08x", (MPIG_PTR_CAST) real_vc, *mpi_errno_p));
    *failed_p = FALSE;

    MPIG_UNUSED_VAR(fcname);

    /* increment the reference count on the real VC to reflect the internal reference to it.  this is to prevent the real VC from
       disappearing before the connection completes.  (this could occur if the local process did a send, cancelled that send, and
       then called MPI_{Finalize,Comm_free,Comm_disconnect}. */
    {
	bool_t was_inuse;
	mpig_cm_xio_vc_inc_ref_count(real_vc, &was_inuse);
	if (was_inuse == FALSE)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_PT2PT,
		"ERROR: attempt to connect a VC with no references; likely a dangling pointer, vc=" MPIG_PTR_FMT,
		(MPIG_PTR_CAST) real_vc));
	    MPIU_ERR_SETFATALANDJUMP1(*mpi_errno_p, MPI_ERR_OTHER, "**globus|vc_ptr_bad", "**globus|vc_ptr_bad %p", real_vc);
	}   /* --END ERROR HANDLING-- */
    }
    
    /* change the state of the real VC to 'connecting' so that any additional send operations know not to initiate another
       register connect.  this also allows the the code that handles incoming connections to detect a head-to-head connection
       scenerio. */
    mpig_cm_xio_vc_set_state(real_vc, MPIG_CM_XIO_VC_STATE_CONNECTING);

    /* allocate and initialize a temporary VC for use during connection establishment. */
    tmp_vc = (mpig_vc_t *) MPIU_Malloc(sizeof(mpig_vc_t));
    MPIU_ERR_CHKANDJUMP1((tmp_vc == NULL), *mpi_errno_p, MPI_ERR_OTHER, "**nomem", "**nomem %s",
	"temp VC for an outgoing connection");
    tmp_vc_cm = &tmp_vc->cm.xio;
    mpig_vc_construct(tmp_vc);
    mpig_cm_xio_vc_construct(tmp_vc);

    /* add the temp VC to the list of active VCs */
    mpig_cm_xio_vc_list_add(tmp_vc);

    /* pg and pg_rank are stored in the temp VC so the real VC can be located again later, adjusting the PG reference count
       accordingly.  MT-NOTE: the real VC mutex must be released since the process group is locked below.  A thread should never
       attempt to acquire the mutex of a process group while holding the mutex of a VC belonging to that thread.  In fact, with
       current implementation, where the PG module uses a single global mutex, a thread should never attempt to acquire the mutex
       of any PG while holding the mutex of any VC. */
    mpig_vc_mutex_unlock(real_vc);
    {
	mpig_pg_mutex_lock(pg);
	{
	    mpig_pg_inc_ref_count(pg);
	}
	mpig_pg_mutex_unlock(pg);
	mpig_vc_set_pg_info(tmp_vc, pg, pg_rank);
    }
    mpig_vc_mutex_lock(real_vc);
    
    /* create an XIO handle and initiate the connection */
    grc = globus_xio_handle_create(&tmp_vc_cm->handle, mpig_cm_xio_conn_stack);
    MPIU_ERR_CHKANDJUMP1((grc), *mpi_errno_p, MPI_ERR_OTHER, "**globus|xio_handle_create",
	"**globus|xio_handle_create %s", globus_error_print_chain(globus_error_peek(grc)));

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
	"temp VC constructed; registering open operation to form connection: tmp_vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) tmp_vc));

    /* register an ansychronous connect to the process specified in the contact string field located in the real VC.  if the
       registration is successful, update the temp VC state. */
    mpig_vc_mutex_lock(tmp_vc);
    tmp_vc_locked = TRUE;
    {
	grc = globus_xio_register_open(tmp_vc_cm->handle, real_vc_cm->cs, mpig_cm_xio_conn_attrs,
	    mpig_cm_xio_client_handle_open,(void *) tmp_vc);
	MPIU_ERR_CHKANDJUMP1((grc), *mpi_errno_p, MPI_ERR_OTHER, "**globus|cm_xio|xio_reg_open",
	    "**globus|cm_xio|xio_reg_open %s", globus_error_print_chain(globus_error_peek(grc)));

	mpig_cm_xio_vc_set_state(tmp_vc, MPIG_CM_XIO_VC_STATE_CONNECT_OPENING);
    }
    mpig_vc_mutex_unlock(tmp_vc);
    tmp_vc_locked = FALSE;

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"exiting: real_vc=" MPIG_PTR_FMT ", tmp_vc=" MPIG_PTR_FMT ", handle=" MPIG_PTR_FMT ", mpi_errno=0x%08x",
	(MPIG_PTR_CAST) real_vc, (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) (tmp_vc_cm ? tmp_vc_cm->handle : NULL), *mpi_errno_p));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_client_connect);
    return;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	/* release the temp VC mutex if it is being held by the current context */
	mpig_vc_mutex_unlock_conditional(tmp_vc, (tmp_vc_locked));
    
	/* close the handle and destroy the temp VC object, releasing the associated PG reference and remove the temp VC from
	   active VC list */
	if (tmp_vc != NULL)
	{
	    mpig_cm_xio_tmp_vc_destroy(tmp_vc, mpi_errno_p, &failed);
	    MPIU_ERR_CHKANDSTMT1((failed), *mpi_errno_p, MPI_ERR_INTERN, {;}, "**globus|cm_xio|tmp_vc_destroy",
		"**globus|cm_xio|tmp_vc_destroy %p", tmp_vc);
	}

	/* if the reference count of the real VC was bumped, then decrement it.  then, change the state of the real VC to the
	   failed state */
	if (mpig_cm_xio_vc_get_state(real_vc) == MPIG_CM_XIO_VC_STATE_CONNECTING)
	{
	    bool_t inuse;
	    mpig_cm_xio_vc_dec_ref_count(real_vc, &inuse);
	}
	mpig_cm_xio_vc_set_state(real_vc, MPIG_CM_XIO_VC_STATE_FAILED_CONNECTING);

	/* FT: add code to make multiple connection attempts before declaring the VC as failed? */
	
	mpig_cm_xio_fault_handle_sync_vc_error(real_vc, mpi_errno_p);

	*failed_p = TRUE;
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_client_connect() */


/*
 * void mpig_cm_xio_client_handle_open([IN] handle, [IN] result, [IN/MOD] arg (vc))
 *
 * MT-NOTE: the VC mutex is _NOT_ held by the caller when this routine is called.  this routine is responsible for performing any
 * necessary locking or RC acquire operations.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_client_handle_open
MPIG_STATIC void mpig_cm_xio_client_handle_open(const globus_xio_handle_t handle, const globus_result_t op_grc, void * const arg)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vc_t * const tmp_vc = (mpig_vc_t *) arg;
    struct mpig_cm_xio_vc * const tmp_vc_cm = &tmp_vc->cm.xio;
    bool_t tmp_vc_locked = FALSE;
    bool_t failed;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_client_handle_open);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_client_handle_open);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"entering: tmp_vc=" MPIG_PTR_FMT ", handle=" MPIG_PTR_FMT", op_grc=%d",
	(MPIG_PTR_CAST) tmp_vc,	(MPIG_PTR_CAST) handle, op_grc));

    mpig_vc_mutex_lock(tmp_vc);
    tmp_vc_locked = TRUE;
    {
	int rc;

	MPIU_Assert(mpig_cm_xio_vc_get_state(tmp_vc) == MPIG_CM_XIO_VC_STATE_CONNECT_OPENING);
	MPIU_ERR_CHKANDJUMP4((op_grc), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|client_handle_open",
	    "**globus|cm_xio|client_handle_open %p %s %d %s", tmp_vc, mpig_vc_get_pg_id(tmp_vc), mpig_vc_get_pg_rank(tmp_vc),
	    globus_error_print_chain(globus_error_peek(op_grc)));

	/* Copy magic string into the VC message buffer */
	mpig_databuf_reset(tmp_vc_cm->msgbuf);
	rc = MPIU_Strncpy(mpig_databuf_get_ptr(tmp_vc_cm->msgbuf), MPIG_CM_XIO_PROTO_CONNECT_MAGIC,
	    mpig_databuf_get_size(tmp_vc_cm->msgbuf));
	MPIU_Assert(rc == 0 && "Connect magic string did not fit in the VC message buffer");
	mpig_databuf_inc_eod(tmp_vc_cm->msgbuf, strlen(MPIG_CM_XIO_PROTO_CONNECT_MAGIC));

	/* Send the magic string */
	mpig_cm_xio_register_write_vc_msgbuf(tmp_vc, mpig_cm_xio_client_handle_send_magic, &mpi_errno, &failed);
	MPIU_ERR_CHKANDJUMP1((failed), mpi_errno, MPI_ERR_INTERN, "**globus|cm_xio|client_reg_send_magic",
	    "**globus|cm_xio|client_reg_send_magic %p", tmp_vc);

	mpig_cm_xio_vc_set_state(tmp_vc, MPIG_CM_XIO_VC_STATE_CONNECT_SENDING_MAGIC);
    }
    mpig_vc_mutex_unlock(tmp_vc);
    tmp_vc_locked = FALSE;

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"exiting: tmp_vc=" MPIG_PTR_FMT ", mpi_errno=0x%08x", (MPIG_PTR_CAST) tmp_vc, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_client_handle_open);
    return;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	MPIU_Assertp(FALSE && "XXX: FIX ERROR HANDLING CODE"); /* [BRT] */
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_client_handle_open() */


/*
 * void mpig_cm_xio_client_handle_send_magic(
 *          [IN] handle, [IN] result, [IN] sbuf, [IN] sbuf_len, [IN] nbytes, [IN] NULL, [IN/MOD] vc)
 *
 * MT-NOTE: the VC mutex is _NOT_ held by the caller when this routine is called.  this routine is responsible for performing any
 * necessary locking or RC acquire operations.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_client_handle_send_magic
MPIG_STATIC void mpig_cm_xio_client_handle_send_magic(
    const globus_xio_handle_t handle, const globus_result_t op_grc, globus_byte_t * const sbuf, const globus_size_t sbuf_len,
    const globus_size_t nbytes, const globus_xio_data_descriptor_t dd, void * const arg)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vc_t * const tmp_vc = (mpig_vc_t *) arg;
    struct mpig_cm_xio_vc * const tmp_vc_cm = &tmp_vc->cm.xio;
    bool_t tmp_vc_locked = FALSE;
    bool_t failed;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_client_handle_send_magic);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_client_handle_send_magic);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"entering: tmp_vc=" MPIG_PTR_FMT ", handle=" MPIG_PTR_FMT ", op_grc=%d, sbuf=" MPIG_PTR_FMT ", sbuf_len=" MPIG_SIZE_FMT
	", nbytes=" MPIG_SIZE_FMT, (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) handle, op_grc, (MPIG_PTR_CAST) sbuf, sbuf_len,
	nbytes));

    mpig_vc_mutex_lock(tmp_vc);
    tmp_vc_locked = TRUE;
    {
	MPIU_Assert(mpig_cm_xio_vc_get_state(tmp_vc) == MPIG_CM_XIO_VC_STATE_CONNECT_SENDING_MAGIC);
	MPIU_ERR_CHKANDJUMP4((op_grc), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|client_handle_send_magic",
	    "**globus|cm_xio|client_handle_send_magic %p %s %d %s", tmp_vc, mpig_vc_get_pg_id(tmp_vc),
	    mpig_vc_get_pg_rank(tmp_vc), globus_error_print_chain(globus_error_peek(op_grc)));
	MPIU_Assert(nbytes == strlen(MPIG_CM_XIO_PROTO_CONNECT_MAGIC) && "not all of the connect magic string was sent");
	
	/* register a read operation to receive the magic string from the accepting side */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
	    "registering receive for magic string"));
	mpig_databuf_reset(tmp_vc_cm->msgbuf);
	tmp_vc_cm->msg_hdr_size = 0;
	/* FIXME: [BRT] this should read minimally one byte and maximally the length of the magic string.  A timeout should be
	   associated with the receive in case the sender is not a valid MPIG process and never sends us anything. */
	mpig_cm_xio_register_read_vc_msgbuf(tmp_vc, strlen(MPIG_CM_XIO_PROTO_ACCEPT_MAGIC), strlen(MPIG_CM_XIO_PROTO_ACCEPT_MAGIC),
	    mpig_cm_xio_client_handle_recv_magic, &mpi_errno, &failed);
	MPIU_ERR_CHKANDJUMP1((failed), mpi_errno, MPI_ERR_INTERN, "**globus|cm_xio|client_reg_recv_magic",
	    "**globus|cm_xio|client_reg_recv_magic %p", tmp_vc);
	
	mpig_cm_xio_vc_set_state(tmp_vc, MPIG_CM_XIO_VC_STATE_CONNECT_RECEIVING_MAGIC);
    }
    mpig_vc_mutex_unlock(tmp_vc);
    tmp_vc_locked = FALSE;

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"exiting: tmp_vc=" MPIG_PTR_FMT ", mpi_errno=0x%08x", (MPIG_PTR_CAST) tmp_vc, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_client_handle_send_magic);
    return;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	MPIU_Assertp(FALSE && "XXX: FIX ERROR HANDLING CODE"); /* [BRT] */
	goto fn_return;
    }
}
/* mpig_cm_xio_client_handle_send_magic() */


/*
 * void mpig_cm_xio_client_handle_recv_magic(
 *          [IN] handle, [IN] result, [IN] rbuf, [IN] rbuf_len, [IN] nbytes, [IN] NULL, [IN/MOD] vc)
 *
 * MT-NOTE: the VC mutex is _NOT_ held by the caller when this routine is called.  this routine is responsible for performing any
 * necessary locking or RC acquire operations.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_client_handle_recv_magic
MPIG_STATIC void mpig_cm_xio_client_handle_recv_magic(
    const globus_xio_handle_t handle, const globus_result_t op_grc, globus_byte_t * const rbuf, const globus_size_t rbuf_len,
    const globus_size_t nbytes, const globus_xio_data_descriptor_t dd, void * const arg)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vc_t * const tmp_vc = (mpig_vc_t *) arg;
    struct mpig_cm_xio_vc * const tmp_vc_cm = &tmp_vc->cm.xio;
    bool_t tmp_vc_locked = FALSE;
    bool_t failed;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_client_handle_recv_magic);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_client_handle_recv_magic);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"entering: tmp_vc=" MPIG_PTR_FMT ", handle=" MPIG_PTR_FMT ", op_grc=%d, rbuf=" MPIG_PTR_FMT ", rbuf_len=" MPIG_SIZE_FMT
	", nbytes=" MPIG_SIZE_FMT, (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) handle, op_grc, (MPIG_PTR_CAST) rbuf, rbuf_len,
	nbytes));

    mpig_vc_mutex_lock(tmp_vc);
    tmp_vc_locked = TRUE;
    {
	size_t pg_id_size;
	int rc;
	
	MPIU_Assert(mpig_cm_xio_vc_get_state(tmp_vc) == MPIG_CM_XIO_VC_STATE_CONNECT_RECEIVING_MAGIC);
	MPIU_ERR_CHKANDJUMP4((op_grc), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|client_handle_recv_magic",
	    "**globus|cm_xio|client_handle_recv_magic %p %s %d %s", tmp_vc,  mpig_vc_get_pg_id(tmp_vc),
	    mpig_vc_get_pg_rank(tmp_vc), globus_error_print_chain(globus_error_peek(op_grc)));

	/* FIXME: [BRT] this should read minimally one byte and maximally the length of the magic string.  A timeout should be
	   associated with the receive in case the sender is not a valid MPIG process and never sends us anything. */
	MPIU_Assert(nbytes == strlen(MPIG_CM_XIO_PROTO_ACCEPT_MAGIC) && "magic string received from accepting side of connection");
	
	mpig_databuf_inc_eod(tmp_vc_cm->msgbuf, nbytes);

	/* compare the string to make sure the expect value was received. */
	rc = strncmp(mpig_databuf_get_ptr(tmp_vc_cm->msgbuf), MPIG_CM_XIO_PROTO_ACCEPT_MAGIC,
	    strlen(MPIG_CM_XIO_PROTO_ACCEPT_MAGIC));
	MPIU_ERR_CHKANDJUMP3((rc != 0), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|client_magic_mismatch",
	    "**globus|cm_xio|client_magic_mismatch %p %s %d", tmp_vc, mpig_vc_get_pg_id(tmp_vc), mpig_vc_get_pg_rank(tmp_vc));

	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
	    "magic string matched.  sending the open request to the remote process."));
	
	/* send the open request message */
	mpig_databuf_reset(tmp_vc_cm->msgbuf);
	mpig_cm_xio_msg_hdr_put_init(tmp_vc, tmp_vc_cm->msgbuf);
	mpig_cm_xio_msg_hdr_put_msg_type(tmp_vc, tmp_vc_cm->msgbuf, MPIG_CM_XIO_MSG_TYPE_OPEN_REQ);
	mpig_cm_xio_msg_hdr_put_endian(tmp_vc, tmp_vc_cm->msgbuf, MPIG_MY_ENDIAN);
	mpig_cm_xio_msg_hdr_put_df(tmp_vc, tmp_vc_cm->msgbuf, GLOBUS_DC_FORMAT_LOCAL);
	mpig_cm_xio_msg_hdr_put_rank(tmp_vc, tmp_vc_cm->msgbuf, mpig_process.my_pg_size);
	mpig_cm_xio_msg_hdr_put_rank(tmp_vc, tmp_vc_cm->msgbuf, mpig_process.my_pg_rank);
	pg_id_size = strlen(mpig_process.my_pg->id) + 1;
	MPIU_Strncpy(mpig_databuf_get_eod_ptr(tmp_vc_cm->msgbuf), mpig_process.my_pg->id, pg_id_size);
	mpig_databuf_inc_eod(tmp_vc_cm->msgbuf, pg_id_size);
	mpig_cm_xio_msg_hdr_put_msg_size(tmp_vc, tmp_vc_cm->msgbuf);

	/* send the open request message */
	mpig_cm_xio_register_write_vc_msgbuf(tmp_vc, mpig_cm_xio_client_handle_send_open_req, &mpi_errno, &failed);
	MPIU_ERR_CHKANDJUMP3((failed), mpi_errno, MPI_ERR_INTERN,"**globus|cm_xio|reg_send_open_resp",
	    "**globus|cm_xio|reg_send_open_resp %p %s %d", tmp_vc, tmp_vc->pg_id, tmp_vc->pg_rank);

	mpig_cm_xio_vc_set_state(tmp_vc, MPIG_CM_XIO_VC_STATE_CONNECT_SENDING_OPEN_REQ);
    }
    mpig_vc_mutex_unlock(tmp_vc);
    tmp_vc_locked = FALSE;

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"exiting: tmp_vc=" MPIG_PTR_FMT ", mpi_errno=0x%08x", (MPIG_PTR_CAST) tmp_vc, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_client_handle_recv_magic);
    return;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	MPIU_Assertp(FALSE && "XXX: FIX ERROR HANDLING CODE"); /* [BRT] */
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_client_handle_recv_magic() */


/*
 * void mpig_cm_xio_client_handle_send_open_req(
 *          [IN] handle, [IN] result, [IN] sbuf, [IN] sbuf_len, [IN] nbytes, [IN] NULL, [IN/MOD] vc)
 *
 * this handler gets invoked after sending an OPEN_REQ message over the temporary VC.
 *
 * MT-NOTE: the VC mutex is _NOT_ held by the caller when this routine is called.  this routine is responsible for performing any
 * necessary locking or RC acquire operations.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_client_handle_send_open_req
MPIG_STATIC void mpig_cm_xio_client_handle_send_open_req(
    const globus_xio_handle_t handle, const globus_result_t op_grc, globus_byte_t * const sbuf, const globus_size_t sbuf_len,
    const globus_size_t nbytes, const globus_xio_data_descriptor_t dd, void * const arg)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vc_t * const tmp_vc = (mpig_vc_t *) arg;
    struct mpig_cm_xio_vc * const tmp_vc_cm = &tmp_vc->cm.xio;
    bool_t tmp_vc_locked = FALSE;
    bool_t failed;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_client_handle_send_open_req);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_client_handle_send_open_req);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "entering: tmp_vc=" MPIG_PTR_FMT ", handle=" MPIG_PTR_FMT
	", op_grc=%d, sbuf=" MPIG_PTR_FMT ", sbuf_len=" MPIG_SIZE_FMT ", nbytes=" MPIG_SIZE_FMT, (MPIG_PTR_CAST) tmp_vc,
	(MPIG_PTR_CAST) handle, op_grc, (MPIG_PTR_CAST) sbuf, sbuf_len, nbytes));

    mpig_vc_mutex_lock(tmp_vc);
    tmp_vc_locked = TRUE;
    {
	MPIU_Assert(mpig_cm_xio_vc_get_state(tmp_vc) == MPIG_CM_XIO_VC_STATE_CONNECT_SENDING_OPEN_REQ);
	MPIU_ERR_CHKANDJUMP4((op_grc), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|client_handle_recv_magic",
	    "**globus|cm_xio|client_handle_recv_magic %p %s %d %s", tmp_vc,  mpig_vc_get_pg_id(tmp_vc),
	    mpig_vc_get_pg_rank(tmp_vc), globus_error_print_chain(globus_error_peek(op_grc)));
	MPIU_Assert(nbytes == mpig_databuf_get_eod(tmp_vc_cm->msgbuf) &&
	    "open request sent to accepting side of connection");

	/* register a read operation to receive the open response control message */
	mpig_databuf_reset(tmp_vc_cm->msgbuf);
	tmp_vc_cm->msg_hdr_size = 0;
	mpig_cm_xio_register_read_vc_msgbuf(tmp_vc, mpig_cm_xio_msg_hdr_remote_sizeof_msg_size(tmp_vc),
	    mpig_databuf_get_size(tmp_vc_cm->msgbuf), mpig_cm_xio_client_handle_recv_open_resp, &mpi_errno, &failed);
	MPIU_ERR_CHKANDJUMP3((failed), mpi_errno, MPI_ERR_INTERN, "**globus|cm_xio|client_reg_recv_open_req",
	    "**globus|cm_xio|client_reg_recv_open_req %p %s %d", tmp_vc, mpig_vc_get_pg_id(tmp_vc), mpig_vc_get_pg_rank(tmp_vc));
	
	mpig_cm_xio_vc_set_state(tmp_vc, MPIG_CM_XIO_VC_STATE_CONNECT_RECEIVING_OPEN_RESP);
    }
    mpig_vc_mutex_unlock(tmp_vc);
    tmp_vc_locked = FALSE;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"exiting: tmp_vc=" MPIG_PTR_FMT ", mpi_errno=0x%08x", (MPIG_PTR_CAST) tmp_vc, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_client_handle_send_open_req);
    return;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	MPIU_Assertp(FALSE && "XXX: FIX ERROR HANDLING CODE"); /* [BRT] */
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_client_handle_send_open_req() */


/*
 * void mpig_cm_xio_client_handle_recv_open_resp_ack(
 *           [IN] handle, [IN] result, [IN] rbuf, [IN] rbuf_len, [IN] nbytes, [IN] NULL, [IN/MOD] vc)
 *
 * MT-NOTE: this function assumes that the VC mutex is held by the current context.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_client_handle_recv_open_resp
void mpig_cm_xio_client_handle_recv_open_resp(
    const globus_xio_handle_t handle, const globus_result_t op_grc, globus_byte_t * const rbuf, const globus_size_t rbuf_len,
    const globus_size_t nbytes, const globus_xio_data_descriptor_t dd, void * const arg)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vc_t * const tmp_vc = (mpig_vc_t *) arg;
    struct mpig_cm_xio_vc * const tmp_vc_cm = &tmp_vc->cm.xio;
    bool_t tmp_vc_locked = FALSE;
    mpig_vc_t * real_vc = NULL;
    struct mpig_cm_xio_vc * real_vc_cm = NULL;
    bool_t real_vc_locked = FALSE;
    mpig_cm_xio_msg_types_t msg_type;
    mpig_cm_xio_conn_open_resp_t open_resp = MPIG_CM_XIO_CONN_OPEN_RESP_UNDEFINED;
    mpig_pg_t * pg = NULL;
    int pg_rank;
    bool_t failed;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_client_handle_recv_open_resp);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_client_handle_recv_open_resp);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"entering: tmp_vc=" MPIG_PTR_FMT ", handle=" MPIG_PTR_FMT ", op_grc=%d, rbuf=" MPIG_PTR_FMT ", rbuf_len=" MPIG_SIZE_FMT
	", nbytes=" MPIG_SIZE_FMT, (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) handle, op_grc, (MPIG_PTR_CAST) rbuf, rbuf_len,
	nbytes));

    MPIG_UNUSED_VAR(fcname);

    mpig_vc_mutex_lock(tmp_vc);
    tmp_vc_locked = TRUE;
    {
	MPIU_Assert(mpig_cm_xio_vc_get_state(tmp_vc) == MPIG_CM_XIO_VC_STATE_CONNECT_RECEIVING_OPEN_RESP);
	MPIU_ERR_CHKANDJUMP4((op_grc), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|client_handle_recv_magic",
	    "**globus|cm_xio|client_handle_recv_magic %p %s %d %s", tmp_vc,  mpig_vc_get_pg_id(tmp_vc),
	    mpig_vc_get_pg_rank(tmp_vc), globus_error_print_chain(globus_error_peek(op_grc)));

	/* adjust end of message buffer to account for the number of bytes received */
	mpig_databuf_inc_eod(tmp_vc_cm->msgbuf, nbytes);

	/* if the message header size has not been determined, then extract the header size.  if the message buffer does not
	   contain the complete header, the register a read for the the remaining bytes.  assuming an error does not occur, the
	   entire header will be in the VC message buffer when the next callback occurs. */
	if (tmp_vc_cm->msg_hdr_size == 0)
	{
	    MPIU_Assert(nbytes >= mpig_cm_xio_msg_hdr_remote_sizeof_msg_size(tmp_vc));
	    mpig_cm_xio_msg_hdr_get_msg_size(tmp_vc, tmp_vc_cm->msgbuf, &tmp_vc_cm->msg_hdr_size);
	    tmp_vc_cm->msg_hdr_size -= mpig_cm_xio_msg_hdr_remote_sizeof_msg_size(tmp_vc);

	    if (mpig_databuf_get_remaining_bytes(tmp_vc_cm->msgbuf) < tmp_vc_cm->msg_hdr_size)
	    {
		/* not all of the open request header was received.  try to get the remainder of it. */
		MPIU_Size_t bytes_needed = tmp_vc_cm->msg_hdr_size - mpig_databuf_get_remaining_bytes(tmp_vc_cm->msgbuf);
		
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		    "not all of the open response message was received; posting a read for the remaining bytes, tmp_vc="
		    MPIG_PTR_FMT ", nbytes=%u", (MPIG_PTR_CAST) tmp_vc, (unsigned) bytes_needed));
	    
		mpig_cm_xio_register_read_vc_msgbuf(tmp_vc, bytes_needed, bytes_needed, mpig_cm_xio_client_handle_recv_open_resp,
		    &mpi_errno, &failed);
		MPIU_ERR_CHKANDJUMP((failed), mpi_errno, MPI_ERR_INTERN, "**globus|cm_xio|client_reg_recv_open_req");

		mpig_vc_mutex_unlock(tmp_vc);
		tmp_vc_locked = FALSE;
		goto fn_return;
	    }
	}

	/* if all of the message is not available, then report an incomplete message eror to the remote side */
	if (mpig_databuf_get_remaining_bytes(tmp_vc_cm->msgbuf) < tmp_vc_cm->msg_hdr_size)
	{
	    /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		"message received is incomplete: tmp_vc=" MPIG_PTR_FMT ", expected=%d, received=" MPIG_SIZE_FMT,
		(MPIG_PTR_CAST) tmp_vc, tmp_vc_cm->msg_hdr_size, mpig_databuf_get_remaining_bytes(tmp_vc_cm->msgbuf)));
	    MPIU_Assertp(FALSE && "not all of open response message was received"); /* [BRT] */
	    /* --END ERROR HANDLING-- */
	}
	
	/* get the PG and PG rank so that the real VC can be determined */
	pg = mpig_vc_get_pg(tmp_vc);
	pg_rank = mpig_vc_get_pg_rank(tmp_vc);

	/* get the message type and and open resp */
	mpig_cm_xio_msg_hdr_get_msg_type(tmp_vc, tmp_vc_cm->msgbuf, &msg_type);
	MPIU_Assert(msg_type == MPIG_CM_XIO_MSG_TYPE_OPEN_RESP);
	mpig_cm_xio_msg_hdr_get_conn_open_resp(tmp_vc, tmp_vc_cm->msgbuf, &open_resp);
    }
    mpig_vc_mutex_unlock(tmp_vc);
    tmp_vc_locked = FALSE;

    /* get the real VC using the PG and the PG rank from the temp VC */
    mpig_pg_rc_acq(pg, TRUE);
    {
	mpig_pg_get_vc(pg, pg_rank, &real_vc);
	real_vc_cm = &real_vc->cm.xio;
    }
    mpig_pg_rc_rel(pg, FALSE);
    
    /* the connection has been established; update the real update the real VC and get the data flowing */
    mpig_vc_mutex_lock(real_vc);
    real_vc_locked = TRUE;
    {
	switch (open_resp)
	{
	    case MPIG_CM_XIO_CONN_OPEN_RESP_ACK:
	    {
		/* set the handle on the real VC, and change the state to connected */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		    "connection ACKed; change state of real VC to connected: tmp_vc=" MPIG_PTR_FMT ", real_vc=" MPIG_PTR_FMT,
		    (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) real_vc));
		real_vc_cm->handle = handle;
		mpig_cm_xio_vc_set_state(real_vc, MPIG_CM_XIO_VC_STATE_CONNECTED);
		mpig_cm_xio_vc_list_add(real_vc);

		/* move any data in the temp VC's message buffer to the real VC's message buffer */
		if (mpig_databuf_get_remaining_bytes(tmp_vc_cm->msgbuf) > 0)
		{
		    memcpy(mpig_databuf_get_ptr(real_vc_cm->msgbuf), mpig_databuf_get_pos_ptr(tmp_vc_cm->msgbuf),
			mpig_databuf_get_remaining_bytes(tmp_vc_cm->msgbuf));
		    mpig_databuf_set_eod(real_vc_cm->msgbuf, mpig_databuf_get_remaining_bytes(tmp_vc_cm->msgbuf));
		}
		
		/* start receiving messages on the real VC.  note: it was not necesary to transfer the receive buffer from the
		   temp VC to the real VC since we know that the buffer was depleted earlier in this routine */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VC | MPIG_DEBUG_LEVEL_VCCM,
		    "connection established; starting communication engines: vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) real_vc));
		
		mpig_cm_xio_recv_next_msg(real_vc, &mpi_errno, &failed);
		MPIU_ERR_CHKANDJUMP((failed), mpi_errno, MPI_ERR_INTERN, "**globus|xio_cm|recv_next_msg");

		/* if the real VC has any sends queued up, start send them */
		mpig_cm_xio_send_next_sreq(real_vc, &mpi_errno, &failed);
		MPIU_ERR_CHKANDJUMP((failed), mpi_errno, MPI_ERR_INTERN, "**globus|xio_cm|send_next_sreq");

		break;
	    }

	    case MPIG_CM_XIO_CONN_OPEN_RESP_NAK:
	    {
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		    "connection NAKed; waiting for remote connect to finish: tmp_vc=" MPIG_PTR_FMT ", real_vc=" MPIG_PTR_FMT,
		    (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) real_vc));

		break;
	    }

	    case MPIG_CM_XIO_CONN_OPEN_RESP_MSG_INCOMPLETE:
	    case MPIG_CM_XIO_CONN_OPEN_RESP_MSG_TYPE_BAD:
	    case MPIG_CM_XIO_CONN_OPEN_RESP_MSG_TOO_LONG:
	    case MPIG_CM_XIO_CONN_OPEN_RESP_VC_NOT_XIO:
	    case MPIG_CM_XIO_CONN_OPEN_RESP_VC_CONNECTED:
	    case MPIG_CM_XIO_CONN_OPEN_RESP_VC_DISCONNECTING:
	    case MPIG_CM_XIO_CONN_OPEN_RESP_VC_FAILED:
	    {
		MPIU_Assertp(FALSE && "open response contained an error condition"); /* [BRT] FIXME: */
		break;
	    }

	    default:
	    {
		MPIU_Assertp(FALSE && "unknown open response"); /* [BRT] FIXME: */
	    }
	}
    }
    mpig_vc_mutex_unlock(real_vc);
    real_vc_locked = FALSE;
    
  fn_return:
    /* release the temporary VC, closing the connection if an ACK wasn't received, and release the associated PG */
    mpig_vc_mutex_unlock_conditional(tmp_vc, (tmp_vc_locked));
    if (open_resp == MPIG_CM_XIO_CONN_OPEN_RESP_ACK)
    {
	tmp_vc_cm->handle = NULL;
    }
    mpig_cm_xio_tmp_vc_destroy(tmp_vc, &mpi_errno, &failed);
    MPIU_ERR_CHKANDSTMT1((failed), mpi_errno, MPI_ERR_INTERN, {;}, "**globus|cm_xio|tmp_vc_destroy",
	"**globus|cm_xio|tmp_vc_destroy %p", tmp_vc);

    /* the internal reference to the VC established in mpig_cm_xio_client_connect() needs to be release */
    mpig_vc_mutex_unlock_conditional(real_vc, (real_vc_locked));
    if (real_vc)
    {
	mpig_vc_release_ref(real_vc, &mpi_errno, &failed);
    }
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"exiting: tmp_vc=" MPIG_PTR_FMT ", real_vc=" MPIG_PTR_FMT ", mpi_errno=0x%08x",
	(MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) real_vc, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_client_handle_recv_open_resp);
    return;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	MPIU_Assertp(FALSE && "XXX: FIX ERROR HANDLING CODE"); /* [BRT] */
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_client_handle_recv_open_resp() */

#endif /* MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS */
/**********************************************************************************************************************************
					    END CLIENT SIDE CONNECTION ESTABLISHMENT
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						    BEGIN CONNECTION TEARDOWN
**********************************************************************************************************************************/
#if !defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS)

MPIG_STATIC void mpig_cm_xio_disconnect(mpig_vc_t * vc, int * mpi_errno_p, bool_t * failed_p);

MPIG_STATIC void mpig_cm_xio_disconnect_handle_recv_close_msg(mpig_vc_t * vc, int * mpi_errno_p, bool_t * failed_p);

MPIG_STATIC void mpig_cm_xio_disconnect_enq_close_msg(mpig_vc_t * vc, bool_t cr, bool_t ack, int * mpi_errno_p, bool_t * failed_p);

MPIG_STATIC void mpig_cm_xio_disconnect_handle_send_close_msg(
    globus_xio_handle_t handle, globus_result_t op_grc, globus_xio_iovec_t * iov, int iov_cnt, globus_size_t nbytes,
    globus_xio_data_descriptor_t dd, void * arg);

MPIG_STATIC void mpig_cm_xio_disconnect_handle_close(globus_xio_handle_t handle, globus_result_t op_grc, void * arg);


#else /* defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS) */


/*
 * void mpig_cm_xio_disconnect([IN/MOD] vc, [IN/OUT] mpi_errno, [OUT] failed)
 *
 * MT-NOTE: this function assumes that the VC mutex is held by the current context.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_disconnect
MPIG_STATIC void mpig_cm_xio_disconnect(mpig_vc_t * const vc, int * const mpi_errno_p, bool_t * const failed_p)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    struct mpig_cm_xio_vc * const vc_cm = &vc->cm.xio;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_disconnect);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_disconnect);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"entering: vc=" MPIG_PTR_FMT ", mpi_errno=0x%08x", (MPIG_PTR_CAST) vc, *mpi_errno_p));
    *failed_p = FALSE;

    MPIG_UNUSED_VAR(fcname);

    MPIU_Assertp(vc_cm->handle != NULL);

    if (mpig_cm_xio_vc_get_state(vc) == MPIG_CM_XIO_VC_STATE_CONNECTED)
    {
	/* if the VC is in the connected state, then send a close request (CR) message */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
	    "VC connected; sending CR: vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) vc));
	    
	MPIU_Assert(vc->ref_count == 0);
	mpig_cm_xio_disconnect_enq_close_msg(vc, TRUE, FALSE, mpi_errno_p, failed_p);
	MPIU_ERR_CHKANDJUMP((*failed_p), *mpi_errno_p, MPI_ERR_OTHER, "**globus|cm_xio|disconnect_enq_close_msg");
	mpig_cm_xio_vc_set_state(vc, MPIG_CM_XIO_VC_STATE_DISCONNECT_SENDING_CLOSE_REQ_AWAITING_CLOSE_REQ);

	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
	    "CR message enqueued; VC status updated: vc=" MPIG_PTR_FMT ", new_vc_state=%s",
	    (MPIG_PTR_CAST) vc, mpig_cm_xio_vc_state_get_string(mpig_cm_xio_vc_get_state(vc))));
    }
    else if (mpig_cm_xio_vc_get_state(vc) == MPIG_CM_XIO_VC_STATE_CONNECTED_RECEIVED_CLOSE_REQ)
    {
	/* if the VC has already received a close request (CR) from the remote process, then send a close request and acknowledge
	   the reception of the CR from the close request */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
	    "VC connected, but received CR; sending CR/ACK: vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) vc));
	    
	MPIU_Assert(vc->ref_count == 0);
	mpig_cm_xio_disconnect_enq_close_msg(vc, TRUE, TRUE, mpi_errno_p, failed_p);
	MPIU_ERR_CHKANDJUMP((*failed_p), *mpi_errno_p, MPI_ERR_OTHER, "**globus|cm_xio|disconnect_enq_close_msg");
	mpig_cm_xio_vc_set_state(vc, MPIG_CM_XIO_VC_STATE_DISCONNECT_SENDING_ACK_AWAITING_ACK);

	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
	    "CR/ACK message enqueued; VC status updated: vc=" MPIG_PTR_FMT ", new_vc_state=%s",
	    (MPIG_PTR_CAST) vc, mpig_cm_xio_vc_state_get_string(mpig_cm_xio_vc_get_state(vc))));
    }
    else if (mpig_cm_xio_vc_is_undefined(vc))
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
	    "ERROR: VC state is undefined: vc=" MPIG_PTR_FMT ", vc_state=%s",
	    (MPIG_PTR_CAST) vc, mpig_cm_xio_vc_state_get_string(mpig_cm_xio_vc_get_state(vc))));
	MPIU_Assertp(FALSE && "VC state is undefined during the initiation of the disconnect protocol");
    }   /* --END ERROR HANDLING-- */
    else if (mpig_cm_xio_vc_state_is_valid(vc))
    {
	/* if the VC state is valid, but not one of the above, the assume the disconnect is caused by an increment-decrement from
	   an incoming connection for this VC that was NAKed by the server. */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
	    "VC is not in a state where it make sense to disconnect; ignoring disconnect request: vc=" MPIG_PTR_FMT
	    ", vc_state=%s", (MPIG_PTR_CAST) vc, mpig_cm_xio_vc_state_get_string(mpig_cm_xio_vc_get_state(vc))));
    }
    else
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM,
	    "encountered unexpected state: vc_state_value=%d", mpig_cm_xio_vc_get_state(vc)));
	MPIU_Assertp(FALSE && "encountered unexpected VC state during the initiation of the disconnect protocol");
    }   /* --END ERROR HANDLING-- */
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"exiting: vc=" MPIG_PTR_FMT ", mpi_errno=0x%08x, failed=%s", (MPIG_PTR_CAST) vc, *mpi_errno_p, MPIG_BOOL_STR(*failed_p)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_disconnect);
    return;
    
  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	mpig_cm_xio_fault_handle_sync_vc_error(vc, mpi_errno_p);

	*failed_p = TRUE;
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_disconnect() */


/*
 * void mpig_cm_xio_disconnect_enq_close_msg([IN/MOD] vc, [IN] ack, [IN/OUT] mpi_errno, [OUT] failed)
 *
 * vc [IN/MOD] - connection over which to send the synchronous send acknowledgement
 * ack [IN] - close request flag
 * ack [IN] - close ack flag
 * mpi_errno [IN/OUT] - MPI error code
 * failed [OUT] - TRUE if the routine failed; FALSE otherwise
 *
 * MT-NOTE: this routine assumes that the VC mutex is held by the current context.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_disconnect_enq_close_msg
MPIG_STATIC void mpig_cm_xio_disconnect_enq_close_msg(
    mpig_vc_t * const vc, const bool_t cr, const bool_t ack, int * const mpi_errno_p, bool_t * const failed_p)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPID_Request * sreq = NULL;
    struct mpig_cm_xio_request * sreq_cm = NULL;
    bool_t failed;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_disconnect_enq_close_msg);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_disconnect_enq_close_msg);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"entering: vc=" MPIG_PTR_FMT ", cr=%s, ack=%s, mpi_errno=0x%08x", (MPIG_PTR_CAST) vc, MPIG_BOOL_STR(cr),
	MPIG_BOOL_STR(ack), *mpi_errno_p));
    *failed_p = FALSE;

    /* allocate a new send request */
    mpig_request_alloc(&sreq);
    MPIU_ERR_CHKANDJUMP1((sreq == NULL), *mpi_errno_p, MPI_ERR_OTHER, "**nomem", "**nomem %s", "sreq for close msg");
    sreq_cm = &sreq->cm.xio;

    mpig_request_construct(sreq);
    mpig_cm_xio_request_construct(sreq);
    mpig_request_set_params(sreq, MPID_REQUEST_UNDEFINED, MPIG_REQUEST_TYPE_INTERNAL, 0, 0, NULL, 0, MPI_DATATYPE_NULL,
	MPI_PROC_NULL, MPI_ANY_TAG, 0, vc);
    mpig_cm_xio_request_set_state(sreq, MPIG_CM_XIO_REQ_STATE_SEND_CTRL_MSG);
    mpig_cm_xio_request_set_msg_type(sreq, MPIG_CM_XIO_MSG_TYPE_CLOSE);
    sreq_cm->gcb = mpig_cm_xio_disconnect_handle_send_close_msg;
    
    /* pack message header */
    mpig_cm_xio_msg_hdr_put_init(vc, sreq_cm->msgbuf);
    mpig_cm_xio_msg_hdr_put_msg_type(vc, sreq_cm->msgbuf, mpig_cm_xio_request_get_msg_type(sreq));
    mpig_cm_xio_msg_hdr_put_bool(vc, sreq_cm->msgbuf, cr);
    mpig_cm_xio_msg_hdr_put_bool(vc, sreq_cm->msgbuf, ack);
    mpig_cm_xio_msg_hdr_put_msg_size(vc, sreq_cm->msgbuf);

    mpig_iov_reset(sreq_cm->iov, 0);
    mpig_iov_add_entry(sreq_cm->iov, mpig_databuf_get_ptr(sreq_cm->msgbuf), mpig_databuf_get_eod(sreq_cm->msgbuf));

    /* send message */
    mpig_cm_xio_send_enq_sreq(vc, sreq, mpi_errno_p, &failed);
    MPIU_ERR_CHKANDJUMP((failed), *mpi_errno_p, MPI_ERR_OTHER, "**globus|cm_xio|send_enq_sreq");
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"exiting: vc=" MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT ", mpi_errno=0x%08x",
	(MPIG_PTR_CAST) vc, MPIG_HANDLE_VAL(sreq), (MPIG_PTR_CAST) sreq, *mpi_errno_p));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_disconnect_enq_close_msg);
    return;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	*failed_p = TRUE;
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_disconnect_enq_close_msg() */


/*
 * void mpig_cm_xio_disconnect_handle_send_close_msg(
 *          [IN] handle, [IN] op_grc, [IN] iov, [IN] iov_cnt, [IN] nbytes, [IN] dd, [IN/MOD] vc)
 *
 * MT-NOTE: this routine assumes that the VC mutex is _NOT_ held by the current context.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_disconnect_handle_send_close_msg
MPIG_STATIC void mpig_cm_xio_disconnect_handle_send_close_msg(
    const globus_xio_handle_t handle, const globus_result_t op_grc, globus_xio_iovec_t * const iov, const int iov_cnt,
    const globus_size_t nbytes, const globus_xio_data_descriptor_t dd, void * const arg)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vc_t * vc = (mpig_vc_t *) arg;
    struct mpig_cm_xio_vc * vc_cm = &vc->cm.xio;
    bool_t vc_locked = FALSE;
    MPID_Request * sreq;
    bool_t failed;
    globus_result_t grc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_disconnect_handle_send_close_msg);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_disconnect_handle_send_close_msg);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"entering: vc=" MPIG_PTR_FMT ", handle=" MPIG_PTR_FMT ", op_grc=%d, iov=" MPIG_PTR_FMT ", iov_cnt=%d, nbytes="
	MPIG_SIZE_FMT, (MPIG_PTR_CAST) vc, (MPIG_PTR_CAST) handle, op_grc, (MPIG_PTR_CAST) iov, iov_cnt, nbytes));

    MPIU_ERR_CHKANDJUMP1((op_grc), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|disconnect_handle_send_close_msg",
	"**globus|cm_xio|disconnect_handle_send_close_msg %s", globus_error_print_chain(globus_error_peek(op_grc)));

    mpig_vc_mutex_lock(vc);
    vc_locked = TRUE;
    {
	/* destroy the send request */
	sreq = vc_cm->active_sreq;
	MPIU_Assert(mpig_cm_xio_request_get_state(sreq) == MPIG_CM_XIO_REQ_STATE_SEND_CTRL_MSG);
	MPIU_Assert(mpig_cm_xio_request_get_msg_type(sreq) == MPIG_CM_XIO_MSG_TYPE_CLOSE);
	mpig_request_destroy(sreq);

	switch(mpig_cm_xio_vc_get_state(vc))
	{
	    case MPIG_CM_XIO_VC_STATE_DISCONNECT_SENDING_CLOSE_REQ_AWAITING_CLOSE_REQ:
	    {
		/* finished sending our CR.  waiting for remote process to send a CR or CR/ACK */
		mpig_cm_xio_vc_set_state(vc, MPIG_CM_XIO_VC_STATE_DISCONNECT_SENT_CLOSE_REQ_AWAITING_CLOSE_REQ);
		
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		    "finished sending CR; VC state updated; waiting for CR from the remote process: vc=" MPIG_PTR_FMT
		    ", new_vc_state=%s, sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT, (MPIG_PTR_CAST) vc,
		    mpig_cm_xio_vc_state_get_string(mpig_cm_xio_vc_get_state(vc)), MPIG_HANDLE_VAL(sreq), (MPIG_PTR_CAST) sreq));
		
		break;
	    }
	    case  MPIG_CM_XIO_VC_STATE_DISCONNECT_SENDING_CLOSE_REQ_RECEIVED_CLOSE_REQ:
	    {
		/* finished sending our CR, and we have received a CR from the remote proess.  send an ACK to the
		   remote process */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		    "finished sending CR message; already received a CR from the remote process; sending ACK: vc="
		    MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT, (MPIG_PTR_CAST) vc,
		    MPIG_HANDLE_VAL(sreq), (MPIG_PTR_CAST) sreq));
		
		mpig_cm_xio_disconnect_enq_close_msg(vc, FALSE, TRUE, &mpi_errno, &failed);
		MPIU_ERR_CHKANDJUMP((failed), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|disconnect_enq_close_msg");
		mpig_cm_xio_vc_set_state(vc, MPIG_CM_XIO_VC_STATE_DISCONNECT_SENDING_ACK_AWAITING_ACK);
		
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		    "ACK message enqueued; VC status updated: vc=" MPIG_PTR_FMT ", new_vc_state=%s",
		    (MPIG_PTR_CAST) vc, mpig_cm_xio_vc_state_get_string(mpig_cm_xio_vc_get_state(vc))));
		
		break;
	    }

	    case  MPIG_CM_XIO_VC_STATE_DISCONNECT_SENDING_CLOSE_REQ_RECEIVED_ACK:
	    {
		/* finished sending our CR, and we have received an ACK or CR/ACK from the remote proess.  send an ACK to the
		   remote process */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		    "finished sending the CR; already received ACK or CR/ACK from the remote process; sending ACK:"
		    " vc=" MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT, (MPIG_PTR_CAST) vc,
		    MPIG_HANDLE_VAL(sreq), (MPIG_PTR_CAST) sreq));

		mpig_cm_xio_disconnect_enq_close_msg(vc, FALSE, TRUE, &mpi_errno, &failed);
		MPIU_ERR_CHKANDJUMP((failed), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|disconnect_enq_close_msg");
		mpig_cm_xio_vc_set_state(vc, MPIG_CM_XIO_VC_STATE_DISCONNECT_SENDING_ACK_RECEIVED_ACK);

		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		    "ACK message enqueued; VC status updated: vc=" MPIG_PTR_FMT ", new_vc_state=%s",
		    (MPIG_PTR_CAST) vc, mpig_cm_xio_vc_state_get_string(mpig_cm_xio_vc_get_state(vc))));
		
		break;
	    }

	    case  MPIG_CM_XIO_VC_STATE_DISCONNECT_SENDING_ACK_AWAITING_ACK:
	    {
		/* finished sending our CR/ACK or ACK, and we have received a CR but not an ACK from the remote proess.  wait for
		   the ACK from the remote process */
		mpig_cm_xio_vc_set_state(vc, MPIG_CM_XIO_VC_STATE_DISCONNECT_SENT_ACK_AWAITING_ACK);
		
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		    "finished sending the CR or CR/ACK; VC state updated; awaiting ACK from the remote process: vc=" MPIG_PTR_FMT
		    ", new_vc_state=%s, sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT, (MPIG_PTR_CAST) vc,
		    mpig_cm_xio_vc_state_get_string(mpig_cm_xio_vc_get_state(vc)), MPIG_HANDLE_VAL(sreq), (MPIG_PTR_CAST) sreq));
		
		break;
	    }

	    case  MPIG_CM_XIO_VC_STATE_DISCONNECT_SENDING_ACK_RECEIVED_ACK:
	    {
		/* finished sending our CR/ACK or ACK, and we have received an ACK or CR/ACK from the remote proess.  close the
		   VC */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		    "finished sending the CR/ACK or ACK; already received ACK or CR/ACK from the remote process; "
		    " closing VC: vc=" MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT,
		    (MPIG_PTR_CAST) vc, MPIG_HANDLE_VAL(sreq), (MPIG_PTR_CAST) sreq));
		
		grc = globus_xio_register_close(vc_cm->handle, NULL, mpig_cm_xio_disconnect_handle_close, vc);
		MPIU_ERR_CHKANDJUMP1((grc), mpi_errno, MPI_ERR_INTERN, "**globus|cm_xio|xio_reg_close",
		    "**globus|cm_xio|xio_reg_close %s", globus_error_print_chain(globus_error_peek(grc)));
		mpig_cm_xio_vc_set_state(vc, MPIG_CM_XIO_VC_STATE_DISCONNECT_CLOSING_VC);
		
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		    "close registered; VC state updated; vc=" MPIG_PTR_FMT ", new_vc_state=%s",
		    (MPIG_PTR_CAST) vc, mpig_cm_xio_vc_state_get_string(mpig_cm_xio_vc_get_state(vc))));
		
		break;
	    }

	    default:
	    {   /* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM | MPIG_DEBUG_LEVEL_VC,
		    "encountered unexpected state: %s (%d)",
		    mpig_cm_xio_vc_state_get_string(mpig_cm_xio_vc_get_state(vc)), mpig_cm_xio_vc_get_state(vc)));
		MPIU_Assertp(FALSE && "encountered unexpected VC state after sending close message");
		
		break;
	    }   /* --END ERROR HANDLING-- */
	}
	
	/* send the next message in the send queue, if one exists, and if the VC is still in the connected state */
	vc_cm->active_sreq = NULL;
	if (mpig_cm_xio_vc_is_connected(vc))
	{
	    mpig_cm_xio_send_next_sreq(vc, &mpi_errno, &failed);
	    MPIU_ERR_CHKANDJUMP((failed), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|send_next_sreq");
	}

    }
    mpig_vc_mutex_unlock(vc);
    vc_locked = FALSE;

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"exiting: vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) vc));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_disconnect_handle_send_close_msg);
    return;
    
  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	mpig_vc_mutex_lock_conditional(vc, (!vc_locked));
	{
	    mpig_cm_xio_fault_handle_async_vc_error(vc, &mpi_errno);
	}
	mpig_vc_mutex_unlock(vc);

	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_disconnect_handle_send_close_msg() */


/*
 * void mpig_cm_xio_disconnect_handle_recv_close_msg([IN/MOD] vc, [IN/OUT] mpi_errno, [OUT] failed)
 *
 * MT-NOTE: this function assumes that the VC mutex is held by the current context.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_disconnect_handle_recv_close_msg
MPIG_STATIC void mpig_cm_xio_disconnect_handle_recv_close_msg(
    mpig_vc_t * const vc, int * const mpi_errno_p, bool_t * const failed_p)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    struct mpig_cm_xio_vc * const vc_cm = &vc->cm.xio;
    bool_t cr;
    bool_t ack;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_disconnect_handle_recv_close_msg);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_disconnect_handle_recv_close_msg);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"entering; vc=" MPIG_PTR_FMT ", mpi_errno=0x%08x", (MPIG_PTR_CAST) vc, *mpi_errno_p));
    *failed_p = FALSE;
    
    /* unpack message header */
    MPIU_Assert(vc_cm->msg_hdr_size == 2 * mpig_cm_xio_msg_hdr_remote_sizeof_bool(vc));
    mpig_cm_xio_msg_hdr_get_bool(vc, vc_cm->msgbuf, &cr);
    mpig_cm_xio_msg_hdr_get_bool(vc, vc_cm->msgbuf, &ack);

    MPIG_UNUSED_VAR(fcname);

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
	"received close message: vc=" MPIG_PTR_FMT ", vc_state=%s, CR=%s, ACK=%s", (MPIG_PTR_CAST) vc,
	mpig_cm_xio_vc_state_get_string(mpig_cm_xio_vc_get_state(vc)), MPIG_BOOL_STR(cr), MPIG_BOOL_STR(ack)));
    switch(mpig_cm_xio_vc_get_state(vc))
    {
	case MPIG_CM_XIO_VC_STATE_CONNECTED:
	{
	    /* received close request (CR) from the remote process; change the state to received close but still connected.
	       wait for VC reference count to reach zero before sending CR/ACK response. */
	    MPIU_Assert(cr == TRUE && ack == FALSE);
	    mpig_cm_xio_vc_set_state(vc, MPIG_CM_XIO_VC_STATE_CONNECTED_RECEIVED_CLOSE_REQ);
	    
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		"VC is still referenced; VC state updated; wait for VC reference count to reach zero: vc=" MPIG_PTR_FMT
		", new_vc_state=%s", (MPIG_PTR_CAST) vc, mpig_cm_xio_vc_state_get_string(mpig_cm_xio_vc_get_state(vc))));
	    
	    break;
	}

	case MPIG_CM_XIO_VC_STATE_DISCONNECT_SENDING_CLOSE_REQ_AWAITING_CLOSE_REQ:
	{
	    MPIU_Assert(cr == TRUE);
	    if (ack == FALSE)
	    {
		/* received a CR but still sending a CR.  this is a head-to-head disconnect scenario.  wait for send to complete
		   or an ACK to received from remote process. */
		mpig_cm_xio_vc_set_state(vc, MPIG_CM_XIO_VC_STATE_DISCONNECT_SENDING_CLOSE_REQ_RECEIVED_CLOSE_REQ);
		
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		    "still sending CR; VC state updated; awaiting CR from the remote process: vc=" MPIG_PTR_FMT
		    ", new_vc_state=%s", (MPIG_PTR_CAST) vc, mpig_cm_xio_vc_state_get_string(mpig_cm_xio_vc_get_state(vc))));
	    }
	    else
	    {
		/* received the CR/ACK for the CR still being sent.  this can happen if the read callback occurs before the
		   callback for the write.  wait for send to complete. */
		mpig_cm_xio_vc_set_state(vc, MPIG_CM_XIO_VC_STATE_DISCONNECT_SENDING_CLOSE_REQ_RECEIVED_ACK);
		
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		    "still sending CR; VC state updated; wait for send (CR) to complete: vc=" MPIG_PTR_FMT
		    ", new_vc_state=%s", (MPIG_PTR_CAST) vc, mpig_cm_xio_vc_state_get_string(mpig_cm_xio_vc_get_state(vc))));
	    }
	    
	    break;
	}
	
	case MPIG_CM_XIO_VC_STATE_DISCONNECT_SENDING_CLOSE_REQ_RECEIVED_CLOSE_REQ:
	{
	    /* received the ACK for the CR still being sent.  this can happen if the read callback occurs before the callback for
	       the write.  wait for send to complete. */
	    MPIU_Assert(cr == FALSE && ack == TRUE);
	    mpig_cm_xio_vc_set_state(vc, MPIG_CM_XIO_VC_STATE_DISCONNECT_SENDING_CLOSE_REQ_RECEIVED_ACK);
	    
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		"still sending CR; VC state updated; wait for send (CR) to complete: vc=" MPIG_PTR_FMT
		", new_vc_state=%s", (MPIG_PTR_CAST) vc, mpig_cm_xio_vc_state_get_string(mpig_cm_xio_vc_get_state(vc))));
	    
	    break;
	}
	
	case MPIG_CM_XIO_VC_STATE_DISCONNECT_SENT_CLOSE_REQ_AWAITING_CLOSE_REQ:
	{
	    /* CR has been sent, and a CR or CR/ACK has been received.  send an ACK to the remote process. */
	    MPIU_Assert(cr == TRUE);

	    mpig_cm_xio_disconnect_enq_close_msg(vc, FALSE, TRUE, mpi_errno_p, failed_p);
	    MPIU_ERR_CHKANDJUMP((*failed_p), *mpi_errno_p, MPI_ERR_OTHER, "**globus|cm_xio|disconnect_enq_close_msg");
	    if (ack == FALSE)
	    {
		/* received a CR after sending a CR.  this is a head-to-head disconnect scenario. */
		mpig_cm_xio_vc_set_state(vc, MPIG_CM_XIO_VC_STATE_DISCONNECT_SENDING_ACK_AWAITING_ACK);
		
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		    "ACK message enqueued; VC state updated; wait for ACK to arrive or send (ACK) to complete: vc=" MPIG_PTR_FMT
		    ", new_vc_state=%s", (MPIG_PTR_CAST) vc, mpig_cm_xio_vc_state_get_string(mpig_cm_xio_vc_get_state(vc))));
	    }
	    else
	    {
		/* received the CR/ACK after sending a CR */
		mpig_cm_xio_vc_set_state(vc, MPIG_CM_XIO_VC_STATE_DISCONNECT_SENDING_ACK_RECEIVED_ACK);
		
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		    "ACK message enqueued; VC state updated; wait for send (ACK) to complete: vc=" MPIG_PTR_FMT
		    ", new_vc_state=%s", (MPIG_PTR_CAST) vc, mpig_cm_xio_vc_state_get_string(mpig_cm_xio_vc_get_state(vc))));
	    }
	    
	    break;
	}
	
	case MPIG_CM_XIO_VC_STATE_DISCONNECT_SENDING_ACK_AWAITING_ACK:
	{
	    /* received an ACK, but still sending an ACK */
	    MPIU_Assert(cr == FALSE && ack == TRUE);
	    mpig_cm_xio_vc_set_state(vc, MPIG_CM_XIO_VC_STATE_DISCONNECT_SENDING_ACK_RECEIVED_ACK);
	    
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		"still sending ACK; VC state updated; wait for send (ACK) to complete: vc=" MPIG_PTR_FMT
		", new_vc_state=%s", (MPIG_PTR_CAST) vc, mpig_cm_xio_vc_state_get_string(mpig_cm_xio_vc_get_state(vc))));
	    
	    break;
	}
	
	case MPIG_CM_XIO_VC_STATE_DISCONNECT_SENT_ACK_AWAITING_ACK:
	{
	    /* received ACK from remote processs.  time to close the connection */
	    globus_result_t grc;
	    
	    grc = globus_xio_register_close(vc_cm->handle, NULL, mpig_cm_xio_disconnect_handle_close, vc);
	    MPIU_ERR_CHKANDJUMP1((grc), *mpi_errno_p, MPI_ERR_INTERN, "**globus|cm_xio|xio_reg_close",
		"**globus|cm_xio|xio_reg_close %s", globus_error_print_chain(globus_error_peek(grc)));
	    mpig_cm_xio_vc_set_state(vc, MPIG_CM_XIO_VC_STATE_DISCONNECT_CLOSING_VC);
		
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		"close registered; VC state updated; vc=" MPIG_PTR_FMT ", new_vc_state=%s",
		(MPIG_PTR_CAST) vc, mpig_cm_xio_vc_state_get_string(mpig_cm_xio_vc_get_state(vc))));
	    
	    break;
	}
	
	default:
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM | MPIG_DEBUG_LEVEL_VC,
		"encountered unexpected state: %s (%d)",
		mpig_cm_xio_vc_state_get_string(mpig_cm_xio_vc_get_state(vc)), mpig_cm_xio_vc_get_state(vc)));
	    MPIU_Assertp(FALSE && "encountered unexpected VC state after sending close message");
	    break;
	}   /* --END ERROR HANDLING-- */
    }
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"exiting: vc=" MPIG_PTR_FMT ", mpi_errno=0x%08x, failed=%s", (MPIG_PTR_CAST) vc,
	*mpi_errno_p, MPIG_BOOL_STR(*failed_p)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_disconnect_handle_recv_close_msg);
    return;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	MPIU_Assertp(FALSE && "XXX: FIX ERROR HANDLING CODE"); /* [BRT] */
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_disconnect_handle_recv_close_msg() */


/*
 * mpig_cm_xio_disconnect_handle_close([IN] handle, [IN] result, [IN] NULL)
 *
 * This function is called by globus_xio when a registered close operation has completed for a handle that has no VC associated
 * with it.
 *
 * Parameters:
 *
 * handle [IN] - globus_xio handle that has been closed, and thus is no longer valid
 * result [IN] - globus result indicating any error
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_disconnect_handle_close
MPIG_STATIC void mpig_cm_xio_disconnect_handle_close(globus_xio_handle_t handle, globus_result_t op_grc, void * arg)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vc_t * vc = (mpig_vc_t *) arg;
    struct mpig_cm_xio_vc * vc_cm = &vc->cm.xio;
    mpig_pg_t * pg;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_disconnect_handle_close);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_disconnect_handle_close);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"entering: vc= " MPIG_PTR_FMT ", op_grc=%d", (MPIG_PTR_CAST) vc, op_grc));

    MPIU_ERR_CHKANDJUMP1((op_grc), mpi_errno, MPI_ERR_INTERN, "**globus|cm_xio|disconnect_handle_close",
	"**globus|cm_xio|disconnect_handle_close %s", globus_error_print_chain(globus_error_peek(op_grc)));

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM | MPIG_DEBUG_LEVEL_VC,
	"VC disconnected: vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) vc));
    mpig_vc_mutex_lock(vc);
    {
	MPIU_Assert(mpig_cm_xio_vc_get_state(vc) == MPIG_CM_XIO_VC_STATE_DISCONNECT_CLOSING_VC);
	
	pg = mpig_vc_get_pg(vc);
	vc_cm->handle = NULL;
	mpig_cm_xio_vc_set_state(vc, MPIG_CM_XIO_VC_STATE_UNCONNECTED);

#if 0
	/* MPI-2-XXX: the following will be needed for MPI-2 disconnect/reconnect.  however, there is a race condition that must
	   be solved first.  the remote process may still be in the disconnect phase when its connection server receives the new
	   connection request.  in this case, the connection must be delayed, probably by having the connection server enqueue it
	   in a pending connections queue and the disconnect engine complete the connection if the VC is found in the queue. */
	if (mpig_cm_xio_sendq_empty(vc) == FALSE)
	{
	    bool_t failed;
	    
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM | MPIG_DEBUG_LEVEL_VC,
		"VC send queue not empty; initiating reconnect: vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) vc));
	    mpig_cm_xio_client_connect(vc, &mpi_errno, &failed);
	    MPIU_ERR_CHKANDJUMP((failed), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|connect");
	}
#else
	/* MPI-2-XXX: these assertions are only true for MPI-1 code.  they should be removed when MPI-2 support is added. */
	MPIU_Assert(mpig_cm_xio_sendq_empty(vc) == TRUE);
	MPIU_Assert(vc->ref_count == 0);
#endif

	/* the only reason that the VC reference count would not be zero is if the code above initiated a connect.  in that case,
	   we don't want to remove the VC from the list of active VCs. */
	if (vc->ref_count == 0)
	{
	    mpig_cm_xio_vc_list_remove(vc);
	}
    }
    mpig_vc_mutex_unlock(vc);
    mpig_pg_release_ref(pg);
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"exiting: vc= " MPIG_PTR_FMT ", mpi_errno=0x%08x", (MPIG_PTR_CAST) vc, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_disconnect_handle_close);
    return;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	/* report the error through MPID_Progress_{poke,test,wait}() */
	mpig_cm_xio_fault_handle_async_error(&mpi_errno);

	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_disconnect_handle_close() */

#endif /* MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS */
/**********************************************************************************************************************************
						     END CONNECTION TEARDOWN
**********************************************************************************************************************************/
