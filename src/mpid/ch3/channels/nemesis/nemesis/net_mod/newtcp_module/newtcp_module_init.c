/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

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

    /* set up listener socket */
    saddr.sin_family      = AF_INET;
    saddr.sin_addr.s_addr = htonl (INADDR_ANY);
    saddr.sin_port        = htons (0);
    
    ret = bind (MPID_nem_tcp_module_listen_fd, (struct sockaddr *)&saddr, sizeof (saddr));
    MPIU_ERR_CHKANDJUMP3 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**sock|poll|bind", "**sock|poll|bind %d %d %s", ntohs (saddr.sin_port), errno, strerror (errno));

    len = sizeof (saddr);
    ret = getsockname (vc->ch.net.tcp.lmt_desc, (struct sockaddr *)&saddr, &len);
    MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %s %d", strerror (errno), errno);

    set_sockopts (vc->ch.net.tcp.lmt_desc);
        
    ret = listen (vc->ch.net.tcp.lmt_desc, SOMAXCONN);	      
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

    for (index = 0; index < num_module_elements; ++index)
    {
	MPID_nem_queue_enqueue (MPID_nem_tcp_module_free_queue, &module_elements[index]);
    }

    *module_recv_queue = MPID_nem_tcp_module_recv_queue;
    *module_free_queue = MPID_nem_tcp_module_free_queue;

 fn_exit:
    return mpi_errno;
 fn_fail:
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
    
    /* for now, we're establishing a connection between the remote node statically */

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
    
    option = 0;
    ret = setsockopt (fd, IPPROTO_TCP, TCP_NODELAY, &option, sizeof(int));
    MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %s %d", strerror (errno), errno);

    option = 128*1024;
    setsockopt (fd, SOL_SOCKET, SO_RCVBUF, &option, sizeof(int));
    MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %s %d", strerror (errno), errno);
    setsockopt (fd, SOL_SOCKET, SO_SNDBUF, &option, sizeof(int));
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



