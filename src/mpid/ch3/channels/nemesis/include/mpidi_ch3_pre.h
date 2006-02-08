/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED)
#define MPICH_MPIDI_CH3_PRE_H_INCLUDED
#include <mpid_nem_datatypes.h>

/*#define MPID_USE_SEQUENCE_NUMBERS*/
/*#define MPIDI_CH3_CHANNEL_RNDV*/
/*#define HAVE_CH3_PRE_INIT*/
/* #define MPIDI_CH3_HAS_NO_DYNAMIC_PROCESS */
#define MPIDI_DEV_IMPLEMENTS_KVS

struct tcp_module_internal_queue;
typedef struct MPIDI_CH3I_VC
{
    int port_name_tag;
    int pg_rank;
    struct MPID_Request *recv_active;

    int is_local;
    int send_seqno;
    MPID_nem_fbox_mpich2_t *fbox_out;
    MPID_nem_fbox_mpich2_t *fbox_in;
    MPID_nem_queue_ptr_t recv_queue;
    MPID_nem_queue_ptr_t free_queue;

#if MPID_NEM_NET_MODULE == MPID_NEM_GM_MODULE
    unsigned port_id;
    unsigned node_id;
    unsigned char unique_id[6]; /* GM unique id length is 6 bytes.  GM doesn't define a constant. */
#elif MPID_NEM_NET_MODULE == MPID_NEM_TCP_MODULE
    int node_id; 
    int desc;
    struct sockaddr_in sock_id;
    int left2write;
    int left2read_head; 
    int left2read;
    int toread;
    struct tcp_module_internal_queue *internal_recv_queue;
    struct tcp_module_internal_queue *internal_free_queue;
#endif
} MPIDI_CH3I_VC;

#define MPIDI_CH3_VC_DECL MPIDI_CH3I_VC ch;

/* typedef struct MPIDI_CH3_PG */
/* { */
/*     char *kvs_name; */
/* } MPIDI_CH3_PG; */


/* #define MPIDI_CH3_PG_DECL MPIDI_CH3_PG ch; */

/*
 * MPIDI_CH3_CA_ENUM (additions to MPIDI_CA_t)
 */
#define MPIDI_CH3_CA_ENUM			\
MPIDI_CH3I_CA_END_NEMESIS_CHANNEL

/*
 * MPIDI_CH3_REQUEST_DECL (additions to MPID_Request)
 */
#define MPIDI_CH3_REQUEST_DECL			\
struct MPIDI_CH3I_Request			\
{						\
    MPIDI_VC_t *vc;				\
    int iov_offset;				\
    MPIDI_CH3_Pkt_t pkt;			\
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
    MPIDI_DBG_PRINTF((55, FCNAME, "  dev.ca = %d\n", (req)->dev.ca));		\
    MPIDI_DBG_PRINTF((55, FCNAME, "  dev.state = 0x%x\n", (req)->dev.state));	\
    MPIDI_DBG_PRINTF((55, FCNAME, "    type = %d\n",				\
		      MPIDI_Request_get_type(req)));				\
} while (0)
#else
#define DUMP_REQUEST(req) do { } while (0)
#endif

#define MPIDI_POSTED_RECV_ENQUEUE_HOOK(x) MPIDI_CH3I_Posted_recv_enqueued(x)
#define MPIDI_POSTED_RECV_DEQUEUE_HOOK(x) MPIDI_CH3I_Posted_recv_dequeued(x)

#endif /* !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED) */

