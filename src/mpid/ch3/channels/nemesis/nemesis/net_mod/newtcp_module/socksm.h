/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef SOCKSM_H
#define SOCKSM_H

#include <sys/poll.h>
#include "newtcp_module_impl.h"

enum SOCK_CONSTS {  //more type safe than #define's
    LISTENQLEN = 10,
    POLL_CONN_TIME = 2
};

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

enum CONSTS {
    CONN_PLFD_TBL_INIT_SIZE = 20,
    CONN_PLFD_TBL_GROW_SIZE = 10,
    CONN_INVALID_FD = -1,
    NEGOMSG_DATA_LEN = 4, // Length of data during negotiatiion message exchanges.
    SLEEP_INTERVAL = 1500,
    PROGSM_TIMES = 20
};

typedef enum {
    MPID_NEM_NEWTCP_MODULE_SOCK_ERROR_EOF, //either a socket error or EOF received from peer
    MPID_NEM_NEWTCP_MODULE_SOCK_CONNECTED,
    MPID_NEM_NEWTCP_MODULE_SOCK_NOEVENT // No poll event on socket
}MPID_NEM_NEWTCP_MODULE_SOCK_STATUS_t;

#define M_(x) x
#define CONN_TYPE_ M_(TYPE_CONN), M_(TYPE_ACPT)

// Note :  '_' denotes sub-states
// For example, CSO_DISCCONNECTING_DISCREQSENT, CSO_DISCONNECTING_DISCRSPRCVD are sub-states
// of CSO_DISCONNECTING
// LSO - Listening SOcket states
#define LISTEN_STATE_                           \
    M_(LISTEN_STATE_CLOSED),                    \
    M_(LISTEN_STATE_LISTENING)

/*
  CONN_STATE - Connection states of socket
  TC = Type connected (state of a socket that was issued a connect on)
  TA = Type Accepted (state of a socket returned by accept)
  TS = Type Shared (state of either TC or TA)

  C - Connection sub-states
  D - Disconnection sub-states
*/

#define CONN_STATE_                             \
    M_(CONN_STATE_TS_CLOSED),                   \
    M_(CONN_STATE_TC_C_CNTING),                 \
    M_(CONN_STATE_TC_C_CNTD),                   \
    M_(CONN_STATE_TC_C_RANKSENT),               \
    M_(CONN_STATE_TA_C_CNTD),                   \
    M_(CONN_STATE_TA_C_RANKRCVD),               \
    M_(CONN_STATE_TS_COMMRDY),                  \
    M_(CONN_STATE_TS_D_DCNTING),                \
    M_(CONN_STATE_TS_D_REQSENT),                \
    M_(CONN_STATE_TS_D_REQRCVD),                \
    M_(CONN_STATE_TS_D_QUIESCENT)

//REQ - Request, RSP - Response

typedef enum CONN_TYPE {CONN_TYPE_, CONN_TYPE_SIZE} Conn_type_t;
typedef enum MPID_nem_newtcp_module_Listen_State {LISTEN_STATE_, LISTEN_STATE_SIZE} 
    MPID_nem_newtcp_module_Listen_State_t ;

typedef enum MPID_nem_newtcp_module_Conn_State {CONN_STATE_, CONN_STATE_SIZE} 
    MPID_nem_newtcp_module_Conn_State_t;

/*
  Note: event numbering starts from 1, as 0 is assumed to be the state of all-events cleared
 */
typedef enum sockconn_event {EVENT_CONNECT = 1, EVENT_DISCONNECT} 
    sockconn_event_t;

#undef M_
#define M_(x) #x

#if defined(SOCKSM_H_DEFGLOBALS_)
const char *const CONN_TYPE_STR[CONN_TYPE_SIZE] = {CONN_TYPE_};
const char *const LISTEN_STATE_STR[LISTEN_STATE_SIZE] = {LISTEN_STATE_};
const char *const CONN_STATE_STR[CONN_STATE_SIZE] = {CONN_STATE_};
#elif defined(SOCKSM_H_EXTERNS_)
extern const char *const CONN_TYPE_STR[];
extern const char *const LISTEN_STATE_STR[];
extern const char *const CONN_STATE_STR[];
#endif
#undef M_


struct sockconn;
typedef struct sockconn sockconn_t;
typedef struct pollfd pollfd_t;

typedef int (*handler_func_t) (const pollfd_t *const plfd, sockconn_t *const conn);

struct sockconn{
    int fd;
    //enum CONN_TYPE conn_type; //FIXME: seems not used/needed
    int pg_rank;
    char *pg_id;
    MPID_nem_newtcp_module_Conn_State_t state;
    MPIDI_VC_t *vc;
    //Conn_type_t conn_type; // May be useful for debugging/analyzing purposes.
    handler_func_t handler;
    sockconn_event_t pending_event;
};

enum MSG_NAME {MSGNAME_RANK, MSGNAME_DISC};
enum MSG_TYPE {MSGTYPE_REQ, MSGTYPE_INFO, MSGTYPE_ACK, MSGTYPE_NAK};

#define MPID_nem_newtcp_module_vc_is_connected(vc) ((vc)->ch.sc && (vc)->ch.sc->state == CONN_STATE_TS_COMMRDY)

#endif
