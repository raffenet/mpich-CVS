/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#if !defined(MPIG_CM_XIO_H_INCLUDED)
#define MPIG_CM_XIO_H_INCLUDED

#include "globus_xio.h"

#define MPIG_CM_PROTO_VERSION 1
#define MPIG_CM_XIO_SEND_BUF_SIZE 256
#define MPIG_CM_XIO_RECV_BUF_SIZE 256

/*
 * Add communication module types to be included in the enumeration of modules
 */
#define MPIG_CM_TYPE_XIO_LIST			\
    ,MPIG_CM_TYPE_XIO


/*
 * Define the communication module structure to be included in a VC
 */
#define MPIG_VC_CM_XIO_DECL						\
struct mpig_vc_cm_xio							\
{									\
    /* Contact string for remote process listener */			\
    char * cs;								\
									\
    /* Data format of remote machine */					\
    int df;								\
									\
    /* Handle to the XIO connection */					\
    globus_xio_handle_t handle;						\
									\
    /* Active send and receive requests */				\
    struct MPID_Request * active_sreq;					\
    struct MPID_Request * active_rreq;					\
									\
    /* Send queue */							\
    struct MPID_Request * sendq_head;					\
    struct MPID_Request * sendq_tail;					\
									\
    /* Receive State */							\
    mpig_cm_xio_recv_state_t recv_state;				\
    int msg_hdr_size;							\
    									\
    /* Internal receive buffer for small headers and messages */	\
    unsigned char rbuf[MPIG_CM_XIO_RECV_BUF_SIZE];			\
    char * rbufp;							\
    int rbufc;								\
}									\
xio;


/*
 * Define the communication module structure to be included in a request
 */
#define MPIG_REQUEST_CM_XIO_DECL												\
struct mpig_request_cm_xio													\
{																\
    /* Type of message being sent or received */										\
    mpig_cm_xio_msg_types_t msg_type;												\
																\
    /* Information about the user buffer being sent/received.  The segment object is used to help process buffers that are	\
       noncontiguous or data that is heterogeneous. */										\
    int buf_contig;														\
    MPI_Aint buf_pos;														\
    MPI_Aint buf_size;														\
    MPID_Segment seg;														\
																\
    /* The I/O vector define the data to be transferred/received */								\
    MPID_IOV iov[MPID_IOV_LIMIT];												\
    int iov_cnt;														\
    int iov_bytes;														\
																\
    /* This completion counter is used to identify when all of the data has been received.  It is indepent of the request	\
       completion counter. */													\
    MPI_Aint recv_data_cc;													\
																\
    /* This field is the size of the data received, or to be received. */							\
    MPI_Aint recv_data_size;													\
																\
    /* Space for a message header, and perhaps a small amount of data */							\
    unsigned char msgbuf[MPIG_CM_XIO_SEND_BUF_SIZE];										\
																\
    /* Temporary data storage used for things like unexpected eager messages and packing/unpacking buffers. */			\
    struct mpig_databuf * databuf;												\
}																\
xio;


/*
 * Types of messages that can be sent or received
 */
typedef enum mpig_cm_xio_msg_types
{
    MPIG_CM_XIO_MSG_TYPE_FIRST = 0,
    MPIG_CM_XIO_MSG_TYPE_EAGER_SEND,
    MPIG_CM_XIO_MSG_TYPE_RNDV_RTS_SEND,
    MPIG_CM_XIO_MSG_TYPE_RNDV_CTS,
    MPIG_CM_XIO_MSG_TYPE_RNDV_DATA,
    MPIG_CM_XIO_MSG_TYPE_OPEN_REQ,
    MPIG_CM_XIO_MSG_TYPE_OPEN_RESP,
    MPIG_CM_XIO_MSG_TYPE_LAST
}
mpig_cm_xio_msg_types_t;


/*
 * Message reception state
 */
typedef enum mpig_cm_xio_recv_state
{
    MPIG_CM_XIO_RECV_STATE_GET_HEADER_SIZE = 0,
    MPIG_CM_XIO_RECV_STATE_GET_HEADER = 2,
    MPIG_CM_XIO_RECV_STATE_GET_REQ_DATA_AND_RELOAD_IOV = 3,
    MPIG_CM_XIO_RECV_STATE_GET_REQ_DATA_AND_COMPLETE = 4
}
mpig_cm_xio_recv_state_t;


/*
 * Global funciton prototypes
 */
int mpig_cm_xio_init(int * argc, char *** argv);

int mpig_cm_xio_finalize(void);

int mpig_cm_xio_add_contact_info(struct mpig_bc * bc);

int mpig_cm_xio_select_module(struct mpig_bc * bc, struct mpig_vc * vc, int * flag);


#endif /* !defined(MPICH2_MPIG_CM_XIO_H_INCLUDED) */
