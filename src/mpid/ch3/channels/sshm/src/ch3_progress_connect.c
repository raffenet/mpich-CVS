/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "ch3i_progress.h"

volatile unsigned int MPIDI_CH3I_progress_completions = 0;

int shutting_down = FALSE;

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Shm_connect
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Shm_connect(MPIDI_VC *vc, char *business_card, int *flag)
{
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
    if (strcmp(MPIDI_CH3I_Process.pg->shm_hostname, hostname) != 0)
    {
	*flag = FALSE;
	/*MPIU_DBG_PRINTF(("%s != %s\n", MPIDI_CH3I_Process.pg->shm_hostname, hostname));*/
	return MPI_SUCCESS;
    }

    *flag = TRUE;
    /*MPIU_DBG_PRINTF(("%s == %s\n", MPIDI_CH3I_Process.pg->shm_hostname, hostname));*/

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
#ifdef MPICH_DBG_OUTPUT
	memset(vc->ch.write_shmq->packet[i].data, 0, MPIDI_CH3I_PACKET_SIZE);
#endif
    }

    /* send the queue connection information */
    /*MPIU_DBG_PRINTF(("write_shmq: %p, name - %s\n", vc->ch.write_shmq, vc->ch.shm_write_queue_info.key));*/
    shm_info.info = vc->ch.shm_write_queue_info;
    shm_info.pg_id = 0;
    shm_info.pg_rank = MPIR_Process.comm_world->rank;
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
int MPIDI_CH3I_VC_post_connect(MPIDI_VC * vc)
{
    char * key;
    char * val;
    int key_max_sz;
    int val_max_sz;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    int connected;
    MPIDI_VC *iter;
    int count = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);

    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));

    if (vc->ch.state != MPIDI_CH3I_VC_STATE_UNCONNECTED)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**vc_state", "**vc_state %d", vc->ch.state);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_VC_POST_CONNECT);
	return mpi_errno;
    }

    vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTING;

    /* get the business card */
    mpi_errno = PMI_KVS_Get_key_length_max(&key_max_sz);
    if (mpi_errno != PMI_SUCCESS)
    {
    }
    key = MPIU_Malloc(key_max_sz);
    if (key == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }
    mpi_errno = PMI_KVS_Get_value_length_max(&val_max_sz);
    if (mpi_errno != PMI_SUCCESS)
    {
    }
    val = MPIU_Malloc(val_max_sz);
    if (val == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }

    rc = snprintf(key, key_max_sz, "P%d-businesscard", vc->ch.pg_rank);
    if (rc < 0 || rc > key_max_sz)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**snprintf", "**snprintf %d", rc);
	return mpi_errno;
    }
    rc = PMI_KVS_Get(vc->ch.pg->kvs_name, key, val, val_max_sz);
    if (rc != PMI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get %d", rc);
	return mpi_errno;
    }

    MPIU_DBG_PRINTF(("%s: %s\n", key, val));

    /* attempt to connect through shared memory */
    connected = FALSE;
    MPIU_DBG_PRINTF(("business card: <%s> = <%s>\n", key, val));
    mpi_errno = MPIDI_CH3I_Shm_connect(vc, val, &connected);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**post_connect", "**post_connect %s", "MPIDI_CH3I_Shm_connect");
	return mpi_errno;
    }
    if (!connected)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "unable to establish a shared memory queue connection");
	return mpi_errno;
    }

    MPIU_Free(val);
    MPIU_Free(key);

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
	MPIDI_CH3I_Process.pg->nShmWaitSpinCount = 1;

    vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTED;
    vc->ch.bShm = TRUE;
    vc->ch.shm_reading_pkt = TRUE;
    vc->ch.send_active = MPIDI_CH3I_SendQ_head(vc); /* MT */

    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME connection_alloc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
MPIDI_CH3I_Connection_t * connection_alloc(void)
{
    MPIDI_CH3I_Connection_t * conn;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_ALLOC);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_ALLOC);
    conn = MPIU_Malloc(sizeof(MPIDI_CH3I_Connection_t));
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_ALLOC);
    return conn;
}

#undef FUNCNAME
#define FUNCNAME connection_free
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void connection_free(MPIDI_CH3I_Connection_t * conn)
{
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_FREE);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_FREE);

    MPIU_Free(conn);

    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_FREE);
}

#undef FUNCNAME
#define FUNCNAME connection_send_fail
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void connection_send_fail(MPIDI_CH3I_Connection_t * conn, int mpi_errno)
{
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_SEND_FAIL);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_SEND_FAIL);

    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);

    if (conn->send_active)
    { 
	MPID_Abort(conn->send_active->comm, mpi_errno, 13);
    }
    else
    {
	MPID_Abort(NULL, mpi_errno, 13);
    }

    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_SEND_FAIL);
}

#undef FUNCNAME
#define FUNCNAME connection_recv_fail
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void connection_recv_fail(MPIDI_CH3I_Connection_t * conn, int mpi_errno)
{
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_RECV_FAIL);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_RECV_FAIL);

    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
    MPID_Abort(NULL, mpi_errno, 13);

    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_RECV_FAIL);
}
