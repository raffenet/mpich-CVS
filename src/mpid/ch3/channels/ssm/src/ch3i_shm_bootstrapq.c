/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpidi_ch3_impl.h"

#define MQSHM_END -1

typedef struct mqshm_msg_t
{
    int tag, next, length;
    unsigned char data[BOOTSTRAP_MAX_MSG_SIZE];
} mqshm_msg_t;

typedef struct mqshm_t
{
    MPIDU_Process_lock_t lock;
    /*int inuse;*/ /* use to test that the lock is working */
    int first;
    int last;
    int next_free;
    mqshm_msg_t msg[BOOTSTRAP_MAX_NUM_MSGS];
} mqshm_t;

typedef struct mqshm_node_t
{
    int id;
    mqshm_t *q_ptr;
    MPIDI_CH3I_Shmem_block_request_result shm_info;
    struct mqshm_node_t *next;
} mqshm_node_t;

static int next_id = 0;
static mqshm_node_t *q_list = NULL;

static mqshm_t * id_to_queue(const int id)
{
    mqshm_node_t *iter = q_list;
    while (iter)
    {
	/*printf("[%d] id_to_queue checking %d == %d\n", MPIR_Process.comm_world->rank, id, iter->id);fflush(stdout);*/
	if (iter->id == id)
	    return iter->q_ptr;
	iter = iter->next;
    }
    return NULL;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_mqshm_create
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_mqshm_create(const char *name, const int initialize, int *id)
{
    int mpi_errno = MPI_SUCCESS;
    mqshm_node_t *node;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_MQSHM_CREATE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_MQSHM_CREATE);

    /* allocate a node to put in the global list */
    node = MPIU_Malloc(sizeof(mqshm_node_t));
    if (node == NULL)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_MQSHM_CREATE);
	return mpi_errno;
    }
    /*printf("[%d] allocated %d bytes for a mqshm_node_t\n", MPIR_Process.comm_world->rank, sizeof(mqshm_node_t));fflush(stdout);*/
#ifdef USE_POSIX_SHM
    sprintf(node->shm_info.name, "/"MPICH_MSG_QUEUE_PREFIX"%s", name);
#elif defined(USE_SYSV_SHM)
    sprintf(node->shm_info.name, "/tmp/"MPICH_MSG_QUEUE_PREFIX"%s", name);
#else
    strcpy(node->shm_info.name, name);
#endif
    /* allocate the shared memory for the queue */
    mpi_errno = MPIDI_CH3I_SHM_Get_mem_named(sizeof(mqshm_t), &node->shm_info);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_MQSHM_CREATE);
	return mpi_errno;
    }
    /*printf("[%d] shm_info.addr = %p, shm_info.size = %u, name = %s\n", MPIR_Process.comm_world->rank, node->shm_info.addr, node->shm_info.size, name);fflush(stdout);*/
    node->q_ptr = (mqshm_t*)node->shm_info.addr;
    node->id = next_id++;
    node->next = q_list;
    q_list = node;
    *id = node->id;
    /*printf("[%d] q_list node: q_ptr = %p, id = %d, next = %p\n",
	   MPIR_Process.comm_world->rank, node->q_ptr, node->id, node->next);
	   fflush(stdout);*/

    if (initialize)
    {
	/* initialize the queue */
	for (i=0; i<BOOTSTRAP_MAX_NUM_MSGS-1; i++)
	{
	    node->q_ptr->msg[i].next = i+1;
	    /*printf("[%d] msg[%d].next = %d\n",
	      MPIR_Process.comm_world->rank, i, node->q_ptr->msg[i].next);*/
	}
	node->q_ptr->msg[BOOTSTRAP_MAX_NUM_MSGS-1].next = MQSHM_END;
	/*printf("[%d] msg[%d].next = %d\n",
	       MPIR_Process.comm_world->rank,
	       BOOTSTRAP_MAX_NUM_MSGS-1,
	       node->q_ptr->msg[BOOTSTRAP_MAX_NUM_MSGS-1].next);
	       fflush(stdout);*/
	node->q_ptr->first = MQSHM_END;
	node->q_ptr->last = MQSHM_END;
	node->q_ptr->next_free = 0;
	/*node->q_ptr->inuse = 0;*/
	MPIDU_Process_lock_init(&node->q_ptr->lock);
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_MQSHM_CREATE);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_mqshm_close
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_mqshm_close(int id)
{
    int mpi_errno = MPI_SUCCESS;
    mqshm_node_t *trailer, *iter;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_MQSHM_CLOSE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_MQSHM_CLOSE);
    trailer = iter = q_list;
    while (iter)
    {
	if (iter->id == id)
	{
	    MPIDI_CH3I_SHM_Release_mem(&iter->shm_info);
	    if (trailer != iter)
	    {
		trailer->next = iter->next;
	    }
	    else
	    {
		q_list = iter->next;
	    }
	    MPIU_Free(iter);
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_MQSHM_UNLINK);
	    return MPI_SUCCESS;
	}
	if (trailer != iter)
	    trailer = trailer->next;
	iter = iter->next;
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_MQSHM_CLOSE);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_mqshm_unlink
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_mqshm_unlink(int id)
{
    int mpi_errno = MPI_SUCCESS;
    mqshm_node_t *iter = q_list;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_MQSHM_UNLINK);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_MQSHM_UNLINK);
    while (iter)
    {
	if (iter->id == id)
	{
	    mpi_errno = MPIDI_CH3I_SHM_Unlink_mem(&iter->shm_info);
	    if (mpi_errno != MPI_SUCCESS)
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**mqshm_unlink", 0);
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_MQSHM_UNLINK);
	    return MPI_SUCCESS;
	}
	iter = iter->next;
    }
    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**arg", 0);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_MQSHM_UNLINK);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_mqshm_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_mqshm_send(const int id, const void *buffer, const int length, const int tag, int *num_sent, int blocking)
{
    int mpi_errno = MPI_SUCCESS;
    mqshm_t *q_ptr;
    int index;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_MPIDI_CH3I_MQSHM_SEND);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_MPIDI_CH3I_MQSHM_SEND);
    if (length > BOOTSTRAP_MAX_MSG_SIZE)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**arg", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_MPIDI_CH3I_MQSHM_SEND);
	return mpi_errno;
    }
    /*printf("[%d] send: looking up id %d\n", MPIR_Process.comm_world->rank, id);fflush(stdout);*/
    q_ptr = id_to_queue(id);
    /*printf("[%d] send: id %d -> %p\n", MPIR_Process.comm_world->rank, id, q_ptr);fflush(stdout);*/
    if (q_ptr == NULL)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**arg", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_MPIDI_CH3I_MQSHM_SEND);
	return mpi_errno;
    }
    do
    {
	MPIDU_Process_lock(&q_ptr->lock);
	/*
	if (q_ptr->inuse)
	{
	    printf("Error, multiple processes acquired the lock.\n");
	    fflush(stdout);
	}
	q_ptr->inuse = 1;
	*/
	index = q_ptr->next_free;
	if (index != MQSHM_END)
	{
	    q_ptr->next_free = q_ptr->msg[index].next;
	    /*printf("[%d] send: writing %d bytes to index %d with tag %d\n", MPIR_Process.comm_world->rank, length, index, tag);fflush(stdout);*/
	    memcpy(q_ptr->msg[index].data, buffer, length);
	    q_ptr->msg[index].tag = tag;
	    q_ptr->msg[index].length = length;
	    q_ptr->msg[index].next = MQSHM_END;
	    if (q_ptr->first == MQSHM_END)
	    {
		/*printf("[%d] send: setting first and last to %d\n", MPIR_Process.comm_world->rank, index);fflush(stdout);*/
		q_ptr->first = index;
		q_ptr->last = index;
	    }
	    else
	    {
		/*printf("[%d] send: old_last = %d, new last = %d\n",
		  MPIR_Process.comm_world->rank, q_ptr->last, index);fflush(stdout);*/
		q_ptr->msg[q_ptr->last].next = index;
		q_ptr->last = index;
	    }
	    *num_sent = length;
	    /*q_ptr->inuse = 0;*/
	    MPIDU_Process_unlock(&q_ptr->lock);
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_MPIDI_CH3I_MQSHM_SEND);
	    return MPI_SUCCESS;
	}
	/*q_ptr->inuse = 0;*/
	MPIDU_Process_unlock(&q_ptr->lock);
	MPIDU_Yield();
    } while (blocking);
    *num_sent = 0;
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_MPIDI_CH3I_MQSHM_SEND);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_mqshm_receive
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_mqshm_receive(const int id, const int tag, void *buffer, const int maxlen, int *length, const int blocking)
{
    int mpi_errno = MPI_SUCCESS;
    mqshm_t *q_ptr;
    int index, last_index = MQSHM_END;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_MQSHM_RECEIVE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_MQSHM_RECEIVE);
    /*printf("[%d] recv: looking up id %d\n", MPIR_Process.comm_world->rank, id);fflush(stdout);*/
    q_ptr = id_to_queue(id);
    /*printf("[%d] recv: id %d -> %p\n", MPIR_Process.comm_world->rank, id, q_ptr);fflush(stdout);*/
    if (q_ptr == NULL)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**arg", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_MQSHM_RECEIVE);
	return mpi_errno;
    }
    do
    {
	MPIDU_Process_lock(&q_ptr->lock);
	/*
	if (q_ptr->inuse)
	{
	    printf("Error, multiple processes acquired the lock.\n");
	    fflush(stdout);
	}
	q_ptr->inuse = 1;
	*/
	index = q_ptr->first;
	while (index != MQSHM_END)
	{
	    /*printf("[%d] recv: checking if msg[%d].tag %d == %d\n",
	      MPIR_Process.comm_world->rank, index, q_ptr->msg[index].tag, tag);fflush(stdout);*/
	    if (q_ptr->msg[index].tag == tag)
	    {
		/* remove the node from the queue */
		if (last_index == MQSHM_END)
		{
		    /*printf("[%d] recv: removing index %d from the head\n", MPIR_Process.comm_world->rank, index);fflush(stdout);*/
		    q_ptr->first = q_ptr->msg[index].next;
		}
		else
		{
		    /*printf("[%d] recv: removing index %d\n", MPIR_Process.comm_world->rank, index);fflush(stdout);*/
		    q_ptr->msg[last_index].next = q_ptr->msg[index].next;
		}
		/* validate the message */
		if (maxlen < q_ptr->msg[index].length)
		{
		    /*q_ptr->inuse = 0;*/
		    MPIDU_Process_unlock(&q_ptr->lock);
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**arg", 0);
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_MQSHM_RECEIVE);
		    return mpi_errno;
		}
		/* copy the message */
		memcpy(buffer, q_ptr->msg[index].data, q_ptr->msg[index].length);
		*length = q_ptr->msg[index].length;
		/* add the node to the free list */
		q_ptr->msg[index].next = q_ptr->next_free;
		q_ptr->next_free = index;
		/*q_ptr->inuse = 0;*/
		MPIDU_Process_unlock(&q_ptr->lock);
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_MQSHM_RECEIVE);
		return mpi_errno;
	    }
	    last_index = index;
	    index = q_ptr->msg[index].next;
	}
	/*q_ptr->inuse = 0;*/
	MPIDU_Process_unlock(&q_ptr->lock);
	/*printf("<%d>", MPIR_Process.comm_world->rank);*/
	MPIDU_Yield();
    } while (blocking);
    *length = 0; /* zero length signals no message received? */
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_MQSHM_RECEIVE);
    return MPI_SUCCESS;
}
