/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef SCTP_MODULE_IMPL_H
#define SCTP_MODULE_IMPL_H
#include "mpid_nem_impl.h"
#include "sctp_module.h"
#include "all_hash.h"
#include "sctp_module_queue.h"
/* #include <linux/types.h> */ /* not needed for SCTP */
/* #include <sys/types.h>  */ /* not needed for SCTP */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <fcntl.h>  /* new for SCTP */
#include <errno.h>
#include <netdb.h> /* needed for gethostbyname */
#include <arpa/inet.h> /* needed for inet_pton  */

#define SCTP_POLL_FREQ_MULTI 5 
#define SCTP_POLL_FREQ_ALONE 1
#define SCTP_POLL_FREQ_NO   -1
#define SCTP_END_STRING "NEM_SCTP_MOD_FINALIZE"

/* typedefs */

/*   sendq */
typedef struct MPID_nem_sctp_module_send_q_element
{
    struct MPID_nem_sctp_module_send_q_element *next;
    size_t len;                        /* number of bytes left to sent */
    char *start;                       /* pointer to next byte to send */
    int stream;
    char buf[MPID_NEM_MAX_PACKET_LEN]; /* data to be sent */
} MPID_nem_sctp_module_send_q_element_t;

/*   hash table entry */
typedef struct hash_entry
{
    sctp_assoc_t assoc_id;
    MPIDI_VC_t * vc;
} MPID_nem_sctp_hash_entry;

/* globals */

/*   queues */
extern MPID_nem_queue_ptr_t MPID_nem_sctp_module_free_queue;
extern MPID_nem_queue_ptr_t MPID_nem_process_recv_queue;
extern MPID_nem_queue_ptr_t MPID_nem_process_free_queue;

/*   ints */
extern int MPID_nem_sctp_onetomany_fd; /* needed for all communication */
extern int MPID_nem_sctp_port; /* needed for init and bizcards */

/*   hash table for association ID -> VC */
extern HASH* MPID_nem_sctp_assocID_table;


/* functions */

/* determine the stream # of a req. copied from UBC sctp channel so could be irrelevant */
/*    we want to use context so that collectives don't
 *    cause head-of-line blocking for p2p...
 */
 /* the following keeps stream s.t. 0 <= stream < MPICH_SCTP_NUM_STREAMS */
#define Req_Stream_from_match(match) (abs((match.tag) + (match.context_id))% MPICH_SCTP_NUM_STREAMS)
#define REQ_Stream(req) Req_Stream_from_match(req->dev.match)
int Req_Stream_from_pkt_and_req(MPIDI_CH3_Pkt_t * pkt, MPID_Request * sreq);


int MPID_nem_sctp_module_send_progress();
int MPID_nem_sctp_module_send_init();
int MPID_nem_sctp_module_send_finalize();


/* Send queue macros */
#define Q_EMPTY(q) GENERIC_Q_EMPTY (q)
#define Q_HEAD(q) GENERIC_Q_HEAD (q)
#define Q_ENQUEUE_EMPTY(qp, ep) GENERIC_Q_ENQUEUE_EMPTY (qp, ep, next)
#define Q_ENQUEUE(qp, ep) GENERIC_Q_ENQUEUE (qp, ep, next)
#define Q_ENQUEUE_EMPTY_MULTIPLE(qp, ep0, ep1) GENERIC_Q_ENQUEUE_EMPTY_MULTIPLE (qp, ep0, ep1, next)
#define Q_ENQUEUE_MULTIPLE(qp, ep0, ep1) GENERIC_Q_ENQUEUE_MULTIPLE (qp, ep0, ep1, next)
#define Q_DEQUEUE(qp, ep) GENERIC_Q_DEQUEUE (qp, ep, next)
#define Q_REMOVE_ELEMENTS(qp, ep0, ep1) GENERIC_Q_REMOVE_ELEMENTS (qp, ep0, ep1, next)

/* VC list macros */
#define VC_L_EMPTY(q) GENERIC_L_EMPTY (q)
#define VC_L_HEAD(q) GENERIC_L_HEAD (q)
#define VC_L_ADD_EMPTY(qp, ep) GENERIC_L_ADD_EMPTY (qp, ep, ch.sctp_sendl_next, ch.sctp_sendl_prev)
#define VC_L_ADD(qp, ep) GENERIC_L_ADD (qp, ep, ch.sctp_sendl_next, ch.sctp_sendl_prev)
#define VC_L_REMOVE(qp, ep) GENERIC_L_REMOVE (qp, ep, ch.sctp_sendl_next, ch.sctp_sendl_prev)


#endif /* SCTP_MODULE_IMPL_H */