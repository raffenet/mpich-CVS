/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "newtcp_module_impl.h"
#define NUM_PREALLOC_SENDQ 10

MPID_nem_queue_ptr_t MPID_nem_tcp_module_recv_queue = 0;
MPID_nem_queue_ptr_t MPID_nem_tcp_module_free_queue = 0;
MPID_nem_queue_ptr_t MPID_nem_process_recv_queue = 0;
MPID_nem_queue_ptr_t MPID_nem_process_free_queue = 0;
int MPID_nem_tcp_module_listen_fd = 0;

static MPID_nem_queue_t _recv_queue;
static MPID_nem_queue_t _free_queue;

static int set_sockopts (int fd);
static int get_addr_port_from_bc (const char *business_card, char addr[], int max_addr_len, int *port);

#define MPIDI_CH3I_PORT_KEY "port"
#define MPIDI_CH3I_ADDR_KEY "addr"

int MPID_nem_newtcp_module_init (MPID_nem_queue_ptr_t proc_recv_queue, MPID_nem_queue_ptr_t proc_free_queue,
                                 MPID_nem_cell_ptr_t proc_elements, int num_proc_elements, MPID_nem_cell_ptr_t module_elements,
                                 int num_module_elements, MPID_nem_queue_ptr_t *module_recv_queue, MPID_nem_queue_ptr_t *module_free_queue,
                                 int ckpt_restart, MPIDI_PG_t *pg_p, int pg_rank, char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    struct sockaddr_in saddr;
    int i;
    MPID_nem_newtcp_module_send_q_element_t *sendq_e;
    MPIU_CHKPMEM_DECL(1);
  
    /* set up listener socket */
    MPID_nem_tcp_module_listen_fd = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    MPIU_ERR_CHKANDJUMP2 (MPID_nem_tcp_module_listen_fd == -1, mpi_errno, MPI_ERR_OTHER, "**sock_create", "**sock_create %s %d", strerror (errno), errno);

    mpi_errno = MPID_nem_newtcp_module_bind (MPID_nem_tcp_module_listen_fd);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    len = sizeof (saddr);
    ret = getsockname (vc->ch.fd, (struct sockaddr *)&saddr, &len);
    MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %s %d", strerror (errno), errno);

    set_sockopts (vc->ch.fd);
        
    ret = listen (vc->ch.fd, SOMAXCONN);	      
    MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**listen", "**listen %s %d", errno, strerror (errno));  

    /* create business card */
    mpi_errno = MPID_nem_newtcp_module_get_business_card (bc_val_p, val_max_sz_p);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    /* save references to queues */
    MPID_nem_process_recv_queue = proc_recv_queue;
    MPID_nem_process_free_queue = proc_free_queue;

    MPID_nem_tcp_module_recv_queue = &_recv_queue;
    MPID_nem_tcp_module_free_queue = &_free_queue;

    /* set up network module queues */
    MPID_nem_queue_init (MPID_nem_tcp_module_recv_queue);
    MPID_nem_queue_init (MPID_nem_tcp_module_free_queue);

    for (i = 0; i < num_module_elements; ++i)
    {
	MPID_nem_queue_enqueue (MPID_nem_tcp_module_free_queue, &module_elements[i]);
    }

    *module_recv_queue = MPID_nem_tcp_module_recv_queue;
    *module_free_queue = MPID_nem_tcp_module_free_queue;

    /* preallocate sendq elements */
    MPIU_CHKPMEM_MALLOC (sendq_e, MPID_nem_newtcp_module_send_q_element_t, NUM_PREALLOC_SENDQ * sizeof(send_queue_element_t), mpi_errno,
                         "send queue element");
    for (i = 0; i < NUM_PREALLOC_SENDQ; ++i)
    {
        Q_ENQUEUE (&MPID_nem_newtcp_module_free_buffers, sendq_e[i]);
    }
    
    /* initialize receive tempbuf */
    MPID_nem_newtcp_module_recv_tmpbuf.start = NULL;
    MPID_nem_newtcp_module_recv_tmpbuf.len = 0;

    MPIU_CHKPMEM_COMMIT();    
 fn_exit:
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
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
    int sock_id_len = sizeof(sock_id);

    mpi_errno = MPIU_Str_add_string_arg (bc_val_p, val_max_sz_p, MPIDI_CH3I_ADDR_KEY, MPID_nem_hostname);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	if (mpi_errno == MPIU_STR_NOMEM) {
	    MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard_len");
	}
	else {
	    MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard");
	}
	return mpi_errno;
    }

    ret = getsockname (MPID_nem_tcp_module_listen_fd, (struct sockaddr *)&sock_id, &len);	 
    MPIU_ERR_CHKANDJUMP1 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**getsockname", "**getsockname %s", strerror (errno));
    
    mpi_errno = MPIU_Str_add_int_arg (bc_val_p, val_max_sz_p, MPIDI_CH3I_PORT_KEY, sock_id.sin_port);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	if (mpi_errno == MPIU_STR_NOMEM) {
	    MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard_len");
	}
	else {
	    MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard");
	}
	return mpi_errno;
    }

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_connect_to_root
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_connect_to_root (const char *business_card, MPIDI_VC_t *new_vc)
{
    int mpi_errno = MPI_SUCCESS;
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**notimpl", 0);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_vc_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_vc_init (MPIDI_VC_t *vc, const char *business_card)
{
    int mpi_errno = MPI_SUCCESS;
    char addr[MAX_HOST_NAME];
    int port;    

    mpi_errno = get_addr_port_from_bc (business_card, addr, MAX_HOST_NAME, &port);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME set_sockopts
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int set_sockopts (int fd)
{
    int mpi_errno = MPI_SUCCESS;
    int option;
    int ret;
    int len;

    /* I heard you have to read the options after setting them in some implementations */
    option = 0;
    len = sizeof(int);
    ret = setsockopt (fd, IPPROTO_TCP, TCP_NODELAY, &option, len);
    MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %s %d", strerror (errno), errno);
    ret = getsockopt (fd, IPPROTO_TCP, TCP_NODELAY, &option, &len);
    MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %s %d", strerror (errno), errno);

    option = 128*1024;
    len = sizeof(int);
    setsockopt (fd, SOL_SOCKET, SO_RCVBUF, &option, len);
    MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %s %d", strerror (errno), errno);
    getsockopt (fd, SOL_SOCKET, SO_RCVBUF, &option, &len);
    MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %s %d", strerror (errno), errno);
    setsockopt (fd, SOL_SOCKET, SO_SNDBUF, &option, len);
    MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %s %d", strerror (errno), errno);
    getsockopt (fd, SOL_SOCKET, SO_SNDBUF, &option, &len);
    MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %s %d", strerror (errno), errno);

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME get_addr_port_from_bc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int get_addr_port_from_bc (const char *business_card, char addr[], int max_addr_len, int *port)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    int len;
    int tmp_port_id;
    
    ret = MPIU_Str_get_string_arg (business_card, MPIDI_CH3I_ADDR_KEY, addr, max_addr_len);
    MPIU_ERR_CHKANDJUMP (ret != MPIU_STR_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**argstr_missinghost");

    mpi_errno = MPIU_Str_get_int_arg (business_card, MPIDI_CH3I_PORT_KEY, port);
    MPIU_ERR_CHKANDJUMP (ret != MPIU_STR_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**argstr_missingport");

 fn_exit:
    return mpi_errno;
 fn_fail:
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
    return mpi_errno;
 fn_fail:
    goto fn_exit;

}
