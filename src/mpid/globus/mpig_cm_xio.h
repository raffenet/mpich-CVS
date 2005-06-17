/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#if !defined(MPICH2_MPIG_CM_XIO_H_INCLUDED)
#define MPICH2_MPIG_CM_XIO_H_INCLUDED

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
    volatile struct MPID_Request * active_sreq;				\
    volatile struct MPID_Request * active_rreq;				\
									\
    /* Send queue */							\
    volatile struct MPID_Request * sendq_head;				\
    volatile struct MPID_Request * sendq_tail;				\
									\
    /* Receive State */							\
    mpig_cm_xio_recv_state_t recv_state;				\
    int msg_hdr_sz;							\
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
#define MPIG_REQUEST_CM_XIO_DECL												  \
struct mpig_request_cm_xio													  \
{																  \
    /* Size of the data received, or to be received. */										  \
    MPI_Aint recv_data_sz;													  \
																  \
    /* segment, segment_first, and segment_size are used when processing non-contiguous datatypes */				  \
    MPID_Segment seg;														  \
    MPI_Aint seg_first;														  \
    MPI_Aint seg_sz;														  \
																  \
    /* iov and iov_count define the data to be transferred/received */								  \
    MPID_IOV iov[MPID_IOV_LIMIT];												  \
    int iov_cnt;														  \
																  \
    /* The CA (completion action) field identifies the action to take once the operation described by the iov has completed. */	  \
    /* XXX: MPIDI_CA_t ca; */													  \
																  \
    /* Space for a message header, and perhaps a small amount of data */							  \
    unsigned char msgbuf[MPIG_CM_XIO_SEND_BUF_SIZE];										  \
																  \
    /* Temporary data storage used for things like unexpected eager messages and packing/unpacking buffers. */			  \
    struct mpig_databuf * databuf;												  \
																  \
    /* Extra data to receive resulting from a truncated or canceled message */							  \
    MPI_Aint recv_pending_count;												  \
}																  \
xio;


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
