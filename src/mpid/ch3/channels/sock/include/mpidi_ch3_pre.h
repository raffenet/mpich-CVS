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
    char * pg_id;
    int size;
    struct MPIDI_VC * vc_table;
    struct MPIDI_CH3I_Process_group_s *next; /* we keep track of all pg's in a list */
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
    int pg_id_len;											                          \
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

#endif /* !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED) */
