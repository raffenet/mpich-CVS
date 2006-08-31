/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef NEWTCP_MODULE_IMPL_H
#define NEWTCP_MODULE_IMPL_H

#include "mpid_nem_impl.h"
#include "newtcp_module.h"
#include <linux/types.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include "newtcp_module_queue.h"
#include "socksm.h"

/* typedefs */
typedef struct MPID_nem_newtcp_module_send_q_element
{
    struct MPID_nem_newtcp_module_send_q_element *next;
    size_t len;                        /* number of bytes left to sent */
    char *start;                       /* pointer to next byte to send */
    char buf[MPID_NEM_MAX_PACKET_LEN]; /* data to be sent */
} MPID_nem_newtcp_module_send_q_element_t;


/* globals */
extern MPID_nem_queue_ptr_t MPID_nem_newtcp_module_free_queue;
extern MPID_nem_queue_ptr_t MPID_nem_process_recv_queue;
extern MPID_nem_queue_ptr_t MPID_nem_process_free_queue;
extern int MPID_nem_newtcp_module_listen_fd;


/* functions */
int MPID_nem_newtcp_module_send_queue (MPIDI_VC_t *vc);
int MPID_nem_newtcp_module_send_init (void);
int MPID_nem_newtcp_module_poll_init (void);
int MPID_nem_newtcp_module_connect (struct MPIDI_VC *const vc);
int MPID_nem_newtcp_module_connection_progress (MPIDI_VC_t *vc);
int MPID_nem_newtcp_module_connpoll (void);
int MPID_nem_newtcp_module_init_sm (void);
int MPID_nem_newtcp_module_set_sockopts (int fd);
MPID_NEM_NEWTCP_MODULE_SOCK_STATUS_t MPID_nem_newtcp_module_check_sock_status(const pollfd_t *const plfd);
int MPID_nem_newtcp_module_send_progress (void);
int MPID_nem_newtcp_module_poll_finalize (void);
int MPID_nem_newtcp_module_send_finalize (void);
int MPID_nem_newtcp_module_bind (int sockfd);
int MPID_nem_newtcp_module_recv_handler (struct pollfd *pfd, sockconn_t *sc);
int MPID_nem_newtcp_module_conn_est (struct pollfd *pfd, sockconn_t *sc);
int MPID_nem_newtcp_module_get_conninfo (struct MPIDI_VC *vc, struct sockaddr_in *addr, char **pg_id, int *pg_rank);
int MPID_nem_newtcp_module_get_vc_from_conninfo (char *pg_id, int pg_rank, struct MPIDI_VC **vc);
int MPID_nem_newtcp_module_is_sock_connected(int fd);
int MPID_nem_newtcp_module_disconnect (struct MPIDI_VC *const vc);
int MPID_nem_newtcp_module_state_listening_handler(pollfd_t *const l_plfd, sockconn_t *const l_sc);

/* Macros */

/* system call wrapper -- This retries the syscall each time it is interrupted.  
   Example usage:  instead of writing "ret = write(fd, buf, len);" 
   use: "CHECK_EINTR(ret, write(fd, buf, len)); 
 Caution:
 (1) Some of the system calls have value-result parameters. Those system calls
 should not be used within CHECK_EINTR macro or should be used with CARE.
 For eg. accept, the last parameter (addrlen) is a value-result one. So, even if the
 system call is interrupted, addrlen should be initialized to appropriate value before
 calling it again.

 (2) connect should not be called within a loop. In case, the connect is interrupted after
 the TCP handshake is initiated, calling connect again will only fail. So, select/poll
 should be called to check the status of the socket.
 I don't know what will happen, if a connect is interrupted even before the system call
 tries to initiate TCP handshake. No book/manual doesn't seem to explain this scenario.
*/
#define CHECK_EINTR(var, func) do {             \
        (var) = (func);                         \
    } while ((var) == -1 && errno == EINTR)

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
#define VC_L_ADD_EMPTY(qp, ep) GENERIC_L_ADD_EMPTY (qp, ep, ch.newtcp_sendl_next, ch.newtcp_sendl_prev)
#define VC_L_ADD(qp, ep) GENERIC_L_ADD (qp, ep, ch.newtcp_sendl_next, ch.newtcp_sendl_prev)
#define VC_L_REMOVE(qp, ep) GENERIC_L_REMOVE (qp, ep, ch.newtcp_sendl_next, ch.newtcp_sendl_prev)

/* stack macros */
#define S_EMPTY(s) GENERIC_S_EMPTY (s)
#define S_TOP(s) GENERIC_S_TOP (s)
#define S_PUSH(sp, ep) GENERIC_S_PUSH (sp, ep, next)
#define S_PUSH_MULTIPLE(sp, ep0, ep1) GENERIC_S_PUSH_MULTIPLE (sp, ep0, ep1, next)
#define S_POP(sp, ep) GENERIC_S_POP (sp, ep, next)

#endif /* NEWTCP_MODULE_IMPL_H */
