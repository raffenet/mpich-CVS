/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "newtcp_module_impl.h"
#include "socksm.h"

//FIXME - remove this. to be defined elsewhere
#define MPIDI_CH3I_VC_STATE_DISCONNECTED 0
#define MPIDI_CH3I_VC_STATE_CONNECTED 1

typedef struct freenode {
    int index;
    struct freenode* next;
} freenode_t;

struct {
    freenode_t *head, *tail;
} freeq = {NULL, NULL};

int g_tbl_size = 0;
int g_tbl_capacity = CONN_PLFD_TBL_INIT_SIZE;
int g_tbl_grow_size = CONN_PLFD_TBL_GROW_SIZE;

sockconn_t *g_conn_tbl = NULL;
pollfd_t *g_plfd_tbl = NULL;

handler_func_t conn_state_handlers[CONN_STATE_SIZE];

void prog_sm() {
}

int alloc_conn_plfd_tbls() {
    int i, mpi_errno = MPI_SUCCESS;

    g_tbl_size = g_tbl_capacity;
    //g_nactconns = 0;    
  
    MPIU_Assert(g_conn_tbl == NULL);
    MPIU_Assert(g_plfd_tbl == NULL);

    g_conn_tbl = MPIU_Malloc(g_tbl_capacity, sizeof(sockconn_t));
    MPIU_ERR_CHKANDJUMP (g_conn_tbl == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem");
    g_plfd_tbl = MPIU_Malloc(g_tbl_capacity, sizeof(pollfd_t));
    MPIU_ERR_CHKANDJUMP (g_plfd_tbl == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem");
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

int free_conn_plfd_tbls() {
    int mpi_errno = MPI_SUCCESS;
    MPIU_Free(g_conn_tbl);
    MPIU_Free(g_plfd_tbl);
    return mpi_errno;
}

/*
 FIXME: Consult Darius for efficient memcpy and whether doing this is worth overall.

 Reason for not doing realloc for both conn and plfd tables :
   Either both the tables have to be expanded or both should remain the same size, if
   enough memory could not be allocated, as we have only one set of variables to control
   the size of the tables. Also, it is not useful to expand one table and leave the other
   at the same size, 'coz of memory allocation failures.
 */
int expand_conn_plfd_tbls() {
    int mpi_errno = MPI_SUCCESS; 
    sockconn_t *new_conn_tbl = NULL;
    pollfd_t *new_plfd_tbl = NULL;
    int new_capacity = g_tbl_capacity + g_tbl_grow_size, i;
    
    new_conn_tbl = (sockconn_t *)MPIU_Malloc(new_capacity * sizoef(sockconn_t));
    MPIU_ERR_CHKANDJUMP (new_conn_tbl == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem");
    new_plfd_tbl = (pollfd_t *)MPIU_Malloc(new_capacity * sizoef(pollfd_t));
    MPIU_ERR_CHKANDJUMP (new_plfd_tbl == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem");

    memcpy(new_conn_tbl, g_conn_tbl, g_tbl_capacity * sizeof(sockconn_t));
    memcpy(new_plfd_tbl, g_plfd_tbl, g_tbl_capacity * sizeof(pollfd_t));
    MPIU_Free(g_conn_tbl);
    MPIU_Free(g_plfd_tbl);
    g_conn_tbl = new_conn_tbl;
    g_plfd_tbl = new_plfd_tbl;
    g_tbl_capacity = new_capacity;
 fn_exit:
    return mpi_errno;
 fn_fail:
    if (new_conn_tbl) {
        free(new_conn_tbl);
        goto fn_exit;
    }
}

/*
  Finds the first free entry in the connection table/pollfd table. Note that both the
  tables are indexed the same. i.e. each entry in one table corresponds to the
  entry of the same index in the other table. If an entry is valid in one table, then
  it is valid in the other table too.

  This function finds the first free entry in both the tables by checking the queue of
  free elements. If the free queue is empty, then it returns the next available slot
  in the tables. If the size of the slot is already full, then this expands the table
  and then returns the next available slot
 */
int find_free_entry(int *index) {
    int mpi_errno = MPI_SUCCESS;

    freenode_t *node;

    if (!Q_EMPTY(freeq)) {
        Q_DEQUEUE(freeq, node);
        *index = node->index;
        MPIU_Free(node);
        goto fn_exit;
    }

    if (g_tbl_size == g_tbl_capacity) {
        mpi_errno = expand_conn_plfd_tbls();
        if (mpi_errno != MPI_SUCCESS)
            goto fn_fail;
    }
    *index = g_tbl_size;
    ++g_tbl_size;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

int init() {
  conn_state_handlers[CONN_STATE_TC_C_CNTING] = state_tc_c_cnting_handler;
  conn_state_handlers[CONN_STATE_TC_C_CNTD] = state_tc_c_cntd_handler;
  conn_state_handlers[CONN_STATE_TC_C_RANKSENT] = state_c_ranksent_handler;
  conn_state_handlers[CONN_STATE_TA_C_CNTD] = state_l_cntd_handler;
  conn_state_handlers[CONN_STATE_TA_C_RANKRCVD] = state_l_rankrcvd_handler;
  conn_state_handlers[CONN_STATE_TS_COMMRDY] = state_commrdy_handler;

  conn_state_handlers[CONN_STATE_TS_D_DCNTING] = state_d_dcnting_handler;
  conn_state_handlers[CONN_STATE_TS_D_REQSENT] = state_d_reqsent_handler;
  conn_state_handlers[CONN_STATE_TS_D_REQRCVD] = state_d_reqrcvd_handler;
  conn_state_handlers[CONN_STATE_TS_D_QUIESCENT] = state_d_quiescent_handler;
  return 0;
}

int MPID_nem_newtcp_module_connect (struct MPIDI_VC *const vc) {
    sockconn_t *conn = NULL;
    pollfd_t *plfd = NULL;
    int index = -1;
    int mpi_errno = MPI_SUCCESS;
    freenode_t *node;

    if (vc->ch.state == MPIDI_CH3I_VC_STATE_DISCONNECTED) {
        struct sockaddr_in sock_addr;
        socklen_t socklen = sizeof(SA_IN);
        int rc = 0;

        MPIU_Assert(vc->ch.sockconn == NULL);
        mpi_errno = find_free_entry(&index);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP (mpi_errno);
        
        conn = &g_conn_tbl[index];
        plfd = &g_plfd_tbl[index];

        mpi_errno = MPID_nem_newtcp_module_get_conninfo(vc, &sock_addr, &conn->pg_id, 
                                                        &conn->pg_rank); //FIXME check pg_id
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP (mpi_errno);

        CHECK_EINTR(conn->fd, socket(AF_INET, SOCK_STREAM, 0));
        MPIU_ERR_CHKANDJUMP2(conn->fd == -1, mpi_errno, MPI_ERR_OTHER, "**sock_create", 
                             "**sock_create %s %d", strerror(errno), errno);
        mpi_errno = set_sockopts(conn->fd);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP (mpi_errno);

        rc = connect(conn->fd, (SA*)addr, sizeof(SA)); 
        //connect should not be called with CHECK_EINTR macro
        MPIU_ERR_CHKANDJUMP2 (rc < 0 && errno != EINPROGRESS, mpi_errno, MPI_ERR_OTHER,
                              "**sock_connect", "**sock_connect %s %d", strerror (errno),
                              errno);
        conn->state = (rc == 0) ? CONN_STATE_TC_C_CNTD : CONN_STATE_TC_C_CNTING;
        conn->vc = vc;
        conn->pending_event = 0; //clear pending events.
        conn->handler = conn_state_handlers[conn->state];
        // FIXME trace state transitions
    }
    else if (vc->ch.state == MPIDI_CH3I_VC_STATE_CONNECTED) {
        switch(conn->state) {
        case CONN_STATE_TS_D_DCNTING:
            conn->state = CONN_STATE_TS_C_COMMRDY;
            conn->pending_event = 0;
            break;
        case CONN_STATE_TS_D_REQRCVD:
            send_cmdmsg(conn->fd, MSGNAME_DISC, MSGTYPE_NAK);
            conn->state = CONN_STATE_TS_C_COMMRDY;
            conn->pending_event = 0;
            break;
        case CONN_STATE_TS_D_REQSENT:
        case CONN_STATE_TS_D_QUIESCENT:
            conn->pending_event = EVENT_CONNECT;
            break;
        default:
            break;
        }
    }
    else 
        assert(0);

 fn_exit:
    prog_sm();
    return mpi_errno;
 fn_fail:
    if (index != -1) {
        if (conn->fd != CONN_INVALID_FD) {
            close(conn->fd);
            conn->fd = CONN_INVALID_FD;
        }
        node = MPIU_Malloc(sizeof(freenode_t));
        MPIU_ERR_CHKANDSTMT1(node == NULL, mpi_errno, MPI_ERR_OTHER, goto fn_exit, 
                             "**nomem", "hole in the tables with index = %d not enqueued in"
                             "the freeq", index);
        node->index = index;
        //Note: MPIU_ERR_CHKANDJUMP should not be used here as it will be recursive 
        //within fn_fail 
        Q_ENQUEUE(freeq, node);
    }
    goto fn_exit;
}

/*
  N1: If the tcp connect negotiation is in progress, closing the socket descriptor
  will result in an error. So, it ia good to wait until the socket is connected and hence
  the disconnect event is queued here. This is the only state a disconnect event is 
  queued. This event is handled in the CONN_STATE_TC_C_CNTD state handler function.
  FIXME: Make sure the pending event is handled in the CNTD state handler function.

 */
int MPID_nem_newtcp_module_disconnect (struct MPIDI_VC *const vc) {
  CONNDB *conn = NULL;
  int mpi_errno = MPI_SUCCESS;

  //FIXME check whether a (different/new) error has to be reported stating the VC is already
  // disconnected.
  if (vc->ch.state == MPIDI_CH3I_VC_STATE_DISCONNECTED)
      goto fn_exit;
  else if (vc->ch.state == MPIDI_CH3I_VC_STATE_CONNECTED) {
      switch(conn->state) {
      case CONN_STATE_TC_C_CNTING:
          conn->pending_event = EVENT_DISCONNECT; // (N1)
          break;
      case CONN_STATE_TC_C_CNTD:
      case CONN_STATE_TC_C_RANKSENT:
      case CONN_STATE_TA_C_RANKRCVD:
          // No need to finish negotiations to move to CONN_STATE_TS_COMMRDY state.
          // Just close the connection from the current state ignoring the outstanding
          // negotiation messages.
          conn->state = CONN_STATE_TS_D_QUIESCENT;
          conn->handler = conn_state_handlers[conn->state];
          break;
      case CONN_STATE_TS_COMMRDY:
          conn->state = CONN_STATE_TS_D_DCNTING;
          conn->handler = conn_state_handlers[conn->state];
          break;
      case CONN_STATE_TS_D_DCNTING:
      case CONN_STATE_TS_D_REQSENT:
      case CONN_STATE_TS_D_REQRCVD:
      case CONN_STATE_TS_D_QUIESCENT: // already disconnecting. nothing more to do
      default:
          break;
      }
  }
  else
      MPIU_Assert(0);
 fn_exit:
  prog_sm();
  return 0;
 fn_fail:
  goto fn_exit;
}
