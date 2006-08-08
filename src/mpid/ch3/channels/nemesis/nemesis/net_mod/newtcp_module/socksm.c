/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "newtcp_module_impl.h"
#include "socksm.h"


// FIXME trace/log all the state transitions

//FIXME - remove this. to be defined elsewhere
#define MPID_NEM_NEWTCP_MODULE_VC_STATE_DISCONNECTED 0
#define MPID_NEM_NEWTCP_MODULE_VC_STATE_CONNECTED 1

typedef struct freenode {
    int index;
    struct freenode* next;
} freenode_t;

struct {
    freenode_t *head, *tail;
} freeq = {NULL, NULL};

static int g_tbl_size = 0;
static int g_tbl_capacity = CONN_PLFD_TBL_INIT_SIZE;
static int g_tbl_grow_size = CONN_PLFD_TBL_GROW_SIZE;

static sockconn_t *g_conn_tbl = NULL;
static pollfd_t *g_plfd_tbl = NULL;

static handler_func_t conn_state_handlers[CONN_STATE_SIZE];

#define IS_WRITEABLE(plfd)                      \
    (plfd->revents & POLLOUT) ? 1 : 0

#define IS_READABLE(plfd)                       \
    (plfd->revents & POLLIN) ? 1 : 0

#define IS_READ_WRITEABLE(plfd)                                 \
    (plfd->revents & POLLIN && plfd->revents & POLLOUT) ? 1 : 0

#define IS_SAME_CONNECTION(iconn, conn)                \
    iconn->pg_rank == conn->pg_rank                 \
        && (strcmp(iconn->pg_id, conn->pg_id) == 0)

#undef FUNCNAME
#define FUNCNAME prog_sm
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void prog_sm() {
}

#undef FUNCNAME
#define FUNCNAME alloc_conn_plfd_tbls
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int alloc_conn_plfd_tbls()
{
    int i, mpi_errno = MPI_SUCCESS;
    MPIU_CHKPMEM_DECL (2);

    g_tbl_size = g_tbl_capacity;
    //g_nactconns = 0;    
  
    MPIU_Assert(g_conn_tbl == NULL);
    MPIU_Assert(g_plfd_tbl == NULL);

    MPIU_CHKPMEM_MALLOC (g_conn_tbl, sockconn_t *, g_tbl_capacity * sizeof(sockconn_t), 
                         mpi_errno, "connection table");
    MPIU_CHKPMEM_MALLOC (g_plfd_tbl, pollfd_t *, g_tbl_capacity * sizeof(pollfd_t), 
                         mpi_errno, "pollfd table");
    MPIU_CHKPMEM_COMMIT();
 fn_exit:
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME free_conn_plfd_tbls
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int free_conn_plfd_tbls()
{
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
#undef FUNCNAME
#define FUNCNAME expand_conn_plfd_tbls
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int expand_conn_plfd_tbls()
{
    int mpi_errno = MPI_SUCCESS; 
    sockconn_t *new_conn_tbl = NULL;
    pollfd_t *new_plfd_tbl = NULL;
    int new_capacity = g_tbl_capacity + g_tbl_grow_size, i;
    MPIU_CHKPMEM_DECL (2);

    MPIU_CHKPMEM_MALLOC (new_conn_tbl, sockconn_t *, new_capacity * sizeof(sockconn_t), 
                         mpi_errno, "expanded connection table");
    MPIU_CHKPMEM_MALLOC (new_plfd_tbl, pollfd_t *, new_capacity * sizeof(pollfd_t), 
                         mpi_errno, "expanded pollfd table");
    MPID_NEM_MEMCPY (new_conn_tbl, g_conn_tbl, g_tbl_capacity * sizeof(sockconn_t));
    MPID_NEM_MEMCPY (new_plfd_tbl, g_plfd_tbl, g_tbl_capacity * sizeof(pollfd_t));
    MPIU_Free(g_conn_tbl);
    MPIU_Free(g_plfd_tbl);
    g_conn_tbl = new_conn_tbl;
    g_plfd_tbl = new_plfd_tbl;
    g_tbl_capacity = new_capacity;
    MPIU_CHKPMEM_COMMIT();    
 fn_exit:
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
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
#undef FUNCNAME
#define FUNCNAME find_free_entry
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int find_free_entry(int *index)
{
    int mpi_errno = MPI_SUCCESS;

    freenode_t *node;

    //FIXME-IMP Uncomment
    /*     if (!Q_EMPTY(freeq)) { */  
    /*         Q_DEQUEUE(&freeq, node); */
    /*         *index = node->index; */
    /*         MPIU_Free(node); */
    /*         goto fn_exit; */
    /*     } */

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

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_connect
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPID_nem_newtcp_module_connect (struct MPIDI_VC *const vc) 
{
    sockconn_t *conn = NULL;
    pollfd_t *plfd = NULL;
    int index = -1;
    int mpi_errno = MPI_SUCCESS;
    freenode_t *node;

    if (vc->ch.state == MPID_NEM_NEWTCP_MODULE_VC_STATE_DISCONNECTED) {
        struct sockaddr_in *sock_addr;
        socklen_t socklen = sizeof(SA_IN);
        int rc = 0;

        MPIU_Assert(vc->ch.sc == NULL);
        mpi_errno = find_free_entry(&index);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP (mpi_errno);
        
        conn = &g_conn_tbl[index];
        plfd = &g_plfd_tbl[index];

        
        //mpi_errno = MPID_nem_newtcp_module_get_conninfo(vc, &sock_addr); 
        //if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP (mpi_errno);
        sock_addr = &(vc->ch.sock_id);

        CHECK_EINTR(conn->fd, socket(AF_INET, SOCK_STREAM, 0));
        MPIU_ERR_CHKANDJUMP2(conn->fd == -1, mpi_errno, MPI_ERR_OTHER, "**sock_create", 
                             "**sock_create %s %d", strerror(errno), errno);
        mpi_errno = MPID_nem_newtcp_module_set_sockopts(conn->fd);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP (mpi_errno);

        rc = connect(conn->fd, (SA*)sock_addr, sizeof(SA)); 
        //connect should not be called with CHECK_EINTR macro
        MPIU_ERR_CHKANDJUMP1 (rc < 0 && errno != EINPROGRESS, mpi_errno, MPI_ERR_OTHER,
                              "**sock_connect", "**sock_connect %d", errno);
        conn->state = (rc == 0) ? CONN_STATE_TC_C_CNTD : CONN_STATE_TC_C_CNTING;
        conn->pg_id = (char *)vc->pg->id; //FIXME remove cast later when not required
        // FIXME Discuss life-time of pg_id. Here it is assumed to be lifetime of vc to 
        // avoid copying back and forth
        conn->pg_rank = vc->pg_rank;
        conn->vc = vc;
        conn->pending_event = 0; //clear pending events.
        conn->handler = conn_state_handlers[conn->state];
    }
    else if (vc->ch.state == MPID_NEM_NEWTCP_MODULE_VC_STATE_CONNECTED) {
        switch(conn->state) {
        case CONN_STATE_TS_D_DCNTING:
            conn->state = CONN_STATE_TS_COMMRDY;
            conn->pending_event = 0;
            break;
        case CONN_STATE_TS_D_REQRCVD:
            //send_cmdmsg(conn->fd, MSGNAME_DISC, MSGTYPE_NAK); //FIXME-IMP
            conn->state = CONN_STATE_TS_COMMRDY;
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
        MPIU_ERR_CHKANDSTMT(node == NULL, mpi_errno, MPI_ERR_OTHER, goto fn_exit, "**nomem");
        node->index = index;
        //Note: MPIU_ERR_CHKANDJUMP should not be used here as it will be recursive 
        //within fn_fail 
        Q_ENQUEUE(&freeq, node);
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
#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_disconnect
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_disconnect (struct MPIDI_VC *const vc)
{
    sockconn_t *conn = NULL;
    int mpi_errno = MPI_SUCCESS;

    //FIXME check whether a (different/new) error has to be reported stating the VC is already
    // disconnected.
    if (vc->ch.state == MPID_NEM_NEWTCP_MODULE_VC_STATE_DISCONNECTED)
        goto fn_exit;
    else if (vc->ch.state == MPID_NEM_NEWTCP_MODULE_VC_STATE_CONNECTED) {
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

#undef FUNCNAME
#define FUNCNAME state_tc_c_cnting_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int state_tc_c_cnting_handler(const pollfd_t *const plfd, sockconn_t *const conn)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_NEM_NEWTCP_MODULE_SOCK_STATUS_t stat;

    stat = MPID_nem_newtcp_module_check_sock_status(plfd);
    if (stat == MPID_NEM_NEWTCP_MODULE_SOCK_CONNECTED) {
        conn->state = CONN_STATE_TC_C_CNTD;
        conn->handler = conn_state_handlers[conn->state];
    }
    else if (stat == MPID_NEM_NEWTCP_MODULE_SOCK_ERROR_EOF) {
        conn->state = CONN_STATE_TS_D_QUIESCENT;
        conn->handler = conn_state_handlers[conn->state];
        //FIXME: retry 'n' number of retries before signalling an error to VC layer.
    }
    else { //stat == MPID_NEM_NEWTCP_MODULE_SOCK_NOEVENT
        /*
          Still connecting... let it. While still connecting, even if
          a duplicate connection exists and this connection can be closed, it can get
          tricky. close/shutdown on a unconnected fd will fail anyways. So, let it either
          connect or let the connect itself fail on the fd before making a transition
          from this state. However, we are relying on the time taken by connect to 
          report an error. If we want control over that time, fix the code to poll for
          that amount of time or change the socket option to control the time-out of
          connect.
        */
    }
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;

}

#undef FUNCNAME
#define FUNCNAME is_valid_state
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int is_valid_state(sockconn_t *conn) {
    int i, found = FALSE;

    for(i = 0; i < g_tbl_size && !found; i++) {
        sockconn_t *iconn = &g_conn_tbl[i];
        MPID_nem_newtcp_module_Conn_State_t istate = iconn->state;
        if (iconn != conn && iconn->fd != CONN_INVALID_FD 
            && IS_SAME_CONNECTION(iconn, conn)) {
            switch (conn->state) {
            case CONN_STATE_TC_C_CNTD:
                if (istate == CONN_STATE_TC_C_CNTING ||
                    istate == CONN_STATE_TC_C_CNTD ||
                    istate == CONN_STATE_TC_C_RANKSENT)
                    found = TRUE;
                break;
            case CONN_STATE_TC_C_RANKSENT:
                if (istate == CONN_STATE_TC_C_CNTING ||
                    istate == CONN_STATE_TC_C_CNTD ||
                    istate == CONN_STATE_TC_C_RANKSENT ||
                    istate == CONN_STATE_TS_D_DCNTING ||
                    istate == CONN_STATE_TS_D_REQSENT)
                    found = TRUE;
                break;
            }
        }      
    }

    //if found, then the state of the connection is not valid. A bug in the state
    //machine code.
    return !found;
}

#undef FUNCNAME
#define FUNCNAME found_better_conn
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int found_better_conn(sockconn_t *conn) {
    int i, found=FALSE;
  
    for(i = 0; i < g_tbl_size && !found; i++) {
        sockconn_t *iconn = &g_conn_tbl[i];
        MPID_nem_newtcp_module_Conn_State_t istate = iconn->state;
        if (iconn != conn && iconn->fd != CONN_INVALID_FD 
            && IS_SAME_CONNECTION(iconn, conn)) {
            switch (conn->state) {
            case CONN_STATE_TC_C_CNTD:
                if (istate == CONN_STATE_TS_COMMRDY ||
                    istate == CONN_STATE_TA_C_RANKRCVD)
                    found = TRUE;
                break;
                // Add code for other states here, if need be.
            }
        }
    }
    return found;
}

#undef FUNCNAME
#define FUNCNAME state_tc_c_cntd_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int state_tc_c_cntd_handler(const pollfd_t *const plfd, sockconn_t *const conn)
{
    int mpi_errno = MPI_SUCCESS;
    assert(is_valid_state(conn));
    if (found_better_conn(conn)) {
        conn->state = CONN_STATE_TS_D_QUIESCENT;
        conn->handler = conn_state_handlers[conn->state];
    }
    else {
        if (IS_WRITEABLE(plfd) && send_rank(conn->fd) == 0)
            conn->state = CONN_STATE_TC_C_RANKSENT;
        else {
            //Remain in the same state
        }
    }
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;

}

#undef FUNCNAME
#define FUNCNAME state_c_ranksent_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int state_c_ranksent_handler(const pollfd_t *const plfd, sockconn_t *const conn)
{
    int mpi_errno = MPI_SUCCESS;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;

}

#undef FUNCNAME
#define FUNCNAME state_l_cntd_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int state_l_cntd_handler(const pollfd_t *const plfd, sockconn_t *const conn)
{
    int mpi_errno = MPI_SUCCESS;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;

}

#undef FUNCNAME
#define FUNCNAME state_l_rankrcvd_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int state_l_rankrcvd_handler(const pollfd_t *const plfd, sockconn_t *const conn)
{
    int mpi_errno = MPI_SUCCESS;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;

}

#undef FUNCNAME
#define FUNCNAME state_commrdy_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int state_commrdy_handler(const pollfd_t *const plfd, sockconn_t *const conn)
{
    int mpi_errno = MPI_SUCCESS;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;

}

#undef FUNCNAME
#define FUNCNAME state_d_dcnting_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int state_d_dcnting_handler(const pollfd_t *const plfd, sockconn_t *const conn)
{
    int mpi_errno = MPI_SUCCESS;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;

}

#undef FUNCNAME
#define FUNCNAME state_d_reqsent_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int state_d_reqsent_handler(const pollfd_t *const plfd, sockconn_t *const conn)
{
    int mpi_errno = MPI_SUCCESS;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;

}

#undef FUNCNAME
#define FUNCNAME state_d_reqrcvd_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int state_d_reqrcvd_handler(const pollfd_t *const plfd, sockconn_t *const conn)
{
    int mpi_errno = MPI_SUCCESS;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;

}

#undef FUNCNAME
#define FUNCNAME state_d_quiescent_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int state_d_quiescent_handler(const pollfd_t *const plfd, sockconn_t *const conn)
{
    int mpi_errno = MPI_SUCCESS;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;

}

#undef FUNCNAME
#define FUNCNAME init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int init()
{
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

//FIXME-Darius
int MPID_nem_newtcp_module_connection_progress (MPIDI_VC_t *vc)
{
    return 0;
}
