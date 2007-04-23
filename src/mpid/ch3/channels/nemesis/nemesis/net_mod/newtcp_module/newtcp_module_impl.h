/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef NEWTCP_MODULE_IMPL_H
#define NEWTCP_MODULE_IMPL_H

#include "mpid_nem_impl.h"
#include "newtcp_module.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include "socksm.h"

/* globals */
extern MPID_nem_queue_ptr_t MPID_nem_newtcp_module_free_queue;
extern MPID_nem_queue_ptr_t MPID_nem_process_recv_queue;
extern MPID_nem_queue_ptr_t MPID_nem_process_free_queue;
extern int MPID_nem_newtcp_module_listen_fd;
extern int MPID_nem_newtcp_module_main_to_comm_fd;
extern int MPID_nem_newtcp_module_called_finalize;

extern pthread_t MPID_nem_newtcp_module_comm_thread_handle;

#define MPID_NEM_NEWTCP_MODULE_VC_STATE_DISCONNECTED 0
#define MPID_NEM_NEWTCP_MODULE_VC_STATE_CONNECTING 1
#define MPID_NEM_NEWTCP_MODULE_VC_STATE_CONNECTED 2

/*S
  MPIDU_Sock_ifaddr_t - Structure to hold an Internet address.

+ len - Length of the address.  4 for IPv4, 16 for IPv6.
- ifaddr - Address bytes (as bytes, not characters)

S*/
typedef struct MPIDU_Sock_ifaddr_t {
    int len, type;
    unsigned char ifaddr[16];
} MPIDU_Sock_ifaddr_t;

#define MPIDI_CH3I_PORT_KEY "port"
#define MPIDI_CH3I_ADDR_KEY "addr"
#define MPIDI_CH3I_IFNAME_KEY "ifname"

#define MPID_nem_newtcp_queue_fields1           \
    MPID_Request * volatile head;               \
    MPID_Request * volatile tail

#define MPID_nem_newtcp_queue_fields2           \
    MPID_Request *my_head

struct MPID_nem_newtcp_queue_fields1_tmp {MPID_nem_newtcp_queue_fields1;};
struct MPID_nem_newtcp_queue_fields2_tmp {MPID_nem_newtcp_queue_fields2;};

typedef struct MPID_nem_newtcp_send_queue
{
    MPID_nem_newtcp_queue_fields1;
    char padding1[MPID_NEM_CACHE_LINE_LEN - sizeof(struct MPID_nem_newtcp_queue_fields1_tmp)];
    MPID_nem_newtcp_queue_fields2;
    char padding2[MPID_NEM_CACHE_LINE_LEN - sizeof(struct MPID_nem_newtcp_queue_fields2_tmp)];
} MPID_nem_newtcp_send_queue_t;

/* The vc provides a generic buffer in which network modules can store
   private fields This removes all dependencies from the VC struction
   on the network module, facilitating dynamic module loading. */
typedef struct {
    struct sockaddr_in sock_id;
    struct MPID_nem_new_tcp_module_sockconn *sc;
    MPID_nem_newtcp_send_queue_t *send_queue;
} MPID_nem_newtcp_module_vc_area;
/* accessor macro to private fields in VC */
#define VC_FIELD(vc, field) (((MPID_nem_newtcp_module_vc_area *)((MPIDI_CH3I_VC *)(vc)->channel_private)->netmod_area.padding)->field)

typedef enum {
    MPID_NEM_NEWTCP_MODULE_REFRESH_STATE,
    MPID_NEM_NEWTCP_MODULE_CONNECT,
    MPID_NEM_NEWTCP_MODULE_FINALIZE,
    MPID_NEM_NEWTCP_MODULE_DISCONNECT
} MPID_nem_newtcp_module_poke_msg_type_t;

typedef struct {
    MPID_nem_newtcp_module_poke_msg_type_t type;
    MPIDI_VC_t *vc;
} MPID_nem_newtcp_module_poke_msg_t;



/* functions */
int MPID_nem_newtcp_module_send_init(void);
int MPID_nem_newtcp_module_send_queued(MPIDI_VC_t * const vc);
int MPID_nem_newtcp_module_poll_init(void);
int MPID_nem_newtcp_module_connect(MPIDI_VC_t * const vc);
int MPID_nem_newtcp_module_connection_progress(MPIDI_VC_t * const vc);
int MPID_nem_newtcp_module_connpoll(void);
int MPID_nem_newtcp_module_init_sm(void);
int MPID_nem_newtcp_module_set_sockopts(int fd);
MPID_NEM_NEWTCP_MODULE_SOCK_STATUS_t MPID_nem_newtcp_module_check_sock_status(const pollfd_t *const plfd);
int MPID_nem_newtcp_module_poll_finalize(void);
int MPID_nem_newtcp_module_send_finalize(void);
int MPID_nem_newtcp_module_bind(int sockfd);
int MPID_nem_newtcp_module_recv_handler(struct pollfd *pfd, sockconn_t *sc);
int MPID_nem_newtcp_module_conn_est(MPIDI_VC_t * const vc);
int MPID_nem_newtcp_module_get_conninfo(MPIDI_VC_t *vc, struct sockaddr_in *addr, char **pg_id, int *pg_rank);
int MPID_nem_newtcp_module_get_vc_from_conninfo(char *pg_id, int pg_rank, struct MPIDI_VC **vc);
int MPID_nem_newtcp_module_is_sock_connected(int fd);
int MPID_nem_newtcp_module_disconnect(MPIDI_VC_t * const vc);
int MPID_nem_newtcp_module_state_listening_handler(pollfd_t * const l_plfd, sockconn_t * const l_sc);
int MPID_nem_newtcp_module_state_poke_handler(pollfd_t * const l_plfd, sockconn_t * const l_sc);

int MPID_nem_newtcp_iSendContig(MPIDI_VC_t *vc, MPID_Request *sreq, void *hdr, MPIDI_msg_sz_t hdr_sz, void *data, MPIDI_msg_sz_t data_sz);
int MPID_nem_newtcp_iStartContigMsg(MPIDI_VC_t *vc, void *hdr, MPIDI_msg_sz_t hdr_sz, void *data, MPIDI_msg_sz_t data_sz,
                                    MPID_Request **sreq_ptr);
int MPID_nem_newtcp_SendEagerNoncontig(MPIDI_VC_t *vc, MPID_Request *sreq, void *header, MPIDI_msg_sz_t hdr_sz);
int MPID_nem_newtcp_module_get_addr_port_from_bc (const char *business_card, struct in_addr *addr, in_port_t *port);
int MPID_nem_newtcp_module_get_listen_fd(void);

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

/* set or unset the writeable bit in a plfd */
#define SET_PLFD(vc) VC_FIELD(vc, sc)->g_plfd_tbl[VC_FIELD(vc, sc)->index].events |= POLLOUT
#define UNSET_PLFD(vc) VC_FIELD(vc, sc)->g_plfd_tbl[VC_FIELD(vc, sc)->index].events &= ~POLLOUT

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

/* stack macros */
#define S_EMPTY(s) GENERIC_S_EMPTY (s)
#define S_TOP(s) GENERIC_S_TOP (s)
#define S_PUSH(sp, ep) GENERIC_S_PUSH (sp, ep, next)
#define S_PUSH_MULTIPLE(sp, ep0, ep1) GENERIC_S_PUSH_MULTIPLE (sp, ep0, ep1, next)
#define S_POP(sp, ep) GENERIC_S_POP (sp, ep, next)

#endif /* NEWTCP_MODULE_IMPL_H */
