/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED)
#define MPICH_MPIDI_CH3_PRE_H_INCLUDED

/*#define MPICH_DBG_OUTPUT*/

#include "ibu.h"

typedef struct MPIDI_Process_group_s
{
    volatile int ref_count;
    char * kvs_name;
    char * pg_id;
    int rank;
    int size;
    struct MPIDI_VC * vc_table;
    struct MPIDI_Process_group_s *next;
}
MPIDI_CH3I_Process_group_t;

#define MPIDI_CH3_PKT_ENUM
#define MPIDI_CH3_PKT_DEFS
#define MPIDI_CH3_PKT_DECL

typedef enum MPIDI_CH3I_VC_state
{
    MPIDI_CH3I_VC_STATE_UNCONNECTED,
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
    ibu_t ibu;
    struct MPID_Request * send_active;
    struct MPID_Request * recv_active;
    struct MPID_Request * req;
    int reading_pkt;
    MPIDI_CH3I_VC_state_t state;
} MPIDI_CH3I_VC;

#define MPIDI_CH3_VC_DECL MPIDI_CH3I_VC ch;

/*
 * MPIDI_CH3_CA_ENUM (additions to MPIDI_CA_t)
 *
 * MPIDI_CH3I_CA_HANDLE_PKT - The completion of a packet request (send or
 * receive) needs to be handled.
 */
#define MPIDI_CH3_CA_ENUM			\
MPIDI_CH3I_CA_HANDLE_PKT,			\
MPIDI_CH3I_CA_END_IB,


/*
 * MPIDI_CH3_REQUEST_DECL (additions to MPID_Request)
 */
#define MPIDI_CH3_REQUEST_DECL						\
struct MPIDI_CH3I_Request						\
{									\
    /* iov_offset points to the current head element in the IOV */	\
    int iov_offset;							\
    									\
    /*  pkt is used to temporarily store a packet header associated	\
       with this request */						\
    MPIDI_CH3_Pkt_t pkt;						\
} ch;

#endif /* !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED) */
