/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDPRE_H_INCLUDED)
#define MPICH_MPIDPRE_H_INCLUDED

#include <assert.h>

/* Include definitions from the channel which must exist before items in this
   file (mpidimpl.h) or the file it includes (mpiimple.h) can be defined.
   NOTE: This include requires the channel to copy mpidi_ch3_pre.h to the
   src/include directory in the build tree. */
#include "mpidi_ch3_pre.h"

#if defined(HAVE_SYS_TYPES_H)
#include <sys/types.h>
#endif
#include <sys/uio.h>	/* needed for struct iovec */
#if !defined(IOV_MAX)
#define IOV_MAX 16
#endif

#include "mpid_datatype.h"

typedef struct MPIDI_VC
{
    volatile int ref_count;
    int lpid;
    
# if defined(MPIDI_CH3_VC_DECL)
    MPIDI_CH3_VC_DECL
# endif
}
MPIDI_VC;

typedef struct MPIDI_Message_match
{
# if defined(HAVE_INT32_T) && defined(HAVE_INT16_T)    
    int32_t tag;
    int16_t rank;
    int16_t context_id;
# else
    int tag;
    int rank;
    int context_id;
# endif
}
MPIDI_Message_match;

/*
 * MPIDI_CH3_Pkt_type_t
 *
 */
typedef enum 
{
    MPIDI_CH3_PKT_EAGER_SEND = 0,
    MPIDI_CH3_PKT_RNDV_REQ_TO_SEND,
    MPIDI_CH3_PKT_RNDV_CLR_TO_SEND,
    MPIDI_CH3_PKT_RNDV_SEND,
    MPIDI_CH3_PKT_CANCEL_SEND_REQ,
    MPIDI_CH3_PKT_CANCEL_SEND_RESP,
    MPIDI_CH3_PKT_PUT,
    MPIDI_CH3_PKT_FLOW_CNTL_UPDATE,
    MPIDI_CH3_PKT_END_CH3,
# if defined(MPIDI_CH3_PKT_ENUM)
    MPIDI_CH3_PKT_ENUM
# endif    
}
MPIDI_CH3_Pkt_type_t;

typedef struct
{
    MPIDI_CH3_Pkt_type_t type;  /* XXX - uint8_t to conserve space ??? */
    MPIDI_Message_match match;
    MPI_Request sender_req_id;
    long data_sz;
}
MPIDI_CH3_Pkt_eager_send_t;

typedef struct
{
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_Message_match match;
    MPI_Request sender_req_id;
    long data_sz;
}
MPIDI_CH3_Pkt_rndv_req_to_send_t;

typedef struct
{
    MPIDI_CH3_Pkt_type_t type;
    MPI_Request sender_req_id;
    MPI_Request receiver_req_id;
}
MPIDI_CH3_Pkt_rndv_clr_to_send_t;

typedef struct
{
    MPIDI_CH3_Pkt_type_t type;
    MPI_Request receiver_req_id;
}
MPIDI_CH3_Pkt_rndv_send_t;

typedef struct
{
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_Message_match match;
    MPI_Request sender_req_id;
}
MPIDI_CH3_Pkt_cancel_send_req_t;

typedef struct
{
    MPIDI_CH3_Pkt_type_t type;
    MPI_Request sender_req_id;
    int ack;
}
MPIDI_CH3_Pkt_cancel_send_resp_t;


#if defined(MPIDI_CH3_PKT_DEFS)
MPIDI_CH3_PKT_DEFS
#endif

typedef union
{
    MPIDI_CH3_Pkt_type_t type;
    MPIDI_CH3_Pkt_eager_send_t eager_send;
    MPIDI_CH3_Pkt_rndv_req_to_send_t rndv_req_to_send;
    MPIDI_CH3_Pkt_rndv_clr_to_send_t rndv_clr_to_send;
    MPIDI_CH3_Pkt_rndv_send_t rndv_send;
    MPIDI_CH3_Pkt_cancel_send_req_t cancel_send_req;
    MPIDI_CH3_Pkt_cancel_send_resp_t cancel_send_resp;
# if defined(MPIDI_CH3_PKT_DECL)
    MPIDI_CH3_PKT_DECL
# endif
}
MPIDI_CH3_Pkt_t;

/*
 * MPIDI_CH3_CA_t
 *
 * An enumeration of the actions to perform when the requested I/O operation
 * has completed.
 *
 * MPIDI_CH3_CA_NONE - Do nothing.  Used in situations where the request will
 * be referenced later by an incoming packet.
 *
 * MPIDI_CH3_CA_RELOAD_IOV - This request contains more segments of data than
 * the IOV or buffer space allow.  Since the previously request operation has
 * completed, the IOV in the request should be reload at this time.
 *
 * MPIDI_CH3_CA_COMPLETE - The last operation for this request has completed.
 * The completion counter should be decremented.  If it has reached zero, then
 * the request should be released by calling MPID_Request_release().
 *
 * MPIDI_CH3_CA_COPY_COMPLETE - This is a special case of the
 * MPIDI_CH3_CA_COMPLETE.  The data is stored has been stored into a temporary
 * buffer and needs to be copied into the user buffer before the completion
 * counter can be decremented, etc.
 *
 * MPIDI_CH3_CA_END_CH3 - This not a real action, but rather a marker.  All
 * actions numerically less than MPID_CA_END are defined by channel device.
 * Any actions numerically greater than MPIDI_CA_END are internal to the
 * channel instance and must be handled by the channel instance.
 */
typedef enum
{
    MPIDI_CH3_CA_NONE,
    MPIDI_CH3_CA_RELOAD_IOV,
    MPIDI_CH3_CA_COMPLETE,
    MPIDI_CH3_CA_COPY_COMPLETE,
    MPIDI_CH3_CA_END_CH3,
# if defined(MPIDI_CH3_CA_ENUM)
    MPIDI_CH3_CA_ENUM
# endif
}
MPIDI_CA_t;

#define MPID_REQUEST_DECL						\
struct MPIDI_Request							\
{									\
    MPIDI_Message_match match;						\
									\
    /* TODO - user_buf, user_count, and datatype define a segment	\
       and should be replaced by an MPID_Segment once one is defined	\
       (is this true???) */						\
    void * user_buf;							\
    int user_count;							\
    MPI_Datatype datatype;						\
									\
    MPIDI_VC * vc;							\
									\
    /* iov and iov_count define the data to be transferred; iov_offset	\
       indicates the progress made in terms of an offset into iov */	\
    struct iovec iov[IOV_MAX];						\
    int iov_count;							\
    int iov_offset;							\
									\
    /* ca (completion action) identifies the action to take once the	\
       operation described by the iov has completed */			\
    MPIDI_CA_t ca;							\
									\
    /* tmp_buf and tmp_sz describe temporary storage used for things	\
       like unexpected eager messages and packing/unpacking buffers. */	\
    void * tmp_buf;							\
    long tmp_sz;							\
									\
    long recv_data_sz;							\
    MPI_Request sender_req_id;						\
									\
    unsigned state;							\
    									\
    struct MPID_Request * next;						\
} ch3;

#if defined(MPIDI_CH3_REQUEST_DECL)
#define MPID_DEV_REQUEST_DECL			\
MPID_REQUEST_DECL				\
MPIDI_CH3_REQUEST_DECL
#else
#define MPID_DEV_REQUEST_DECL			\
MPID_REQUEST_DECL
#endif

#endif /* !defined(MPICH_MPIDPRE_H_INCLUDED) */
