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

#define MPIG_CM_XIO_TCP_PROTO_VERSION 1

#if !defined(MPIG_CM_XIO_EAGER_MAX_SIZE)
#define MPIG_CM_XIO_EAGER_MAX_SIZE (256*1024)
#endif

/*
 * ADI3 function prototypes
 */
MPIG_STATIC int mpig_cm_xio_adi3_send(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
				      int ctxoff, MPID_Request ** sreqp);

MPIG_STATIC int mpig_cm_xio_adi3_recv(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
				      int ctxoff, MPI_Status * status, MPID_Request ** rreqp);


/*
 * ADI3 function table
 */
MPIG_STATIC mpig_cm_funcs_t mpig_cm_xio_funcs =
{
#if XXX
    mpig_cm_xio_adi3_send,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    mpig_cm_xio_adi3_recv,
    NULL
#else
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
#endif    
};


/*
 * Message handling functions
 */
MPIG_STATIC int mpig_cm_xio_handle_hdr_eager_send(mpig_vc_t * vc);
MPIG_STATIC int mpig_cm_xio_handle_hdr_open_req(mpig_vc_t * vc);
MPIG_STATIC int mpig_cm_xio_handle_hdr_open_resp(mpig_vc_t * vc);

typedef int (*mpig_cm_xio_handle_hdr_fn_t)(mpig_vc_t * vc);

typedef enum mpig_cm_xio_msg_types
{
    MPIG_CM_XIO_MSG_TYPE_FIRST = 0,
    MPIG_CM_XIO_MSG_TYPE_EAGER_SEND,
    MPIG_CM_XIO_MSG_TYPE_EAGER_RSEND,
    MPIG_CM_XIO_MSG_TYPE_EAGER_SSEND,
    MPIG_CM_XIO_MSG_TYPE_RNDV_RTS_SEND,
    MPIG_CM_XIO_MSG_TYPE_RNDV_RTS_RSEND,
    MPIG_CM_XIO_MSG_TYPE_RNDV_RTS_SSEND,
    MPIG_CM_XIO_MSG_TYPE_RNDV_CTS,
    MPIG_CM_XIO_MSG_TYPE_RNDV_DATA,
    MPIG_CM_XIO_MSG_TYPE_OPEN_REQ,
    MPIG_CM_XIO_MSG_TYPE_OPEN_RESP,
    MPIG_CM_XIO_MSG_TYPE_LAST
}
mpig_cm_xio_msg_types_t;

#if XXX
MPIG_STATIC mpig_cm_xio_handle_hdr_fn_t mpig_cm_xio_handle_hdr_funcs[] =
{
    NULL, /* MPIG_CM_XIO_MSG_TYPE_FIRST */
    mpig_cm_xio_handle_hdr_eager_send,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    mpig_cm_xio_handle_hdr_open_req,
    mpig_cm_xio_handle_hdr_open_resp,
    NULL  /* MPIG_CM_XIO_MSG_TYPE_LAST */
};
#endif


/*
 * Error handling macros
 */
#define MPIG_CM_XIO_ENQ_ERROR(mpi_errno_)				\
{									\
    /* XXX: add error to error stack, and awaken progress engine */	\
}


#define mpig_cm_xio_hdr_put_init(hdr_buf_, hdr_sz_)					\
{											\
    /* leave room for the message header size, which is always the first byte  */	\
    *(hdr_sz_) = 1;									\
}											\

#define mpig_cm_xio_hdr_get_init(hdr_buf_, hdr_pos_)				\
{										\
    /* skip over message header size, which is always in the first byte */	\
    *(hdr_pos_) = 1;								\
}										\

#define mpig_cm_xio_hdr_put_msg_size(hdr_buf_, hdr_sz_);	\
{								\
    *(hdr_buf_) = (unsigned char) *(hdr_sz_);			\
}

#define mpig_cm_xio_hdr_get_msg_size(hdr_buf_, hdr_sz_);	\
{								\
    *(hdr_sz_) = (unsigned char) hdr_buf_[0];			\
}

#define mpig_cm_xio_hdr_put_msg_type(hdr_buf_, hdr_sz_, msg_type_)	\
{									\
    (hdr_buf_)[*(hdr_sz_)] = (unsigned char) (msg_type_);		\
    *(hdr_sz_) += 1;							\
}

#define mpig_cm_xio_hdr_get_msg_type(hdr_buf_, hdr_pos_, msg_type_)	\
{									\
    *(msg_type_) = ((hdr_buf_)[*(hdr_pos_)];				\
    *(hdr_pos_) += 1;							\
}

#define mpig_cm_xio_hdr_put_envelope(hdr_buf_, hdr_sz_, envl_)			\
{										\
    /* XXX: needs data conversion */						\
    memcpy((hdr_buf_) + *(hdr_sz_), &(envl_)->rank, sizeof(mpig_rank_t));	\
    *(hdr_sz_) += sizeof(mpig_rank_t);						\
    memcpy((hdr_buf_) + *(hdr_sz_), &(envl_)->tag, sizeof(mpig_tag_t));		\
    *(hdr_sz_) += sizeof(mpig_tag_t);						\
    memcpy((hdr_buf_) + *(hdr_sz_), &(envl_)->ctx, sizeof(mpig_ctx_t));		\
    *(hdr_sz_) += sizeof(mpig_ctx_t);						\
}

#define mpig_cm_xio_hdr_get_envelope(hdr_buf_, hdr_pos_, envl_)			\
{										\
    /* XXX: needs data conversion */						\
    memcpy(&(envl_)->rank, (hdr_buf_) + *(hdr_pos_), sizeof(mpig_rank_t));	\
    *(hdr_sz_) += sizeof(mpig_rank_t);						\
    memcpy(&(envl_)->tag, (hdr_buf_) + *(hdr_pos_), sizeof(mpig_tag_t));	\
    *(hdr_sz_) += sizeof(mpig_tag_t);						\
    memcpy(&(envl_)->ctx, (hdr_buf_) + ((hdr_pos_), sizeof(mpig_ctx_t));	\
    *(hdr_sz_) += sizeof(mpig_ctx_t);						\
}

#define mpig_cm_xio_hdr_put_data_size(hdr_buf_, hdr_sz_, data_sz_);	\
{									\
    /* XXX: needs data conversion */					\
    MPIU_Assert(sizeof(data_sz_) == sizeof(MPI_Aint));			\
    memcpy((hdr_buf_) + *(hdr_sz_), &(data_sz_), sizeof(MPI_Aint));	\
    *(hdr_sz_) += sizeof(MPI_Aint);					\
}

#define mpig_cm_xio_hdr_get_data_size(hdr_buf_, hdr_pos_, data_sz_);	\
{									\
    /* XXX: needs data conversion */					\
    MPIU_Assert(sizeof(*data_sz_) == sizeof(MPI_Aint));			\
    memcpy((hdr_buf_) + *(hdr_pos_), (data_sz_), sizeof(MPI_Aint));	\
    *(hdr_pos_) += sizeof(MPI_Aint);					\
}

/*
 * Send queue routines
 *
 * NOTE: These routines are NOT thread safe.  They assume that the VC is already held.  These routines also modify the send
 * request, but it is unlikely that other routines are modifying it while it is on the queue.
 */
#define mpig_cm_xio_sendq_enq(vc_, sreq_)											\
{																\
    MPIG_DBG_PRINTF((20, FCNAME, "sreq enqueued at tail, vc=%p, sreq=0x%08x, sreqp=%p", (vc_), (sreq_)->handle, (sreq_)));	\
    (sreq_)->dev.next = NULL;													\
    if ((vc_)->cm.xio.sendq_tail != NULL)											\
    {																\
	(vc_)->cm.xio.sendq_tail->dev.next = (sreq_);										\
    }																\
    else															\
    {																\
	(vc_)->cm.xio.sendq_head = (sreq_);											\
    }																\
    (vc_)->cm.xio.sendq_tail = (sreq_);												\
}

#define mpig_cm_xio_sendq_enq_head(vc_, sreq_)											\
{																\
    MPIG_DBG_PRINTF((20, FCNAME, "sreq enqueued at head, vc=%p, sreq=0x%08x, sreqp=%p", (vc_), (sreq_)->handle, (sreq_)));	\
    (sreq)->dev.next = (vc_)->cm.xio.sendq_head;										\
    if ((vc_)->cm.xio..sendq_tail == NULL)											\
    {																\
	(vc_)->cm.xio.sendq_tail = (sreq_);											\
    }																\
    (vc_)->cm.xio.sendq_head = (sreq_);												\
}

#define mpig_cm_xio_sendq_deq(vc_)												\
{																\
    MPIG_DBG_PRINTF((20, FCNAME, "sreq dequeued, vc=%p, sreq=0x%08x, sreqp=%p", (vc_), mpig_cm_xio_sendq_head(vc_)->handle,	\
		     mpig_cm_xio_sendq_head(vc_)));										\
    (vc_)->cm.xio.sendq_head = (vc_)->cm.xio.sendq_head->dev.next;								\
    if ((vc_)->cm.xio.sendq_head == NULL)											\
    {																\
	(vc_)->cm.xio.sendq_tail = NULL;											\
    }																\
}

#define mpig_cm_xio_sendq_head(vc_) ((vc_)->cm.xio.sendq_head)

#define mpig_cm_xio_sendq_empty(vc_) ((vc_)->cm.xio.sendq_head == NULL)


/*
 * Prototypes for globus_xio handler routines and other internal routines
 */
MPIG_STATIC int mpig_cm_xio_vc_create(mpig_vc_t * vc);

MPIG_STATIC void mpig_cm_xio_handle_server_accept(globus_xio_server_t server, globus_xio_handle_t handle,
						  globus_result_t result, void * arg);

MPIG_STATIC void mpig_cm_xio_handle_server_open(globus_xio_handle_t handle, globus_result_t result, void * arg);

MPIG_STATIC void mpig_cm_xio_handle_server_msg(globus_xio_handle_t handle, globus_result_t op_grc, globus_byte_t * rbuf,
					       globus_size_t rbuf_len, globus_size_t nb_read, globus_xio_data_descriptor_t dd,
					       void * arg);
    
MPIG_STATIC void mpig_cm_xio_handle_incoming_msg(globus_xio_handle_t handle, globus_result_t op_grc, globus_byte_t * rbuf,
						 globus_size_t rbuf_len, globus_size_t nb_read, globus_xio_data_descriptor_t dd,
						 void * arg);
    
MPIG_STATIC void mpig_cm_xio_handle_close(globus_xio_handle_t handle, globus_result_t result, void * arg);

MPIG_STATIC int mpig_cm_xio_send_msg(mpig_vc_t * vc, MPID_Request * sreq);


/*
 * General module specific variables
 */
MPIG_STATIC globus_xio_stack_t mpig_cm_xio_stack;
MPIG_STATIC globus_xio_attr_t mpig_cm_xio_attrs;
MPIG_STATIC globus_xio_server_t mpig_cm_xio_server;
MPIG_STATIC char * mpig_cm_xio_server_cs = NULL;

MPIG_STATIC char mpig_cm_xio_connect_phrase[] = "MPICH2-GLOBUS-CM-XIO CONNECT\n";
MPIG_STATIC char mpig_cm_xio_accept_phrase[] = "MPICH2-GLOBUS-CM-XIO ACCEPT\n";


/*
 * mpig_cm_xio_init()
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

    fprintf(stderr, "%d: contact string: %s\n", mpig_process.my_pg_rank, mpig_cm_xio_server_cs);

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
 * mpig_cm_xio_finalize()
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

    mpig_cm_xio_vc_create(vc);
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
/* int mpig_cm_xio_select_module([IN] business card, [IN/OUT] virtual connection, [OUT] flag) */


#if XXX
/*
 * int mpig_cm_xio_adi3_send(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_adi3_send
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_STATIC int mpig_cm_xio_adi3_send(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
				      int ctxoff, MPID_Request ** sreqp)
{
    mpig_ctx_t ctx;
    MPI_Aint data_sz;
    int dt_contig;
    MPID_Datatype * dt_ptr;
    MPI_Aint dt_true_lb;
    int found;
    MPID_Request * sreq;
    mpig_vc_state_t vc_state;
    mpig_vc_t * vc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_adi3_send);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_adi3_send);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    vc = mpig_comm_get_vc(comm, rank);
    MPIU_Assert(mpig_vc_get_cm_type(vc) == MPIG_CM_TYPE_XIO);

    ctx = comm->context_id + ctxoff;
    
    /*
     * Create a new send request
     */
    sreq = mpig_recvq_deq_unexp_or_enq_posted(rank, tag, ctx, &found);
    MPIU_ERR_CHKANDJUMP1((sreq == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "send request");

    /* set MPICH fields */
    MPIU_Object_set_ref(sreq, 2);
    sreq->kind = MPID_REQUEST_SEND;
    sreq->cc_ptr = &sreq->cc;
    sreq->cc = 1;
    /* sreq->comm = NULL; preset in create */
    MPIR_Status_set_empty(&sreq->status);
	
    /* set device fields */
    mpig_request_state_init(sreq);
    mpig_request_set_buffer(sreq, buf, cnt, dt);
    mpig_request_set_envelope(sreq, comm->rank, tag, ctx);
    /* sreq->dev.dtp = NULL; preset in create */

    /*
     * Get datatype information
     */
    mpig_datatype_get_info(cnt, dt, &dt_contig, &data_sz, &dt_ptr, &dt_true_lb);

    /*
     * pack message header
     */

    if (data_sz <= MPIG_CM_XIO_EAGER_MAX_SIZE)
    {
	char * hdr_buf = sreq->cm.xio.msgbuf;
	int hdr_sz = 0;
	
	mpig_cm_xio_hdr_put_init(hdr_buf, &hdr_sz)
	mpig_cm_xio_hdr_put_msg_type(hdr_buf, &hdr_sz, MPIG_CM_XIO_MSG_TYPE_EAGER_SEND);
	mpig_cm_xio_hdr_put_envelope(hdr_buf, &hdr_sz, &sreq->dev.envl);
	mpig_cm_xio_hdr_put_data_size(hdr_buf, &hdr_sz, data_sz);
	mpig_cm_xio_hdr_put_msg_size(hdr_buf, &hdr_sz);

	sreq->cm.xio.iov[0].MPID_IOV_BUF = hdr_buf;
	sreq->cm.xio.iov[0].MPID_IOV_LEN = hdr_sz;
	sreq->cm.xio.iov_cnt = 1;
	
	if (data_sz > 0)
	{
	    if (dt_contig)
	    {
		sreq->cm.xio.iov[sreq->cm.xio.iov_cnt].MPID_IOV_BUF = (char *) buf + dt_true_lb;
		sreq->cm.xio.iov[sreq->cm.xio.iov_cnt].MPID_IOV_LEN = data_sz;
		sreq->cm.xio.iov_cnt += 1;
	    }
	    else
	    {
		/* XXX: implement send for noncontiguous datatypes */
		MPIU_Assert(dt_contig);
	    }
	}
    }
    else
    {
	/* XXX: implement rendezvous protocol */
	MPIU_Assert(data_sz <= MPIG_CM_XIO_EAGER_MAX_SIZE);
    }

    mpi_errno = mpig_cm_xio_send_msg(vc, sreq);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
    
  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_adi3_send);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* int mpig_cm_xio_adi3_send(...) */


/*
 * int mpig_cm_xio_adi3_recv(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_adi3_recv
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_STATIC int mpig_cm_xio_adi3_recv(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,
				      int ctxoff, MPI_Status * status, MPID_Request ** rreqp)
{
    int ctx = comm->context_id + ctxoff;
    MPID_Request * rreq;
    int found;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_adi3_recv);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_adi3_recv);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    MPIG_DBG_PRINTF((15, FCNAME, "receive envelope, rank=%d, tag=%d, context=%d", rank, tag, ctx));

    rreq = mpig_recvq_deq_unexp_or_enq_posted(rank, tag, comm->context_id + ctxoff, &found);
    MPIU_ERR_CHKANDJUMP1((rreq == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "receive request");
    
    /* set MPICH fields */
    MPIU_Object_set_ref(rreq, 2);
    rreq->kind = MPID_REQUEST_RECV;
    rreq->cc_ptr = &rreq->cc;
    rreq->comm = NULL;
    MPIR_Status_set_procnull(&rreq->status);
	
    rreq->comm = comm;
    MPIR_Comm_add_ref(comm);
    mpig_request_set_buffer(rreq, buf, cnt, dt);

    /* set device fields */
    mpig_request_state_init(rreq);
	
    if (found)
    {
	MPIDI_Comm_get_vc(comm, rreq->dev.match.rank, &vc);

	/* Message was found in the unexepected queue */
	MPIDI_DBG_PRINTF((15, FCNAME, "request found in unexpected queue"));

	if (MPIDI_Request_get_msg_type(rreq) == MPIDI_REQUEST_EAGER_MSG)
	{
	    int recv_pending;
	    
	    /* This is an eager message. */
	    MPIDI_DBG_PRINTF((15, FCNAME, "eager message in the request"));

	    /* If this is a eager synchronous message, then we need to send an acknowledgement back to the sender. */
	    if (MPIDI_Request_get_sync_send_flag(rreq))
	    {
		MPIDI_CH3_Pkt_t upkt;
		MPIDI_CH3_Pkt_eager_sync_ack_t * const esa_pkt = &upkt.eager_sync_ack;
		MPID_Request * esa_req;
		    
		MPIDI_DBG_PRINTF((30, FCNAME, "sending eager sync ack"));
		MPIDI_Pkt_init(esa_pkt, MPIDI_CH3_PKT_EAGER_SYNC_ACK);
		esa_pkt->sender_req_id = rreq->dev.sender_req_id;
		mpi_errno = MPIDI_CH3_iStartMsg(vc, esa_pkt, sizeof(*esa_pkt), &esa_req);
		/* --BEGIN ERROR HANDLING-- */
		if (mpi_errno != MPI_SUCCESS)
		{
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
		    goto fn_exit;
		}
		/* --END ERROR HANDLING-- */
		if (esa_req != NULL)
		{
		    MPID_Request_release(esa_req);
		}
	    }
	    
            MPIDI_Request_recv_pending(rreq, &recv_pending);
	    if (!recv_pending)
	    {
		/* All of the data has arrived, we need to unpack the data and then free the buffer and the request. */
		if (rreq->dev.recv_data_sz > 0)
		{
		    MPIDI_CH3U_Request_unpack_uebuf(rreq);
		    MPIU_Free(rreq->dev.tmpbuf);
		}
		
		mpi_errno = rreq->status.MPI_ERROR;
		if (status != MPI_STATUS_IGNORE)
		{
		    *status = rreq->status;
		}
		
		MPID_Request_release(rreq);
		rreq = NULL;
		
		goto fn_exit;
	    }
	    else
	    {
		/* The data is still being transfered across the net.  We'll leave it to the progress engine to handle once the
		   entire message has arrived. */
		if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
		{
		    MPID_Datatype_get_ptr(datatype, rreq->dev.datatype_ptr);
		    MPID_Datatype_add_ref(rreq->dev.datatype_ptr);
		}
	    }
	}
	else if (MPIDI_Request_get_msg_type(rreq) == MPIDI_REQUEST_RNDV_MSG)
	{
	    /* A rendezvous request-to-send (RTS) message has arrived.  We need to send a CTS message to the remote process. */
	    MPID_Request * cts_req;
	    MPIDI_CH3_Pkt_t upkt;
	    MPIDI_CH3_Pkt_rndv_clr_to_send_t * cts_pkt = &upkt.rndv_clr_to_send;
		
	    MPIDI_DBG_PRINTF((15, FCNAME, "rndv RTS in the request, sending rndv CTS"));
	    
	    MPIDI_Pkt_init(cts_pkt, MPIDI_CH3_PKT_RNDV_CLR_TO_SEND);
	    cts_pkt->sender_req_id = rreq->dev.sender_req_id;
	    cts_pkt->receiver_req_id = rreq->handle;
	    mpi_errno = MPIDI_CH3_iStartMsg(vc, cts_pkt, sizeof(*cts_pkt), &cts_req);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|ctspkt", 0);
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */
	    if (cts_req != NULL)
	    {
		/* FIXME: Ideally we could specify that a req not be returned.  This would avoid our having to decrement the
		   reference count on a req we don't want/need. */
		MPID_Request_release(cts_req);
	    }

	    if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
	    {
		MPID_Datatype_get_ptr(datatype, rreq->dev.datatype_ptr);
		MPID_Datatype_add_ref(rreq->dev.datatype_ptr);
	    }
	}
    }
    else
    {
	/* Message has yet to arrived.  The request has been placed on the list of posted receive requests and populated with
           information supplied in the arguments. */
	MPIG_DBG_PRINTF((15, FCNAME, "request allocated in posted queue, req=0x%08, ptr=%p", rreq->handle, rreq));
	
	rreq->cc = 1;
	if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
	{
	    MPID_Datatype_get_ptr(datatype, rreq->dev.dtp);
	    MPID_Datatype_add_ref(rreq->dev.dtp);
	}

	rreq->dev.recv_pending_count = 1;
	/* XXX: MT: mpig_request_unlock(rreq); */
    }
    
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_adi3_recv);
    return mpi_errno;

}
/* int mpig_cm_xio_adi3_recv(...) */

    
/*
 * int mpig_cm_xio_send_req([IN] vc, [IN] sreq)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_send_msg
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_STATIC int mpig_cm_xio_send_msg(mpig_vc_t * vc, MPID_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_send_msg);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_send_msg);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    
    mpig_vc_lock(vc);
    
    vc_state = mpig_vc_get_state(vc);
    if (vc_state == MPIG_VC_STATE_CONNECTED)
    {
	int q_empty = mpig_cm_xio_sendq_empty(vc);
	
	mpig_cm_xio_sendq_enq(vc, sreq);
	if (q_empty)
	{
	    mpig_cm_xio_start_send(vc);
	}
    }
    else if (vc_state == MPIG_VC_STATE_CONNECTING || vc_state == MPIG_VC_STATE_DISCONNECTING)
    {
	mpig_cm_xio_sendq_enq(vc, sreq);
    }
    else if (vc_state == MPIG_VC_STATE_UNCONNECTED)
    {
	mpig_cm_xio_sendq_enq(vc, sreq);
	mpig_cm_xio_start_connect(vc);
    }
    else /* vc is unitialized */
    {
	/* XXX: error */
	mpi_errno = MPI_ERR_INTERN;
	goto fn_fail;
    }

  fn_return:
    mpig_vc_unlock(vc);
    
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_send_msg);

  fn_fail:
    got fn_fail;
}
/* int mpig_cm_xio_send_msg([IN] vc, [IN] sreq) */
#endif


/*
 * mpig_cm_xio_vc_create()
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_vc_create
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_STATIC int mpig_cm_xio_vc_create(mpig_vc_t * vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_vc_create);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_vc_create);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    
    /* vc->conn.xio.handle = GLOBUS_HANDLE_XIO_NULL; */
    vc->cm.xio.df = -1;
    vc->cm.xio.active_sreq = NULL;
    vc->cm.xio.active_rreq = NULL;
    vc->cm.xio.sendq_head = NULL;
    vc->cm.xio.sendq_tail = NULL;

    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_vc_create);
    return mpi_errno;
}
/* mpig_cm_xio_vc_create() */

/*
 * mpig_cm_xio_handle_server_accept()
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_handle_server_accept
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_STATIC void mpig_cm_xio_handle_server_accept(globus_xio_server_t server, globus_xio_handle_t handle, globus_result_t op_grc,
						  void * arg)
{
#if XXX
    static int errcnt = 0;
    globus_result_t grc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_handle_server_accept);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_handle_server_accept);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));


    if (op_grc == GLOBUS_SUCCESS)
    {    
	/* complete the formation of the new connection */
	grc = globus_xio_register_open(handle, NULL, mpig_cm_xio_attrs, mpig_cm_xio_handle_server_open, NULL);
	if (grc == GLOBUS_SUCCESS)
	{
	    errcnt = 0;
	}
	else
	{ 
	    /* --BEGIN ERROR HANDLING-- */
	    errcnt += 1;
	
	    MPIU_ERR_SETANDSTMT1(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|xio_reg_open", "**globus|xio_reg_open %s",
				 globus_error_print_chain(globus_error_peek(grc)));
	    grc = globus_xio_register_close(handle, NULL, mpig_cm_xio_handle_close, NULL);
	    MPIU_ERR_CHKANDSTMT1((grc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_INTERN, {;}, "**globus|xio_reg_close",
				 "**globus|xio_reg_close %s", globus_error_print_chain(globus_error_peek(grc)));
	    /* --END ERROR HANDLING-- */
	}
    }
    else
    { 
	/* --BEGIN ERROR HANDLING-- */
	errcnt += 1;
	
	/* check for repeated connection attempt errors */
	MPIU_ERR_CHKANDJUMP((errcnt > 255), mpi_errno, MPI_ERR_OTHER, "**globus|xio_server_multiple_fail");
	/* --END ERROR HANDLING-- */
    }

    /* tell the server to continue listening for new connections */
    grc = globus_xio_server_register_accept(mpig_cm_xio_server, mpig_cm_xio_handle_server_accept, NULL);
    MPIU_ERR_CHKANDSTMT1((grc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_OTHER, {;}, "**globus|xio_reg_accept",
			 "**globus|xio_reg_accept %s", globus_error_print_chain(globus_error_peek(grc)));

    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_handle_server_accept);
    return;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPIG_CM_XIO_ENQ_ERROR(mpi_errno);
    goto fn_return;
    /* --END ERROR HANDLING-- */
#endif    
}
/* mpig_cm_xio_handle_server_accept() */


#if XXX
/*
 * mpig_cm_xio_handle_server_open()
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_handle_server_open
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_STATIC void mpig_cm_xio_handle_server_open(globus_xio_handle_t handle, globus_result_t result, void * arg)
{
    MPIU_CHKPMEM_DECL(1);
    mpig_vc_t * vc;
    globus_result_t grc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_handle_server_open);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_handle_server_open);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    if (op_grc != GLOBUS_SUCCESS) then goto fn_fail;
    
    /* allocate and initialize a new connection structure (VC really, just for consistency) */
    MPIU_CHKPMEM_MALLOC(vc, mpig_vc_t *, sizeof(mpig_vc_t), mpi_errno, "temp VC for incoming connection");
    mpig_vc_create(vc);
    mpig_vc_set_state(vc,MPIG_VC_STATE_TEMPORARY);
    mpig_cm_xio_vc_create(vc);
    vc->cm.xio.handle = handle;

    /* XXX: Post a receive for the connect phrase.  The connect/accept phrases are used just to verify that an incoming
       connection is a MPIG process, and not a port scanner. */

    /* post a receive for the connect message */
    grc = globus_xio_register_read(vc->cm.xio.handle, (void *) vc->cm.xio.rbuf, MPIG_CM_XIO_RECV_BUF_SIZE, 1, NULL,
				   mpig_cm_xio_handle_incoming_msg, (void *) vc);
    
    MPIU_ERR_CHKANDSTMT1((grc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_OTHER, {;}, "**globus|xio_reg_read",
			 "**globus|xio_reg_read %s", globus_error_print_chain(globus_error_peek(grc)));

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_handle_server_open);
    return;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPIU_CHKPMEM_REAP();
    grc = globus_xio_register_close(handle, NULL, mpig_cm_xio_handle_close, NULL);
    MPIU_ERR_CHKANDSTMT1((grc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_INTERN, {;}, "**globus|xio_reg_close",
			 "**globus|xio_reg_close %s", globus_error_print_chain(globus_error_peek(grc)));
    MPIG_CM_XIO_ENQ_ERROR(mpi_errno);
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_handle_server_open() */


/*
 * mpig_cm_xio_handle_msg_header()
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_handle_msg_header
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_STATIC void mpig_cm_xio_handle_msg_header(
    globus_xio_handle_t handle, globus_result_t op_grc, globus_byte_t * rbuf, globus_size_t rbuf_len, globus_size_t nb_read,
    globus_xio_data_descriptor_t dd, void * arg)
{
    mpig_vc_t * vc = (mpig_vc_t *) arg;
    struct mpig_vc_cm_xio * vc_cm = &vc->cm.xio;
    globus_result_t grc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_handle_server_msg);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_handle_server_msg);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_Assert(op_grc != GLOBUS_SUCCESS); /* XXX */

    MPIU_Assert(vc_cm->rbufp == rbuf);
    vc_cm->rbufc += nb_len;
    
    /*
     * process message(s)
     */
    while (vc_cm->rbufc > 0)
    { 
	switch(vc_cm->recv_state)
	{
	    case MPIG_CM_XIO_RECV_STATE_GET_HEADER_SIZE:
	    {
		MPIU_Assert(vc_cm->rbufc > 0);
		
		vc_cm->msg_sz = (int) *(vc_cm->rbufp++);
		vc_cm->rbufc -= 1;
	    }

	    case MPIG_CM_XIO_RECV_STATE_GET_HEADER:
	    {
		int msg_type;
		
		if (conn->rbufc < vc_cm->msg_sz)
		{
		    conn->rbufp = rbuf;
		    conn->rbufc = 0;
		    bytes_needed = conn->msg.generic.size;
		    break;
		}

		msg_type = (int) *(vc_cm->rbufp++);
		vc_cm->rbufc -= 1;

		mpig_cm_xio_msg_handlers[msg_type](vc);
		
	    }
	
	    case MPIG_CM_XIO_RECV_STATE_GET_REQ_DATA_AND_RELOAD:
	    case MPIG_CM_XIO_RECV_STATE_GET_REQ_DATA_AND_COMPLETE:
	    {
		if (conn->rbufc < 1)
		{
		    conn->rbufp = rbuf;
		    conn->rbufc = 0;
		    bytes_needed = conn->msg.generic.size;
		    break;
		}
		
		MPIU_ERR_CHKANDJUMP((conn->rbufc < conn->msg.generic.size - 2), mpi_errno, MPI_ERR_INTERN,
				    "**globus|xio_msg_hdr_trunc");

		conn->msg.generic.type = rbuf[rbufc++];
	    }
	}

	/* move an remaining data to the beginning of the buffer */
	if (vc_cm->rbufc > 0)
	{
	    memmove(vc_cm->rbuf, vc_cm->rbufp, vc_cm->rbufc);
	    vc_cm->rbufp = vc_cm->rbuf;
	}
	
	/* receive the process group id and rank of the connecting process */
    grc = globus_xio_register_read(conn.handle, (void *) conn.rbufp, 2 * sizeof(uint32_t), 2 * sizeof(unint32_t),
				   mpig_cm_xio_handle_server_got_id, (void *) conn);
    MPIU_ERR_CHKANDSTMT1((grc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_OTHER, {;}, "**globus|xio_reg_read",
			 "**globus|xio_reg_read %s", globus_error_print_chain(globus_error_peek(grc)));

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    return;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    grc = globus_xio_register_close(handle, NULL, mpig_cm_xio_handle_close, NULL);
    MPIU_ERR_CHKANDSTMT1((grc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_INTERN, {;}, "**globus|xio_reg_close",
			 "**globus|xio_reg_close %s", globus_error_print_chain(globus_error_peek(grc)));
    MPIG_CM_XIO_ENQ_ERROR(mpi_errno);
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_handle_msg_header() */


/*
 * mpig_cm_xio_close([IN] handle, [IN] result, [IN] arg)
 *
 * This function is called by globus_xio when a registered close operation has completed.  This function may be called when a
 * handle is closed after a failed operation.  In that case, the function returns immediately.
 *
 * Parameters:
 *
 * handle - globus_xio handle that has been closed, and thus is no longer valid
 * result - globus result indicating any error
 * arg - pointer to the virtual connection (VC) object being closed; may be NULL
 */
MPIG_STATIC void mpig_cm_xio_handle_close(globus_xio_handle_t handle, globus_result_t result, void * arg)
{
    mpig_vc_t * vc = (mpig_vc_t *) arg;
    
    if (arg == NULL)
    {
	goto fn_return;
    }


    
  fn_return: ;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/*
 * mpig_cm_xio_connect(vc)
 *
 * This function assumes that the VC mutex is held when the function is called.  Furthermore, the function may only be called
 * when a connection is not established or knowingly in the process of being established.
 */
int mpig_cm_xio_connect(mpig_vc * vc)
{
    enum
    {
	FUNC_STATE_ENTER,
	FUNC_STATE_CREATE_HANDLE,
	FUNC_STATE_REG_OPEN,
	FUNC_STATE_EXIT
    }
    state = FUNC_STATE_ENTER;
    int size;
    MPIU_CHKPMEM_DECL(1);
    mpig_cm_xio_conn_t * conn;
    globus_result_t grc;
    int mpi_errno = MPI_SUCCESS;
    
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    /* allocate and initialize a new connection structure */
    MPIU_CHKPMEM_MALLOC(conn, mpig_cm_xio_conn_t *, sizeof(mpig_cm_xio_conn_t), mpi_errno, "mpig_cm_xio connection object");

    /* create an XIO handle and initiate the connection */
    state = FUNC_STATE_CREATE_HANDLE;
    grc = globus_xio_handle_create(&conn.handle, mpig_cm_xio_stack);
    MPIU_ERR_CHKANDJUMP1((grc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|xio_handle_create",
			 "**globus|xio_handle_create %s", globus_error_print_chain(globus_error_peek(grc)));

    state = FUNC_STATE_REG_OPEN;
    grc = globus_xio_register_open(conn->handle, vc->xio.cs, mpig_cm_xio_attrs, mpig_cm_xio_handle_client_open, (void *) conn);
    MPIU_ERR_CHKANDJUMP1((grc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|xio_reg_open",
			 "**globus|xio_reg_open %s", globus_error_print_chain(globus_error_peek(grc)));

    state = FUNC_STATE_EXIT;

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    return mpi_errno;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (state > STATE_CREATE_HANDLE)
    { 
	grc = globus_xio_register_close(handle, NULL, mpig_cm_xio_handle_close, NULL);
	MPIU_ERR_CHKANDSTMT1((grc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_INTERN, {;}, "**globus|xio_reg_close",
			     "**globus|xio_reg_close %s", globus_error_print_chain(globus_error_peek(grc)));
    }
    MPIU_CHKPMEM_REAP();
    MPIG_CM_XIO_ENQ_ERROR(mpi_errno);
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_connect() */
#endif
