/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED)
#define MPICH_MPIDI_CH3_PRE_H_INCLUDED

#include "mpidu_sock.h"

/*#define MPID_USE_SEQUENCE_NUMBERS*/

typedef struct MPIDI_CH3I_Process_group_s
{
    volatile int ref_count;
    char * kvs_name;
    int size;
    struct MPIDI_VC * vc_table;
}
MPIDI_CH3I_Process_group_t;

typedef struct MPIDI_CH3I_Acceptq_s
{
    struct MPIDI_VC *vc;
    struct MPIDI_CH3I_Acceptq_s *next;
}
MPIDI_CH3I_Acceptq_t;

#define MPIDI_CH3_PKT_ENUM			\
MPIDI_CH3I_PKT_SC_OPEN_REQ,			\
MPIDI_CH3I_PKT_SC_CONN_ACCEPT,		        \
MPIDI_CH3I_PKT_SC_OPEN_RESP,			\
MPIDI_CH3I_PKT_SC_CLOSE

#define MPIDI_CH3_PKT_DEFS													  \
typedef struct															  \
{																  \
    MPIDI_CH3_Pkt_type_t type;													  \
    /* FIXME - We need a little security here to avoid having a random port scan crash the process.  Perhaps a "secret" value for \
       each process could be published in the key-val space and subsequently sent in the open pkt. */				  \
																  \
    /* FIXME - We need some notion of a global process group ID so that we can tell the remote process which process is		  \
       connecting to it */													  \
    MPIDI_CH3I_Process_group_t * pg_ptr;											  \
    int pg_rank;														  \
}																  \
MPIDI_CH3I_Pkt_sc_open_req_t;													  \
                                                                                                                                  \
typedef struct															  \
{																  \
    MPIDI_CH3_Pkt_type_t type;													  \
    int ack;															  \
}																  \
MPIDI_CH3I_Pkt_sc_open_resp_t;													  \
																  \
typedef struct															  \
{																  \
    MPIDI_CH3_Pkt_type_t type;													  \
}																  \
MPIDI_CH3I_Pkt_sc_close_t;                                                                                                        \
                                                                                                                                  \
typedef MPIDI_CH3I_Pkt_sc_close_t MPIDI_CH3I_Pkt_sc_conn_accept_t;

#define MPIDI_CH3_PKT_DECL			\
MPIDI_CH3I_Pkt_sc_open_req_t sc_open_req;	\
MPIDI_CH3I_Pkt_sc_conn_accept_t sc_conn_accept;	\
MPIDI_CH3I_Pkt_sc_open_resp_t sc_open_resp;	\
MPIDI_CH3I_Pkt_sc_close_t sc_close;

typedef enum MPIDI_CH3I_VC_state
{
    MPIDI_CH3I_VC_STATE_UNCONNECTED,
    MPIDI_CH3I_VC_STATE_CONNECTING,
    MPIDI_CH3I_VC_STATE_CONNECTED,
    MPIDI_CH3I_VC_STATE_FAILED
}
MPIDI_CH3I_VC_state_t;

typedef struct MPIDI_CH3I_VC
{
    MPIDI_CH3I_Process_group_t * pg;
    int pg_rank;
    struct MPID_Request * sendq_head;
    struct MPID_Request * sendq_tail;
    MPIDI_CH3I_VC_state_t state;
    MPIDU_Sock_t sock;
    struct MPIDI_CH3I_Connection * conn;
} MPIDI_CH3I_VC;

#define MPIDI_CH3_VC_DECL MPIDI_CH3I_VC ch;


/*
 * MPIDI_CH3_CA_ENUM (additions to MPIDI_CA_t)
 */
#define MPIDI_CH3_CA_ENUM			\
MPIDI_CH3I_CA_END_SOCK_CHANNEL


/*
 * MPIDI_CH3_REQUEST_DECL (additions to MPID_Request)
 */
#define MPIDI_CH3_REQUEST_DECL									\
struct MPIDI_CH3I_Request									\
{												\
    /* iov_offset points to the current head element in the IOV */				\
    int iov_offset;										\
												\
    /*  pkt is used to temporarily store a packet header associated with this request */	\
    MPIDI_CH3_Pkt_t pkt;									\
} ch;

#define MPID_STATE_LIST_CH3 \
MPID_STATE_MPIDI_CH3_CANCEL_SEND, \
MPID_STATE_MPIDI_CH3_COMM_SPAWN, \
MPID_STATE_MPIDI_CH3_FINALIZE, \
MPID_STATE_MPIDI_CH3_INIT, \
MPID_STATE_MPIDI_CH3_IREAD, \
MPID_STATE_MPIDI_CH3_ISEND, \
MPID_STATE_MPIDI_CH3_ISENDV, \
MPID_STATE_MPIDI_CH3_ISTARTMSG, \
MPID_STATE_MPIDI_CH3_ISTARTMSGV, \
MPID_STATE_MPIDI_CH3_IWRITE, \
MPID_STATE_MPIDI_CH3_PROGRESS_INIT, \
MPID_STATE_MPIDI_CH3_PROGRESS_FINALIZE, \
MPID_STATE_MPIDI_CH3_PROGRESS_START, \
MPID_STATE_MPIDI_CH3_PROGRESS_END, \
MPID_STATE_MPIDI_CH3_PROGRESS, \
MPID_STATE_MPIDI_CH3_PROGRESS_POKE, \
MPID_STATE_MPIDI_CH3_REQUEST_CREATE, \
MPID_STATE_MPIDI_CH3_REQUEST_ADD_REF, \
MPID_STATE_MPIDI_CH3_REQUEST_RELEASE_REF, \
MPID_STATE_MPIDI_CH3_REQUEST_DESTROY, \
MPID_STATE_MPIDI_CH3I_LISTENER_GET_PORT, \
MPID_STATE_MPIDI_CH3I_PROGRESS_FINALIZE, \
MPID_STATE_MPIDI_CH3I_PROGRESS_INIT, \
MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT, \
MPID_STATE_MPIDI_CH3I_VC_POST_READ, \
MPID_STATE_MPIDI_CH3I_VC_POST_WRITE, \
MPID_STATE_MPIDI_CH3I_GET_BUSINESS_CARD, \
MPID_STATE_MPIDI_CH3U_BUFFER_COPY, \
MPID_STATE_CONNECTION_ALLOC, \
MPID_STATE_CONNECTION_FREE, \
MPID_STATE_CONNECTION_POST_SENDQ_REQ, \
MPID_STATE_CONNECTION_POST_SEND_PKT, \
MPID_STATE_CONNECTION_POST_RECV_PKT, \
MPID_STATE_CONNECTION_SEND_FAIL, \
MPID_STATE_CONNECTION_RECV_FAIL, \
MPID_STATE_UPDATE_REQUEST, \
MPID_STATE_MPIDI_COMM_SPAWN, \
SOCK_STATE_LIST

#endif /* !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED) */
