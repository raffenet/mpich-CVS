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

typedef enum mpig_cm_xio_open_resp
{
    MPIG_CM_XIO_OPEN_RESP_FIRST = 0,
    MPIG_CM_XIO_OPEN_RESP_UNDEFINED,
    MPIG_CM_XIO_OPEN_RESP_VC_ACK,
    MPIG_CM_XIO_OPEN_RESP_VC_NAK,
    MPIG_CM_XIO_OPEN_RESP_ERR_MSG_INCOMPLETE,
    MPIG_CM_XIO_OPEN_RESP_ERR_MSG_TYPE_BAD,
    MPIG_CM_XIO_OPEN_RESP_ERR_MSG_TOO_LONG,
    MPIG_CM_XIO_OPEN_RESP_ERR_VC_NOT_XIO,
    MPIG_CM_XIO_OPEN_RESP_ERR_VC_CONNECTED,
    MPIG_CM_XIO_OPEN_RESP_ERR_VC_DISCONNECTING,
    MPIG_CM_XIO_OPEN_RESP_ERR_VC_FAILED,
    MPIG_CM_XIO_OPEN_RESP_LAST
}
mpig_cm_xio_open_resp_t;


globus_xio_driver_t mpig_cm_xio_conn_transport_driver;
const char * mpig_cm_xio_conn_transport_driver_name = "tcp";
MPIG_STATIC globus_xio_stack_t mpig_cm_xio_conn_stack;
MPIG_STATIC globus_xio_attr_t mpig_cm_xio_conn_attrs;
MPIG_STATIC globus_xio_server_t mpig_cm_xio_server_handle;
MPIG_STATIC char * mpig_cm_xio_server_cs = NULL;


static int mpig_cm_xio_conn_init(void);

static int mpig_cm_xio_conn_finalize(void);

static int mpig_cm_xio_conn_destroy_tmp_vc(mpig_vc_t * tmp_vc);

static void mpig_cm_xio_conn_handle_close(globus_xio_handle_t handle, globus_result_t op_grc, void * arg);

static const char * mpig_cm_xio_conn_open_resp_get_string(mpig_cm_xio_open_resp_t resp);


#else /* defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS) */


/*
 * <mpi_errno> mpig_cm_xio_conn_init([IN/MOD] tmp_vc, [IN/OUT] mpi_errno)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_conn_init
static int mpig_cm_xio_conn_init(void)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    globus_result_t grc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_conn_init);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_conn_init);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "entering"));

    /* initialize the vc tracking list */
    mpig_cm_xio_vc_list_init();
    
    /* build stack of communication drivers */
    grc = globus_xio_driver_load(mpig_cm_xio_conn_transport_driver_name, &mpig_cm_xio_conn_transport_driver);
    if (grc)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM, "ERROR: call to %s() failed: driver=%s, msg=%s",
	    "globus_xio_driver_load", mpig_cm_xio_conn_transport_driver_name, globus_error_print_chain(globus_error_peek(grc))));
	MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|conn_driver_load_transport",
	    "**globus|cm_xio|conn_driver_destroy_load_transport %s %s", mpig_cm_xio_conn_transport_driver_name,
	    globus_error_print_chain(globus_error_peek(grc)));
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */

    grc = globus_xio_stack_init(&mpig_cm_xio_conn_stack, NULL);
    if (grc)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM, "ERROR: call to %s() failed: msg=%s",
	    "globus_xio_stack_init", globus_error_print_chain(globus_error_peek(grc))));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|conn_stack_init",
	    "**globus|cm_xio|conn_stack_init %s", globus_error_print_chain(globus_error_peek(grc)));
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */

    grc = globus_xio_stack_push_driver(mpig_cm_xio_conn_stack, mpig_cm_xio_conn_transport_driver);
    if (grc)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM, "ERROR: call to %s() failed: driver=%s, msg=%s",
	    "globus_xio_stack_push_driver", mpig_cm_xio_conn_transport_driver_name,
	    globus_error_print_chain(globus_error_peek(grc))));
	MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|conn_stack_push_driver",
	    "**globus|cm_xio|conn_stack_push_driver %s %s", mpig_cm_xio_conn_transport_driver_name,
	    globus_error_print_chain(globus_error_peek(grc)));
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */

#   if XXX_SECURE
    {
	globus_xio_driver_t gsi_driver;
	
        grc = globus_xio_driver_load("gsi", &gsi_driver);
	if (grc)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM, "ERROR: call to %s() failed: driver=%s, msg=%s",
		"globus_xio_driver_load", "gsi", globus_error_print_chain(globus_error_peek(grc))));
	    MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|conn_driver_load_transform",
		"**globus|cm_xio|conn_driver_destroy_load_transform %s %s", "gsi",
		globus_error_print_chain(globus_error_peek(grc)));
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

        grc = globus_xio_stack_push_driver(mpig_cm_xio_conn_stack, gsi_driver);
	if (grc)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM, "ERROR: call to %s() failed: driver=%s, msg=%s",
	    "globus_xio_stack_push_driver", "gsi", globus_error_print_chain(globus_error_peek(grc))));
	    MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|conn_stack_push_driver",
		"**globus|cm_xio|conn_stack_push_driver %s %s", "gsi", globus_error_print_chain(globus_error_peek(grc)));
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */
    }
#   endif


    /* set TCP options and parameters */
    grc = globus_xio_attr_init(&mpig_cm_xio_conn_attrs);
    if (grc)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM, "ERROR: call to %s() failed: msg=%s",
	    "globus_xio_attr_init", globus_error_print_chain(globus_error_peek(grc))));
	MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|conn_attr_create",
	    "**globus|cm_xio|conn_attr_create %s %s", "TCP_SET_NODELAY", globus_error_print_chain(globus_error_peek(grc)));
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */

    grc = globus_xio_attr_cntl(mpig_cm_xio_conn_attrs, mpig_cm_xio_conn_transport_driver, GLOBUS_XIO_TCP_SET_NODELAY, GLOBUS_TRUE);
    if (grc)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM, "ERROR: call to %s() failed: attr=%s, msg=%s",
	    "globus_xio_attr_cntl", "TCP_SET_NODELAY", globus_error_print_chain(globus_error_peek(grc))));
	MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|conn_attr_add",
	    "**globus|cm_xio|conn_attr_add %s %s", "TCP_SET_NODELAY", globus_error_print_chain(globus_error_peek(grc)));
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */

#   if XXX
    {
	grc = globus_xio_attr_cntl(mpig_cm_xio_conn_attrs, mpig_cm_xio_conn_transport_driver, GLOBUS_XIO_TCP_SET_SNDBUF, buf_size);
	if (grc)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM, "ERROR: call to %s() failed: attr=%s, msg=%s",
		"globus_xio_attr_cntl", "TCP_SET_SNDBUF", globus_error_print_chain(globus_error_peek(grc))));
	    MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|conn_attr_add",
		"**globus|cm_xio|conn_attr_add %s %s", "TCP_SET_SNDBUF", globus_error_print_chain(globus_error_peek(grc)));
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

	grc = globus_xio_attr_cntl(mpig_cm_xio_conn_attrs, mpig_cm_xio_conn_transport_driver, GLOBUS_XIO_TCP_SET_RCVBUF, buf_size);
	if (grc)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM, "ERROR: call to %s() failed: attr=%s, msg=%s",
		"globus_xio_attr_cntl", "TCP_SET_RCVBUF", globus_error_print_chain(globus_error_peek(grc))));
	    MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|conn_attr_add",
		"**globus|cm_xio|conn_attr_add %s %s", "TCP_SET_RCVBUF", globus_error_print_chain(globus_error_peek(grc)));
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */
    }
#    endif
    
    /* establish server to start listening for new connections */
    grc = globus_xio_server_create(&mpig_cm_xio_server_handle, mpig_cm_xio_conn_attrs, mpig_cm_xio_conn_stack);
    if (grc)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM, "ERROR: call to %s() failed: msg=%s",
	    "globus_xio_server_create", globus_error_print_chain(globus_error_peek(grc))));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|server_create",
	    "**globus|cm_xio|server_create %s", globus_error_print_chain(globus_error_peek(grc)));
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */

    grc = globus_xio_server_get_contact_string(mpig_cm_xio_server_handle, &mpig_cm_xio_server_cs);
    if (grc)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM, "ERROR: call to %s() failed: msg=%s",
	    "globus_xio_server_get_contact_string", globus_error_print_chain(globus_error_peek(grc))));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|server_get_cs",
	    "**globus|cm_xio|server_get_cs %s", globus_error_print_chain(globus_error_peek(grc)));
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */

    mpi_errno = mpig_cm_xio_server_listen(mpig_cm_xio_server_handle);
    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|server_listen");

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_conn_init);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_conn_init() */


/*
 * <mpi_errno> mpig_cm_xio_conn_finalize([IN/MOD] tmp_vc, [IN/OUT] mpi_errno)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_conn_finalize
static int mpig_cm_xio_conn_finalize(void)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    globus_result_t grc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_conn_finalize);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_conn_finalize);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "entering"));

    /* disable the connection server and free any resources used by it */
    grc = globus_xio_server_close(mpig_cm_xio_server_handle);
    if (grc)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM, "ERROR: call to %s() failed: msg=%s",
	    "globus_xio_server_close", globus_error_print_chain(globus_error_peek(grc))));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|xio_close_server",
	    "**globus|cm_xio|xio_close_server %s", globus_error_print_chain(globus_error_peek(grc)));
    }   /* --END ERROR HANDLING-- */

    /* free the memory used to store the contact string */
    if (mpig_cm_xio_server_cs != NULL)
    { 
	globus_libc_free(mpig_cm_xio_server_cs);
    }

    /* NOTE: prior to calling this routine, MPID_Finalize() dissolved all communicators, so there should not be any external VC
     * references outstanding.  that is not to say that the VCs are completely inactive.  some may be going through the close
     * protocol, while others may still be receiving messages from the remote process.  at this point message reception from a
     * remote process is only valid, meaning the application is a valid MPI application, if the remote process intends to cancel
     * those messages; otherwise, the messages will go unmatched, possibly causing the remote process to hang.  for example, a
     * rendezvous message arriving at the is point for which no matching receive exists would never be sent a clear-to-send.  as
     * a result, the remote process would hang waiting for the CTS, and this local process would hang wait for the ACK to its
     * close request.
     *
     * FIXME: the example above might be solved by having the RTS receive message handler detect the state of the connection and
     * send a NAK, allowing the remote process to return an error to the user.
     */
    
    /* wait for all of the connections to close */
    mpig_cm_xio_vc_list_wait_empty();
    
    /* shutdown the vc tracking list */
    mpig_cm_xio_vc_list_finalize();
    
    /* XXX: unload globus XIO drivers? */
    grc = globus_xio_attr_destroy(mpig_cm_xio_conn_attrs);
    if (grc)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM, "ERROR: call to %s() failed: msg=%s",
	    "globus_xio_attr_destroy", globus_error_print_chain(globus_error_peek(grc))));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|conn_attr_destroy",
	    "**globus|cm_xio|conn_attr_destroy %s", globus_error_print_chain(globus_error_peek(grc)));
    }   /* --END ERROR HANDLING-- */

    grc = globus_xio_stack_destroy(mpig_cm_xio_conn_stack);
    if (grc)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM, "ERROR: call to %s() failed: msg=%s",
	    "globus_xio_stack_destroy", globus_error_print_chain(globus_error_peek(grc))));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|conn_stack_destroy",
	    "**globus|cm_xio|conn_stack_destroy %s", globus_error_print_chain(globus_error_peek(grc)));
    }   /* --END ERROR HANDLING-- */

    grc = globus_xio_driver_unload(mpig_cm_xio_conn_transport_driver);
    if (grc)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM, "ERROR: call to %s() failed: driver=%s, msg=%s",
	    "globus_xio_driver_unload", mpig_cm_xio_conn_transport_driver_name, globus_error_print_chain(globus_error_peek(grc))));
	MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|conn_driver_unload_transport",
	    "**globus|cm_xio|conn_driver_destroy_unload_transport %s %s", mpig_cm_xio_conn_transport_driver_name,
	    globus_error_print_chain(globus_error_peek(grc)));
    }   /* --END ERROR HANDLING-- */

    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_conn_finalize);
    return mpi_errno;
}
/* mpig_cm_xio_conn_finalize() */


/*
 * <mpi_errno> mpig_cm_xio_conn_destroy_tmp_vc([IN/MOD] tmp_vc)
 *
 * MT-NOTE: this routine asssumes that the temp VC's mutex and the associated PG's mutex are _NOT_ held by the current context.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_conn_destroy_tmp_vc
static int mpig_cm_xio_conn_destroy_tmp_vc(mpig_vc_t * const tmp_vc)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    struct mpig_cm_xio_vc * const tmp_vc_cm = &tmp_vc->cm.xio;
    bool_t tmp_vc_destruct_now = FALSE;
    mpig_pg_t * pg = NULL;
    globus_result_t grc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_conn_destroy_tmp_vc);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_conn_destroy_tmp_vc);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "entering: tmp_vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) tmp_vc));

    /* get information about the PG and the XIO handle from the temp VC */
    mpig_vc_mutex_lock(tmp_vc);
    {
	pg = mpig_vc_get_pg(tmp_vc);
	
	if (tmp_vc_cm->handle != NULL)
	{
	    /* if the temp VC has an XIO handle associated with it, then close the handle first.  destruction of the temp VC is
	       delayed until the close operation completes, unless the register close fails. */

	    grc = globus_xio_register_close(tmp_vc_cm->handle, NULL, mpig_cm_xio_conn_handle_close, tmp_vc);
	    if (grc)
	    {   /* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM, "ERROR: call to %s() failed: %s",
		    "globus_xio_register_close", globus_error_print_chain(globus_error_peek(grc))));
		MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER,"**globus|cm_xio|xio_reg_close",
		    "**globus|cm_xio|xio_reg_close %s", globus_error_print_chain(globus_error_peek(grc)));

		/*FIXME: the reason we defer destruction of the temp VC until the close operation completes is to prevent a
		  pending read or write operation's callback from firing after the destruction of the temp VC object.  by
		  destroying the temp VC when register close fails, it may be possible for a handler to fire afterwards and
		  attempt to lock a VC that is no longer valid.  the only way I can see to prevent this from happening is to
		  apply a reference count to the temp VC to insure that all pending operation have completed.  however, this is a
		  nontrivial change because appropriate increments and decrements would need to be thread throughout this module,
		  and handled properly in the face of errors which is tricky.  thus far, the problem has only appeared once, and
		  that was do to a bug in the server code.  so, in the interest of getting a release out the door, adding
		  reference counts to the temp VCs is left as a future task. */
		tmp_vc_destruct_now = TRUE;
	    }   /* --END ERROR HANDLING-- */

	    tmp_vc_cm->handle = NULL;
	}
	else
	{
	    /* no XIO handle is associated with the temp VC, destroy the temp VC now */
	    tmp_vc_destruct_now = TRUE;
	}

	if (tmp_vc_destruct_now)
	{
	    mpig_cm_xio_vc_list_remove(tmp_vc);
	}
    }
    mpig_vc_mutex_unlock(tmp_vc);

    if (tmp_vc_destruct_now)
    {
	/* if the temp VC holds a reference to the the PG, then release the PG */
	if (pg != NULL) mpig_pg_release_ref(pg);

	/* destroy the temp VC and release the memory used by it */
	mpig_vc_destruct(tmp_vc);
	MPIU_Free(tmp_vc);
    }

    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "exiting: tmp_vc=" MPIG_PTR_FMT ", mpi_errno="
	MPIG_ERRNO_FMT, (MPIG_PTR_CAST) tmp_vc, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_conn_destroy_tmp_vc);
    return mpi_errno;
}
/* mpig_cm_xio_conn_destroy_tmp_vc() */


/*
 * <void> mpig_cm_xio_conn_handle_close([IN] handle, [IN] result, [IN] tmp_vc)
 *
 * This function is called by globus_xio when a registered close operation has completed for a handle typically associated with a
 * temporary VC.
 *
 * Parameters:
 *
 * handle [IN] - globus_xio handle that has been closed, and thus is no longer valid
 *
 * result [IN] - globus result indicating any error
 *
 * tmp_vc [IN] - the temporary VC to be destroyed.  this value may be NULL, in which case the only effect is to close the handle.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_conn_handle_close
static void mpig_cm_xio_conn_handle_close(globus_xio_handle_t handle, globus_result_t op_grc, void * arg)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vc_t * const tmp_vc = (mpig_vc_t *) arg;
    mpig_pg_t * pg = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_conn_handle_close);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_conn_handle_close);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "entering: op_grc=%d", op_grc));

    if (op_grc)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM, "ERROR: %s operation failed: %s",
	    "globus_xio_register_close", globus_error_print_chain(globus_error_peek(op_grc))));
	/*
	 * XXX: should we ignore errors associated with closing a temp VC?  an error should only occur if data in internal
	 * buffers could not be sent.  this seems serious, but should it be reported?  might such an error cause the remote
	 * process to hang?  if so, then we need to report the error; otherwise ignoring it seems the safest bet.
	 *
	 * MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|conn_handle_close",
	 *    "**globus|conn_handle_close %s", globus_error_print_chain(globus_error_peek(op_grc)));
	 */
    }   /* --END ERROR HANDLING-- */

    if (tmp_vc != NULL)
    {
	/* get information about the PG from the VC, and remove the VC from the VC tracking list */
	mpig_vc_mutex_lock(tmp_vc);
	{
	    pg = mpig_vc_get_pg(tmp_vc);
	    mpig_cm_xio_vc_list_remove(tmp_vc);
	}
	mpig_vc_mutex_unlock(tmp_vc);

	/* if the temp VC holds a reference to the the PG, then release the PG */
	if (pg != NULL) mpig_pg_release_ref(pg);

	/* destroy the temp VC and release the memory used by it */
	mpig_vc_destruct(tmp_vc);
	MPIU_Free(tmp_vc);
    }

    if (mpi_errno) goto fn_fail;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_conn_handle_close);
    return;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	/* report the error through MPID_Progress_{poke,test,wait}() */
	mpig_cm_xio_fault_handle_async_error(mpi_errno);

	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_conn_handle_close() */

/*
 * <const char *> mpig_cm_xio_conn_open_resp_get_string([IN] open_resp)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_conn_open_resp_get_string
static const char * mpig_cm_xio_conn_open_resp_get_string(const mpig_cm_xio_open_resp_t open_resp)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const char * str;
    
    MPIG_UNUSED_VAR(fcname);

    switch(open_resp)
    {
	case MPIG_CM_XIO_OPEN_RESP_FIRST:
	    str = "MPIG_CM_XIO_OPEN_RESP_FIRST";
	    break;
	case MPIG_CM_XIO_OPEN_RESP_UNDEFINED:
	    str = "MPIG_CM_XIO_OPEN_RESP_UNDEFINED";
	    break;
	case MPIG_CM_XIO_OPEN_RESP_VC_ACK:
	    str = "MPIG_CM_XIO_OPEN_RESP_VC_ACK";
	    break;
	case MPIG_CM_XIO_OPEN_RESP_VC_NAK:
	    str = "MPIG_CM_XIO_OPEN_RESP_VC_NAK";
	    break;
	case MPIG_CM_XIO_OPEN_RESP_ERR_MSG_INCOMPLETE:
	    str = "MPIG_CM_XIO_OPEN_RESP_ERR_MSG_INCOMPLETE";
	    break;
	case MPIG_CM_XIO_OPEN_RESP_ERR_MSG_TYPE_BAD:
	    str = "MPIG_CM_XIO_OPEN_RESP_ERR_MSG_TYPE_BAD";
	    break;
	case MPIG_CM_XIO_OPEN_RESP_ERR_MSG_TOO_LONG:
	    str = "MPIG_CM_XIO_OPEN_RESP_ERR_MSG_TOO_LONG";
	    break;
	case MPIG_CM_XIO_OPEN_RESP_ERR_VC_NOT_XIO:
	    str = "MPIG_CM_XIO_OPEN_RESP_ERR_VC_NOT_XIO";
	    break;
	case MPIG_CM_XIO_OPEN_RESP_ERR_VC_CONNECTED:
	    str = "MPIG_CM_XIO_OPEN_RESP_ERR_VC_CONNECTED";
	    break;
	case MPIG_CM_XIO_OPEN_RESP_ERR_VC_DISCONNECTING:
	    str = "MPIG_CM_XIO_OPEN_RESP_ERR_VC_DISCONNECTING";
	    break;
	case MPIG_CM_XIO_OPEN_RESP_ERR_VC_FAILED:
	    str = "MPIG_CM_XIO_OPEN_RESP_ERR_VC_FAILED";
	    break;
	case MPIG_CM_XIO_OPEN_RESP_LAST:
	    str = "MPIG_CM_XIO_OPEN_RESP_LAST";
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

static int mpig_cm_xio_server_listen(globus_xio_server_t server);

static void mpig_cm_xio_server_handle_connection(
    globus_xio_server_t server, globus_xio_handle_t handle, globus_result_t op_grc, void * arg);

static void mpig_cm_xio_server_handle_open(globus_xio_handle_t handle, globus_result_t op_grc, void * arg);

static void mpig_cm_xio_server_handle_recv_magic(
    globus_xio_handle_t handle, globus_result_t op_grc, globus_byte_t * rbuf, globus_size_t rbuf_len, globus_size_t nbytes,
    globus_xio_data_descriptor_t dd, void * arg);

static void mpig_cm_xio_server_handle_send_magic(
    globus_xio_handle_t handle, globus_result_t op_grc, globus_byte_t * rbuf, globus_size_t rbuf_len, globus_size_t nbytes,
    globus_xio_data_descriptor_t dd, void * arg);

static void mpig_cm_xio_server_handle_recv_incoming_open_req(
    globus_xio_handle_t handle, globus_result_t op_grc, globus_byte_t * rbuf, globus_size_t rbuf_len, globus_size_t nbytes,
    globus_xio_data_descriptor_t dd, void * arg);

static int mpig_cm_xio_server_handle_recv_open_vc_req(mpig_vc_t * tmp_vc);

static int mpig_cm_xio_server_send_open_vc_resp(mpig_vc_t * tmp_vc, mpig_cm_xio_open_resp_t resp,
    globus_xio_data_callback_t callback_fn);

static void mpig_cm_xio_server_handle_send_open_resp_vc_ack(
    globus_xio_handle_t handle, globus_result_t op_grc, globus_byte_t * rbuf, globus_size_t rbuf_len, globus_size_t nbytes,
    globus_xio_data_descriptor_t dd, void * arg);

static void mpig_cm_xio_server_handle_send_open_resp_vc_nak(
    globus_xio_handle_t handle, globus_result_t op_grc, globus_byte_t * rbuf, globus_size_t rbuf_len, globus_size_t nbytes,
    globus_xio_data_descriptor_t dd, void * arg);

static int mpig_cm_xio_server_send_open_error_resp(mpig_vc_t * tmp_vc, mpig_cm_xio_open_resp_t resp);

static void mpig_cm_xio_server_handle_send_open_error_resp(
    globus_xio_handle_t handle, globus_result_t op_grc, globus_byte_t * rbuf, globus_size_t rbuf_len, globus_size_t nbytes,
    globus_xio_data_descriptor_t dd, void * arg);

static int mpig_cm_xio_server_handle_recv_open_port_req(mpig_vc_t * tmp_vc);


#define MPIG_CM_XIO_CONN_VOTE(pg_id_, pg_rank_)											\
    ((mpig_pg_compare_ids(mpig_process.my_pg->id, (pg_id_)) < 0 ||								\
     (mpig_pg_compare_ids(mpig_process.my_pg->id, (pg_id_)) == 0 && mpig_process.my_pg_rank < (pg_rank_))) ? TRUE : FALSE)


#else /* defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS) */


/*
 * <mpi_errno> mpig_cm_xio_server_listen([IN] server)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_server_listen
static int mpig_cm_xio_server_listen(globus_xio_server_t server)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    globus_result_t grc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_server_listen);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_server_listen);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "entering"));

    MPIG_UNUSED_VAR(fcname);

    grc = globus_xio_server_register_accept(server, mpig_cm_xio_server_handle_connection, NULL);
    if (grc)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM, "ERROR: call to %s() failed: %s",
	    "globus_xio_server_register_accept", globus_error_print_chain(globus_error_peek(grc))));
	MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {;}, "**globus|cm_xio|xio_server_reg_accept",
	    "**globus|cm_xio|xio_server_reg_accept %s", globus_error_print_chain(globus_error_peek(grc)));
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_server_listen);
    return mpi_errno;
    
  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_server_listen() */


/*
 * void mpig_cm_xio_server_handle_connection([IN] server, [IN] handle, [IN] op_grc, [IN] NULL)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_server_handle_connection
static void mpig_cm_xio_server_handle_connection(const globus_xio_server_t server, const globus_xio_handle_t handle,
    const globus_result_t op_grc, void * const arg)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vc_t * tmp_vc = NULL;
    globus_result_t grc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_server_handle_connection);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_server_handle_connection);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "entering: handle=" MPIG_PTR_FMT ", op_grc=%d",
	(MPIG_PTR_CAST) handle, op_grc));

    /* if the accept was cancelled, then we are likely shutting down.  XXX: add code to verify this? */
    if (globus_xio_error_is_canceled(op_grc))
    {
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM, "register accept cancelled; the XIO CM must be shutting down."));
	goto fn_return;
    }
    
    if (op_grc) goto fn_fail;
    
    /* allocate a temporary VC for use during connection establishment */
    tmp_vc = (mpig_vc_t *) MPIU_Malloc(sizeof(mpig_vc_t));
    if (tmp_vc == NULL)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM,
	    "ERROR: malloc failed when attempting to allocate a temp VC for an incoming connection"));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "temp VC for an incoming connection");
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */
    
    /* initialize the temp VC object */
    mpig_vc_construct(tmp_vc);
    mpig_vc_mutex_lock(tmp_vc);
    {
	mpig_cm_xio_vc_construct(tmp_vc);
	mpig_cm_xio_vc_set_state(tmp_vc, MPIG_CM_XIO_VC_STATE_SERVER_OPEN);
	mpig_cm_xio_vc_list_add(tmp_vc);
	tmp_vc->cm.xio.handle = handle;
    }
    mpig_vc_mutex_unlock(tmp_vc);
	
    /* complete the formation of the new connection */
    grc = globus_xio_register_open(handle, NULL, mpig_cm_xio_conn_attrs, mpig_cm_xio_server_handle_open, tmp_vc);
    if (grc)
    {   /* --END ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM, "ERROR: call to %s() failed: %s",
	    "globus_xio_register_open", globus_error_print_chain(globus_error_peek(grc))));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|server_handle_connection",
	    "**globus|cm_xio|server_handle_connection %s", globus_error_print_chain(globus_error_peek(grc)));

	goto fn_fail;
    }   /* --END ERROR HANDLING-- */

    mpig_cm_xio_mutex_lock();
    {
	mpig_cm_xio_server_accept_errcnt = 0;
    }
    mpig_cm_xio_mutex_unlock();
    
    /* tell the server to continue listening for new connections */
    mpi_errno = mpig_cm_xio_server_listen(server);
    if (mpi_errno)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR, "FATAL ERROR: the server failed to register a new accept operation"));
	MPIU_ERR_SETFATALANDSTMT(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|cm_xio|server_listen");
	goto fn_fail;
    }	/* --END ERROR HANDLING-- */

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_server_handle_connection);
    return;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	/* if a handle was passed in, then close it */
	if (op_grc == GLOBUS_SUCCESS)
	{
	    /* close the connection, and destroy the temp VC object if one was created*/
	    if (tmp_vc != NULL)
	    {
		int mrc;
		mrc = mpig_cm_xio_conn_destroy_tmp_vc(tmp_vc);
		MPIU_ERR_CHKANDSTMT((mrc), mrc, MPI_ERR_OTHER, {MPIU_ERR_ADD(mpi_errno, mrc);},
		    "**globus|cm_xio|tmp_vc_destroy");
	    }
	    else
	    {
		grc = globus_xio_register_close(handle, NULL, mpig_cm_xio_conn_handle_close, NULL);
		if (grc)
		{
		    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM, "ERROR: call to %s() failed: %s",
			"globus_xio_register_close", globus_error_print_chain(globus_error_peek(grc))));
		    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER,"**globus|cm_xio|xio_reg_close",
			"**globus|cm_xio|xio_reg_close %s", globus_error_print_chain(globus_error_peek(grc)));
		}
	    }
	}
	
	mpig_cm_xio_mutex_lock();
	{
	    mpig_cm_xio_server_accept_errcnt += 1;
	    
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM,
		"WARNING: %s() operation failed for the %d time: %s", "globus_xio_server_register_accept",
		mpig_cm_xio_server_accept_errcnt, globus_error_print_chain(globus_error_peek(op_grc))));

	    /* check for repeated connection attempt errors */
	    if (mpig_cm_xio_server_accept_errcnt > MPIG_CM_XIO_MAX_ACCEPT_ERRORS)
	    {
		MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|server_handle_connection",
		    "**globus|cm_xio|server_handle_connection %s", globus_error_print_chain(globus_error_peek(op_grc)));
		
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR,
		    "FATAL ERROR: server has repeatedly failed to establish new connections; reporting error: errcnt=%d",
		    mpig_cm_xio_server_accept_errcnt));
		MPIU_ERR_SETFATALANDSTMT(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|cm_xio|server_multiple_failures");
	    }
	}
	mpig_cm_xio_mutex_unlock();
    
	/* report the error through MPID_Progress_{poke,test,wait}() */
	if (mpi_errno) mpig_cm_xio_fault_handle_async_error(mpi_errno);
	
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_server_handle_connection() */


/*
 * void mpig_cm_xio_server_handle_open([IN] handle, [IN] result, [IN/MOD] vc)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_server_handle_open
static void mpig_cm_xio_server_handle_open(const globus_xio_handle_t handle, const globus_result_t op_grc, void * const arg)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vc_t * const tmp_vc = (mpig_vc_t *) arg;
    bool_t tmp_vc_locked = FALSE;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_server_handle_open);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_server_handle_open);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "entering: handle=" MPIG_PTR_FMT ", op_grc=%d",
	(MPIG_PTR_CAST) handle, op_grc));

    if (op_grc)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM, "ERROR: %s operation failed: handle=" MPIG_PTR_FMT
	    ", msg=%s", "globus_xio_register_open", (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) handle,
	    globus_error_print_chain(globus_error_peek(op_grc))));
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */
    
    mpig_vc_mutex_lock(tmp_vc);
    tmp_vc_locked = TRUE;
    {
	const size_t magic_size = strlen(MPIG_CM_XIO_PROTO_CONNECT_MAGIC);
	
	MPIU_Assert(mpig_cm_xio_vc_get_state(tmp_vc) == MPIG_CM_XIO_VC_STATE_SERVER_OPEN);
	
	/* Register a receive for the magic string.  The magic string exchange is used just to verify that an incoming connection
	   is most probably a MPIG process, and not a port scanner.  The exchange does not prevent malicious attacks. */
	mpig_cm_xio_register_read_vc_msgbuf(tmp_vc, magic_size, magic_size, mpig_cm_xio_server_handle_recv_magic, &mpi_errno);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM,
		"ERROR: registering a read operation to receive the magic string from the client failed"));
	    MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|server_reg_recv_magic");
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */
	
	mpig_cm_xio_vc_set_state(tmp_vc, MPIG_CM_XIO_VC_STATE_SERVER_RECEIVING_MAGIC);
    }
    mpig_vc_mutex_unlock(tmp_vc);
    tmp_vc_locked = FALSE;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "exiting: tmp_vc=" MPIG_PTR_FMT ", mpi_errno="
	MPIG_ERRNO_FMT, (MPIG_PTR_CAST) tmp_vc, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_server_handle_open);
    return;
    
  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	int mrc;

	/* release the temp VC mutex if it is being held by the current context */
	mpig_vc_mutex_unlock_conditional(tmp_vc, (tmp_vc_locked));
	    
	/* close the connection and destroy the temp VC object, releasing the associated PG reference */
	mrc = mpig_cm_xio_conn_destroy_tmp_vc(tmp_vc);
	MPIU_ERR_CHKANDSTMT((mrc), mrc, MPI_ERR_OTHER, {MPIU_ERR_ADD(mpi_errno, mrc);}, "**globus|cm_xio|tmp_vc_destroy");
	
	/* report any errors through MPID_Progress_{poke,test,wait}() */
	if (mpi_errno) mpig_cm_xio_fault_handle_async_error(mpi_errno);

	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_server_handle_open() */


/*
 * <void> mpig_cm_xio_server_handle_recv_magic([IN] handle, [IN] result, [IN] rbuf, [IN] rbuf_len, [IN] nbytes, [IN] NULL,
 *     [IN/MOD] vc)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_server_handle_recv_magic
static void mpig_cm_xio_server_handle_recv_magic(const globus_xio_handle_t handle, const globus_result_t op_grc,
    globus_byte_t * const rbuf, const globus_size_t rbuf_len, const globus_size_t nbytes, const globus_xio_data_descriptor_t dd,
    void * const arg)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vc_t * const tmp_vc = (mpig_vc_t *) arg;
    struct mpig_cm_xio_vc * const tmp_vc_cm = &tmp_vc->cm.xio;
    bool_t tmp_vc_locked = FALSE;
    int rc;
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
	MPIU_Assert(mpig_cm_xio_vc_get_state(tmp_vc) == MPIG_CM_XIO_VC_STATE_SERVER_RECEIVING_MAGIC);
	
	if (op_grc)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM, "ERROR: %s operation failed: tmp_vc=" MPIG_PTR_FMT
		", handle=" MPIG_PTR_FMT ", msg=%s", "globus_xio_register_recv", (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) handle,
		globus_error_print_chain(globus_error_peek(op_grc))));
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

	MPIU_Assert(nbytes == strlen(MPIG_CM_XIO_PROTO_CONNECT_MAGIC));

	/* adjust end of message buffer to account for the number of bytes received */
	mpig_databuf_inc_eod(tmp_vc_cm->msgbuf, nbytes);

	/* compare the string to make sure the expect value was received. */
	rc = strncmp(mpig_databuf_get_ptr(tmp_vc_cm->msgbuf), MPIG_CM_XIO_PROTO_CONNECT_MAGIC,
	    strlen(MPIG_CM_XIO_PROTO_CONNECT_MAGIC));
	if (rc)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM, "ERROR: magic string received from the client "
		"did not match expected value: tmp_vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) tmp_vc));
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM, "magic string matched; sending magic string to the client: tmp_vc="
	    MPIG_PTR_FMT, (MPIG_PTR_CAST) tmp_vc));
	
	/* copy magic string into the VC message buffer */
	mpig_databuf_reset(tmp_vc_cm->msgbuf);
	rc = MPIU_Strncpy(mpig_databuf_get_ptr(tmp_vc_cm->msgbuf), MPIG_CM_XIO_PROTO_ACCEPT_MAGIC,
	    mpig_databuf_get_size(tmp_vc_cm->msgbuf));
	MPIU_Assert(rc == 0);
	mpig_databuf_inc_eod(tmp_vc_cm->msgbuf, strlen(MPIG_CM_XIO_PROTO_ACCEPT_MAGIC));

	/* Send the magic string */
	mpig_cm_xio_register_write_vc_msgbuf(tmp_vc, mpig_cm_xio_server_handle_send_magic, &mpi_errno);
	MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|server_reg_send_magic");

	mpig_cm_xio_vc_set_state(tmp_vc, MPIG_CM_XIO_VC_STATE_SERVER_SENDING_MAGIC);
    }
    mpig_vc_mutex_unlock(tmp_vc);
    tmp_vc_locked = FALSE;

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"exiting: tmp_vc=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, (MPIG_PTR_CAST) tmp_vc, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_server_handle_recv_magic);
    return;
    
  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	int mrc;
	
	/* release the temp VC mutex if it is being held by the current context */
	mpig_vc_mutex_unlock_conditional(tmp_vc, (tmp_vc_locked));
	
	/* close the connection and destroy the temp VC object, releasing the associated PG reference */
	mrc = mpig_cm_xio_conn_destroy_tmp_vc(tmp_vc);
	MPIU_ERR_CHKANDSTMT((mrc), mrc, MPI_ERR_OTHER, {MPIU_ERR_ADD(mpi_errno, mrc);}, "**globus|cm_xio|tmp_vc_destroy");
    
	/* report the error through MPID_Progress_{poke,test,wait}() */
	if (mpi_errno) mpig_cm_xio_fault_handle_async_error(mpi_errno);

	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_server_handle_recv_magic() */


/*
 * void mpig_cm_xio_server_handle_send_magic(
 *          [IN] handle, [IN] result, [IN] sbuf, [IN] sbuf_len, [IN] nbytes, [IN] NULL, [IN/MOD] vc)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_server_handle_send_magic
static void mpig_cm_xio_server_handle_send_magic(
    const globus_xio_handle_t handle, const globus_result_t op_grc, globus_byte_t * const sbuf, globus_size_t const sbuf_len,
    const globus_size_t nbytes, const globus_xio_data_descriptor_t dd, void * const arg)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vc_t * const tmp_vc = (mpig_vc_t *) arg;
    struct mpig_cm_xio_vc * const tmp_vc_cm = &tmp_vc->cm.xio;
    bool_t tmp_vc_locked = FALSE;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_server_handle_send_magic);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_server_handle_send_magic);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "entering: tmp_vc=" MPIG_PTR_FMT ", handle=" MPIG_PTR_FMT
	", op_grc=%d, sbuf=" MPIG_PTR_FMT ", sbuf_len=" MPIG_SIZE_FMT ", nbytes=" MPIG_SIZE_FMT, (MPIG_PTR_CAST) tmp_vc,
	(MPIG_PTR_CAST) handle, op_grc, (MPIG_PTR_CAST) sbuf, sbuf_len,	nbytes));

    mpig_vc_mutex_lock(tmp_vc);
    tmp_vc_locked = TRUE;
    {
	MPIU_Assert(mpig_cm_xio_vc_get_state(tmp_vc) == MPIG_CM_XIO_VC_STATE_SERVER_SENDING_MAGIC);
	
	if (op_grc)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM, "ERROR: %s operation failed: tmp_vc=" MPIG_PTR_FMT
		", handle=" MPIG_PTR_FMT ", msg=%s", "globus_xio_register_write", (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) handle,
		globus_error_print_chain(globus_error_peek(op_grc))));
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */
    
	MPIU_Assert(nbytes == strlen(MPIG_CM_XIO_PROTO_ACCEPT_MAGIC));
	
	/* register a read operation to receive the open request control message */
	mpig_databuf_reset(tmp_vc_cm->msgbuf);
	tmp_vc_cm->msg_hdr_size = 0;
	mpig_cm_xio_register_read_vc_msgbuf(tmp_vc, mpig_cm_xio_msg_hdr_remote_sizeof_msg_size(tmp_vc),
	    mpig_databuf_get_size(tmp_vc_cm->msgbuf), mpig_cm_xio_server_handle_recv_incoming_open_req, &mpi_errno);
	MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|server_reg_recv_open_req");
	
	mpig_cm_xio_vc_set_state(tmp_vc, MPIG_CM_XIO_VC_STATE_SERVER_RECEIVING_OPEN_REQ);
    }
    mpig_vc_mutex_unlock(tmp_vc);
    tmp_vc_locked = FALSE;

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "exiting: tmp_vc=" MPIG_PTR_FMT ", mpi_errno="
	MPIG_ERRNO_FMT, (MPIG_PTR_CAST) tmp_vc, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_server_handle_send_magic);
    return;
    
  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	int mrc;
	
	/* release the temp VC mutex if it is being held by the current context */
	mpig_vc_mutex_unlock_conditional(tmp_vc, (tmp_vc_locked));
    
	/* close the connection and destroy the temp VC object, releasing the associated PG reference */
	mrc = mpig_cm_xio_conn_destroy_tmp_vc(tmp_vc);
	MPIU_ERR_CHKANDSTMT((mrc), mrc, MPI_ERR_OTHER, {MPIU_ERR_ADD(mpi_errno, mrc);}, "**globus|cm_xio|tmp_vc_destroy");

	/* report the error through MPID_Progress_{poke,test,wait}() */
	if (mpi_errno) mpig_cm_xio_fault_handle_async_error(mpi_errno);

	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_server_handle_send_magic() */


/*
 * <void> mpig_cm_xio_server_handle_recv_incoming_open_req([IN] handle, [IN] result, [IN] rbuf, [IN] rbuf_len, [IN] nbytes,
 *     [IN] NULL, [IN/MOD] vc)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_server_handle_recv_incoming_open_req
static void mpig_cm_xio_server_handle_recv_incoming_open_req(const globus_xio_handle_t handle, const globus_result_t op_grc,
    globus_byte_t * const rbuf, const globus_size_t rbuf_len, const globus_size_t nbytes, const globus_xio_data_descriptor_t dd,
    void * const arg)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vc_t * const tmp_vc = (mpig_vc_t *) arg;
    struct mpig_cm_xio_vc * const tmp_vc_cm = &tmp_vc->cm.xio;
    bool_t tmp_vc_locked = FALSE;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_server_handle_recv_incoming_open_req);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_server_handle_recv_incoming_open_req);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"entering: tmp_vc=" MPIG_PTR_FMT ", handle=" MPIG_PTR_FMT ", op_grc=%d, rbuf=" MPIG_PTR_FMT ", rbuf_len=" MPIG_SIZE_FMT
	", nbytes=" MPIG_SIZE_FMT, (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) handle, op_grc, (MPIG_PTR_CAST) rbuf, rbuf_len,
	nbytes));

    MPIG_UNUSED_VAR(fcname);

    /* if the receive failed, then the connection is probably toast.  close the handle and free the temp VC. */
	
    mpig_vc_mutex_lock(tmp_vc);
    tmp_vc_locked = TRUE;
    {
	mpig_cm_xio_msg_type_t req_msg_type;

	MPIU_Assert(mpig_cm_xio_vc_get_state(tmp_vc) == MPIG_CM_XIO_VC_STATE_SERVER_RECEIVING_OPEN_REQ);
	
	if (op_grc)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM, "ERROR: %s operation failed: tmp_vc=" MPIG_PTR_FMT
		", handle=" MPIG_PTR_FMT ", msg=%s", "globus_xio_register_write", (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) handle,
		globus_error_print_chain(globus_error_peek(op_grc))));
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */
    
	/* adjust the end of the message buffer to account for the number of bytes received */
	mpig_databuf_inc_eod(tmp_vc_cm->msgbuf, nbytes);

	if (tmp_vc_cm->msg_hdr_size == 0)
	{
	    /* if the message header size has not been determined, then extract the header size.  if the message buffer does not
	       contain the complete header, the register a read for the the remaining bytes.  assuming an error does not occur,
	       the entire header will be in the VC message buffer when the next callback occurs. */
	    MPIU_Assert(nbytes >= mpig_cm_xio_msg_hdr_remote_sizeof_msg_size(tmp_vc));
	    mpig_cm_xio_msg_hdr_get_msg_size(tmp_vc, tmp_vc_cm->msgbuf, &tmp_vc_cm->msg_hdr_size);
	    tmp_vc_cm->msg_hdr_size -= mpig_cm_xio_msg_hdr_remote_sizeof_msg_size(tmp_vc);

	    if (mpig_databuf_get_remaining_bytes(tmp_vc_cm->msgbuf) < tmp_vc_cm->msg_hdr_size)
	    {
		MPIU_Size_t bytes_needed = tmp_vc_cm->msg_hdr_size - mpig_databuf_get_remaining_bytes(tmp_vc_cm->msgbuf);
		
		/* not all of the open request header was received.  try to get the remainder of it. */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM, "not all of the open request message was received; posting a read for "
		    "the remaining bytes, tmp_vc=" MPIG_PTR_FMT ", nbytes=%u", (MPIG_PTR_CAST) tmp_vc, (unsigned) bytes_needed));
	    
		mpig_cm_xio_register_read_vc_msgbuf(tmp_vc, bytes_needed, bytes_needed,
		    mpig_cm_xio_server_handle_recv_incoming_open_req, &mpi_errno);
		MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|server_reg_recv_open_req");

		goto tmp_vc_unlock;
	    }
	}
	
	if (mpig_databuf_get_remaining_bytes(tmp_vc_cm->msgbuf) == tmp_vc_cm->msg_hdr_size)
	{
	    /* get message type and verify it is an open request */
	    mpig_cm_xio_msg_hdr_get_msg_type(tmp_vc, tmp_vc_cm->msgbuf, &req_msg_type);
	    
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM, "message received: tmp_vc=" MPIG_PTR_FMT ", msg_type_id=%d, msg_type=%s",
		(MPIG_PTR_CAST) tmp_vc, req_msg_type, mpig_cm_xio_msg_type_get_string(req_msg_type)));
	    
	    switch (req_msg_type)
	    {
		case MPIG_CM_XIO_MSG_TYPE_OPEN_VC_REQ:
		{
		    mpi_errno = mpig_cm_xio_server_handle_recv_open_vc_req(tmp_vc);
		    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|server_handle_recv_open_vc_req");
		    break;
		}
	    
		case MPIG_CM_XIO_MSG_TYPE_OPEN_PORT_REQ:
		{
		    mpi_errno = mpig_cm_xio_server_handle_recv_open_port_req(tmp_vc);
		    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|server_handle_recv_open_port_req");
		    break;
		}

		default:
		{   /* --BEGIN ERROR HANDLING-- */
		    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM, "ERROR: message received is not a valid "
			"open request: tmp_vc=" MPIG_PTR_FMT ", msg_type=%d", (MPIG_PTR_CAST) tmp_vc, (int) req_msg_type));

		    mpi_errno = mpig_cm_xio_server_send_open_error_resp(tmp_vc, MPIG_CM_XIO_OPEN_RESP_ERR_MSG_TYPE_BAD);
		    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|server_reg_send_open_error_resp");
		    break;
		}   /* --END ERROR HANDLING-- */
	    }
	}
	else
	{   /* --BEGIN ERROR HANDLING-- */
	    /* if all of the message header is not available, then report an incomplete message eror to the remote side */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM, "ERROR: message received is incomplete: tmp_vc="
		MPIG_PTR_FMT ", expected=%d, received=" MPIG_SIZE_FMT, (MPIG_PTR_CAST) tmp_vc, tmp_vc_cm->msg_hdr_size,
		mpig_databuf_get_remaining_bytes(tmp_vc_cm->msgbuf)));
	    
	    mpi_errno = mpig_cm_xio_server_send_open_error_resp(tmp_vc, MPIG_CM_XIO_OPEN_RESP_ERR_MSG_INCOMPLETE);
	    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|server_reg_send_open_error_resp");
	}   /* --END ERROR HANDLING-- */

      tmp_vc_unlock: ;
    }
    mpig_vc_mutex_unlock(tmp_vc);
    tmp_vc_locked = FALSE;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "exiting: tmp_vc=" MPIG_PTR_FMT ", mpi_errno="
	MPIG_ERRNO_FMT, (MPIG_PTR_CAST) tmp_vc, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_server_handle_recv_incoming_open_req);
    return;
    
  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	int mrc;
	
	/* release the temp VC mutex if it is being held by the current context */
	mpig_vc_mutex_unlock_conditional(tmp_vc, (tmp_vc_locked));
    
	/* close the connection and destroy the temp VC object, releasing the associated PG reference */
	mrc = mpig_cm_xio_conn_destroy_tmp_vc(tmp_vc);
	MPIU_ERR_CHKANDSTMT((mrc), mrc, MPI_ERR_OTHER, {MPIU_ERR_ADD(mpi_errno, mrc);}, "**globus|cm_xio|tmp_vc_destroy");
	
	/* report the error(s) through MPID_Progress_{poke,test,wait}() */
	if (mpi_errno) mpig_cm_xio_fault_handle_async_error(mpi_errno);

	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_server_handle_recv_incoming_open_req() */


/*
 * <void> mpig_cm_xio_server_handle_recv_open_vc_req([IN] handle, [IN] result, [IN] rbuf, [IN] rbuf_len, [IN] nbytes, [IN] NULL,
 *     [IN/MOD] vc)
 *
 * MT-NOTE: this function assumes that the temp VC mutex is held by the calling thread.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_server_handle_recv_open_vc_req
static int mpig_cm_xio_server_handle_recv_open_vc_req(mpig_vc_t * tmp_vc)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    struct mpig_cm_xio_vc * const tmp_vc_cm = &tmp_vc->cm.xio;
    mpig_vc_t * real_vc = NULL;
    struct mpig_cm_xio_vc * real_vc_cm = NULL;
    bool_t real_vc_locked = FALSE;
    mpig_endian_t endian;
    int df;
    mpig_pg_t * pg = NULL;
    bool_t new_pg;
    char * pg_id = NULL;
    size_t pg_id_size = 0;
    int pg_size = -1;
    int pg_rank = -1;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_server_handle_recv_open_vc_req);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_server_handle_recv_open_vc_req);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "entering: tmp_vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) tmp_vc));

    MPIG_UNUSED_VAR(fcname);

    /* unpack information from message header */
    mpig_cm_xio_msg_hdr_get_endian(tmp_vc, tmp_vc_cm->msgbuf, &endian);
    mpig_cm_xio_msg_hdr_get_df(tmp_vc, tmp_vc_cm->msgbuf, &df);
    mpig_cm_xio_vc_set_endian(tmp_vc, endian);
    mpig_cm_xio_vc_set_data_format(tmp_vc, df);
    mpig_cm_xio_msg_hdr_get_conn_seqnum(tmp_vc, tmp_vc_cm->msgbuf, &tmp_vc_cm->conn_seqnum);
    mpig_cm_xio_msg_hdr_get_rank(tmp_vc, tmp_vc_cm->msgbuf, &pg_size);
    mpig_cm_xio_msg_hdr_get_rank(tmp_vc, tmp_vc_cm->msgbuf, &pg_rank);
    pg_id = (char *) mpig_databuf_get_pos_ptr(tmp_vc_cm->msgbuf);
    pg_id_size = strlen(pg_id) + 1;
    mpig_databuf_inc_pos(tmp_vc_cm->msgbuf, pg_id_size);
	
    /* verify that the message buffer is now empty.  extra data should never be received. */
    if (mpig_databuf_get_remaining_bytes(tmp_vc_cm->msgbuf) != 0)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM, "ERROR: message received contains extra data: tmp_vc="
	    MPIG_PTR_FMT ", bytes=%d", (MPIG_PTR_CAST) tmp_vc, (int) mpig_databuf_get_remaining_bytes(tmp_vc_cm->msgbuf)));

	mpi_errno = mpig_cm_xio_server_send_open_error_resp(tmp_vc, MPIG_CM_XIO_OPEN_RESP_ERR_MSG_TOO_LONG);
	MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|server_reg_send_open_error_resp");
    }   /* --END ERROR HANDLING-- */

    /* given the PG ID and rank, acquire a reference to the the process group object (creating it if necessary).  the reference
       is associated with the temp VC. */
    mpi_errno = mpig_pg_acquire_ref_locked(pg_id, pg_size, &pg, &new_pg);
    MPIU_ERR_CHKANDJUMP2((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|pg_acquire_ref",
	"**globus|pg_acquire_ref %s %d", pg_id, pg_size);
    {
	/* set the PG information fields in the temp VC */
	mpig_vc_set_pg_info(tmp_vc, pg, pg_rank);

	/* get the real VC using the process group and the rank */
	mpig_pg_get_vc(pg, pg_rank, &real_vc);
	real_vc_cm = &real_vc->cm.xio;
	    
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM, "open request received: pg_id=%s, pg_rank=%d, tmp_vc=" MPIG_PTR_FMT
	    ", real_vc=" MPIG_PTR_FMT, pg_id, pg_rank, (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) real_vc));
	    
	mpig_vc_mutex_lock(real_vc);
	real_vc_locked = TRUE;
	{
	    bool_t was_inuse;
		
	    /* verify that the real VC has been initialized by the XIO CM.  if it hasn't been initialized, then do so; also,
	       increment the PG reference count to indicate that a new VC is active. */
	    if (mpig_vc_get_cm_type(real_vc) == MPIG_CM_TYPE_UNDEFINED)
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
	if (mpig_vc_get_cm_type(real_vc) != MPIG_CM_TYPE_XIO)
	{   /* --BEGIN ERROR HANDLING-- */
	    /* if the real VC is managed by another CM then send an error to the remote process */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM,
		"ERROR: the real VC is not managed by the XIO communication module: tmp_vc=" MPIG_PTR_FMT ", real_vc="
		MPIG_PTR_FMT, (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) real_vc));
		
	    mpi_errno = mpig_cm_xio_server_send_open_error_resp(tmp_vc, MPIG_CM_XIO_OPEN_RESP_ERR_VC_NOT_XIO);
	    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_INTERN, "**globus|cm_xio|server_reg_send_open_error_resp");
	}   /* --END ERROR HANDLING-- */
	else if (tmp_vc_cm->conn_seqnum < real_vc_cm->conn_seqnum)
	{
	    /*
	     * if the connect sequence number is less than expected then two possibilities exist.  (1) this is an old
	     * head-to-head connection where the remote process was the loser and the connect was simply delayed.  (2) the remote
	     * process, which should have won any head-to-head tried to form the same connection twice.  the latter is obviously
	     * a bug and is reported as such.
	     */
	    if (MPIG_CM_XIO_CONN_VOTE(pg_id, pg_rank) == FALSE)
	    {
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		    "real VC connected; local process won; NAKing open req: tmp_vc=" MPIG_PTR_FMT ", real_vc=" MPIG_PTR_FMT,
		    (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) real_vc));
		
		mpi_errno = mpig_cm_xio_server_send_open_vc_resp(tmp_vc, MPIG_CM_XIO_OPEN_RESP_VC_NAK,
		    mpig_cm_xio_server_handle_send_open_resp_vc_nak);
		MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|server_reg_send_open_resp_vc_nak");
	    }
	    else
	    {   /* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		    "ERROR: real VC connected; winner connecting again: tmp_vc=" MPIG_PTR_FMT ", real_vc=" MPIG_PTR_FMT,
		    (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) real_vc));
		
		mpi_errno = mpig_cm_xio_server_send_open_error_resp(tmp_vc, MPIG_CM_XIO_OPEN_RESP_ERR_VC_CONNECTED);
		MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_INTERN, "**globus|cm_xio|server_reg_send_open_resp_vc_error");
	    }   /* --END ERROR HANDLING-- */
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

	    /* increment the connection sequence number.  FT-FIXME: more thought needs to be given to what should happen to the
	       sequence number in the face of an error. */
	    MPIU_Assertp(tmp_vc_cm->conn_seqnum == real_vc_cm->conn_seqnum);
	    real_vc_cm->conn_seqnum += 1;
	
	    mpi_errno = mpig_cm_xio_server_send_open_vc_resp(tmp_vc, MPIG_CM_XIO_OPEN_RESP_VC_ACK,
		mpig_cm_xio_server_handle_send_open_resp_vc_ack);
	    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|server_reg_send_open_resp_vc_ack");

	    mpig_cm_xio_vc_set_state(real_vc, MPIG_CM_XIO_VC_STATE_SERVER_ACCEPTING);
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
	    if (MPIG_CM_XIO_CONN_VOTE(pg_id, pg_rank))
	    {
		/*
		 * the remote process wins.  send an ACK to the remote process.  see the unconnected case above for the details.
		 */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		    "real VC connecting; remote process wins; ACKing open req: tmp_vc=" MPIG_PTR_FMT ", real_vc=" MPIG_PTR_FMT,
		    (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) real_vc));
		    
		/* increment the connection sequence number.  FT-FIXME: more thought needs to be given to what should happen to
		   the sequence number in the face of an error. */
		MPIU_Assertp(tmp_vc_cm->conn_seqnum == real_vc_cm->conn_seqnum);
		real_vc_cm->conn_seqnum += 1;
	
		mpi_errno = mpig_cm_xio_server_send_open_vc_resp(tmp_vc, MPIG_CM_XIO_OPEN_RESP_VC_ACK,
		    mpig_cm_xio_server_handle_send_open_resp_vc_ack);
		MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|server_reg_send_open_resp_vc_ack");

		mpig_cm_xio_vc_set_state(real_vc, MPIG_CM_XIO_VC_STATE_SERVER_ACCEPTING);
	    }
	    else
	    {
		/*
		 * the local process wins.  the temp VC associated with the local process' outgoing connection is the valid one.
		 * tell the remote process to teardown the VC associated associate with his outgoing connection.  the callback
		 * will close specified below will close the local process' side of the connection and dstroy the temp VC
		 * associated with it.
		 */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		    "real VC connecting; local process wins; NAKing open req: tmp_vc=" MPIG_PTR_FMT ", real_vc=" MPIG_PTR_FMT,
		    (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) real_vc));
		
		mpi_errno = mpig_cm_xio_server_send_open_vc_resp(tmp_vc, MPIG_CM_XIO_OPEN_RESP_VC_NAK,
		    mpig_cm_xio_server_handle_send_open_resp_vc_nak);
		MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|server_reg_send_open_resp_vc_nak");
	    }
	}
	else if (mpig_cm_xio_vc_is_connected(real_vc))
	{
	    /* everything that could occur here is covered by code that detects connections with old sequence numbers */
	    MPIU_Assertp(mpig_cm_xio_vc_is_connected(real_vc) == FALSE &&
		"this case should have been covered by the sequence number verification check");
	}
	else if (mpig_cm_xio_vc_is_disconnecting(real_vc) || mpig_cm_xio_vc_is_closing(real_vc))
	{
	    /*
	     * MPI-2-FIXME: it may be possible for a disconnect followed by a reconnect to cause a new connection to be requested
	     * from the remote process while the current connection is executing the close protocol.  we must enqueue the temp VC
	     * of the incoming connection until the disconnect has completed.
	     */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM, "real VC is disconnecting: tmp_vc=" MPIG_PTR_FMT ", real_vc=" MPIG_PTR_FMT,
		(MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) real_vc));
	    MPIU_Assertp(FALSE && "MPI-2-FIXME: disconnect - reconnect not implemented");
	}
	else if (mpig_cm_xio_vc_has_failed(real_vc))
	{   /* --BEGIN ERROR HANDLING-- */
	    /*
	     * the real VC has already failed.  however, the remote process may not be aware of this.  for example, if a
	     * connection request from the local process never made it to the remote process, then the local process would
	     * have marked the local VC object as failed, but the VC object at the remote would still be in the unconnected
	     * state.  therefore, we reply with a failure response to make the state of the two objects consistent.
	     */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		"real VC has already failed; sending failure message: tmp_vc=" MPIG_PTR_FMT ", real_vc=" MPIG_PTR_FMT,
		(MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) real_vc));
	    
	    mpi_errno = mpig_cm_xio_server_send_open_error_resp(tmp_vc, MPIG_CM_XIO_OPEN_RESP_ERR_VC_FAILED);
	    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|server_reg_send_open_resp_vc_error");
	}   /* --END ERROR HANDLING-- */
	else
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		"ERROR: real VC is in an unknown state: tmp_vc=" MPIG_PTR_FMT ", real_vc=" MPIG_PTR_FMT "state=%d",
		(MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) real_vc, (int) mpig_cm_xio_vc_get_state(real_vc)));
	    MPIU_Assertp(FALSE && "VC state is unknown!  Memory has likely been corrupted...");
	}   /* --END ERROR HANDLING-- */
    }
    mpig_vc_mutex_unlock(real_vc);
    real_vc_locked = FALSE;

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "exiting: tmp_vc=" MPIG_PTR_FMT "real_vc=" MPIG_PTR_FMT
	", mpi_errno=" MPIG_ERRNO_FMT, (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) real_vc, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_server_handle_recv_open_vc_req);
    return mpi_errno;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	/* set the state of the real VC back to unconnected; release the reference to the VC (and the reference to the PG if the
	   VC reference count reaches zero) */
	if (real_vc != NULL)
	{
	    mpig_vc_mutex_lock_conditional(real_vc, (!real_vc_locked));
	    {
		/* if a reportable error has occurred, and this connection had been selected, then report the error; otherwise,
		   set the state of the real VC back to unconnected */
		if (mpi_errno && mpig_cm_xio_vc_get_state(real_vc) == MPIG_CM_XIO_VC_STATE_SERVER_ACCEPTING)
		{
		    /* FT-NOTE: eventually, to be more tolerant of faults, we may want to attempt to connect to the remote
		       process if pending send requests are queued on the real VC. */
		    mpig_cm_xio_fault_handle_async_vc_error(real_vc, mpi_errno);
		}
		else
		{
		    mpig_cm_xio_vc_set_state(real_vc, MPIG_CM_XIO_VC_STATE_UNCONNECTED);
		}
	    }
	    mpig_vc_mutex_unlock(real_vc);

	    mpig_vc_release_ref(real_vc);
	}

	/* mpig_cm_xio_server_handle_incoming_open_req() will report the error(s) through MPID_Progress_{poke,test,wait}() */

	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_server_handle_recv_open_vc_req() */


/*
 * <mpi_errno> mpig_cm_xio_server_send_open_vc_resp([IN/MOD] tmp_vc, [IN] resp, [IN] xio_data_callback_fn)
 *
 * MT-NOTE: this function assumes that the temp VC mutex is held by the calling thread.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_server_send_open_vc_resp
static int mpig_cm_xio_server_send_open_vc_resp(mpig_vc_t * const tmp_vc, const mpig_cm_xio_open_resp_t resp,
    const globus_xio_data_callback_t callback_fn)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    struct mpig_cm_xio_vc * const tmp_vc_cm = &tmp_vc->cm.xio;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_server_send_open_vc_resp);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_server_send_open_vc_resp);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "entering: tmp_vc=" MPIG_PTR_FMT ", resp=%s",
	(MPIG_PTR_CAST) tmp_vc, mpig_cm_xio_conn_open_resp_get_string(resp)));

    MPIG_UNUSED_VAR(fcname);

    /* construct the open response message */
    mpig_databuf_reset(tmp_vc_cm->msgbuf);
    mpig_cm_xio_msg_hdr_put_begin(tmp_vc, tmp_vc_cm->msgbuf);
    mpig_cm_xio_msg_hdr_put_msg_type(tmp_vc, tmp_vc_cm->msgbuf, MPIG_CM_XIO_MSG_TYPE_OPEN_VC_RESP);
    mpig_cm_xio_msg_hdr_put_conn_open_resp(tmp_vc, tmp_vc_cm->msgbuf, resp);
    mpig_cm_xio_msg_hdr_put_conn_seqnum(tmp_vc, tmp_vc_cm->msgbuf, tmp_vc_cm->conn_seqnum);
    mpig_cm_xio_msg_hdr_put_end(tmp_vc, tmp_vc_cm->msgbuf);

    /* send the open respoinse message */
    mpig_cm_xio_register_write_vc_msgbuf(tmp_vc, callback_fn, &mpi_errno);
    if (mpi_errno)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM, "ERROR: failed to send open response string: tmp_vc="
	    MPIG_PTR_FMT ", resp=%s", (MPIG_PTR_CAST) tmp_vc, mpig_cm_xio_conn_open_resp_get_string(resp)));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|server_reg_send_open_resp",
	    "**globus|cm_xio|server_reg_send_open_resp %s", mpig_cm_xio_conn_open_resp_get_string(resp));
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */
    
    mpig_cm_xio_vc_set_state(tmp_vc, MPIG_CM_XIO_VC_STATE_SERVER_SENDING_OPEN_RESP);

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "exiting: tmp_vc=" MPIG_PTR_FMT ", mpi_errno="
	MPIG_ERRNO_FMT, (MPIG_PTR_CAST) tmp_vc, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_server_send_open_vc_resp);
    return mpi_errno;
    
  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_server_send_open_vc_resp() */


/*
 * void mpig_cm_xio_server_handle_send_open_resp_vc_ack(
 *          [IN] handle, [IN] result, [IN] sbuf, [IN] sbuf_len, [IN] nbytes, [IN] NULL, [IN/MOD] vc)
 *
 * this handler gets invoked after sending an OPEN_RESP_VC_ACK message using the temporary vc.  this message is confirmation that
 * the process pair is to use the connection associated with temp VC as the connection for the real VC.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_server_handle_send_open_resp_vc_ack
static void mpig_cm_xio_server_handle_send_open_resp_vc_ack(
    const globus_xio_handle_t handle, const globus_result_t op_grc, globus_byte_t * const sbuf, const globus_size_t sbuf_len,
    const globus_size_t nbytes, const globus_xio_data_descriptor_t dd, void * const arg)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vc_t * const tmp_vc = (mpig_vc_t *) arg;
    struct mpig_cm_xio_vc * const tmp_vc_cm = &tmp_vc->cm.xio;
    mpig_vc_t * real_vc = NULL;
    struct mpig_cm_xio_vc * real_vc_cm = NULL;
    bool_t real_vc_locked = FALSE;
    bool_t real_vc_inuse;
    mpig_pg_t * pg;
    int pg_rank;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_server_handle_send_open_resp_vc_ack);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_server_handle_send_open_resp_vc_ack);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"entering: tmp_vc=" MPIG_PTR_FMT ", handle=" MPIG_PTR_FMT ", op_grc=%d, sbuf=" MPIG_PTR_FMT ", sbuf_len=" MPIG_SIZE_FMT
	", nbytes=" MPIG_SIZE_FMT, (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) handle, op_grc, (MPIG_PTR_CAST) sbuf, sbuf_len,
	nbytes));

    /* if send operation completed successfully, clear the handle field in temp VC so that the handle is not closed when the temp
       VC is destroyed.  in addition, extract the the the pg and pg_rank fields from the temp VC so that we can get a pointer to
       the VC. */
    mpig_vc_mutex_lock(tmp_vc);
    {
	MPIU_Assert(mpig_cm_xio_vc_get_state(tmp_vc) == MPIG_CM_XIO_VC_STATE_SERVER_SENDING_OPEN_RESP);

	if (op_grc == GLOBUS_SUCCESS) tmp_vc_cm->handle = NULL;
	pg = tmp_vc->pg;
	pg_rank = tmp_vc->pg_rank;
    }
    mpig_vc_mutex_unlock(tmp_vc);

    /* using the pg and pg rank acquired above, get the real VC */
    mpig_pg_rc_acq(pg, TRUE);
    {
	mpig_pg_get_vc(pg, pg_rank, &real_vc);
	real_vc_cm = &real_vc->cm.xio;
    }
    mpig_pg_rc_rel(pg, FALSE);

    /* now that all useful information has been extracted from the temp VC, close the connection and destroy the temp VC object,
       releasing the reference to the PG */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,"destroying the temp VC: tmp_vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) tmp_vc));
    mpi_errno = mpig_cm_xio_conn_destroy_tmp_vc(tmp_vc);
    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|tmp_vc_destroy");

    mpig_vc_mutex_lock(real_vc);
    real_vc_locked = TRUE;
    {
	/* check if an error occurred while sending the OPEN_RESP_VC_ACK message.  if the send failed, and we have pending
	   messages, then the connection should be marked as failed. */
	if (op_grc)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM, "ERROR: %s operation failed: tmp_vc=" MPIG_PTR_FMT
		", real_vc=" MPIG_PTR_FMT ", msg=\"%s\"", "globus_xio_register_write", (MPIG_PTR_CAST) tmp_vc,
		(MPIG_PTR_CAST) real_vc, globus_error_print_chain(globus_error_peek(op_grc))));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|server_handle_open_resp_vc_ack",
		"**globus|server_handle_open_resp_vc_ack %s", globus_error_print_chain(globus_error_peek(op_grc)));

	    /* FT-FIXME: should we attempt to form the connection in the other direction if the VC send queue is not empty?  for
	       now, an error is reported, and the VC is marked as having failed. */
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */
    
	/* set the handle on the real VC, and change the state to connected */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
	    "change state of real VC to connected; starting communication: real_vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) real_vc));
	real_vc_cm->handle = handle;
	mpig_cm_xio_vc_set_state(real_vc, MPIG_CM_XIO_VC_STATE_CONNECTED);
	mpig_cm_xio_vc_list_add(real_vc);

	/* start receiving messages on the real VC.  note: it was not necesary to transfer the receive buffer from the temp VC to
	   the real VC since we know that the buffer was depleted by mpig_cm_xio_server_handle_recv_open_vc_req(). */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VC | MPIG_DEBUG_LEVEL_VCCM,
	    "connection established; starting communication engines: vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) real_vc));
		
	mpi_errno = mpig_cm_xio_recv_next_msg(real_vc);
	MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|recv_next_msg");

	/* if the real VC has any sends queued up, start send them */
	mpi_errno = mpig_cm_xio_send_next_sreq(real_vc);
	MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|send_next_sreq");

	/* we must release the internal reference to the VC.  mpig_vc_dec_ref_count() is used in favor of the internal routine,
	   mpig_cm_xio_vc_dec_ref_count(), because we want to the disconnect process to be initiated if the VC is no longer
	   referenced.  this could happen if a send on the remote process were canceled before the connection was established and
	   then the commmunicator was freed or disconnected.  however, we do not use mpig_vc_release_ref() because it has the
	   side ffect of releasing the PG, which we cannot do while holding the VC mutex. */
	mpig_vc_dec_ref_count(real_vc, &real_vc_inuse);
    }
    mpig_vc_mutex_unlock(real_vc);
    real_vc_locked = FALSE;

    /* if the real VC is no longer referenced by anyone, the release the PG */
    if (real_vc_inuse == FALSE)
    {
	mpig_pg_release_ref(pg);
    }
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "exiting: tmp_vc=" MPIG_PTR_FMT ", real_vc=" MPIG_PTR_FMT
	", mpi_errno=" MPIG_ERRNO_FMT, (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) real_vc, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_server_handle_send_open_resp_vc_ack);
    return;
    
  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	mpig_vc_mutex_lock_conditional(real_vc, (!real_vc_locked));
	{
	    /* report the error through MPID_Progress_{poke,test,wait}() */
	    mpig_cm_xio_fault_handle_async_vc_error(real_vc, mpi_errno);
	}
	mpig_vc_mutex_unlock(real_vc);
	    
	/* release the internal reference to the VC, and the PG if the VC reference count reaches zero */
	mpig_vc_release_ref(real_vc);
	
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_server_handle_send_open_resp_vc_ack() */


/*
 * void mpig_cm_xio_server_handle_send_open_resp_vc_nak(
 *          [IN] handle, [IN] result, [IN] sbuf, [IN] sbuf_len, [IN] nbytes, [IN] NULL, [IN/MOD] vc)
 *
 * this handler gets invoked after sending an OPEN_RESP_VC_NAK message using the temporary vc
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_server_handle_send_open_resp_vc_nak
static void mpig_cm_xio_server_handle_send_open_resp_vc_nak(
    const globus_xio_handle_t handle, const globus_result_t op_grc, globus_byte_t * const sbuf, const globus_size_t sbuf_len,
    const globus_size_t nbytes, const globus_xio_data_descriptor_t dd, void * const arg)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vc_t * const tmp_vc = (mpig_vc_t *) arg;
    mpig_vc_t * real_vc = NULL;
    mpig_pg_t * pg = NULL;
    int pg_rank;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_handle_send_open_resp_vc_nak);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_handle_send_open_resp_vc_nak);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"entering: tmp_vc=" MPIG_PTR_FMT ", handle=" MPIG_PTR_FMT ", op_grc=%d, sbuf=" MPIG_PTR_FMT ", sbuf_len=" MPIG_SIZE_FMT
	", nbytes=" MPIG_SIZE_FMT, (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) handle, op_grc, (MPIG_PTR_CAST) sbuf, sbuf_len,
	nbytes));

    /* extract the the the pg and pg_rank fields from the temp VC so that we can get a pointer to the VC */
    mpig_vc_mutex_lock(tmp_vc);
    {	 
	MPIU_Assert(mpig_cm_xio_vc_get_state(tmp_vc) == MPIG_CM_XIO_VC_STATE_SERVER_SENDING_OPEN_RESP);
	pg = tmp_vc->pg;
	pg_rank = tmp_vc->pg_rank;
    }
    mpig_vc_mutex_unlock(tmp_vc);

    /* using the pg and pg rank acquired above, get the real VC */
    mpig_pg_rc_acq(pg, TRUE);
    {
	mpig_pg_get_vc(pg, pg_rank, &real_vc);
    }
    mpig_pg_rc_rel(pg, FALSE);

    /* close the connection and destroy the temp VC object, releasing the associated PG reference */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
	"closing and destroying the temp VC: tmp_vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) tmp_vc));
    mpi_errno = mpig_cm_xio_conn_destroy_tmp_vc(tmp_vc);
    MPIU_ERR_CHKANDSTMT((mpi_errno), mpi_errno, MPI_ERR_OTHER, {;}, "**globus|cm_xio|tmp_vc_destroy");
    
    if (op_grc)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM, "ERROR: %s operation failed: tmp_vc=" MPIG_PTR_FMT
	    ", real_vc=" MPIG_PTR_FMT ", msg=\"%s\"", "globus_xio_register_write", (MPIG_PTR_CAST) tmp_vc,
	    (MPIG_PTR_CAST) real_vc, globus_error_print_chain(globus_error_peek(op_grc))));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|server_handle_send_open_resp_vc_nak",
	    "**globus|cm_xio|server_handle_send_open_resp_vc_nak %s", globus_error_print_chain(globus_error_peek(op_grc)));
    }   /* --END ERROR HANDLING-- */
    
    /* release the internal reference to the real VC, and the PG if the VC reference count reaches zero */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
	"releasing the internal reference to the real VC: tmp_vc=" MPIG_PTR_FMT ",real_vc=" MPIG_PTR_FMT,
	(MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) real_vc));
    mpig_vc_release_ref(real_vc);

    if (mpi_errno) goto fn_fail;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "exiting: tmp_vc=" MPIG_PTR_FMT ", real_vc=" MPIG_PTR_FMT
	", mpi_errno=" MPIG_ERRNO_FMT, (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) real_vc, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_handle_send_open_resp_vc_nak);
    return;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	mpig_cm_xio_fault_handle_async_error(mpi_errno);
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_handle_send_open_resp_vc_nak() */


/*
 * void mpig_cm_xio_server_handle_recv_open_port_req([IN] tmp_vc)
 *
 * MT-NOTE: this function assumes that the temp VC mutex is held by the calling thread.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_server_handle_recv_open_port_req
static int mpig_cm_xio_server_handle_recv_open_port_req(mpig_vc_t * tmp_vc)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    struct mpig_cm_xio_vc * const tmp_vc_cm = &tmp_vc->cm.xio;
    mpig_endian_t endian;
    int df;
    MPIU_Size_t remote_data_size;
    char * port_id = NULL;
    size_t port_id_size = 0;
    mpig_connection_t * conn;
    mpig_cm_xio_port_t * port;
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKPMEM_DECL(1);
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_server_handle_recv_open_port_req);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_server_handle_recv_open_port_req);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "entering: tmp_vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) tmp_vc));

    MPIG_UNUSED_VAR(fcname);
    
    /* unpack information from message header */
    mpig_cm_xio_msg_hdr_get_endian(tmp_vc, tmp_vc_cm->msgbuf, &endian);
    mpig_cm_xio_msg_hdr_get_df(tmp_vc, tmp_vc_cm->msgbuf, &df);
    mpig_cm_xio_vc_set_endian(tmp_vc, endian);
    mpig_cm_xio_vc_set_data_format(tmp_vc, df);
    mpig_cm_xio_msg_hdr_get_data_size(tmp_vc, tmp_vc_cm->msgbuf, &remote_data_size);
    port_id = (char *) mpig_databuf_get_pos_ptr(tmp_vc_cm->msgbuf);
    port_id_size = strlen(port_id) + 1;
    mpig_databuf_inc_pos(tmp_vc_cm->msgbuf, port_id_size);
	
    /* verify that the message buffer is now empty.  extra data should never be received. */
    if (mpig_databuf_get_remaining_bytes(tmp_vc_cm->msgbuf) != 0)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM, "ERROR: message received contains extra data: tmp_vc="
	    MPIG_PTR_FMT ", bytes=%d", (MPIG_PTR_CAST) tmp_vc, (int) mpig_databuf_get_remaining_bytes(tmp_vc_cm->msgbuf)));

	mpi_errno = mpig_cm_xio_server_send_open_error_resp(tmp_vc, MPIG_CM_XIO_OPEN_RESP_ERR_MSG_TOO_LONG);
	MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|server_reg_send_open_error_resp");
    }   /* --END ERROR HANDLING-- */

    /* create a new connection object describing the incoming connection */
    MPIU_CHKPMEM_MALLOC(conn, mpig_connection_t *, sizeof(mpig_connection_t), mpi_errno, "incoming port connection object");
    conn->vc = tmp_vc;
    conn->remote_data_size = remote_data_size;
    
    /* find the port in the port list.  if the port is found, return it with its mutex acquired. */
    mpi_errno = mpig_cm_xio_port_find_and_lock(port_id, &port);
    if (mpi_errno)
    {   /* --BEGIN ERROR HANDLING-- */
	/* XXX: send error resp.  WHAT ERRORS CAN OCCCUR HERE??? */
	    goto fn_fail;
    }   /* --END ERROR HANDLING-- */

    if (port == NULL)
    {
	/* XXX: send error message indicating that the port was not found */
	goto fn_return;
    }
    
    /* mpig_cm_xio_port_mutex_lock(port) -- mutex acquired above as part of the find operation */
    {
	/* add the connection object to the port's list of pending connections.  the connection will be acknowledged when the
	   application calls MPI_Comm_accept(). */
	mpi_errno = mpig_cm_xio_port_enqueue_connection(port, conn);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    /* XXX: send error resp */
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

	MPIU_CHKPMEM_COMMIT();
    }
    mpig_cm_xio_port_mutex_unlock(port);
	
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "exiting: tmp_vc=" MPIG_PTR_FMT ", mpi_errno="
	MPIG_ERRNO_FMT, (MPIG_PTR_CAST) tmp_vc, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_server_handle_recv_open_port_req);
    return mpi_errno;
    
  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	MPIU_CHKPMEM_REAP();
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_server_handle_recv_open_port_req() */


/*
 * <mpi_errno> mpig_cm_xio_server_send_open_error_resp([IN/MOD] tmp_vc, [IN] resp)
 *
 * MT-NOTE: this function assumes that the temp VC mutex is held by the calling thread.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_server_send_open_error_resp
static int mpig_cm_xio_server_send_open_error_resp(mpig_vc_t * const tmp_vc, const mpig_cm_xio_open_resp_t resp)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    struct mpig_cm_xio_vc * const tmp_vc_cm = &tmp_vc->cm.xio;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_server_send_open_error_resp);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_server_send_open_error_resp);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "entering: tmp_vc=" MPIG_PTR_FMT ", resp=%s",
	(MPIG_PTR_CAST) tmp_vc, mpig_cm_xio_conn_open_resp_get_string(resp)));

    MPIG_UNUSED_VAR(fcname);

    /* construct the open response message */
    mpig_databuf_reset(tmp_vc_cm->msgbuf);
    mpig_cm_xio_msg_hdr_put_begin(tmp_vc, tmp_vc_cm->msgbuf);
    mpig_cm_xio_msg_hdr_put_msg_type(tmp_vc, tmp_vc_cm->msgbuf, MPIG_CM_XIO_MSG_TYPE_OPEN_ERROR_RESP);
    mpig_cm_xio_msg_hdr_put_end(tmp_vc, tmp_vc_cm->msgbuf);

    /* send the open respoinse message */
    mpig_cm_xio_register_write_vc_msgbuf(tmp_vc, mpig_cm_xio_server_handle_send_open_error_resp, &mpi_errno);
    if (mpi_errno)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM, "ERROR: failed to send open response string: tmp_vc="
	    MPIG_PTR_FMT ", resp=%s", (MPIG_PTR_CAST) tmp_vc, mpig_cm_xio_conn_open_resp_get_string(resp)));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|server_reg_send_open_resp",
	    "**globus|cm_xio|server_reg_send_open_resp %s", mpig_cm_xio_conn_open_resp_get_string(resp));
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */
    
    mpig_cm_xio_vc_set_state(tmp_vc, MPIG_CM_XIO_VC_STATE_SERVER_SENDING_OPEN_RESP);

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "exiting: tmp_vc=" MPIG_PTR_FMT ", mpi_errno="
	MPIG_ERRNO_FMT, (MPIG_PTR_CAST) tmp_vc, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_server_send_open_error_resp);
    return mpi_errno;
    
  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_server_send_open_error_resp() */


/*
 * void mpig_cm_xio_server_handle_send_open_error_resp(
 *          [IN] handle, [IN] result, [IN] sbuf, [IN] sbuf_len, [IN] nbytes, [IN] NULL, [IN/MOD] vc)
 *
 * this handler gets invoked after sending an error message to the remote (connecting) process using the temporary vc.
 * regardless of the outcome, success or failure, this routine has no affect on the real VC.  it's only purpose is to destroy the
 * temp VC.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_server_handle_send_open_error_resp
static void mpig_cm_xio_server_handle_send_open_error_resp(
    const globus_xio_handle_t handle, const globus_result_t op_grc, globus_byte_t * const sbuf, const globus_size_t sbuf_len,
    const globus_size_t nbytes, const globus_xio_data_descriptor_t dd, void * const arg)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vc_t * const tmp_vc = (mpig_vc_t *) arg;
    mpig_vc_t * real_vc = NULL;
    mpig_pg_t * pg = NULL;
    int pg_rank;
    int mrc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_handle_send_open_error_resp);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_handle_send_open_error_resp);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"entering: tmp_vc=" MPIG_PTR_FMT ", handle=" MPIG_PTR_FMT ", op_grc=%d, sbuf=" MPIG_PTR_FMT ", sbuf_len=" MPIG_SIZE_FMT
	", nbytes=" MPIG_SIZE_FMT, (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) handle, op_grc, (MPIG_PTR_CAST) sbuf, sbuf_len,
	nbytes));

    if (op_grc)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM, "ERROR: %s operation failed: %s",
	    "globus_xio_register_write", globus_error_print_chain(globus_error_peek(op_grc))));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|server_handle_send_open_error_resp",
	    "**globus|cm_xio|server_handle_send_open_error_resp %s", globus_error_print_chain(globus_error_peek(op_grc)));
    }   /* --END ERROR HANDLING-- */
    
    /* extract the the the pg and pg_rank fields from the temp VC so that we can get a pointer to the VC */
    mpig_vc_mutex_lock(tmp_vc);
    {	 
	MPIU_Assert(mpig_cm_xio_vc_get_state(tmp_vc) == MPIG_CM_XIO_VC_STATE_SERVER_SENDING_OPEN_RESP);
	pg = tmp_vc->pg;
	pg_rank = tmp_vc->pg_rank;
    }
    mpig_vc_mutex_unlock(tmp_vc);

    /* close the connection and destroy the temp VC object, releasing the associated PG reference */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM, "closing and destroying the temp VC: tmp_vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) tmp_vc));
    mrc = mpig_cm_xio_conn_destroy_tmp_vc(tmp_vc);
    MPIU_ERR_CHKANDSTMT((mrc), mrc, MPI_ERR_OTHER, {MPIU_ERR_ADD(mpi_errno, mrc);}, "**globus|cm_xio|tmp_vc_destroy");

    /* if the temp VC is associated with a process group and a real VC, then decrement the reference counts (as appropriate) */
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
	mpig_vc_release_ref(real_vc);
    }
    
    if (mpi_errno) goto fn_fail;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "exiting: tmp_vc=" MPIG_PTR_FMT ", mpi_errno="
	MPIG_ERRNO_FMT, (MPIG_PTR_CAST) tmp_vc, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_handle_send_open_error_resp);
    return;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	mpig_cm_xio_fault_handle_async_error(mpi_errno);
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_server_handle_send_open_error_resp */

#endif /* MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS */
/**********************************************************************************************************************************
				       END SERVER SIDE (LISTENER) CONNECTION ESTABLISHMENT
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					   BEGIN CLIENT SIDE CONNECTION ESTABLISHMENT
**********************************************************************************************************************************/
#if !defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS)

static int mpig_cm_xio_client_connect(mpig_vc_t * vc);

static void mpig_cm_xio_client_handle_open(globus_xio_handle_t handle, globus_result_t op_grc, void * arg);

static void mpig_cm_xio_client_handle_send_magic(
    globus_xio_handle_t handle, globus_result_t op_grc, globus_byte_t * rbuf, globus_size_t rbuf_len, globus_size_t nbytes,
    globus_xio_data_descriptor_t dd, void * arg);

static void mpig_cm_xio_client_handle_recv_magic(
    globus_xio_handle_t handle, globus_result_t op_grc, globus_byte_t * rbuf, globus_size_t rbuf_len, globus_size_t nbytes,
    globus_xio_data_descriptor_t dd, void * arg);

static void mpig_cm_xio_client_handle_send_open_vc_req(
    globus_xio_handle_t handle, globus_result_t op_grc, globus_byte_t * rbuf, globus_size_t rbuf_len, globus_size_t nbytes,
    globus_xio_data_descriptor_t dd, void * arg);

static void mpig_cm_xio_client_handle_recv_open_vc_resp(
    globus_xio_handle_t handle, globus_result_t op_grc, globus_byte_t * rbuf, globus_size_t rbuf_len, globus_size_t nbytes,
    globus_xio_data_descriptor_t dd, void * arg);


#else /* defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS) */


/*
 * <mpi_errno> mpig_cm_xio_client_connect([IN/MOD] vc)
 *
 * MT-NOTE: this function assumes that the VC mutex is held when the function is called.  furthermore, the function may only be
 * called when a connection is not established or knowingly in the process of being established.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_client_connect
static int mpig_cm_xio_client_connect(mpig_vc_t * const real_vc)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vc_t * tmp_vc = NULL;
    struct mpig_cm_xio_vc * tmp_vc_cm = NULL;
    bool_t tmp_vc_locked = FALSE;
    mpig_pg_t * const pg = mpig_vc_get_pg(real_vc);
    const int pg_rank = mpig_vc_get_pg_rank(real_vc);
    globus_xio_handle_t handle;
    globus_result_t grc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_client_connect);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_client_connect);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "entering: real_vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) real_vc));

    MPIG_UNUSED_VAR(fcname);

    MPIU_Assert(mpig_cm_xio_vc_is_unconnected(real_vc));
    
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
	    MPIU_ERR_SETFATALANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**globus|vc_ptr_bad", "**globus|vc_ptr_bad %p", real_vc);
	}   /* --END ERROR HANDLING-- */
    }
    
    /* create an XIO handle and initiate the connection */
    grc = globus_xio_handle_create(&handle, mpig_cm_xio_conn_stack);
    MPIU_ERR_CHKANDJUMP1((grc), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|client_handle_create",
	"**globus|cm_xio|client_handle_create %s", globus_error_print_chain(globus_error_peek(grc)));
    
    /* change the state of the real VC to 'connecting' so that any additional send operations know not to initiate another
       register connect.  this also allows the the code that handles incoming connections to detect a head-to-head connection
       scenerio. */
    mpig_cm_xio_vc_set_state(real_vc, MPIG_CM_XIO_VC_STATE_CLIENT_CONNECTING);

    /* allocate and initialize a temporary VC for use during connection establishment. */
    tmp_vc = (mpig_vc_t *) MPIU_Malloc(sizeof(mpig_vc_t));
    if (tmp_vc == NULL)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM,
	    "ERROR: malloc failed when attempting to allocate a temp VC for an outgoing connection"));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "temp VC for an outgoing connection");
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */
    tmp_vc_cm = &tmp_vc->cm.xio;
    mpig_vc_construct(tmp_vc);
    mpig_vc_mutex_lock(tmp_vc);
    tmp_vc_locked = TRUE;
    {
	mpig_cm_xio_vc_construct(tmp_vc);
	tmp_vc_cm->handle = handle;

	/* store the connection sequence number that should be used when the open request is sent.  MT-NOTE: this operation must
	   occur BEFORE the real_vc mutex is released below.  this prevents capturing a sequence number that has already been
	   incremented by the server code during a head-to-head connection race. */
	tmp_vc_cm->conn_seqnum = real_vc->cm.xio.conn_seqnum;
	
	/* add the temp VC to the list of active VCs */
	mpig_cm_xio_vc_list_add(tmp_vc);

	/* pg and pg_rank are stored in the temp VC so the real VC can be located again later, adjusting the PG reference count
	   accordingly.  MT-NOTE: the real VC mutex must be released since the process group is locked below.  A thread should
	   never attempt to acquire the mutex of a process group while holding the mutex of a VC belonging to that PG.  In fact,
	   with current implementation, where the PG module uses a single global mutex, a thread should never attempt to acquire
	   the mutex of any PG while holding the mutex of any VC.  temp VCs are exempt from this rule since they are not part of
	   any PG. */
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
    
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
	    "temp VC constructed; registering open operation to form connection: tmp_vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) tmp_vc));

	/* register an ansychronous connect to the process specified in the contact string field located in the real VC.  if the
	   registration is successful, update the temp VC state. */
	grc = globus_xio_register_open(tmp_vc_cm->handle, mpig_cm_xio_vc_get_contact_string(real_vc), mpig_cm_xio_conn_attrs,
	    mpig_cm_xio_client_handle_open,(void *) tmp_vc);
	MPIU_ERR_CHKANDJUMP1((grc), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|xio_reg_open",
	    "**globus|cm_xio|xio_reg_open %s", globus_error_print_chain(globus_error_peek(grc)));

	mpig_cm_xio_vc_set_state(tmp_vc, MPIG_CM_XIO_VC_STATE_CLIENT_OPENING_VC);
    }
    mpig_vc_mutex_unlock(tmp_vc);
    tmp_vc_locked = FALSE;

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "exiting: real_vc=" MPIG_PTR_FMT ", tmp_vc=" MPIG_PTR_FMT
	", handle=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, (MPIG_PTR_CAST) real_vc, (MPIG_PTR_CAST) tmp_vc,
	(MPIG_PTR_CAST) (tmp_vc_cm ? tmp_vc_cm->handle : NULL), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_client_connect);
    return mpi_errno;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	int mrc;
	
	/* release the temp VC mutex if it is being held by the current context */
	mpig_vc_mutex_unlock_conditional(tmp_vc, (tmp_vc_locked));
    
	/* close the handle and destroy the temp VC object, releasing the associated PG reference and remove the temp VC from
	   active VC list */
	if (tmp_vc != NULL)
	{
	    mrc = mpig_cm_xio_conn_destroy_tmp_vc(tmp_vc);
	    MPIU_ERR_CHKANDSTMT((mrc), mrc, MPI_ERR_OTHER, {MPIU_ERR_ADD(mpi_errno, mrc);}, "**globus|cm_xio|tmp_vc_destroy");
	}
	else
	{
	    grc = globus_xio_close(handle, NULL);
	}

	/* if the reference count of the real VC was bumped, then decrement it.  then, change the state of the real VC to the
	   failed state */
	if (mpig_cm_xio_vc_get_state(real_vc) == MPIG_CM_XIO_VC_STATE_CLIENT_CONNECTING)
	{
	    bool_t inuse;
	    mpig_cm_xio_vc_dec_ref_count(real_vc, &inuse);
	    MPIU_Assert(inuse == TRUE);
	}

	/* call the synchronous fault handler to clean up the real VC and set an error state */
	mpi_errno = mpig_cm_xio_fault_handle_sync_vc_error(real_vc, mpi_errno);

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
static void mpig_cm_xio_client_handle_open(const globus_xio_handle_t handle, const globus_result_t op_grc, void * const arg)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vc_t * const tmp_vc = (mpig_vc_t *) arg;
    struct mpig_cm_xio_vc * const tmp_vc_cm = &tmp_vc->cm.xio;
    bool_t tmp_vc_locked = FALSE;
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

	MPIU_Assert(mpig_cm_xio_vc_get_state(tmp_vc) == MPIG_CM_XIO_VC_STATE_CLIENT_OPENING_VC);
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
	mpig_cm_xio_register_write_vc_msgbuf(tmp_vc, mpig_cm_xio_client_handle_send_magic, &mpi_errno);
	MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|client_reg_send_magic",
	    "**globus|cm_xio|client_reg_send_magic %p", tmp_vc);

	mpig_cm_xio_vc_set_state(tmp_vc, MPIG_CM_XIO_VC_STATE_CLIENT_SENDING_VC_MAGIC);
    }
    mpig_vc_mutex_unlock(tmp_vc);
    tmp_vc_locked = FALSE;

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "exiting: tmp_vc=" MPIG_PTR_FMT ", mpi_errno="
	MPIG_ERRNO_FMT, (MPIG_PTR_CAST) tmp_vc, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_client_handle_open);
    return;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	/* FT-FIXME: add code to make multiple connection attempts before declaring the VC as failed? */
	
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
static void mpig_cm_xio_client_handle_send_magic(
    const globus_xio_handle_t handle, const globus_result_t op_grc, globus_byte_t * const sbuf, const globus_size_t sbuf_len,
    const globus_size_t nbytes, const globus_xio_data_descriptor_t dd, void * const arg)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vc_t * const tmp_vc = (mpig_vc_t *) arg;
    struct mpig_cm_xio_vc * const tmp_vc_cm = &tmp_vc->cm.xio;
    bool_t tmp_vc_locked = FALSE;
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
	MPIU_Assert(mpig_cm_xio_vc_get_state(tmp_vc) == MPIG_CM_XIO_VC_STATE_CLIENT_SENDING_VC_MAGIC);
	MPIU_ERR_CHKANDJUMP4((op_grc), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|client_handle_send_magic",
	    "**globus|cm_xio|client_handle_send_magic %p %s %d %s", tmp_vc, mpig_vc_get_pg_id(tmp_vc),
	    mpig_vc_get_pg_rank(tmp_vc), globus_error_print_chain(globus_error_peek(op_grc)));
	MPIU_Assert(nbytes == strlen(MPIG_CM_XIO_PROTO_CONNECT_MAGIC) && "not all of the connect magic string was sent");
	
	/* register a read operation to receive the magic string from the accepting side */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM, "registering receive for magic string"));
	mpig_databuf_reset(tmp_vc_cm->msgbuf);
	tmp_vc_cm->msg_hdr_size = 0;
	/* FIXME: [BRT] this should read minimally one byte and maximally the length of the magic string.  A timeout should be
	   associated with the receive in case the sender is not a valid MPIG process and never sends us anything. */
	mpig_cm_xio_register_read_vc_msgbuf(tmp_vc, strlen(MPIG_CM_XIO_PROTO_ACCEPT_MAGIC), strlen(MPIG_CM_XIO_PROTO_ACCEPT_MAGIC),
	    mpig_cm_xio_client_handle_recv_magic, &mpi_errno);
	MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|client_reg_recv_magic",
	    "**globus|cm_xio|client_reg_recv_magic %p", tmp_vc);
	
	mpig_cm_xio_vc_set_state(tmp_vc, MPIG_CM_XIO_VC_STATE_CLIENT_RECEIVING_VC_MAGIC);
    }
    mpig_vc_mutex_unlock(tmp_vc);
    tmp_vc_locked = FALSE;

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "exiting: tmp_vc=" MPIG_PTR_FMT ", mpi_errno="
	MPIG_ERRNO_FMT, (MPIG_PTR_CAST) tmp_vc, mpi_errno));
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
static void mpig_cm_xio_client_handle_recv_magic(
    const globus_xio_handle_t handle, const globus_result_t op_grc, globus_byte_t * const rbuf, const globus_size_t rbuf_len,
    const globus_size_t nbytes, const globus_xio_data_descriptor_t dd, void * const arg)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vc_t * const tmp_vc = (mpig_vc_t *) arg;
    struct mpig_cm_xio_vc * const tmp_vc_cm = &tmp_vc->cm.xio;
    bool_t tmp_vc_locked = FALSE;
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
	
	MPIU_Assert(mpig_cm_xio_vc_get_state(tmp_vc) == MPIG_CM_XIO_VC_STATE_CLIENT_RECEIVING_VC_MAGIC);
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
	mpig_cm_xio_msg_hdr_put_begin(tmp_vc, tmp_vc_cm->msgbuf);
	mpig_cm_xio_msg_hdr_put_msg_type(tmp_vc, tmp_vc_cm->msgbuf, MPIG_CM_XIO_MSG_TYPE_OPEN_VC_REQ);
	mpig_cm_xio_msg_hdr_put_endian(tmp_vc, tmp_vc_cm->msgbuf, MPIG_MY_ENDIAN);
	mpig_cm_xio_msg_hdr_put_df(tmp_vc, tmp_vc_cm->msgbuf, GLOBUS_DC_FORMAT_LOCAL);
	mpig_cm_xio_msg_hdr_put_conn_seqnum(tmp_vc, tmp_vc_cm->msgbuf, tmp_vc_cm->conn_seqnum);
	mpig_cm_xio_msg_hdr_put_rank(tmp_vc, tmp_vc_cm->msgbuf, mpig_process.my_pg_size);
	mpig_cm_xio_msg_hdr_put_rank(tmp_vc, tmp_vc_cm->msgbuf, mpig_process.my_pg_rank);
	pg_id_size = strlen(mpig_process.my_pg->id) + 1;
	MPIU_Strncpy(mpig_databuf_get_eod_ptr(tmp_vc_cm->msgbuf), mpig_process.my_pg->id, pg_id_size);
	mpig_databuf_inc_eod(tmp_vc_cm->msgbuf, pg_id_size);
	mpig_cm_xio_msg_hdr_put_end(tmp_vc, tmp_vc_cm->msgbuf);

	/* send the open request message */
	mpig_cm_xio_register_write_vc_msgbuf(tmp_vc, mpig_cm_xio_client_handle_send_open_vc_req, &mpi_errno);
	MPIU_ERR_CHKANDJUMP3((mpi_errno), mpi_errno, MPI_ERR_OTHER,"**globus|cm_xio|client_reg_send_open_req",
	    "**globus|cm_xio|client_reg_send_open_req %p %s %d", tmp_vc, tmp_vc->pg_id, tmp_vc->pg_rank);

	mpig_cm_xio_vc_set_state(tmp_vc, MPIG_CM_XIO_VC_STATE_CLIENT_SENDING_OPEN_VC_REQ);
    }
    mpig_vc_mutex_unlock(tmp_vc);
    tmp_vc_locked = FALSE;

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "exiting: tmp_vc=" MPIG_PTR_FMT ", mpi_errno="
	MPIG_ERRNO_FMT, (MPIG_PTR_CAST) tmp_vc, mpi_errno));
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
 * void mpig_cm_xio_client_handle_send_open_vc_req(
 *          [IN] handle, [IN] result, [IN] sbuf, [IN] sbuf_len, [IN] nbytes, [IN] NULL, [IN/MOD] vc)
 *
 * this handler gets invoked after sending an OPEN_REQ message over the temporary VC.
 *
 * MT-NOTE: the VC mutex is _NOT_ held by the caller when this routine is called.  this routine is responsible for performing any
 * necessary locking or RC acquire operations.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_client_handle_send_open_vc_req
static void mpig_cm_xio_client_handle_send_open_vc_req(
    const globus_xio_handle_t handle, const globus_result_t op_grc, globus_byte_t * const sbuf, const globus_size_t sbuf_len,
    const globus_size_t nbytes, const globus_xio_data_descriptor_t dd, void * const arg)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vc_t * const tmp_vc = (mpig_vc_t *) arg;
    struct mpig_cm_xio_vc * const tmp_vc_cm = &tmp_vc->cm.xio;
    bool_t tmp_vc_locked = FALSE;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_client_handle_send_open_vc_req);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_client_handle_send_open_vc_req);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "entering: tmp_vc=" MPIG_PTR_FMT ", handle=" MPIG_PTR_FMT
	", op_grc=%d, sbuf=" MPIG_PTR_FMT ", sbuf_len=" MPIG_SIZE_FMT ", nbytes=" MPIG_SIZE_FMT, (MPIG_PTR_CAST) tmp_vc,
	(MPIG_PTR_CAST) handle, op_grc, (MPIG_PTR_CAST) sbuf, sbuf_len, nbytes));

    mpig_vc_mutex_lock(tmp_vc);
    tmp_vc_locked = TRUE;
    {
	MPIU_Assert(mpig_cm_xio_vc_get_state(tmp_vc) == MPIG_CM_XIO_VC_STATE_CLIENT_SENDING_OPEN_VC_REQ);
	MPIU_ERR_CHKANDJUMP4((op_grc), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|client_handle_recv_magic",
	    "**globus|cm_xio|client_handle_recv_magic %p %s %d %s", tmp_vc,  mpig_vc_get_pg_id(tmp_vc),
	    mpig_vc_get_pg_rank(tmp_vc), globus_error_print_chain(globus_error_peek(op_grc)));
	MPIU_Assert(nbytes == mpig_databuf_get_eod(tmp_vc_cm->msgbuf) &&
	    "open request sent to accepting side of connection");

	/* register a read operation to receive the open response control message */
	mpig_databuf_reset(tmp_vc_cm->msgbuf);
	tmp_vc_cm->msg_hdr_size = 0;
	mpig_cm_xio_register_read_vc_msgbuf(tmp_vc, mpig_cm_xio_msg_hdr_remote_sizeof_msg_size(tmp_vc),
	    mpig_databuf_get_size(tmp_vc_cm->msgbuf), mpig_cm_xio_client_handle_recv_open_vc_resp, &mpi_errno);
	MPIU_ERR_CHKANDJUMP3((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|client_reg_recv_open_conn_resp",
	    "**globus|cm_xio|client_reg_recv_open_req %p %s %d", tmp_vc, mpig_vc_get_pg_id(tmp_vc), mpig_vc_get_pg_rank(tmp_vc));
	
	mpig_cm_xio_vc_set_state(tmp_vc, MPIG_CM_XIO_VC_STATE_CLIENT_RECEIVING_OPEN_VC_RESP);
    }
    mpig_vc_mutex_unlock(tmp_vc);
    tmp_vc_locked = FALSE;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "exiting: tmp_vc=" MPIG_PTR_FMT ", mpi_errno="
	MPIG_ERRNO_FMT, (MPIG_PTR_CAST) tmp_vc, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_client_handle_send_open_vc_req);
    return;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	MPIU_Assertp(FALSE && "XXX: FIX ERROR HANDLING CODE"); /* [BRT] */
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_client_handle_send_open_vc_req() */


/*
 * void mpig_cm_xio_client_handle_recv_open_vc_resp(
 *           [IN] handle, [IN] result, [IN] rbuf, [IN] rbuf_len, [IN] nbytes, [IN] NULL, [IN/MOD] vc)
 *
 * MT-NOTE: this function assumes that the VC mutex is held by the current context.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_client_handle_recv_open_vc_resp
void mpig_cm_xio_client_handle_recv_open_vc_resp(
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
    mpig_cm_xio_msg_type_t msg_type;
    mpig_cm_xio_open_resp_t open_resp = MPIG_CM_XIO_OPEN_RESP_UNDEFINED;
    unsigned conn_seqnum;
    mpig_pg_t * pg = NULL;
    int pg_rank;
    int mrc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_client_handle_recv_open_vc_resp);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_client_handle_recv_open_vc_resp);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM,
	"entering: tmp_vc=" MPIG_PTR_FMT ", handle=" MPIG_PTR_FMT ", op_grc=%d, rbuf=" MPIG_PTR_FMT ", rbuf_len=" MPIG_SIZE_FMT
	", nbytes=" MPIG_SIZE_FMT, (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) handle, op_grc, (MPIG_PTR_CAST) rbuf, rbuf_len,
	nbytes));

    MPIG_UNUSED_VAR(fcname);

    mpig_vc_mutex_lock(tmp_vc);
    tmp_vc_locked = TRUE;
    {
	MPIU_Assert(mpig_cm_xio_vc_get_state(tmp_vc) == MPIG_CM_XIO_VC_STATE_CLIENT_RECEIVING_OPEN_VC_RESP);
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
	    
		mpig_cm_xio_register_read_vc_msgbuf(tmp_vc, bytes_needed, bytes_needed, mpig_cm_xio_client_handle_recv_open_vc_resp,
		    &mpi_errno);
		MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|client_reg_recv_open_req");

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
	MPIU_Assert(msg_type == MPIG_CM_XIO_MSG_TYPE_OPEN_VC_RESP);
	mpig_cm_xio_msg_hdr_get_conn_open_resp(tmp_vc, tmp_vc_cm->msgbuf, &open_resp);

	/* if an ACK was received, then do not close the connection when the temp VC is destroyed */
	if (open_resp == MPIG_CM_XIO_OPEN_RESP_VC_ACK)
	{
	    tmp_vc_cm->handle = NULL;
	}
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
	    case MPIG_CM_XIO_OPEN_RESP_VC_ACK:
	    {
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		    "connection ACKed; change state of real VC to connected: tmp_vc=" MPIG_PTR_FMT ", real_vc=" MPIG_PTR_FMT,
		    (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) real_vc));

		mpig_cm_xio_msg_hdr_get_conn_seqnum(tmp_vc, tmp_vc_cm->msgbuf, &conn_seqnum);
		MPIU_Assert(conn_seqnum == tmp_vc_cm->conn_seqnum);
		
		/* set the handle on the real VC, and change the state to connected */
		real_vc_cm->handle = handle;
		mpig_cm_xio_vc_set_state(real_vc, MPIG_CM_XIO_VC_STATE_CONNECTED);
		mpig_cm_xio_vc_list_add(real_vc);

		/* increment the connection sequence number.  FT-FIXME: more thought needs to be given to what should happen to
		   the sequence number in the face of an error. */
		real_vc_cm->conn_seqnum += 1;
	
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
		
		mpi_errno = mpig_cm_xio_recv_next_msg(real_vc);
		MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|recv_next_msg");

		/* if the real VC has any sends queued up, start send them */
		mpi_errno = mpig_cm_xio_send_next_sreq(real_vc);
		MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|send_next_sreq");

		break;
	    }

	    case MPIG_CM_XIO_OPEN_RESP_VC_NAK:
	    {
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
		    "connection NAKed; waiting for remote connect to finish: tmp_vc=" MPIG_PTR_FMT ", real_vc=" MPIG_PTR_FMT,
		    (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) real_vc));

		mpig_cm_xio_msg_hdr_get_conn_seqnum(tmp_vc, tmp_vc_cm->msgbuf, &conn_seqnum);
		MPIU_Assert(conn_seqnum == tmp_vc_cm->conn_seqnum);
		break;
	    }

	    case MPIG_CM_XIO_OPEN_RESP_ERR_MSG_INCOMPLETE:
	    case MPIG_CM_XIO_OPEN_RESP_ERR_MSG_TYPE_BAD:
	    case MPIG_CM_XIO_OPEN_RESP_ERR_MSG_TOO_LONG:
	    case MPIG_CM_XIO_OPEN_RESP_ERR_VC_NOT_XIO:
	    case MPIG_CM_XIO_OPEN_RESP_ERR_VC_CONNECTED:
	    case MPIG_CM_XIO_OPEN_RESP_ERR_VC_DISCONNECTING:
	    case MPIG_CM_XIO_OPEN_RESP_ERR_VC_FAILED:
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

    /* close the connection and destroy the temp VC object, releasing the associated PG reference */
    mrc = mpig_cm_xio_conn_destroy_tmp_vc(tmp_vc);
    MPIU_ERR_CHKANDSTMT((mrc), mrc, MPI_ERR_OTHER, {MPIU_ERR_ADD(mpi_errno, mrc);}, "**globus|cm_xio|tmp_vc_destroy");

    /* the internal reference to the VC established in mpig_cm_xio_client_connect() needs to be release */
    mpig_vc_mutex_unlock_conditional(real_vc, (real_vc_locked));
    if (real_vc)
    {
	mpig_vc_release_ref(real_vc);
    }
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "exiting: tmp_vc=" MPIG_PTR_FMT ", real_vc=" MPIG_PTR_FMT
	", mpi_errno=" MPIG_ERRNO_FMT, (MPIG_PTR_CAST) tmp_vc, (MPIG_PTR_CAST) real_vc, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_client_handle_recv_open_vc_resp);
    return;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	MPIU_Assertp(FALSE && "XXX: FIX ERROR HANDLING CODE"); /* [BRT] */
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_client_handle_recv_open_vc_resp() */

#endif /* MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS */
/**********************************************************************************************************************************
					    END CLIENT SIDE CONNECTION ESTABLISHMENT
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						    BEGIN CONNECTION TEARDOWN
**********************************************************************************************************************************/
#if !defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS)

static int mpig_cm_xio_disconnect(mpig_vc_t * vc);

static int mpig_cm_xio_disconnect_send_enq_close_msg(mpig_vc_t * vc, bool_t cr, bool_t ack);

static void mpig_cm_xio_disconnect_handle_send_close_msg(
    globus_xio_handle_t handle, globus_result_t op_grc, globus_xio_iovec_t * iov, int iov_cnt, globus_size_t nbytes,
    globus_xio_data_descriptor_t dd, void * arg);

static int mpig_cm_xio_disconnect_handle_recv_close_msg(mpig_vc_t * vc);

static void mpig_cm_xio_disconnect_handle_close(globus_xio_handle_t handle, globus_result_t op_grc, void * arg);


#else /* defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS) */


/*
 * <mpi_errno> mpig_cm_xio_disconnect([IN/MOD] vc)
 *
 * MT-NOTE: this function assumes that the VC mutex is held by the current context.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_disconnect
static int mpig_cm_xio_disconnect(mpig_vc_t * const vc)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    struct mpig_cm_xio_vc * const vc_cm = &vc->cm.xio;
    bool_t vc_was_inuse;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_disconnect);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_disconnect);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "entering: vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) vc));

    MPIG_UNUSED_VAR(fcname);

    MPIU_Assertp(vc_cm->handle != NULL);

    mpig_cm_xio_vc_inc_ref_count(vc, &vc_was_inuse);
    MPIU_Assert(vc_was_inuse == FALSE);

    if (mpig_cm_xio_vc_get_state(vc) == MPIG_CM_XIO_VC_STATE_CONNECTED)
    {
	/* if the VC is in the connected state, then send a close request (CR) message */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM, "VC connected; sending CR: vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) vc));
	    
	mpi_errno = mpig_cm_xio_disconnect_send_enq_close_msg(vc, TRUE, FALSE);
	MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|disconnect_send_enq_close_msg");
	mpig_cm_xio_vc_set_state(vc, MPIG_CM_XIO_VC_STATE_DISCONNECT_SENDING_CLOSE_REQ_AWAITING_CLOSE_REQ);

	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM, "CR message enqueued; VC status updated: vc=" MPIG_PTR_FMT ", new_vc_state=%s",
	    (MPIG_PTR_CAST) vc, mpig_cm_xio_vc_state_get_string(mpig_cm_xio_vc_get_state(vc))));
    }
    else if (mpig_cm_xio_vc_get_state(vc) == MPIG_CM_XIO_VC_STATE_DISCONNECT_RECEIVED_CLOSE_REQ)
    {
	/* if the VC has already received a close request (CR) from the remote process, then send a close request and acknowledge
	   the reception of the CR from the close request */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM,
	    "VC connected, but received CR; sending CR/ACK: vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) vc));
	    
	mpi_errno = mpig_cm_xio_disconnect_send_enq_close_msg(vc, TRUE, TRUE);
	MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|disconnect_send_enq_close_msg");
	mpig_cm_xio_vc_set_state(vc, MPIG_CM_XIO_VC_STATE_DISCONNECT_SENDING_ACK_AWAITING_ACK);

	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM, "CR/ACK message enqueued; VC status updated: vc=" MPIG_PTR_FMT
	    ", new_vc_state=%s", (MPIG_PTR_CAST) vc, mpig_cm_xio_vc_state_get_string(mpig_cm_xio_vc_get_state(vc))));
    }
    else if (mpig_cm_xio_vc_has_failed(vc))
    {
	globus_result_t grc;
	
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM, "VC has failed; registering a close operation: vc=" MPIG_PTR_FMT
	    ", vc_state=%s", (MPIG_PTR_CAST) vc, mpig_cm_xio_vc_state_get_string(mpig_cm_xio_vc_get_state(vc))));
	grc = globus_xio_register_close(vc_cm->handle, NULL, mpig_cm_xio_disconnect_handle_close, vc);
	vc_cm->handle = NULL;
	MPIU_ERR_CHKANDJUMP1((grc), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|xio_reg_close",
	    "**globus|cm_xio|xio_reg_close %s", globus_error_print_chain(globus_error_peek(grc)));
    }
    else if (mpig_cm_xio_vc_is_undefined(vc))
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM, "ERROR: VC state is undefined: vc=" MPIG_PTR_FMT ", vc_state=%s",
	    (MPIG_PTR_CAST) vc, mpig_cm_xio_vc_state_get_string(mpig_cm_xio_vc_get_state(vc))));
	MPIU_Assertp(FALSE && "VC state is undefined during the initiation of the disconnect protocol");
    }   /* --END ERROR HANDLING-- */
    else if (mpig_cm_xio_vc_state_is_valid(vc))
    {
	/* if the VC state is valid, but not one of the above, then consider it an error.  at one point, an incoming connection
	   for this VC that was NAKed by the server could cause a VC ref count increment to one (1) and decrement back to zero
	   (0) causing mpig_cm_xio_disconnect() to be called twice.  however, now that we are bumping the VC ref count in this
	   routine, we shouldn't see anything of this nature. */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM,
	    "ERROR: the VC is not in a state where it make sense to initiate the disconnect protocol: vc=" MPIG_PTR_FMT
	    ", vc_state=%s", (MPIG_PTR_CAST) vc, mpig_cm_xio_vc_state_get_string(mpig_cm_xio_vc_get_state(vc))));
	MPIU_Assertp(FALSE && "encountered a valid but unexpected VC state during the initiation of the disconnect protocol");
    }
    else
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM,
	    "ERROR: encountered unexpected state: vc_state_value=%d", mpig_cm_xio_vc_get_state(vc)));
	MPIU_Assertp(FALSE && "encountered an invalid VC state during the initiation of the disconnect protocol");
    }   /* --END ERROR HANDLING-- */
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "exiting: vc=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT,
	(MPIG_PTR_CAST) vc, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_disconnect);
    return mpi_errno;
    
  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	bool_t vc_inuse;

	/* the calling routine will take care of removing the VC from VC tracking list */
	mpig_cm_xio_vc_dec_ref_count(vc, &vc_inuse);
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_disconnect() */


/*
 * <mpi_errno> mpig_cm_xio_disconnect_send_enq_close_msg([IN/MOD] vc, [IN] cr, [IN] ack)
 *
 * vc [IN/MOD] - connection over which to send the synchronous send acknowledgement
 * cr [IN] - flag indicating if a close request should be sent
 * ack [IN] - flag indicating if an ack of a received close request should be sent
 *
 * MT-NOTE: this routine assumes that the VC mutex is held by the current context.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_disconnect_send_enq_close_msg
static int mpig_cm_xio_disconnect_send_enq_close_msg(mpig_vc_t * const vc, const bool_t cr, const bool_t ack)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPID_Request * sreq = NULL;
    struct mpig_cm_xio_request * sreq_cm = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_disconnect_send_enq_close_msg);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_disconnect_send_enq_close_msg);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "entering: vc=" MPIG_PTR_FMT ", cr=%s, ack=%s",
	(MPIG_PTR_CAST) vc, MPIG_BOOL_STR(cr), MPIG_BOOL_STR(ack)));

    /* allocate a new send request */
    mpig_request_alloc(&sreq);
    if (sreq == NULL)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM,
	    "ERROR: malloc failed when attempting to allocate a send request"));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "sreq for close msg");
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */
    
    sreq_cm = &sreq->cm.xio;

    mpig_request_construct(sreq);
    mpig_cm_xio_request_construct(sreq);
    mpig_request_set_params(sreq, MPID_REQUEST_UNDEFINED, MPIG_REQUEST_TYPE_INTERNAL, 0, 0, NULL, 0, MPI_DATATYPE_NULL,
	MPI_PROC_NULL, MPI_ANY_TAG, 0, vc);
    mpig_cm_xio_request_set_state(sreq, MPIG_CM_XIO_REQ_STATE_SEND_CTRL_MSG);
    mpig_cm_xio_request_set_msg_type(sreq, MPIG_CM_XIO_MSG_TYPE_CLOSE);
    sreq_cm->gcb = mpig_cm_xio_disconnect_handle_send_close_msg;
    
    /* pack message header */
    mpig_cm_xio_msg_hdr_put_begin(vc, sreq_cm->msgbuf);
    mpig_cm_xio_msg_hdr_put_msg_type(vc, sreq_cm->msgbuf, mpig_cm_xio_request_get_msg_type(sreq));
    mpig_cm_xio_msg_hdr_put_bool(vc, sreq_cm->msgbuf, cr);
    mpig_cm_xio_msg_hdr_put_bool(vc, sreq_cm->msgbuf, ack);
    mpig_cm_xio_msg_hdr_put_end(vc, sreq_cm->msgbuf);

    mpig_iov_reset(sreq_cm->iov, 0);
    mpig_iov_add_entry(sreq_cm->iov, mpig_databuf_get_ptr(sreq_cm->msgbuf), mpig_databuf_get_eod(sreq_cm->msgbuf));

    /* send message */
    mpi_errno = mpig_cm_xio_send_enq_sreq(vc, sreq);
    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|send_enq_sreq");
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "exiting: vc=" MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT
	", sreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, (MPIG_PTR_CAST) vc, MPIG_HANDLE_VAL(sreq), (MPIG_PTR_CAST) sreq,
	mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_disconnect_send_enq_close_msg);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_disconnect_send_enq_close_msg() */


/*
 * <void> mpig_cm_xio_disconnect_handle_send_close_msg([IN] handle, [IN] op_grc, [IN] iov, [IN] iov_cnt, [IN] nbytes,
 *     [IN] dd, [IN/MOD] vc)
 *
 * MT-NOTE: this routine assumes that the VC mutex is _NOT_ held by the current context.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_disconnect_handle_send_close_msg
static void mpig_cm_xio_disconnect_handle_send_close_msg(
    const globus_xio_handle_t handle, const globus_result_t op_grc, globus_xio_iovec_t * const iov, const int iov_cnt,
    const globus_size_t nbytes, const globus_xio_data_descriptor_t dd, void * const arg)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vc_t * vc = (mpig_vc_t *) arg;
    struct mpig_cm_xio_vc * vc_cm = &vc->cm.xio;
    bool_t vc_locked = FALSE;
    MPID_Request * sreq;
    globus_result_t grc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_disconnect_handle_send_close_msg);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_disconnect_handle_send_close_msg);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "entering: vc=" MPIG_PTR_FMT ", handle=" MPIG_PTR_FMT
	", op_grc=%d, iov=" MPIG_PTR_FMT ", iov_cnt=%d, nbytes=" MPIG_SIZE_FMT, (MPIG_PTR_CAST) vc, (MPIG_PTR_CAST) handle,
	op_grc, (MPIG_PTR_CAST) iov, iov_cnt, nbytes));

    mpig_vc_mutex_lock(vc);
    vc_locked = TRUE;
    {
	if (op_grc)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM, "ERROR: %s operation failed: vc=" MPIG_PTR_FMT
		", handle=" MPIG_PTR_FMT ", msg=%s", "globus_xio_register_write", (MPIG_PTR_CAST) vc, (MPIG_PTR_CAST) handle,
		globus_error_print_chain(globus_error_peek(op_grc))));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|disconnect_handle_send_close_msg",
		"**globus|cm_xio|disconnect_handle_send_close_msg %s", globus_error_print_chain(globus_error_peek(op_grc)));
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */
    
	/* destroy the send request */
	sreq = vc_cm->active_sreq;
	MPIU_Assert(mpig_cm_xio_request_get_state(sreq) == MPIG_CM_XIO_REQ_STATE_SEND_CTRL_MSG);
	MPIU_Assert(mpig_cm_xio_request_get_msg_type(sreq) == MPIG_CM_XIO_MSG_TYPE_CLOSE);
	mpig_request_destroy(sreq);
	vc_cm->active_sreq = NULL;

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
		
		mpi_errno = mpig_cm_xio_disconnect_send_enq_close_msg(vc, FALSE, TRUE);
		MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|disconnect_send_enq_close_msg");
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

		mpi_errno = mpig_cm_xio_disconnect_send_enq_close_msg(vc, FALSE, TRUE);
		MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|disconnect_send_enq_close_msg");
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
		MPIU_ERR_CHKANDJUMP1((grc), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|xio_reg_close",
		    "**globus|cm_xio|xio_reg_close %s", globus_error_print_chain(globus_error_peek(grc)));
		mpig_cm_xio_vc_set_state(vc, MPIG_CM_XIO_VC_STATE_DISCONNECT_CLOSING_VC);
		vc_cm->handle = NULL;
		
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
	if ((mpig_cm_xio_vc_is_connected(vc) || mpig_cm_xio_vc_is_disconnecting(vc)) && vc_cm->active_sreq == NULL)
	{
	    mpi_errno = mpig_cm_xio_send_next_sreq(vc);
	    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|send_next_sreq");
	}
    }
    mpig_vc_mutex_unlock(vc);
    vc_locked = FALSE;

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "exiting: vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) vc));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_disconnect_handle_send_close_msg);
    return;
    
  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	mpig_vc_mutex_lock_conditional(vc, (!vc_locked));
	{
	    mpig_cm_xio_fault_handle_async_vc_error(vc, mpi_errno);
	}
	mpig_vc_mutex_unlock(vc);

	mpig_vc_release_ref(vc);
	
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_disconnect_handle_send_close_msg() */


/*
 * <mpi_errno> mpig_cm_xio_disconnect_handle_recv_close_msg([IN/MOD] vc)
 *
 * MT-NOTE: this function assumes that the VC mutex is held by the current context.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_disconnect_handle_recv_close_msg
static int mpig_cm_xio_disconnect_handle_recv_close_msg(mpig_vc_t * const vc)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    struct mpig_cm_xio_vc * const vc_cm = &vc->cm.xio;
    bool_t cr;
    bool_t ack;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_disconnect_handle_recv_close_msg);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_disconnect_handle_recv_close_msg);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "entering; vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) vc));
    
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
	    mpig_cm_xio_vc_set_state(vc, MPIG_CM_XIO_VC_STATE_DISCONNECT_RECEIVED_CLOSE_REQ);
	    
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

	    mpi_errno = mpig_cm_xio_disconnect_send_enq_close_msg(vc, FALSE, TRUE);
	    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|disconnect_send_enq_close_msg");
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
	    MPIU_ERR_CHKANDJUMP1((grc), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|xio_reg_close",
		"**globus|cm_xio|xio_reg_close %s", globus_error_print_chain(globus_error_peek(grc)));
	    mpig_cm_xio_vc_set_state(vc, MPIG_CM_XIO_VC_STATE_DISCONNECT_CLOSING_VC);
	    vc_cm->handle = NULL;
	    
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
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "exiting: vc=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT,
	(MPIG_PTR_CAST) vc, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_disconnect_handle_recv_close_msg);
    return mpi_errno;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	bool_t vc_inuse;

	/* release VC reference acquired by the mpig_cm_xio_disconnect() routine.  mpig_cm_xio_recv_handle_incoming_msg() has
	   incremented the VC reference count before calling the handler, so we needn't worry about the reference count reaching
	   zero */
	mpig_cm_xio_vc_dec_ref_count(vc, &vc_inuse);
	MPIU_Assertp(vc_inuse == TRUE);

	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_disconnect_handle_recv_close_msg() */


/*
 * <void> mpig_cm_xio_disconnect_handle_close([IN] handle, [IN] result, [IN] NULL)
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
static void mpig_cm_xio_disconnect_handle_close(globus_xio_handle_t handle, globus_result_t op_grc, void * arg)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vc_t * vc = (mpig_vc_t *) arg;
    struct mpig_cm_xio_vc * vc_cm = &vc->cm.xio;
    bool_t vc_locked = FALSE;
    bool_t vc_inuse;
    mpig_pg_t * pg = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_disconnect_handle_close);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_disconnect_handle_close);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "entering: vc= " MPIG_PTR_FMT ", op_grc=%d",
	(MPIG_PTR_CAST) vc, op_grc));

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM | MPIG_DEBUG_LEVEL_VC, "VC disconnected: vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) vc));
    mpig_vc_mutex_lock(vc);
    vc_locked = TRUE;
    {
	MPIU_Assert(vc_cm->handle == NULL && (mpig_cm_xio_vc_has_failed(vc) ||
	    mpig_cm_xio_vc_get_state(vc) == MPIG_CM_XIO_VC_STATE_DISCONNECT_CLOSING_VC));

	if (op_grc)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VCCM, "ERROR: %s operation failed: vc=" MPIG_PTR_FMT
		", handle=" MPIG_PTR_FMT ", msg=%s", "globus_xio_register_close", (MPIG_PTR_CAST) vc, (MPIG_PTR_CAST) handle,
		globus_error_print_chain(globus_error_peek(op_grc))));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|disconnect_handle_close",
		"**globus|cm_xio|disconnect_handle_close %s", globus_error_print_chain(globus_error_peek(op_grc)));
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

	/* change the state of the VC to 'unconnected' unless the VC has failed */
	if (mpig_cm_xio_vc_has_failed(vc) == FALSE)
	{
	    mpig_cm_xio_vc_set_state(vc, MPIG_CM_XIO_VC_STATE_UNCONNECTED);
	}

#if 0
	/* MPI-2-NOTE: a MPI_Comm_disconnect() followed closely by or simultaneously with a MPI_Comm_connect/accept() can cause
	 * two different scenarios to occur.
	 *
	 * first, an incoming OPEN_VC_REQ message may arrive to establish a connection for this VC before the disconnect protocol
	 * has finished.  in that case, the temp_vc for the new connection is queued.  we need to check that queue and complete
	 * the connection if an entry is found.
	 *
	 * second, after a new communicator referencing this VC is formed using MPI_Comm_connect/accept(), one or more send
	 * requests could have been enqueued while the disconnect protocol was in progress.  if an new connection request has not
	 * been received (see the first scenario), then a new connection must be initiated from this end.
	 */

	/* MPI-2-FIXME: check the pending connection queue for a temp_vc with the same PG id and PG rank as ours */
	if (matching temp VC in pending connnections queue)
	{
	    ...;

	    mpig_cm_xio_vc_inc_ref_count(vc, &vc_was_inuse);
	    vc_inuse = TRUE;
	}
	else if (mpig_cm_xio_sendq_empty(vc) == FALSE)
	{
	    /* MPI-2-FIXME: if no pending connection exists, and the send queue is not empty, then start forming a new
	       connection */
	    bool_t failed;

	    MPIU_Assert(vc_inuse == TRUE);
	    
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_VCCM | MPIG_DEBUG_LEVEL_VC, "VC send queue not empty; initiating reconnect: vc="
		MPIG_PTR_FMT, (MPIG_PTR_CAST) vc));
	    mpi_errno = mpig_cm_xio_client_connect(vc);
	    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|client_connect");
	}
#else
	/* MPI-2-NOTE: this assert is only true for MPI-1 code.  it should be removed when MPI-2 support is added. */
	MPIU_Assert(mpig_cm_xio_sendq_empty(vc) == TRUE);
#endif

	/*
	 * decrement the reference count of the VC to compensate for the bump in mpig_cm_xio_disconnect().  remove the VC from
	 * the VC tracking list if the VC is no longer in use.
	 *
	 * MPI-2-NOTE: the only reason that the VC reference count would not be zero is if the code above initiated a connect or
	 * an incoming connection was pending.  in that case, we don't want to remove the VC from the list of active VCs.
	 */
	pg = mpig_vc_get_pg(vc);
	mpig_cm_xio_vc_dec_ref_count(vc, &vc_inuse);
	if (vc_inuse == FALSE)
	{
	    mpig_cm_xio_vc_list_remove(vc);
	}
    }
    mpig_vc_mutex_unlock(vc);
    vc_locked = FALSE;

    /* if the VC is no longer in use, then release the PG reference */
    if (vc_inuse == FALSE)
    {
	mpig_pg_release_ref(pg);
    }
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VCCM, "exiting: vc= " MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT,
	(MPIG_PTR_CAST) vc, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_disconnect_handle_close);
    return;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	/* report the error through MPID_Progress_{poke,test,wait}() */
	mpig_vc_mutex_lock_conditional(vc, (!vc_locked));
	{
	    mpig_cm_xio_fault_handle_async_vc_error(vc, mpi_errno);
	}
	mpig_vc_mutex_unlock(vc);

	mpig_vc_release_ref(vc);
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_disconnect_handle_close() */

#endif /* MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS */
/**********************************************************************************************************************************
						     END CONNECTION TEARDOWN
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						   BEGIN MPI CONNECT / ACCEPT
**********************************************************************************************************************************/
#if !defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS)

MPIG_STATIC globus_list_t * mpig_cm_xio_port_list = GLOBUS_NULL;

typedef struct mpig_cm_xio_port
{
    globus_mutex_t mutex;
    globus_cond_t cond;
    globus_uuid_t uuid;
    globus_fifo_t connq;
    int waiting;
}
mpig_cm_xio_port_t;


MPIG_STATIC int mpig_cm_xio_port_find_and_lock(const char * port_name, mpig_cm_xio_port_t ** port);

MPIG_STATIC int mpig_cm_xio_port_enqueue_connection(mpig_cm_xio_port_t * port, mpig_connection_t * conn);

MPIG_STATIC int mpig_cm_xio_port_dequeue_connection(mpig_cm_xio_port_t * port, mpig_connection_t ** conn_p);

MPIG_STATIC int mpig_cm_xio_port_has_matching_name(void * port_in, void * port_name_in);

#define mpig_cm_xio_port_mutex_lock(port_) {globus_mutex_lock(&(port_)->mutex);}
#define mpig_cm_xio_port_mutex_unlock(port_) {globus_mutex_unlock(&(port_)->mutex);}


#else

/*
 * <mpi_errno> mpig_port_open([IN] info, [OUT] port_name)
 *
 * Parameters:
 *
 * info [IN] - pointer to an info object containing user specified key-value pairs that could be used to influence the type of
 * port created.  for now the object is ignored.
 *
 * port_name [OUT] - an array of characters in which to string describing the method(s) and address(es) the remote communicator
 * can use to connect to the local (accepting) communicator.  the user allocates this array, which must be at least
 * MPI_MAX_PORT_NAME characters in size.  likewise, the string return must be no longer than MPI_MAX_PORT_NAME, including the
 * termination character ('\0').
 *
 * Returns: an MPI error code
 */
#undef FUNCNAME
#define FUNCNAME mpig_port_open
int mpig_port_open(MPID_Info * info, char * port_name)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_cm_xio_port_t * port;
    mpig_bc_t bc;
    char * bc_str = NULL;
    char uint_str[10];
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKPMEM_DECL(1);
    MPIG_STATE_DECL(MPID_STATE_mpig_port_open);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_port_open);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DYNAMIC, "entering: info=" MPIG_PTR_FMT, (MPIG_PTR_CAST) info));

    /* create a new port object */
    MPIU_CHKPMEM_MALLOC(port, mpig_cm_xio_port_t *, sizeof(mpig_cm_xio_port_t), mpi_errno, "port object");
    globus_mutex_init(&port->mutex, NULL);
    globus_cond_init(&port->cond, NULL);
    globus_uuid_create(&port->uuid);
    globus_fifo_init(&port->connq);
    port->waiting = 0;

    /* create a business card with the contact string and port uuid in it */
    mpig_bc_construct(&bc);
    
    MPIU_Snprintf(uint_str, (size_t) 10, "%u", (unsigned) MPIG_CM_XIO_PROTO_VERSION);
    mpi_errno = mpig_bc_add_contact(&bc, "CM_XIO_CONNACC_PROTO_VERSION", uint_str);
    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|bc_add_contact",
	"**globus|bc_add_contact %s", "CM_XIO_CONNACC_PROTO_VERSION");

    mpi_errno = mpig_bc_add_contact(&bc, "CM_XIO_CONNACC_CONTACT_STRING", mpig_cm_xio_server_cs);
    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|bc_add_contact",
	"**globus|bc_add_contact %s", "CM_XIO_CONNACC_CONTACT_STRING");

    mpi_errno = mpig_bc_add_contact(&bc, "CM_XIO_CONNACC_PORT_UUID", port->uuid.text);
    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|bc_add_contact",
	"**globus|bc_add_contact %s", "CM_XIO_CONNACC_PORT_UUID");

    /* serialize the business card, copy it into the caller's port name string */
    mpi_errno = mpig_bc_serialize_object(&bc, &bc_str);
    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|bc_serialize");
    
    MPIU_ERR_CHKANDJUMP2((strlen(bc_str) + 1 > MPI_MAX_PORT_NAME), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|port_name_too_long",
	"**globus|cm_xio|port_name_too_long %d %d", strlen(bc_str) + 1, MPI_MAX_PORT_NAME);

    MPIU_Strncpy(port_name, bc_str, MPI_MAX_PORT_NAME);

    mpig_bc_free_serialized_object(bc_str);
    bc_str = NULL;

    /* add the port object to the list of port objects */
    mpig_cm_xio_mutex_lock();
    {
	rc = globus_list_insert(&mpig_cm_xio_port_list, port);
    }
    mpig_cm_xio_mutex_unlock();
    MPIU_ERR_CHKANDJUMP((rc), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|port_list_insert");
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DYNAMIC, "exiting: port_name=\"%s\", mpi_errno=" MPIG_ERRNO_FMT,
	(!mpi_errno) ? port_name : "", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_port_open);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	if (port != NULL)
	{
	    globus_mutex_destroy(&port->mutex);
	    globus_cond_destroy(&port->cond);
	    globus_fifo_destroy(&port->connq);
	    port->waiting = -1;
	}

	if (bc_str == NULL)
	{
	    mpig_bc_free_serialized_object(bc_str);
	}

	MPIU_CHKPMEM_REAP();
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_port_open() */


/*
 * <mpi_errno> mpig_port_close([IN] port_name)
 *
 * Parameters:
 *
 *   port_name [IN] - character string containing the name of the port to close
 *
 * Returns: a MPI error code
 */
#undef FUNCNAME
#define FUNCNAME mpig_port_close
int mpig_port_close(const char * port_name)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    globus_list_t * port_list_entry = NULL;
    mpig_cm_xio_port_t * port = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_port_close);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_port_close);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DYNAMIC, "entering: port_name=\"%s\"", port_name));

    /* find the port object in the port list and remove it */
    mpig_cm_xio_mutex_lock();
    {
	port_list_entry = globus_list_search_pred(mpig_cm_xio_port_list, mpig_cm_xio_port_has_matching_name, (char *) port_name);
	if (port_list_entry)
	{
	    port = globus_list_first(port_list_entry);
	    globus_list_remove(&mpig_cm_xio_port_list, port_list_entry);
	}
    }
    mpig_cm_xio_mutex_unlock();

    MPIU_ERR_CHKANDJUMP1((port == NULL), mpi_errno, MPI_ERR_OTHER, "**portexist", "**portexist %s", port_name);
    
    mpig_cm_xio_port_mutex_lock(port);
    {
	/* XXX: send an error message to all incoming connections enqueued on the port (and then close the connections) */
    }
    mpig_cm_xio_port_mutex_unlock(port);

    /* destroy the port object */
    globus_mutex_destroy(&port->mutex);
    globus_cond_destroy(&port->cond);
    globus_fifo_destroy(&port->connq);
    port->waiting = -1;
    MPIU_Free(port);
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DYNAMIC, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_port_close);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_port_close() */


/*
 * <mpi_errno> mpig_port_cm_xio_port_find_and_lock([IN] port_name, [OUT] port)
 *
 * Parameters:
 *
 *   port_name [IN] - character string containing the name of the port to find
 *
 *   port [OUT] - the port object, if one is found; other NULL
 *
 * Returns: a MPI error code
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_port_find_and_lock
MPIG_STATIC int mpig_cm_xio_port_find_and_lock(const char * port_name, mpig_cm_xio_port_t ** port_p)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    globus_list_t * port_list_entry = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_port_find_and_lock);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_port_find_and_lock);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DYNAMIC, "entering: port_name=\"%s\"", port_name));

    mpig_cm_xio_mutex_lock();
    {
	port_list_entry = globus_list_search_pred(mpig_cm_xio_port_list, mpig_cm_xio_port_has_matching_name, (char *) port_name);
	if (port_list_entry != NULL)
	{
	    *port_p = globus_list_first(port_list_entry);
	    mpig_cm_xio_port_mutex_lock(*port_p);
	}
	else
	{
	    *port_p = NULL;
	}
    }
    mpig_cm_xio_mutex_unlock();

    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DYNAMIC, "exiting: port=" MPIG_PTR_FMT, (MPIG_PTR_CAST) *port_p));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_port_find_and_lock);
    return mpi_errno;
}
/* mpig_cm_xio_port_find_and_lock() */


/*
 * <mpi_errno> mpig_port_cm_xio_port_enqueue_connection([IN] port, [IN] conn)
 *
 * Parameters:
 *
 *   port [IN] - the port object on which the connection should be enqueued
 *
 *   conn [IN] - the connection object to be enqueued
 *
 * Returns: a MPI error code
 *
 * MT-NOTE: this routine assumes the port mutex is held by the calling context.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_port_enqueue_connection
MPIG_STATIC int mpig_cm_xio_port_enqueue_connection(mpig_cm_xio_port_t * port, mpig_connection_t * conn)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_port_enqueue_connection);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_port_enqueue_connection);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DYNAMIC, "entering: port=" MPIG_PTR_FMT ", conn=" MPIG_PTR_FMT,
	(MPIG_PTR_CAST) port, (MPIG_PTR_CAST) conn));

    rc = globus_fifo_enqueue(&port->connq, conn);
    MPIU_ERR_CHKANDJUMP((rc), mpi_errno, MPI_ERR_OTHER, "**globus|fifo_enq");
    
    if (port->waiting > 0)
    {
	globus_cond_signal(&port->cond);
    }

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DYNAMIC, "exiting: port=" MPIG_PTR_FMT ", conn=" MPIG_PTR_FMT
	", mpi_errno=", (MPIG_PTR_CAST) port, (MPIG_PTR_CAST) conn, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_port_enqueue_connection);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_port_enqueue_connection() */


/*
 * <mpi_errno> mpig_port_cm_xio_port_dequeue_connection([IN] port, [OUT] conn)
 *
 * Parameters:
 *
 *   port [IN] - the port object on which to dequeue a new connection
 *
 *   conn [OUT] - the new connection object
 *
 * Returns: a MPI error code
 *
 * MT-NOTE: this routine assumes the port mutex is held by the calling context.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_port_dequeue_connection
MPIG_STATIC int mpig_cm_xio_port_dequeue_connection(mpig_cm_xio_port_t * port, mpig_connection_t ** conn_p)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_port_dequeue_connection);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_port_dequeue_connection);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DYNAMIC, "entering: port=" MPIG_PTR_FMT, (MPIG_PTR_CAST) port));

    port->waiting += 1;
    while (globus_fifo_empty(&port->connq))
    {
	globus_cond_wait(&port->cond, &port->mutex);
    }
    port->waiting -= 1;
    
    *conn_p = globus_fifo_dequeue(&port->connq);

    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DYNAMIC, "exiting: port=" MPIG_PTR_FMT ", conn=" MPIG_PTR_FMT
	", mpi_errno=", (MPIG_PTR_CAST) port, (MPIG_PTR_CAST) *conn_p, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_port_dequeue_connection);
    return mpi_errno;
}
/* mpig_cm_xio_port_dequeue_connection() */


#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_port_has_matching_name
MPIG_STATIC int mpig_cm_xio_port_has_matching_name(void * const port_in, void * const port_name_in)
{
    const mpig_cm_xio_port_t * const port = (const mpig_cm_xio_port_t *) port_in;
    const char * port_name = (const char *) port_name_in;

    return (strcmp(port->uuid.text, port_name) == 0);
}
/* mpig_cm_xio_port_has_matching_name */

#endif
/**********************************************************************************************************************************
						   BEGIN MPI CONNECT / ACCEPT
**********************************************************************************************************************************/
