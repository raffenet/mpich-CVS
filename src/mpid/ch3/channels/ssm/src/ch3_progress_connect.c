/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "ch3i_progress.h"

volatile unsigned int MPIDI_CH3I_progress_completion_count = 0;

/* local prototypes */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Connection_terminate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Connection_terminate(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;

    if (vc->ch.bShm)
    {
	/* There is no post_close for shm connections so handle them as closed 
	   immediately. */
	MPIDI_CH3I_SHM_Remove_vc_references(vc);
	MPIDI_CH3U_Handle_connection(vc, MPIDI_VC_EVENT_TERMINATED);
    }
    else
    {
	vc->ch.conn->state = CONN_STATE_CLOSING;
	mpi_errno = MPIDU_Sock_post_close(vc->ch.sock);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER, "**fail");
	    goto fn_exit;
	}
    }

  fn_exit:
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_Remove_vc_read_references
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static void MPIDI_CH3I_SHM_Remove_vc_read_references(MPIDI_VC_t *vc)
{
    MPIDI_VC_t *iter, *trailer;

    /* remove vc from the reading list */
    iter = trailer = MPIDI_CH3I_Process.shm_reading_list;
    while (iter != NULL)
    {
	if (iter == vc)
	{
	    /*printf("[%s%d]removing read vc%d.state = %s (%s)\n", MPIU_DBG_parent_str, MPIR_Process.comm_world->rank, vc->pg_rank, VCState(vc), (vc->pg && vc->pg_rank >= 0) ? vc->pg->id : "?");fflush(stdout);*/
	    /* Mark the connection as NOT read connected so it won't be mistaken for active */
	    /* Since shm connections are uni-directional the following three states are considered active:
	     * MPIDI_VC_STATE_ACTIVE + ch.shm_read_connected = 0 - active in the write direction
	     * MPIDI_VC_STATE_INACTIVE + ch.shm_read_connected = 1 - active in the read direction
	     * MPIDI_VC_STATE_ACTIVE + ch.shm_read_connected = 1 - active in both directions
	     */
	    vc->ch.shm_read_connected = 0;
	    if (trailer != iter)
	    {
		/* remove the vc from the list */
		trailer->ch.shm_next_reader = iter->ch.shm_next_reader;
	    }
	    else
	    {
		/* remove the vc from the head of the list */
		MPIDI_CH3I_Process.shm_reading_list = MPIDI_CH3I_Process.shm_reading_list->ch.shm_next_reader;
	    }
	}
	if (trailer != iter)
	    trailer = trailer->ch.shm_next_reader;
	iter = iter->ch.shm_next_reader;
    }
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_Remove_vc_write_references
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static void MPIDI_CH3I_SHM_Remove_vc_write_references(MPIDI_VC_t *vc)
{
    MPIDI_VC_t *iter, *trailer;

    /* remove the vc from the writing list */
    iter = trailer = MPIDI_CH3I_Process.shm_writing_list;
    while (iter != NULL)
    {
	if (iter == vc)
	{
	    /*printf("[%s%d]removing write vc%d.state = %s (%s)\n", MPIU_DBG_parent_str, MPIR_Process.comm_world->rank, vc->pg_rank, VCState(vc), (vc->pg && vc->pg_rank >= 0) ? vc->pg->id : "?");fflush(stdout);*/
	    if (trailer != iter)
	    {
		/* remove the vc from the list */
		trailer->ch.shm_next_writer = iter->ch.shm_next_writer;
	    }
	    else
	    {
		/* remove the vc from the head of the list */
		MPIDI_CH3I_Process.shm_writing_list = MPIDI_CH3I_Process.shm_writing_list->ch.shm_next_writer;
	    }
	}
	if (trailer != iter)
	    trailer = trailer->ch.shm_next_writer;
	iter = iter->ch.shm_next_writer;
    }
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_Remove_vc_references
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3I_SHM_Remove_vc_references(MPIDI_VC_t *vc)
{
    MPIDI_CH3I_SHM_Remove_vc_read_references(vc);
    MPIDI_CH3I_SHM_Remove_vc_write_references(vc);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_Add_to_reader_list
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3I_SHM_Add_to_reader_list(MPIDI_VC_t *vc)
{
    MPIDI_CH3I_SHM_Remove_vc_read_references(vc);
    /*printf("[%s%d]adding read vc%d.state = %s (%s)\n", MPIU_DBG_parent_str, MPIR_Process.comm_world->rank, vc->pg_rank, VCState(vc), (vc->pg && vc->pg_rank >= 0) ? vc->pg->id : "?");fflush(stdout);*/
    vc->ch.shm_next_reader = MPIDI_CH3I_Process.shm_reading_list;
    MPIDI_CH3I_Process.shm_reading_list = vc;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_Add_to_writer_list
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3I_SHM_Add_to_writer_list(MPIDI_VC_t *vc)
{
    MPIDI_CH3I_SHM_Remove_vc_write_references(vc);
    /*printf("[%s%d]adding write vc%d.state = %s (%s)\n", MPIU_DBG_parent_str, MPIR_Process.comm_world->rank, vc->pg_rank, VCState(vc), (vc->pg && vc->pg_rank >= 0) ? vc->pg->id : "?");fflush(stdout);*/
    vc->ch.shm_next_writer = MPIDI_CH3I_Process.shm_writing_list;
    MPIDI_CH3I_Process.shm_writing_list = vc;
}

#if 0

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
#endif

/* This routine may be called from the common connect code */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Shm_connect
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Shm_connect(MPIDI_VC_t *vc, const char *business_card, int *flag)
{
    int mpi_errno;
    char hostname[256];
    char queue_name[100];
    MPIDI_CH3I_BootstrapQ queue;
    MPIDI_CH3I_Shmem_queue_info shm_info;
    int i;
#ifdef HAVE_SHARED_PROCESS_READ
    char pid_str[20];
#endif

    /* get the host and queue from the business card */
    mpi_errno = MPIU_Str_get_string_arg(business_card, MPIDI_CH3I_SHM_HOST_KEY, hostname, 256);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	/*printf("getstringarg(%s, %s) failed.\n", MPIDI_CH3I_SHM_HOST_KEY, business_card);fflush(stdout);*/
	*flag = FALSE;
/* If there is no shm host key, assume that we can't use shared memory */
/*
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**argstr_shmhost", 0);
*/
	mpi_errno = 0;
	return mpi_errno;
    }
    mpi_errno = MPIU_Str_get_string_arg(business_card, MPIDI_CH3I_SHM_QUEUE_KEY, queue_name, 100);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	*flag = FALSE;
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**argstr_shmq", 0);
	return mpi_errno;
    }

#ifdef HAVE_SHARED_PROCESS_READ
    mpi_errno = MPIU_Str_get_string_arg(business_card, MPIDI_CH3I_SHM_PID_KEY, pid_str, 20);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	*flag = FALSE;
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**argstr_shmpid", 0);
	return mpi_errno;
    }
#endif

    /* compare this host's name with the business card host name */
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
    /*printf("rank %d sending queue(%s)\n", MPIR_Process.comm_world->rank, vc->ch.shm_write_queue_info.name);*/

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
    /*
    printf("comm_world rank %d\nvc->pg_rank %d\nmy_pg_rank %d\n\npg_id:\n<%s>\n",
	MPIR_Process.comm_world->rank,
	vc->pg_rank,
	MPIDI_Process.my_pg_rank,
	vc->pg->id);
    fflush(stdout);
    */
    MPIU_Strncpy(shm_info.pg_id, MPIDI_Process.my_pg->id, 100);
    shm_info.pg_rank = MPIDI_Process.my_pg_rank;
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

#ifdef HAVE_SHARED_PROCESS_READ
#ifdef HAVE_WINDOWS_H
    /*MPIU_DBG_PRINTF(("Opening process[%d]: %d\n", i, pSharedProcess[i].nPid));*/
    vc->ch.hSharedProcessHandle =
	OpenProcess(STANDARD_RIGHTS_REQUIRED | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, 
	FALSE, atoi(pid_str));
    if (vc->ch.hSharedProcessHandle == NULL)
    {
	int err = GetLastError();
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**OpenProcess", "**OpenProcess %d %d", i, err);
	goto fn_fail;
    }
#else
    vc->ch.nSharedProcessID = atoi(pid_str);
    mpi_errno = MPIDI_SHM_InitRWProc( vc->ch.nSharedProcessID, 
			      &vc->ch.nSharedProcessFileDescriptor );
    if (mpi_errno) { goto fn_fail; }
#endif
#endif

    return MPI_SUCCESS;
 fn_fail:
    MPIDI_CH3I_SHM_Unlink_mem(&vc->ch.shm_write_queue_info);
    MPIDI_CH3I_SHM_Release_mem(&vc->ch.shm_write_queue_info);
    *flag = FALSE;
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_VC_post_connect
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_VC_post_connect(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;
    char val[MPIDI_MAX_KVS_VALUE_LEN];
    char host_description[MAX_HOST_DESCRIPTION_LEN];
    int port;
    MPIDU_Sock_ifaddr_t ifaddr;
    int hasIfaddr = 0;
    int rc;
    MPIDI_CH3I_Connection_t * conn;
    int connected;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);

    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));

    if (vc->ch.state != MPIDI_CH3I_VC_STATE_UNCONNECTED)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**vc_state", "**vc_state %d", vc->ch.state);
	goto fn_fail;
    }

    vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTING;

    /* get the business card */
    mpi_errno = MPIDI_PG_GetConnString( vc->pg, vc->pg_rank, val, sizeof(val));
    if (mpi_errno) {
	MPIU_ERR_POP(mpi_errno);
    }

    /* attempt to connect through shared memory */
/*    printf( "trying to connect through shared memory...\n"); fflush(stdout); */
    connected = FALSE;
/*     MPIU_DBG_PRINTF(("business card: <%s> = <%s>\n", key, val)); */
    mpi_errno = MPIDI_CH3I_Shm_connect(vc, val, &connected);
/*    printf( "After attempt to connect, flag = %d and rc = %d\n", connected, 
      mpi_errno ); fflush(stdout); */
    if (mpi_errno != MPI_SUCCESS) 
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**post_connect", "**post_connect %s", "MPIDI_CH3I_Shm_connect");
	goto fn_fail;
    }
    if (connected)
    {
	MPIDI_VC_t *iter;
	int count = 0;

	/*MPIU_DBG_PRINTF(("shmem connected\n"));*/
	MPIDI_CH3I_SHM_Add_to_writer_list(vc);

	/* If there are more shm connections than cpus, reduce the spin count 
	   to one. */
	/* This does not take into account connections between other processes 
	   on the same machine. */
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
	goto fn_exit;
    }

    /* Reset the state if we've failed to connect */
    vc->ch.state = MPIDI_CH3I_VC_STATE_UNCONNECTED;
    mpi_errno = MPIDI_CH3I_Sock_connect( vc, val, sizeof(val) );
#if 0
/*    printf( "Attempting to connect through socket\n" );fflush(stdout); */
    MPIU_DBG_MSG_FMT(CH3_CONNECT,TYPICAL,(MPIU_DBG_FDEST,
	   "vc=%p: Attempting to connect with business card %s", vc, val ));
    /* attempt to connect through sockets */
    mpi_errno = MPIDU_Sock_get_conninfo_from_bc( val, host_description,
						 sizeof(host_description),
						 &port, &ifaddr, &hasIfaddr );
    if (mpi_errno) {
	MPIU_ERR_POP(mpi_errno);
    }

/*    printf ("Allocating connection\n" );fflush(stdout);*/
    mpi_errno = MPIDI_CH3I_Connection_alloc(&conn);
    if (mpi_errno == MPI_SUCCESS)
    {
/*	printf( "Posting socket connection\n" );fflush(stdout);*/
	/* FIXME: This is a hack to allow Windows to continue to use
	   the host description string instead of the interface address
	   bytes when posting a socket connection.  This should be fixed 
	   by changing the Sock_post_connect to only accept interface
	   address.  See also channels/sock/ch3_progress.c */
#ifndef HAVE_WINDOWS_H
	if (hasIfaddr) {
	    mpi_errno = MPIDU_Sock_post_connect_ifaddr(MPIDI_CH3I_sock_set, 
						       conn, &ifaddr, port, 
						       &conn->sock);
	}
	else 
#endif
	{
	    mpi_errno = MPIDU_Sock_post_connect(MPIDI_CH3I_sock_set, conn, 
						host_description, port, 
						&conn->sock);
	}
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
	    if (vc->ch.conn == conn) vc->ch.conn = 0;
	    MPIDI_CH3I_Connection_free(conn);
	}
    }
    else
    {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**ch3|sock|connalloc");
    }
#endif
 fn_exit:
/*    printf("Exiting with %d\n", mpi_errno );fflush(stdout);*/
    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Connection_free
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3I_Connection_free(MPIDI_CH3I_Connection_t * conn)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_CONNECTION_FREE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_CONNECTION_FREE);

    MPIU_Free(conn->pg_id);
    MPIU_Free(conn);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_CONNECTION_FREE);
}

