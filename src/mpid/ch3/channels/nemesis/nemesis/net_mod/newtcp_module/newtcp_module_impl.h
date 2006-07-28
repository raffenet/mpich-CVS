/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef TCP_MODULE_IMPL_H
#define TCP_MODULE_IMPL_H

extern MPID_nem_queue_ptr_t MPID_nem_tcp_module_recv_queue;
extern MPID_nem_queue_ptr_t MPID_nem_tcp_module_free_queue;
extern MPID_nem_queue_ptr_t MPID_nem_process_recv_queue;
extern MPID_nem_queue_ptr_t MPID_nem_process_free_queue;   

extern int MPID_nem_tcp_module_listen_fd;
extern struct {MPIDI_VC_t *head;} MPID_nem_tcp_module_send_list;

typedef MPID_nem_newtcp_module_send_q_element
{
    struct MPID_nem_newtcp_module_send_q_element *next;
    size_t len;                        /* number of bytes left to sent */
    char *start;                       /* pointer to next byte to send */
    char buf[MPID_NEM_MAX_PACKET_LEN]; /* data to be sent */
} MPID_nem_newtcp_module_send_q_element_t;

typedef struct recv_overflow_buf
{
    char *start;
    int len;
    MPIDI_VC_t *vc;
    char buf[MPID_NEM_MAX_PACKET_LEN];
} recv_overflow_buf_t;

extern struct {MPIDI_VC_t *head, *tail;} MPID_nem_newtcp_module_free_buffers;

extern recv_overflow_buf_t MPID_nem_newtcp_module_recv_overflow_buf;

int MPID_nem_newtcp_module_send_queue (MPIDI_VC_t *vc);

/* Send queue functions */
#define Q_EMPTY(q) GENERIC_Q_EMPTY (q)
#define Q_HEAD(q) GENERIC_Q_HEAD (q)
#define Q_ENQUEUE_EMPTY(qp, ep) GENERIC_Q_ENQUEUE_EMPTY (qp, ep, next)
#define Q_ENQUEUE(qp, ep) GENERIC_Q_ENQUEUE (qp, ep, next)
#define Q_ENQUEUE_EMPTY_MULTIPLE(qp, ep0, ep1) GENERIC_Q_ENQUEUE_EMPTY_MULTIPLE (qp, ep0, ep1, next)
#define Q_ENQUEUE_MULTIPLE(qp, ep0, ep1) GENERIC_Q_ENQUEUE_MULTIPLE (qp, ep0, ep1, next)
#define Q_DEQUEUE(qp, ep) GENERIC_Q_DEQUEUE (qp, ep, next)
#define Q_REMOVE_ELEMENTS(qp, ep0, ep1) GENERIC_Q_REMOVE_ELEMENTS (qp, ep0, ep1, next)

/* VC list functions */
#define VC_L_EMPTY(q) GENERIC_L_EMPTY (q)
#define VC_L_HEAD(q) GENERIC_L_HEAD (q)
#define VC_L_ADD_EMPTY(qp, ep) GENERIC_L_ADD_EMPTY (qp, ep, ch.newtcp_sendl_next, ch.newtcp_sendl_prev)
#define VC_L_ADD(qp, ep) GENERIC_L_ADD (qp, ep, ch.newtcp_sendl_next, ch.newtcp_sendl_prev)
#define VC_L_REMOVE(qp, ep) GENERIC_L_REMOVE (qp, ep, ch.newtcp_sendl_next, ch.newtcp_sendl_prev)

#endif /* TCP_MODULE_IMPL_H */
