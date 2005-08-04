/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "ch3i_progress.h"

volatile unsigned int MPIDI_CH3I_progress_completion_count = 0;

MPIDU_Sock_set_t sock_set;
/* int MPIDI_CH3I_listener_port = 0; brad : now in ch3u_get_business_card_sock.c */
MPIDI_CH3I_Connection_t * MPIDI_CH3I_listener_conn = NULL;

int shutting_down = FALSE;

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Connection_terminate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Connection_terminate(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;

    if (vc->ch.bShm)
    {
	/* There is no post_close for shm connections so handle them as closed immediately. */
	MPIDI_CH3U_Handle_connection(vc, MPIDI_VC_EVENT_TERMINATED);
    }
    else
    {
	vc->ch.conn->state = CONN_STATE_CLOSING;
	mpi_errno = MPIDU_Sock_post_close(vc->ch.sock);
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", NULL);
	    goto fn_exit;
	}
    }

  fn_exit:
    return mpi_errno;
}

/* brad : function now in ch3u_get_business_card.c */
#if 0
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Listener_get_port
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Listener_get_port()
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_LISTENER_GET_PORT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_LISTENER_GET_PORT);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_LISTENER_GET_PORT);
    return MPIDI_CH3I_listener_port;
}
#endif

static unsigned int GetIP(char *pszIP)
{
    unsigned int nIP;
    unsigned int a,b,c,d;
    if (pszIP == NULL)
	return 0;
    sscanf(pszIP, "%u.%u.%u.%u", &a, &b, &c, &d);
    /*MPIU_DBG_PRINTF(("mask: %u.%u.%u.%u\n", a, b, c, d));*/
    nIP = (d << 24) | (c << 16) | (b << 8) | a;
    return nIP;
}

static unsigned int GetMask(char *pszMask)
{
    int i, nBits;
    unsigned int nMask = 0;
    unsigned int a,b,c,d;

    if (pszMask == NULL)
	return 0;

    if (strstr(pszMask, "."))
    {
	sscanf(pszMask, "%u.%u.%u.%u", &a, &b, &c, &d);
	/*MPIU_DBG_PRINTF(("mask: %u.%u.%u.%u\n", a, b, c, d));*/
	nMask = (d << 24) | (c << 16) | (b << 8) | a;
    }
    else
    {
	nBits = atoi(pszMask);
	for (i=0; i<nBits; i++)
	{
	    nMask = nMask << 1;
	    nMask = nMask | 0x1;
	}
    }
    /*
    unsigned int a, b, c, d;
    a = ((unsigned char *)(&nMask))[0];
    b = ((unsigned char *)(&nMask))[1];
    c = ((unsigned char *)(&nMask))[2];
    d = ((unsigned char *)(&nMask))[3];
    MPIU_DBG_PRINTF(("mask: %u.%u.%u.%u\n", a, b, c, d));
    */
    return nMask;
}

static int GetHostAndPort(char *host, int *port, char *business_card)
{
    char pszNetMask[50];
    char *pEnv, *token;
    unsigned int nNicNet, nNicMask;
    char *temp, *pszHost, *pszIP, *pszPort;
    unsigned int ip;

    pEnv = getenv("MPICH_NETMASK");
    if (pEnv != NULL)
    {
	MPIU_Strncpy(pszNetMask, pEnv, 50);
	token = strtok(pszNetMask, "/");
	if (token != NULL)
	{
	    token = strtok(NULL, "\n");
	    if (token != NULL)
	    {
		nNicNet = GetIP(pszNetMask);
		nNicMask = GetMask(token);

		/* parse each line of the business card and match the ip address with the network mask */
		temp = MPIU_Strdup(business_card);
		token = strtok(temp, ":\r\n");
		while (token)
		{
		    pszHost = token;
		    pszIP = strtok(NULL, ":\r\n");
		    pszPort = strtok(NULL, ":\r\n");
		    ip = GetIP(pszIP);
		    /*msg_printf("masking '%s'\n", pszIP);*/
		    if ((ip & nNicMask) == nNicNet)
		    {
			/* the current ip address matches the requested network so return these values */
			MPIU_Strncpy(host, pszIP, MAXHOSTNAMELEN); /*pszHost);*/
			*port = atoi(pszPort);
			MPIU_Free(temp);
			return MPI_SUCCESS;
		    }
		    token = strtok(NULL, ":\r\n");
		}
		if (temp)
		    MPIU_Free(temp);
	    }
	}
    }

    temp = MPIU_Strdup(business_card);
    if (temp == NULL)
    {
	/*MPIDI_err_printf("GetHostAndPort", "MPIU_Strdup failed\n");*/
	return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**MPIU_Strdup", 0);
    }
    /* move to the host part */
    token = strtok(temp, ":");
    if (token == NULL)
    {
	MPIU_Free(temp);
	/*MPIDI_err_printf("GetHostAndPort", "invalid business card\n");*/
	return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**business_card", "**business_card %s", business_card); /*"[ch3:mm] GetHostAndPort: Invalid business card - %s", business_card);*/
    }
    /*strcpy(host, token);*/
    /* move to the ip part */
    token = strtok(NULL, ":");
    if (token == NULL)
    {
	MPIU_Free(temp);
	/*MPIDI_err_printf("GetHostAndPort", "invalid business card\n");*/
	return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**business_card", "**business_card %s", business_card); /*"[ch3:mm] GetHostAndPort: Invalid business card - %s", business_card);*/
    }
    MPIU_Strncpy(host, token, MAXHOSTNAMELEN); /* use the ip string instead of the hostname, it's more reliable */
    /* move to the port part */
    token = strtok(NULL, ":");
    if (token == NULL)
    {
	MPIU_Free(temp);
	/*MPIDI_err_printf("GetHostAndPort", "invalid business card\n");*/
	return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**business_card", "**business_card %s", business_card); /*"[ch3:mm] GetHostAndPort: Invalid business card - %s", business_card);*/
    }
    *port = atoi(token);
    MPIU_Free(temp);

    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Shm_connect
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Shm_connect(MPIDI_VC_t *vc, char *business_card, int *flag)
{
    /* brad : this could be static (it is only called in this file) */
    int mpi_errno;
    char hostname[256];
    char queue_name[100];
    MPIDI_CH3I_BootstrapQ queue;
    MPIDI_CH3I_Shmem_queue_info shm_info;
    int i;

    /* get the host and queue from the business card */
    mpi_errno = MPIU_Str_get_string_arg(business_card, MPIDI_CH3I_SHM_HOST_KEY, hostname, 256);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	/*printf("getstringarg(%s, %s) failed.\n", MPIDI_CH3I_SHM_HOST_KEY, business_card);fflush(stdout);*/
	*flag = FALSE;
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**argstr_shmhost", 0);
	return mpi_errno;
    }
    mpi_errno = MPIU_Str_get_string_arg(business_card, MPIDI_CH3I_SHM_QUEUE_KEY, queue_name, 100);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	*flag = FALSE;
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**argstr_shmq", 0);
	return mpi_errno;
    }

    /* compare this host's name with the business card host name */
    /*if (strcmp(MPIDI_Process.my_pg->ch.shm_hostname, hostname) != 0)*/
    if (strcmp(MPIDI_Process.my_pg->ch.shm_hostname, hostname) != 0)
    {
	*flag = FALSE;
	/*MPIU_DBG_PRINTF(("%s != %s\n", MPIDI_Process.my_pg->ch.shm_hostname, hostname));*/
	return MPI_SUCCESS;
    }

    *flag = TRUE;
    /*MPIU_DBG_PRINTF(("%s == %s\n", MPIDI_Process.my_pg->ch.shm_hostname, hostname));*/

    MPIU_DBG_PRINTF(("attaching to queue: %s\n", queue_name));
    mpi_errno = MPIDI_CH3I_BootstrapQ_attach(queue_name, &queue);
    if (mpi_errno != MPI_SUCCESS)
    {
	*flag = FALSE;
	return MPI_SUCCESS;
    }

    /* create the write queue */
    mpi_errno = MPIDI_CH3I_SHM_Get_mem(sizeof(MPIDI_CH3I_SHM_Queue_t), &vc->ch.shm_write_queue_info);
    if (mpi_errno != MPI_SUCCESS)
    {
	*flag = FALSE;
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmconnect_getmem", 0);
	return mpi_errno;
    }
    /* printf("rank %d sending queue(%s) to rank %d\n", MPIR_Process.comm_world->rank, vc->ch.shm_write_queue_info.name,
       vc->ch.pg_rank); */
    
    vc->ch.write_shmq = vc->ch.shm_write_queue_info.addr;
    vc->ch.write_shmq->head_index = 0;
    vc->ch.write_shmq->tail_index = 0;
    MPIDI_DBG_PRINTF((60, FCNAME, "write_shmq head = 0"));
    MPIDI_DBG_PRINTF((60, FCNAME, "write_shmq tail = 0"));
    for (i=0; i<MPIDI_CH3I_NUM_PACKETS; i++)
    {
	vc->ch.write_shmq->packet[i].offset = 0;
	vc->ch.write_shmq->packet[i].avail = MPIDI_CH3I_PKT_EMPTY;
    }

    /* send the queue connection information */
    /*MPIU_DBG_PRINTF(("write_shmq: %p, name - %s\n", vc->ch.write_shmq, vc->ch.shm_write_queue_info.key));*/
    shm_info.info = vc->ch.shm_write_queue_info;
    /*shm_info.pg_id = 0;*/
    /* brad : must do communicator translation in the case of INTERcomms, so that we get
     *          the correct pg_id.  kvs_name wasn't being used for spawned pg's so i
     *          use it to store the pg->id for the intracommunicator used when this 
     *          intercommunicator was created with spawn. if the values are identical, then
     *          its MPI-1, but if not it's MPI-2
     */
    if ( strcmp(vc->pg->id, vc->pg->ch.kvs_name))
    {
        MPIU_Strncpy(shm_info.pg_id, vc->pg->ch.kvs_name, 100);  /* INTERcomm */
        shm_info.is_intercomm = 1;
    }
    else
    {
        MPIU_Strncpy(shm_info.pg_id, vc->pg->id, 100);          /* INTRAcomm */
        shm_info.is_intercomm = 0;
    }
    shm_info.pg_rank = MPIR_Process.comm_world->rank;  /* brad : comm_world!?! will be changed on other side
                                                        *         for INTERcomms
                                                        *  this also implies that pg's map onto MPI_COMM_WORLDs
                                                        */
    shm_info.pid = getpid();
    MPIU_DBG_PRINTF(("MPIDI_CH3I_Shm_connect: sending bootstrap queue info from rank %d to msg queue %s\n", MPIR_Process.comm_world->rank, queue_name));
    mpi_errno = MPIDI_CH3I_BootstrapQ_send_msg(queue, &shm_info, sizeof(shm_info));
    if (mpi_errno != MPI_SUCCESS)
    {
	MPIDI_CH3I_SHM_Unlink_mem(&vc->ch.shm_write_queue_info);
	MPIDI_CH3I_SHM_Release_mem(&vc->ch.shm_write_queue_info);
	*flag = FALSE;
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**boot_send", 0);
	return mpi_errno;
    }

    /* MPIU_Free the queue resource */
    /*MPIU_DBG_PRINTF(("detaching from queue: %s\n", queue_name));*/
    mpi_errno = MPIDI_CH3I_BootstrapQ_detach(queue);
    if (mpi_errno != MPI_SUCCESS)
    {
	MPIDI_CH3I_SHM_Unlink_mem(&vc->ch.shm_write_queue_info);
	MPIDI_CH3I_SHM_Release_mem(&vc->ch.shm_write_queue_info);
	*flag = FALSE;
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**boot_detach", 0);
	return mpi_errno;
    }

    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_VC_post_connect
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_VC_post_connect(MPIDI_VC_t * vc)
{
    char * key = NULL;
    char * val;
    int key_max_sz;
    int val_max_sz;
    char host_description[256];
    int port;
    int rc;
    int found;
    MPIDI_CH3I_Connection_t * conn;
    int mpi_errno = MPI_SUCCESS;
    int connected;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);

    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));

    if (vc->ch.state != MPIDI_CH3I_VC_STATE_UNCONNECTED)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**vc_state", "**vc_state %d", vc->ch.state);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
	return mpi_errno;
    }

    vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTING;

    /* get the business card */
    mpi_errno = PMI_KVS_Get_value_length_max(&val_max_sz);
    if (mpi_errno != PMI_SUCCESS)
    {
    }
    val = MPIU_Malloc(val_max_sz);
    if (val == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
	return mpi_errno;
    }

    /* first lookup bizcard cache to see if bizcard is there. needed for spawn/connect/accept */
    mpi_errno = MPIDI_CH3I_Lookup_bizcard_cache(vc->pg->id, vc->pg_rank, val, 
                                                val_max_sz, &found);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
        goto fn_exit;
    }
    /* --END ERROR HANDLING-- */    

    if (!found) {
        mpi_errno = PMI_KVS_Get_key_length_max(&key_max_sz);
        if (mpi_errno != PMI_SUCCESS)
        {
        }
        key = MPIU_Malloc(key_max_sz);
        if (key == NULL)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
            MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
            return mpi_errno;
        }
        rc = snprintf(key, key_max_sz, "P%d-businesscard", vc->pg_rank);
        if (rc < 0 || rc > key_max_sz)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**snprintf", "**snprintf %d", rc);
            MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
            return mpi_errno;
        }

        rc = PMI_KVS_Get(vc->pg->ch.kvs_name, key, val, val_max_sz);
        if (rc != PMI_SUCCESS)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get %d", rc);
            MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
            return mpi_errno;
        }

        /* brad : key free'd here in sock */

        /* brad : should this be added to the bizcache? */
    }

/*     MPIU_DBG_PRINTF(("%s: %s\n", key, val)); */

    /* attempt to connect through shared memory */
    connected = FALSE;
/*     MPIU_DBG_PRINTF(("business card: <%s> = <%s>\n", key, val)); */
    mpi_errno = MPIDI_CH3I_Shm_connect(vc, val, &connected);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**post_connect", "**post_connect %s", "MPIDI_CH3I_Shm_connect");
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
	return mpi_errno;
    }
    if (connected)
    {
	MPIDI_VC_t *iter;
	int count = 0;

	MPIU_Free(val);
	if(key) MPIU_Free(key);

	/*MPIU_DBG_PRINTF(("shmem connected\n"));*/
	vc->ch.shm_next_writer = MPIDI_CH3I_Process.shm_writing_list;
	MPIDI_CH3I_Process.shm_writing_list = vc;

	/* If there are more shm connections than cpus, reduce the spin count to one. */
	/* This does not take into account connections between other processes on the same machine. */
	iter = MPIDI_CH3I_Process.shm_writing_list;
	while (iter)
	{
	    count++;
	    iter = iter->ch.shm_next_writer;
	}
	if (count >= MPIDI_CH3I_Process.num_cpus)
	{
	    MPIDI_Process.my_pg->ch.nShmWaitSpinCount = 1;
	}

	vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTED;
	vc->ch.bShm = TRUE;
	vc->ch.shm_reading_pkt = TRUE;
	vc->ch.send_active = MPIDI_CH3I_SendQ_head(vc); /* MT */

	MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
	return mpi_errno;
    }

    /* attempt to connect through sockets */
    mpi_errno = MPIU_Str_get_string_arg(val, MPIDI_CH3I_HOST_DESCRIPTION_KEY, host_description, 256);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**argstr_hostd", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
	return mpi_errno;
    }
    mpi_errno = MPIU_Str_get_int_arg(val, MPIDI_CH3I_PORT_KEY, &port);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**argstr_port", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
	return mpi_errno;
    }

    mpi_errno = connection_alloc(&conn);
    if (mpi_errno == MPI_SUCCESS)
    {
	mpi_errno = MPIDU_Sock_post_connect(sock_set, conn, host_description, port, &conn->sock);
	if (mpi_errno == MPI_SUCCESS)
	{
	    vc->ch.sock = conn->sock;
	    vc->ch.conn = conn;
	    conn->vc = vc;
	    conn->state = CONN_STATE_CONNECTING;
	    conn->send_active = NULL;
	    conn->recv_active = NULL;
	}
	else
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|postconnect",
		"**ch3|sock|postconnect %d %d %s", MPIR_Process.comm_world->rank, vc->pg_rank, val);

	    vc->ch.state = MPIDI_CH3I_VC_STATE_FAILED;
	    connection_free(conn);
	}
    }
    else
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|connalloc", NULL);
    }

    MPIU_Free(val);
    if(key) MPIU_Free(key);
 fn_exit:
    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
    return mpi_errno;
}

/*
#undef FUNCNAME
#define FUNCNAME connection_alloc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int connection_alloc(MPIDI_CH3I_Connection_t ** connp)
{
    MPIDI_CH3I_Connection_t * conn = NULL;
    int id_sz;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_ALLOC);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_ALLOC);
    conn = MPIU_Malloc(sizeof(MPIDI_CH3I_Connection_t));
    if (conn == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**ch3|sock|connallocfailed", NULL);
	goto fn_fail;
    }
    conn->pg_id = NULL;
    
    mpi_errno = PMI_Get_id_length_max(&id_sz);
    if (mpi_errno != PMI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_id_length_max",
					 "**pmi_get_id_length_max %d", mpi_errno);
	goto fn_fail;
    }
    conn->pg_id = MPIU_Malloc(id_sz + 1);
    if (conn->pg_id == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", NULL);
	goto fn_fail;
    }

    *connp = conn;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_ALLOC);
    return mpi_errno;

  fn_fail:
    if (conn != NULL)
    {
	if (conn->pg_id != NULL)
	{
	    MPIU_Free(conn->pg_id);
	}
	
	MPIU_Free(conn);
    }

    goto fn_exit;
}
*/

#undef FUNCNAME
#define FUNCNAME connection_free
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void connection_free(MPIDI_CH3I_Connection_t * conn)
{
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_FREE);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_FREE);

    MPIU_Free(conn->pg_id);
    MPIU_Free(conn);

    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_FREE);
}

#undef FUNCNAME
#define FUNCNAME connection_post_sendq_req
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int connection_post_sendq_req(MPIDI_CH3I_Connection_t * conn)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_POST_SENDQ_REQ);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_POST_SENDQ_REQ);
    /* post send of next request on the send queue */
    conn->send_active = MPIDI_CH3I_SendQ_head(conn->vc); /* MT */
    if (conn->send_active != NULL)
    {
	mpi_errno = MPIDU_Sock_post_writev(conn->sock, conn->send_active->dev.iov, conn->send_active->dev.iov_count, NULL);
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = connection_send_fail(conn, mpi_errno);
	}
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_POST_SENDQ_REQ);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME connection_post_send_pkt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int connection_post_send_pkt(MPIDI_CH3I_Connection_t * conn)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_POST_SEND_PKT);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_POST_SEND_PKT);
    
    mpi_errno = MPIDU_Sock_post_write(conn->sock, &conn->pkt, sizeof(conn->pkt), sizeof(conn->pkt), NULL);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = connection_send_fail(conn, mpi_errno);
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_POST_SEND_PKT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME connection_post_recv_pkt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int connection_post_recv_pkt(MPIDI_CH3I_Connection_t * conn)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_POST_RECV_PKT);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_POST_RECV_PKT);

    mpi_errno = MPIDU_Sock_post_read(conn->sock, &conn->pkt, sizeof(conn->pkt), sizeof(conn->pkt), NULL);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = connection_recv_fail(conn, mpi_errno);
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_POST_RECV_PKT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME connection_send_fail
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int connection_send_fail(MPIDI_CH3I_Connection_t * conn, int sock_errno)
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_SEND_FAIL);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_SEND_FAIL);

    mpi_errno = MPIR_Err_create_code(sock_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", NULL);

#   if 0
    {
	conn->state = CONN_STATE_FAILED;
	if (conn->vc != NULL)
	{
	    conn->vc->ch.state = MPIDI_CH3I_VC_STATE_FAILED;
	    MPIDI_CH3U_VC_send_failure(conn->vc, mpi_errno);
	}
    }
#   endif

    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_SEND_FAIL);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME connection_recv_fail
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int connection_recv_fail(MPIDI_CH3I_Connection_t * conn, int sock_errno)
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_RECV_FAIL);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_RECV_FAIL);

    mpi_errno = MPIR_Err_create_code(sock_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", NULL);
    
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_RECV_FAIL);
    return mpi_errno;
}
