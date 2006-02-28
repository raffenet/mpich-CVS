/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

/**********************************************************************************************************************************
						 BEGIN REQUEST OBJECT MANAGEMENT
**********************************************************************************************************************************/
#if !defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS)

MPIG_STATIC void mpig_cm_xio_request_destruct_fn(MPID_Request * req);
MPIG_STATIC const char * mpig_cm_xio_request_state_get_string(mpig_cm_xio_req_states_t req_state);

#define mpig_cm_xio_request_construct(req_)						\
{											\
    (req_)->cm.xio.state = MPIG_CM_XIO_REQ_STATE_UNDEFINED;				\
    (req_)->cm.xio.msg_type = MPIG_CM_XIO_MSG_TYPE_UNDEFINED;				\
    (req_)->cm.xio.cc = 0;								\
    (req_)->cm.xio.buf_type = MPIG_CM_XIO_USERBUF_TYPE_UNDEFINED;			\
    (req_)->cm.xio.stream_pos = 0;							\
    (req_)->cm.xio.stream_size = 0;							\
    mpig_iov_construct((req_)->cm.xio.iov, MPIG_CM_XIO_IOV_NUM_ENTRIES);		\
    (req_)->cm.xio.gcb = NULL;								\
    (req_)->cm.xio.sreq_type = MPIG_REQUEST_TYPE_UNDEFINED;				\
    mpig_databuf_construct((req_)->cm.xio.msgbuf, MPIG_CM_XIO_REQUEST_MSGBUF_SIZE);	\
    (req_)->cm.xio.df = NEXUS_DC_FORMAT_UNKNOWN;					\
    (req_)->cm.xio.databuf = NULL;							\
    (req_)->cm.xio.sendq_next = NULL;							\
											\
    (req_)->dev.cm_destruct = mpig_cm_xio_request_destruct_fn;				\
}

#define mpig_cm_xio_request_destruct(req_)				\
{									\
    (req_)->cm.xio.state = MPIG_CM_XIO_REQ_STATE_UNDEFINED;		\
    (req_)->cm.xio.msg_type = MPIG_CM_XIO_MSG_TYPE_UNDEFINED;		\
    (req_)->cm.xio.cc = 0;						\
    (req_)->cm.xio.buf_type = MPIG_CM_XIO_USERBUF_TYPE_UNDEFINED;	\
    (req_)->cm.xio.stream_pos = 0;					\
    (req_)->cm.xio.stream_size = 0;					\
    mpig_iov_destruct((req_)->cm.xio.iov);				\
    (req_)->cm.xio.gcb = NULL;						\
    (req_)->cm.xio.sreq_type = MPIG_REQUEST_TYPE_UNDEFINED;		\
    mpig_databuf_destruct((req_)->cm.xio.msgbuf);			\
    (req_)->cm.xio.df = NEXUS_DC_FORMAT_UNKNOWN;			\
    if ((req_)->cm.xio.databuf != NULL)					\
    {									\
	mpig_databuf_destroy((req_)->cm.xio.databuf);			\
	(req_)->cm.xio.databuf = NULL;					\
    }									\
    (req_)->cm.xio.sendq_next = NULL;					\
}

#define mpig_cm_xio_request_set_cc(req_, cc_)										\
{															\
    (req_)->cm.xio.cc = (cc_);												\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COUNT | MPIG_DEBUG_LEVEL_REQ,							\
		       "request - set XIO completion count: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT ", cc=%d",	\
		       (req_)->handle, (MPIG_PTR_CAST) (req_), (req_)->cm.xio.cc));					\
}

#define mpig_cm_xio_request_inc_cc(req_, was_complete_p_)									\
{																\
    *(was_complete_p_) = (((req_)->cm.xio.cc)++) ? FALSE : TRUE;								\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COUNT | MPIG_DEBUG_LEVEL_REQ,								\
		       "request - increment XIO completion count: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT ", cc=%d",	\
		       (req_)->handle, (MPIG_PTR_CAST) (req_), (req_)->cm.xio.cc));						\
}

#define mpig_cm_xio_request_dec_cc(req_, is_complete_p_)									\
{																\
    *(is_complete_p_) = (--((req_)->cm.xio.cc)) ? FALSE : TRUE;									\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COUNT | MPIG_DEBUG_LEVEL_REQ,								\
		       "request - decrement XIO completion count: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT ", cc=%d",	\
		       (req_)->handle, (MPIG_PTR_CAST) (req_), (req_)->cm.xio.cc));						\
}

#define mpig_cm_xio_request_set_state(req_, state_)										\
{																\
    (req_)->cm.xio.state = (state_);												\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_REQ,											\
		       "request - set XIO state: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT ", state=%s",			\
		       (req_)->handle, (MPIG_PTR_CAST) (req_), mpig_cm_xio_request_state_get_string((req_)->cm.xio.state)));	\
}

#define mpig_cm_xio_request_get_state(req_) ((req_)->cm.xio.state)

#define mpig_cm_xio_request_set_msg_type(req_, msg_type_)									\
{																\
    (req_)->cm.xio.msg_type = (msg_type_);											\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_REQ,											\
		       "request - set XIO message type: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT ", type=%s",		\
		       (req_)->handle, (MPIG_PTR_CAST) (req_), mpig_cm_xio_msg_type_get_string((req_)->cm.xio.msg_type)));	\
}

#define mpig_cm_xio_request_get_msg_type(req_) ((req_)->cm.xio.msg_type)

#define mpig_cm_xio_request_set_sreq_type(req_, sreq_type_)									\
{																\
    (req_)->cm.xio.sreq_type = (sreq_type_);											\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_REQ,											\
		       "request - set XIO request sreq type: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT ", type=%s",		\
		       (req_)->handle, (MPIG_PTR_CAST) (req_), mpig_request_type_get_string((req_)->cm.xio.sreq_type)));	\
}

#define mpig_cm_xio_request_get_sreq_type(req_) ((req_)->cm.xio.sreq_type)

#else /* defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS) */


/*
 * void mpig_cm_xio_request_destruct_fn([IN/MOD] req)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_request_destruct_fn
MPIG_STATIC void mpig_cm_xio_request_destruct_fn(MPID_Request * const req)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);

    MPIG_UNUSED_VAR(fcname);

    mpig_cm_xio_request_destruct(req);
}
/* mpig_cm_xio_request_destruct_fn() */


/*
 * char * mpig_cm_xio_request_state_get_string([IN] req)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_request_state_get_string
MPIG_STATIC const char * mpig_cm_xio_request_state_get_string(mpig_cm_xio_req_states_t req_state)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const char * str;

    MPIG_UNUSED_VAR(fcname);

    switch(req_state)
    {
	case MPIG_CM_XIO_REQ_STATE_UNDEFINED:
	    str ="MPIG_CM_XIO_REQ_STATE_UNDEFINED";
	    break;
	case MPIG_CM_XIO_REQ_STATE_INACTIVE:
	    str ="MPIG_CM_XIO_REQ_STATE_INACTIVE";
	    break;
	case MPIG_CM_XIO_REQ_STATE_SEND_DATA:
	    str ="MPIG_CM_XIO_REQ_STATE_SEND_DATA";
	    break;
	case MPIG_CM_XIO_REQ_STATE_SEND_RNDV_RTS:
	    str ="MPIG_CM_XIO_REQ_STATE_SEND_RNDV_RTS";
	    break;
	case MPIG_CM_XIO_REQ_STATE_SEND_RNDV_RTS_RECVD_CTS:
	    str ="MPIG_CM_XIO_REQ_STATE_SEND_RTS_RECVD_CTS";
	    break;
	case MPIG_CM_XIO_REQ_STATE_WAIT_RNDV_CTS:
	    str ="MPIG_CM_XIO_REQ_WAIT_RNDV_CTS";
	    break;
	case MPIG_CM_XIO_REQ_STATE_SEND_CTRL_MSG:
	    str ="MPIG_CM_XIO_REQ_STATE_CTRL_SEND_MSG";
	    break;
	case MPIG_CM_XIO_REQ_STATE_SEND_COMPLETE:
	    str ="MPIG_CM_XIO_REQ_STATE_SEND_COMPLETE";
	    break;
	case MPIG_CM_XIO_REQ_STATE_RECV_RREQ_POSTED:
	    str ="MPIG_CM_XIO_REQ_STATE_RECV_RREQ_POSTED";
	    break;
	case MPIG_CM_XIO_REQ_STATE_RECV_POSTED_DATA:
	    str ="MPIG_CM_XIO_REQ_STATE_RECV_POSTED_DATA";
	    break;
	case MPIG_CM_XIO_REQ_STATE_RECV_UNEXP_DATA:
	    str ="MPIG_CM_XIO_REQ_STATE_RECV_UNEXP_DATA";
	    break;
	case MPIG_CM_XIO_REQ_STATE_RECV_UNEXP_DATA_RREQ_POSTED:
	    str ="MPIG_CM_XIO_REQ_STATE_RECV_UNEXP_DATA_RREQ_POSTED";
	    break;
	case MPIG_CM_XIO_REQ_STATE_RECV_UNEXP_DATA_SREQ_CANCELLED:
	    str ="MPIG_CM_XIO_REQ_STATE_RECV_UNEXP_SREQ_CANCELLED";
	    break;
	case MPIG_CM_XIO_REQ_STATE_WAIT_RNDV_DATA:
	    str ="MPIG_CM_XIO_REQ_STATE_RNDV_WAIT_DATA";
	    break;
	case MPIG_CM_XIO_REQ_STATE_RECV_COMPLETE:
	    str ="MPIG_CM_XIO_REQ_STATE_EAGER_RECV_COMPLETE";
	    break;
	default:
	    str = "(unrecognized request state)";
	    break;
    }

    return str;
}
/* mpig_cm_xio_request_state_get_string() */

#endif /* MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS */
/**********************************************************************************************************************************
						  END REQUEST OBJECT MANAGEMENT
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						 BEGIN REQUEST COMPLETION QUEUE
**********************************************************************************************************************************/
#if !defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS)

MPIG_STATIC MPID_Request * mpig_cm_xio_rcq_head = NULL;
MPIG_STATIC MPID_Request * mpig_cm_xio_rcq_tail = NULL;
MPIG_STATIC globus_mutex_t mpig_cm_xio_rcq_mutex;
MPIG_STATIC globus_cond_t mpig_cm_xio_rcq_cond;
MPIG_STATIC int mpig_cm_xio_rcq_blocked = FALSE;
MPIG_STATIC int mpig_cm_xio_rcq_wakeup_pending = FALSE;

MPIG_STATIC void mpig_cm_xio_rcq_init(int * mpi_errno_p, bool_t * failed_p);

MPIG_STATIC void mpig_cm_xio_rcq_finalize(int * mpi_errno_p, bool_t * failed_p);

MPIG_STATIC void mpig_cm_xio_rcq_enq(MPID_Request * req);

MPIG_STATIC void mpig_cm_xio_rcq_deq_wait(MPID_Request ** req, int * mpi_errno_p, bool_t * failed_p);

MPIG_STATIC void mpig_cm_xio_rcq_deq_test(MPID_Request ** req, int * mpi_errno_p, bool_t * failed_p);

MPIG_STATIC void mpig_cm_xio_rcq_wakeup(void);

#else /* defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS) */

/*
 * int mpig_cm_xio_rcq_init([IN/OUT] mpi_errno, [OUT] failed)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_rcq_init
MPIG_STATIC void mpig_cm_xio_rcq_init(int * const mpi_errno_p, bool_t * const failed_p)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int rc;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_rcq_init);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_rcq_init);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS,
		       "entering: mpi_errno=0x%08x", *mpi_errno_p));
    *failed_p = FALSE;
    
    rc = globus_mutex_init(&mpig_cm_xio_rcq_mutex, NULL);
    MPIU_ERR_CHKANDJUMP1((rc != 0), *mpi_errno_p, MPI_ERR_OTHER, "**globus|mutex_init", "**globus|mutex_init %d", rc);
    
    rc = globus_cond_init(&mpig_cm_xio_rcq_cond, NULL);
    MPIU_ERR_CHKANDJUMP1((rc != 0), *mpi_errno_p, MPI_ERR_OTHER, "**globus|cond_init", "**globus|cond_init %d", rc);

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS,
		       "exiting: mpi_errno=0x%08x", *mpi_errno_p));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_rcq_init);
    return;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	*failed_p = TRUE;
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_rcq_init() */


/*
 * int mpig_cm_xio_rcq_finalize([IN/OUT] mpi_errno, [OUT] failed)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_rcq_finalize
MPIG_STATIC void mpig_cm_xio_rcq_finalize(int * mpi_errno_p, bool_t * const failed_p)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_rcq_finalize);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_rcq_finalize);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS,
		       "entering: mpi_errno=0x%08x", *mpi_errno_p));
    *failed_p = FALSE;
    
    rc = globus_mutex_destroy(&mpig_cm_xio_rcq_mutex);
    MPIU_ERR_CHKANDJUMP1((rc != 0), mpi_errno, MPI_ERR_OTHER, "**globus|mutex_destroy", "**globus|mutex_destroy %d", rc);
    
    rc = globus_cond_destroy(&mpig_cm_xio_rcq_cond);
    MPIU_ERR_CHKANDJUMP1((rc != 0), mpi_errno, MPI_ERR_OTHER, "**globus|cond_destroy", "**globus|cond_destroy %d", rc);

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS,
		       "exiting: mpi_errno=0x%08x", *mpi_errno_p));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_rcq_finalize);
    return;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	*failed_p = TRUE;
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_rcq_finalize() */


/*
 * void mpig_cm_xio_rcq_enq([IN/MOD] req)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_rcq_enq
MPIG_STATIC void mpig_cm_xio_rcq_enq(MPID_Request * const req)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_rcq_enq);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_rcq_enq);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS,
		       "entering: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT, req->handle, (MPIG_PTR_CAST) req));
    globus_mutex_lock(&mpig_cm_xio_rcq_mutex);
    {
	if (mpig_cm_xio_rcq_head == NULL)
	{
	    mpig_cm_xio_rcq_head = req;
	}
	else
	{
	    mpig_cm_xio_rcq_tail->dev.next = req;
	}
	mpig_cm_xio_rcq_tail = req;

	/* MT-RC-NOTE: on machines with release consistent memory semantics, the unlock of the RCQ mutex will force changes made
	   to the request to be committed (released), so there is no need to perform an explicit acq/rel. */
	req->dev.next = NULL;

	if (mpig_cm_xio_rcq_blocked)
	{
	    mpig_cm_xio_rcq_wakeup_pending = FALSE;
	    globus_cond_signal(&mpig_cm_xio_rcq_cond);
	}
    }
    globus_mutex_unlock(&mpig_cm_xio_rcq_mutex);

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PROGRESS | MPIG_DEBUG_LEVEL_REQ,
		       "req enqueued on the completion queue: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT,
		       req->handle, (MPIG_PTR_CAST) req));
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS,
		       "exiting: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT, req->handle, (MPIG_PTR_CAST) req));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_rcq_enq);
    return;
}
/* mpig_cm_xio_rcq_enq() */


/*
 * void mpig_cm_xio_rcq_deq_wait([OUT] reqp, [IN/OUT] mpi_errno, [OUT] failed)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_rcq_deq_wait
MPIG_STATIC void mpig_cm_xio_rcq_deq_wait(MPID_Request ** const reqp, int * const mpi_errno_p, bool_t * const failed_p)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPID_Request * req = NULL;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_rcq_deq_wait);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_rcq_deq_wait);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS,
		       "entering: mpi_errno=0x%08x", *mpi_errno_p));
    *failed_p = FALSE;

    globus_poll_nonblocking();
    
    globus_mutex_lock(&mpig_cm_xio_rcq_mutex);
    {
	while (mpig_cm_xio_rcq_head == NULL && !mpig_cm_xio_rcq_wakeup_pending)
	{
	    /* XXX: check for registered asynchronous errors.  if any, return them.  should this be done before calling poll()? */
	    
	    mpig_cm_xio_rcq_blocked = TRUE;
	    globus_cond_wait(&mpig_cm_xio_rcq_cond, &mpig_cm_xio_rcq_mutex);
	    mpig_cm_xio_rcq_blocked = FALSE;
	}
	mpig_cm_xio_rcq_wakeup_pending = FALSE;
	
	req = mpig_cm_xio_rcq_head;
	if (req != NULL)
	{
	    mpig_cm_xio_rcq_head = mpig_cm_xio_rcq_head->dev.next;
	    if (mpig_cm_xio_rcq_head == NULL)
	    {
		mpig_cm_xio_rcq_tail = NULL;
	    }
	    req->dev.next = NULL;
	    
	    /* MT-RC-NOTE: on machines with release consistent memory semantics, the unlock of the RCQ mutex will force changes
	       made to the request to be committed (released), so there is no need to perform an explicit acq/rel. */
	}
    }
    globus_mutex_unlock(&mpig_cm_xio_rcq_mutex);

    *reqp = req;
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PROGRESS | MPIG_DEBUG_LEVEL_REQ,
		       "req dequeued from the completion queue, req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT,
		       MPIG_HANDLE_VAL(*reqp), (MPIG_PTR_CAST) *reqp));
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS,
		       "exiting: reqp=" MPIG_PTR_FMT "mpi_errno=0x%08x, failed=%s",
		       (MPIG_PTR_CAST) req, *mpi_errno_p, MPIG_BOOL_STR(*failed_p)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_rcq_deq_wait);
    return;
}


/*
 * void mpig_cm_xio_rcq_deq_test([OUT] reqp, [IN/OUT] mpi_errno, [OUT] failed)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_rcq_deq_test
MPIG_STATIC void mpig_cm_xio_rcq_deq_test(MPID_Request ** reqp, int * mpi_errno_p, bool_t * const failed_p)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_rcq_deq_test);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_rcq_deq_test);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS,
		       "entering: mpi_errno=0x%08x", *mpi_errno_p));
    *failed_p = FALSE;

    globus_poll_nonblocking();
    
    globus_mutex_lock(&mpig_cm_xio_rcq_mutex);
    {
	/* XXX: check for registered asynchronous errors.  if any, return them.  should the be done before calling poll()? */
    
	*reqp = mpig_cm_xio_rcq_head;
	
	if (mpig_cm_xio_rcq_head != NULL)
	{
	    mpig_cm_xio_rcq_head = mpig_cm_xio_rcq_head->dev.next;
	    if (mpig_cm_xio_rcq_head == NULL)
	    {
		mpig_cm_xio_rcq_tail = NULL;
	    }

	    /* MT-RC-NOTE: on machines with release consistent memory semantics, the unlock of the RCQ mutex will force changes
	       made to the request to be committed (released), so there is no need to perform an explicit acq/rel. */
	    (*reqp)->dev.next = NULL;
	    
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PROGRESS | MPIG_DEBUG_LEVEL_REQ,
			       "req dequeued from the completion queue, req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT,
			       (*reqp)->handle, (MPIG_PTR_CAST) *reqp));
	}

	mpig_cm_xio_rcq_wakeup_pending = FALSE;
    }
    globus_mutex_unlock(&mpig_cm_xio_rcq_mutex);

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS,
		       "exiting: reqp=" MPIG_PTR_FMT "mpi_errno=0x%08x, failed=%s",
		       (MPIG_PTR_CAST) *reqp, *mpi_errno_p, MPIG_BOOL_STR(*failed_p)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_rcq_deq_test);
    return;
}
/* mpig_cm_xio_rcq_deq_test() */


/*
 * void mpig_cm_xio_rcq_wakeup([IN/MOD] req)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_rcq_wakeup
MPIG_STATIC void mpig_cm_xio_rcq_wakeup(void)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_rcq_wakeup);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_rcq_wakeup);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS, "entering"));
    globus_mutex_lock(&mpig_cm_xio_rcq_mutex);
    {
	mpig_cm_xio_rcq_wakeup_pending = TRUE;
	if (mpig_cm_xio_rcq_blocked)
	{
	    globus_cond_signal(&mpig_cm_xio_rcq_cond);
	}
    }
    globus_mutex_unlock(&mpig_cm_xio_rcq_mutex);

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_rcq_wakeup);
    return;
}
/* mpig_cm_xio_rcq_wakeup() */

#endif /* MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS */
/**********************************************************************************************************************************
						 END REQUEST COMPLETION QUEUE
**********************************************************************************************************************************/
