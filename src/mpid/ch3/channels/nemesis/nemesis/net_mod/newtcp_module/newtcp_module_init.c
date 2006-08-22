/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "newtcp_module_impl.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

MPID_nem_queue_ptr_t MPID_nem_newtcp_module_free_queue = 0;
MPID_nem_queue_ptr_t MPID_nem_process_recv_queue = 0;
MPID_nem_queue_ptr_t MPID_nem_process_free_queue = 0;
extern sockconn_t g_lstn_sc;
extern pollfd_t g_lstn_plfd;

static MPID_nem_queue_t _free_queue;

static int get_addr_port_from_bc (const char *business_card, struct in_addr *addr, in_port_t *port);
extern int state_listening_handler(const pollfd_t *const a_plfd, sockconn_t *const a_sc);


#define MPIDI_CH3I_PORT_KEY "port"
#define MPIDI_CH3I_ADDR_KEY "addr"

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_init (MPID_nem_queue_ptr_t proc_recv_queue, MPID_nem_queue_ptr_t proc_free_queue,
                                 MPID_nem_cell_ptr_t proc_elements, int num_proc_elements, MPID_nem_cell_ptr_t module_elements,
                                 int num_module_elements, MPID_nem_queue_ptr_t *module_free_queue,
                                 int ckpt_restart, MPIDI_PG_t *pg_p, int pg_rank, char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    int i;
    MPID_nem_newtcp_module_send_q_element_t *sendq_e;
    MPIDI_NEMTCP_STATE_DECL;  

    /* set up listener socket */
    fprintf(stdout, FCNAME " Enter\n"); fflush(stdout);
    MPIDI_NEMTCP_FUNC_ENTER;
    g_lstn_plfd.fd = g_lstn_sc.fd = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    MPIU_ERR_CHKANDJUMP2 (g_lstn_sc.fd == -1, mpi_errno, MPI_ERR_OTHER, "**sock_create", "**sock_create %s %d", strerror (errno), errno);

    mpi_errno = MPID_nem_newtcp_module_set_sockopts (g_lstn_sc.fd);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    g_lstn_plfd.events = POLLIN;
    mpi_errno = MPID_nem_newtcp_module_bind (g_lstn_sc.fd);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    ret = listen (g_lstn_sc.fd, SOMAXCONN);	      
    MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**listen", "**listen %s %d", errno, strerror (errno));  
    g_lstn_sc.state.lstate = LISTEN_STATE_LISTENING;
    g_lstn_sc.handler = state_listening_handler;

    /* create business card */
    mpi_errno = MPID_nem_newtcp_module_get_business_card (bc_val_p, val_max_sz_p);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    /* save references to queues */
    MPID_nem_process_recv_queue = proc_recv_queue;
    MPID_nem_process_free_queue = proc_free_queue;

    MPID_nem_newtcp_module_free_queue = &_free_queue;

    /* set up network module queues */
    MPID_nem_queue_init (MPID_nem_newtcp_module_free_queue);

    for (i = 0; i < num_module_elements; ++i)
    {
        MPID_nem_queue_enqueue (MPID_nem_newtcp_module_free_queue, &module_elements[i]);
    }

    *module_free_queue = MPID_nem_newtcp_module_free_queue;


    MPID_nem_newtcp_module_init_sm();
    MPID_nem_newtcp_module_send_init();
    MPID_nem_newtcp_module_poll_init();

 fn_exit:
    MPIDI_NEMTCP_FUNC_EXIT;
    fprintf(stdout, FCNAME " Exit\n"); fflush(stdout);
    return mpi_errno;
 fn_fail:
    fprintf(stdout, "failure. mpi_errno = %d\n", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_get_business_card
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_get_business_card (char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    struct sockaddr_in sock_id;
    int sock_id_len;
    struct hostent *hp = NULL;
    size_t len;
    char ipaddr_str[INET_ADDRSTRLEN];
    const char *p;

    MPIDI_NEMTCP_FUNC_ENTER;
    fprintf(stdout, FCNAME " Enter\n"); fflush(stdout);
    /* The business card consists of the numeric ip address (represented as a string), and the port id */
    
    /*printf("MPID_nem_newtcp_module_get_business_card Enter. host=%s\n", MPID_nem_hostname);*/
    hp = gethostbyname (MPID_nem_hostname); 
    
    MPIU_ERR_CHKANDJUMP1 (hp == NULL, mpi_errno, MPI_ERR_OTHER, "**gethostbyname", "**gethostbyname %d", h_errno); 
    MPIU_ERR_CHKANDJUMP (hp->h_addrtype != AF_INET, mpi_errno, MPI_ERR_OTHER, "**gethostbyname"); /*FIXME add error string "**gethostbyname returned other than AF_INET type address"*/
    
    /* printf("MPID_nem_newtcp_module_get_business_card. gethostbyname success \n"); */

    p = inet_ntop (AF_INET, (struct in_addr *)hp->h_addr, ipaddr_str, sizeof(ipaddr_str));

    if (p)
        printf("MPID_nem_newtcp_module_get_business_card. inet_ntop ipaddrstr=%s, \n", 
               ipaddr_str);
    else
        printf("MPID_nem_newtcp_module_get_business_card. inet_ntop errno=%d err=%s, \n", 
               errno, strerror(errno));

    MPIU_ERR_CHKANDJUMP1 (p == NULL, mpi_errno, MPI_ERR_OTHER, "**inet_ntop", "**inet_ntop %s", strerror (errno));
    
    mpi_errno = MPIU_Str_add_string_arg (bc_val_p, val_max_sz_p, MPIDI_CH3I_ADDR_KEY, ipaddr_str);
    if (mpi_errno != MPIU_STR_SUCCESS) {
        if (mpi_errno == MPIU_STR_NOMEM) {
            MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard_len");
        }
        else {
            MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard");
        }
        fprintf(stdout, FCNAME " Exit failure\n"); fflush(stdout);
        MPIDI_NEMTCP_FUNC_EXIT;
        return mpi_errno;
    }

    len = sizeof(sock_id);
    ret = getsockname (g_lstn_sc.fd, (struct sockaddr *)&sock_id, &len);
    MPIU_ERR_CHKANDJUMP1 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**getsockname", "**getsockname %s", strerror (errno));

    mpi_errno = MPIU_Str_add_int_arg (bc_val_p, val_max_sz_p, MPIDI_CH3I_PORT_KEY, sock_id.sin_port);
    if (mpi_errno != MPIU_STR_SUCCESS) {
        if (mpi_errno == MPIU_STR_NOMEM) {
            MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard_len");
        }
        else {
            MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard");
        }
        fprintf(stdout, FCNAME " Exit failure\n"); fflush(stdout);
        MPIDI_NEMTCP_FUNC_EXIT;
        return mpi_errno;
    }
    printf("MPID_nem_newtcp_module_get_business_card. port=%d\n", sock_id.sin_port);
 fn_fail:
 fn_exit:
    fprintf(stdout, "MPID_nem_newtcp_module_get_business_card Exit, mpi_errno=%d\n", mpi_errno); fflush(stdout);
    MPIDI_NEMTCP_FUNC_EXIT;
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_connect_to_root
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_connect_to_root (const char *business_card, MPIDI_VC_t *new_vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_NEMTCP_FUNC_ENTER;
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**notimpl", 0);
    MPIDI_NEMTCP_FUNC_EXIT;
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_vc_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_vc_init (MPIDI_VC_t *vc, const char *business_card)
{
    int mpi_errno = MPI_SUCCESS;
    struct in_addr addr;
    int port;    

    MPIDI_NEMTCP_FUNC_ENTER;
    fprintf(stdout, FCNAME " Enter\n"); fflush(stdout);

    memset (&vc->ch.sock_id, 0, sizeof(vc->ch.sock_id));
    vc->ch.sock_id.sin_family = AF_INET;
    
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);    
    mpi_errno = get_addr_port_from_bc (business_card, &addr, &(vc->ch.sock_id.sin_port));
    vc->ch.sock_id.sin_addr.s_addr = addr.s_addr;
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    vc->ch.fd = 0;
    vc->ch.sc = NULL;
    vc->ch.send_queue.head = vc->ch.send_queue.tail = NULL;
    vc->ch.newtcp_sendl_next = NULL;
    vc->ch.newtcp_sendl_prev = NULL;
    vc->ch.pending_recv.cell = NULL;
    vc->ch.pending_recv.end = NULL;
    vc->ch.pending_recv.len = 0;
    
 fn_exit:
    fprintf(stdout, FCNAME " Exit\n"); fflush(stdout);
    MPIDI_NEMTCP_FUNC_EXIT;
    return mpi_errno;
 fn_fail:
    fprintf(stdout, "failure. mpi_errno = %d\n", mpi_errno);
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME get_addr_port_from_bc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int get_addr_port_from_bc (const char *business_card, struct in_addr *addr, in_port_t *port)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    int len;
    char ipaddr_str[INET_ADDRSTRLEN];
    int tmp_port_id;
    
    MPIDI_NEMTCP_FUNC_ENTER;
    fprintf(stdout, FCNAME " Enter\n"); fflush(stdout);
    ret = MPIU_Str_get_string_arg (business_card, MPIDI_CH3I_ADDR_KEY, ipaddr_str, INET_ADDRSTRLEN);
    MPIU_ERR_CHKANDJUMP (ret != MPIU_STR_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**argstr_missinghost");

    ret = inet_pton (AF_INET, ipaddr_str, addr);
    MPIU_ERR_CHKANDJUMP (ret <= 0, mpi_errno, MPI_ERR_OTHER, "**inet_pton");

    mpi_errno = MPIU_Str_get_int_arg (business_card, MPIDI_CH3I_PORT_KEY, (int *)port);
    MPIU_ERR_CHKANDJUMP (mpi_errno != MPIU_STR_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**argstr_missingport");
    fprintf(stdout, "get_addr_port_from_bc buscard=%s  addr=%s port=%d\n",business_card, ipaddr_str, *port); fflush(stdout);

 fn_exit:
    fprintf(stdout, FCNAME " Exit\n"); fflush(stdout);
    MPIDI_NEMTCP_FUNC_EXIT;
    return mpi_errno;
 fn_fail:
    fprintf(stdout, "failure. mpi_errno = %d\n", mpi_errno);
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

/* MPID_nem_newtcp_module_bind -- if MPICH_PORT_RANGE is set, this
   binds the socket to an available port number in the range.
   Otherwise, it binds it to any addr and any port */
#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_bind
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_bind (int sockfd)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    struct sockaddr_in sin;
    int port, low_port, high_port;
    
    low_port = 0;
    high_port = 0;

    MPIDI_NEMTCP_FUNC_ENTER;
    fprintf(stdout, FCNAME " Enter\n"); fflush(stdout);
    MPIU_GetEnvRange( "MPICH_PORT_RANGE", &low_port, &high_port );

    /* if MPICH_PORT_RANGE is not set, low_port and high_port are 0 so bind will use any available port */
    for (port = low_port; port <= high_port; ++port)
    {
        memset ((void *)&sin, 0, sizeof(sin));
        sin.sin_family      = AF_INET;
        sin.sin_addr.s_addr = htonl(INADDR_ANY);
        sin.sin_port        = htons(port);

        ret = bind (sockfd, (struct sockaddr *)&sin, sizeof(sin));
        if (ret == 0)
            break;
        
        /* check for real error */
        MPIU_ERR_CHKANDJUMP3 (errno != EADDRINUSE && errno != EADDRNOTAVAIL, mpi_errno, MPI_ERR_OTHER, "**sock|poll|bind", "**sock|poll|bind %d %d %s", port, errno, strerror (errno));
    }
    /* check if an available port was found */
    MPIU_ERR_CHKANDJUMP3 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**sock|poll|bind", "**sock|poll|bind %d %d %s", port, errno, strerror (errno));

 fn_exit:
/*     if (ret == 0) */
/*         fprintf(stdout, "sockfd=%d  port=%d bound\n", sockfd, port); */
    fprintf(stdout, FCNAME " Exit\n"); fflush(stdout);
    MPIDI_NEMTCP_FUNC_EXIT;
    return mpi_errno;
 fn_fail:
    fprintf(stdout, "failure. mpi_errno = %d\n", mpi_errno);
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}
