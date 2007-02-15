/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED)
#define MPICH_MPIDI_CH3_PRE_H_INCLUDED
#include "mpid_nem_pre.h"
#include <netinet/in.h>

/*#define MPID_USE_SEQUENCE_NUMBERS*/
/*#define MPIDI_CH3_CHANNEL_RNDV*/
/*#define HAVE_CH3_PRE_INIT*/
/* #define MPIDI_CH3_HAS_NO_DYNAMIC_PROCESS */
#define MPIDI_DEV_IMPLEMENTS_KVS

#ifndef MPID_NEM_NET_MODULE
#error MPID_NEM_NET_MODULE undefined
#endif
#ifndef MPID_NEM_DEFS_H
#error mpid_nem_defs.h must be included with this file
#endif

#if  !defined (MPID_NEM_NO_MODULE)
#error MPID_NEM_*_MODULEs are not defined!  Check for loop in include dependencies.
#endif

#if (MPID_NEM_NET_MODULE == MPID_NEM_IB_MODULE)
#include <stdint.h>
#include <infiniband/verbs.h>
#endif

#if(MPID_NEM_NET_MODULE == MPID_NEM_SCTP_MODULE)

#define MPIDI_CH3_HAS_CHANNEL_CLOSE
#define MPIDI_CH3_CHANNEL_AVOIDS_SELECT

    /* TODO make all of these _ and make all of these adjustable using env var */
#define MPICH_SCTP_NUM_STREAMS 1
#define _MPICH_SCTP_SOCKET_BUFSZ 233016  /* _ because not to confuse with env var */
    /* TODO add port and no_nagle */

    /* stream table */
#define HAVE_NOT_SENT_PG_ID 0
#define HAVE_SENT_PG_ID 1
#define HAVE_NOT_RECV_PG_ID 0
#define HAVE_RECV_PG_ID 1

typedef struct MPID_nem_sctp_stream {
    char have_sent_pg_id;
    char have_recv_pg_id;
} MPID_nem_sctp_stream_t;
#endif
    
typedef enum MPIDI_CH3I_VC_state
{
    MPIDI_CH3I_VC_STATE_UNCONNECTED,
    MPIDI_CH3I_VC_STATE_CONNECTING,
    MPIDI_CH3I_VC_STATE_CONNECTED,
    MPIDI_CH3I_VC_STATE_FAILED
}
MPIDI_CH3I_VC_state_t;

#if(MPID_NEM_NET_MODULE == MPID_NEM_NEWTCP_MODULE)
    struct MPID_nem_new_tcp_module_sockconn;
#endif

#define MPIDI_CH3_PKT_ENUM                      \
        MPIDI_NEM_PKT_LMT_RTS,                  \
        MPIDI_NEM_PKT_LMT_CTS,                  \
        MPIDI_NEM_PKT_LMT_DONE,                 \
        MPIDI_NEM_PKT_LMT_COOKIE

#define MPIDI_CH3_PKT_DEFS                      \
typedef struct MPID_nem_pkt_lmt_rts             \
{                                               \
    MPIDI_CH3_Pkt_type_t type;                  \
    MPIDI_Message_match match;                  \
    MPI_Request sender_req_id;                  \
    MPIDI_msg_sz_t data_sz;                     \
    MPIDI_msg_sz_t cookie_len;                  \
}                                               \
MPID_nem_pkt_lmt_rts_t;                         \
                                                \
typedef struct MPID_nem_pkt_lmt_cts             \
{                                               \
    MPIDI_CH3_Pkt_type_t type;                  \
    MPI_Request sender_req_id;                  \
    MPI_Request receiver_req_id;                \
    MPIDI_msg_sz_t data_sz;                     \
    MPIDI_msg_sz_t cookie_len;                  \
}                                               \
MPID_nem_pkt_lmt_cts_t;                         \
                                                \
typedef struct MPID_nem_pkt_lmt_done            \
{                                               \
    MPIDI_CH3_Pkt_type_t type;                  \
    MPI_Request req_id;                         \
}                                               \
MPID_nem_pkt_lmt_done_t;                        \
                                                \
typedef struct MPID_nem_pkt_lmt_cookie          \
{                                               \
    MPIDI_CH3_Pkt_type_t type;                  \
    MPI_Request req_id;                         \
    MPIDI_msg_sz_t cookie_len;                  \
}                                               \
MPID_nem_pkt_lmt_cookie_t;


#define MPIDI_CH3_PKT_DECL                      \
MPID_nem_pkt_lmt_rts_t lmt_rts;                 \
MPID_nem_pkt_lmt_cts_t lmt_cts;                 \
MPID_nem_pkt_lmt_done_t lmt_done;               \
MPID_nem_pkt_lmt_cookie_t lmt_cookie;


struct MPID_nem_tcp_module_internal_queue;
struct MPIDI_VC;
struct MPID_Request;
struct MPID_nem_copy_buf;
union MPIDI_CH3_Pkt;
struct MPID_nem_lmt_shm_wait_element;

typedef struct MPIDI_CH3I_VC
{
    int pg_rank;
    struct MPID_Request *recv_active;

    int is_local;
    unsigned short send_seqno;
    MPID_nem_fbox_mpich2_t *fbox_out;
    MPID_nem_fbox_mpich2_t *fbox_in;
    MPID_nem_queue_ptr_t recv_queue;
    MPID_nem_queue_ptr_t free_queue;

    int node_id;

    enum {MPID_NEM_VC_STATE_CONNECTED, MPID_NEM_VC_STATE_DISCONNECTED} state;

    /* contig function pointers.  Netmods should set these. */
    int (* iStartMsg)(struct MPIDI_VC *vc, void *hdr, MPIDI_msg_sz_t hdr_sz, struct MPID_Request **sreq_ptr);
    int (* iStartMsgv)(struct MPIDI_VC *vc, MPID_IOV *iov, int n_iov, struct MPID_Request ** sreq_ptr);
    int (* iSend)(struct MPIDI_VC *vc, struct MPID_Request *sreq, void * hdr, MPIDI_msg_sz_t hdr_sz);
    int (* iSendv)(struct MPIDI_VC *vc, struct MPID_Request *sreq, MPID_IOV *iov, int n_iov);

    /* LMT function pointers */
    int (* lmt_initiate_lmt)(struct MPIDI_VC *vc, union MPIDI_CH3_Pkt *rts_pkt, struct MPID_Request *req);
    int (* lmt_start_recv)(struct MPIDI_VC *vc, struct MPID_Request *req, MPID_IOV s_cookie);
    int (* lmt_start_send)(struct MPIDI_VC *vc, struct MPID_Request *sreq, MPID_IOV r_cookie);
    int (* lmt_handle_cookie)(struct MPIDI_VC *vc, struct MPID_Request *req, MPID_IOV cookie);
    int (* lmt_done_send)(struct MPIDI_VC *vc, struct MPID_Request *req);
    int (* lmt_done_recv)(struct MPIDI_VC *vc, struct MPID_Request *req);

    /* LMT shared memory copy-buffer ptr */
    volatile struct MPID_nem_copy_buf *lmt_copy_buf;
    char *lmt_copy_buf_handle;
    int lmt_buf_num;
    struct {struct MPID_nem_lmt_shm_wait_element *head, *tail;} lmt_queue;
    struct MPID_nem_lmt_shm_wait_element *lmt_active_lmt;
    int lmt_enqueued; /* FIXME: used for debugging */

#if(MPID_NEM_NET_MODULE == MPID_NEM_ERROR_MODULE)
#error Error in definition of MPID_NEM_*_MODULE macros
#elif (MPID_NEM_NET_MODULE == MPID_NEM_NO_MODULE)
#elif (MPID_NEM_NET_MODULE == MPID_NEM_GM_MODULE)
    unsigned gm_port_id;
    unsigned gm_node_id; 
    unsigned char gm_unique_id[6]; /* GM unique id length is 6 bytes.  GM doesn't define a constant. */
#elif (MPID_NEM_NET_MODULE == MPID_NEM_MX_MODULE)  
    unsigned int       remote_endpoint_id; /* uint32_t equivalent */
    unsigned long long remote_nic_id;      /* uint64_t equivalent */
#elif (MPID_NEM_NET_MODULE == MPID_NEM_ELAN_MODULE)
    void *rxq_ptr_array; 
    int   vpid;
#elif (MPID_NEM_NET_MODULE == MPID_NEM_TCP_MODULE)
    int desc;
    struct sockaddr_in sock_id;
    int left2write;
    int left2read_head; 
    int left2read;
    int toread;
    struct MPID_nem_tcp_module_internal_queue *internal_recv_queue;
    struct MPID_nem_tcp_module_internal_queue *internal_free_queue;
#elif (MPID_NEM_NET_MODULE == MPID_NEM_SCTP_MODULE)
    int fd;
    MPID_nem_sctp_stream_t stream_table[MPICH_SCTP_NUM_STREAMS];
    struct sockaddr_in to_address;
    void * conn_pkt;
    struct
    {
        struct MPID_nem_sctp_module_send_q_element *head;
        struct MPID_nem_sctp_module_send_q_element *tail;
    } send_queue;
    struct MPIDI_VC *sctp_sendl_next;
    struct MPIDI_VC *sctp_sendl_prev;
    
#elif (MPID_NEM_NET_MODULE == MPID_NEM_NEWTCP_MODULE)
    struct sockaddr_in sock_id;
    struct MPID_nem_new_tcp_module_sockconn *sc;
    struct
    {
        struct MPID_nem_newtcp_module_send_q_element *head;
        struct MPID_nem_newtcp_module_send_q_element *tail;
    } send_queue;
    struct MPIDI_VC *newtcp_sendl_next;
    struct MPIDI_VC *newtcp_sendl_prev;
    struct 
    {
        MPID_nem_cell_t *cell;
        char *end;
        int len;
    } pending_recv;

#elif (MPID_NEM_NET_MODULE == MPID_NEM_IB_MODULE)
    uint32_t  ud_qpn;
    uint16_t  ud_dlid;
    uint64_t  node_guid;
    struct ibv_ah *ud_ah;
    int conn_status;
    struct ibv_qp *qp;
    struct MPID_nem_ib_module_queue_t *ib_send_queue;
    struct MPID_nem_ib_module_queue_t *ib_recv_queue;
    uint32_t  avail_send_wqes;
    char   in_queue;

#else
#error One of the MPID_NEM_*_MODULE must be defined
#endif
    /* FIXME: ch3 assumes there is a field called sendq_head in the ch
       portion of the vc.  This is unused in nemesis and should be set
       to NULL */
    void *sendq_head;
} MPIDI_CH3I_VC;

#define MPIDI_CH3_VC_DECL MPIDI_CH3I_VC ch;

/* typedef struct MPIDI_CH3_PG */
/* { */
/*     char *kvs_name; */
/* } MPIDI_CH3_PG; */


/* #define MPIDI_CH3_PG_DECL MPIDI_CH3_PG ch; */

/*
 * MPIDI_CH3_REQUEST_DECL (additions to MPID_Request)
 */
#define MPIDI_CH3_REQUEST_DECL                                                                                                  \
    struct MPIDI_CH3I_Request                                                                                                   \
    {                                                                                                                           \
        MPIDI_VC_t          *vc;                                                                                                \
        int                  iov_offset;                                                                                        \
        int                  noncontig;                                                                                         \
        MPIDI_msg_sz_t       header_sz;                                                                                         \
	/*        MPIDI_CH3_Pkt_t      pkt;	*/                                                                              \
                                                                                                                                \
        MPI_Request          lmt_req_id;     /* request id of remote side */                                                    \
        struct MPID_Request *lmt_req;        /* pointer to original send/recv request */                                        \
        MPIDI_msg_sz_t       lmt_data_sz;    /* data size to be transferred, after checking for truncation */                   \
        MPID_IOV             lmt_tmp_cookie; /* temporary storage for received cookie */                                        \
    } ch;

#if 0
#define DUMP_REQUEST(req) do {							\
    int i;									\
    MPIDI_DBG_PRINTF((55, FCNAME, "request %p\n", (req)));			\
    MPIDI_DBG_PRINTF((55, FCNAME, "  handle = %d\n", (req)->handle));		\
    MPIDI_DBG_PRINTF((55, FCNAME, "  ref_count = %d\n", (req)->ref_count));	\
    MPIDI_DBG_PRINTF((55, FCNAME, "  cc = %d\n", (req)->cc));			\
    for (i = 0; i < (req)->iov_count; ++i)					\
        MPIDI_DBG_PRINTF((55, FCNAME, "  dev.iov[%d] = (%p, %d)\n", i,		\
                (req)->dev.iov[i].MPID_IOV_BUF,					\
                (req)->dev.iov[i].MPID_IOV_LEN));				\
    MPIDI_DBG_PRINTF((55, FCNAME, "  dev.iov_count = %d\n",			\
			 (req)->dev.iov_count));				\
    MPIDI_DBG_PRINTF((55, FCNAME, "  dev.state = 0x%x\n", (req)->dev.state));	\
    MPIDI_DBG_PRINTF((55, FCNAME, "    type = %d\n",				\
		      MPIDI_Request_get_type(req)));				\
} while (0)
#else
#define DUMP_REQUEST(req) do { } while (0)
#endif

#define MPIDI_POSTED_RECV_ENQUEUE_HOOK(x) MPIDI_CH3I_Posted_recv_enqueued(x)
#define MPIDI_POSTED_RECV_DEQUEUE_HOOK(x) MPIDI_CH3I_Posted_recv_dequeued(x)

typedef struct MPIDI_CH3I_comm
{
    int local_size;      /* number of local procs in this comm */
    int local_rank;      /* my rank among local procs in this comm */
    int *local_ranks;    /* list of ranks of procs local to this node */
    int external_size;   /* number of procs in external set */
    int external_rank;   /* my rank among external set, or -1 if I'm not in external set */
    int *external_ranks; /* list of ranks of procs in external set */
    struct MPID_nem_barrier_vars *barrier_vars; /* shared memory variables used in barrier */
}
MPIDI_CH3I_comm_t;

#ifdef ENABLED_SHM_COLLECTIVES
#define HAVE_DEV_COMM_HOOK
#define MPID_Dev_comm_create_hook(comm_) do {           \
        int _mpi_errno;                                 \
        _mpi_errno = MPIDI_CH3I_comm_create (comm_);    \
        if (_mpi_errno) MPIU_ERR_POP (_mpi_errno);      \
    } while(0)

#define MPID_Dev_comm_destroy_hook(comm_) do {          \
        int _mpi_errno;                                 \
        _mpi_errno = MPIDI_CH3I_comm_destroy (comm_);   \
        if (_mpi_errno) MPIU_ERR_POP (_mpi_errno);      \
    } while(0)

#endif
#define MPID_DEV_COMM_DECL MPIDI_CH3I_comm_t ch;

/*
 * MPID_Progress_state - device/channel dependent state to be passed between 
 * MPID_Progress_{start,wait,end}
 *
 */
typedef struct MPIDI_CH3I_Progress_state
{
    int completion_count;
}
MPIDI_CH3I_Progress_state;

#define MPIDI_CH3_PROGRESS_STATE_DECL MPIDI_CH3I_Progress_state ch;

#endif /* !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED) */

