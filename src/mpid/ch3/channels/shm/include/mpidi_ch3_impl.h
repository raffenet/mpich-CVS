/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_IMPL_H_INCLUDED)
#define MPICH_MPIDI_CH3_IMPL_H_INCLUDED

#include "mpidi_ch3i_shm_conf.h"
#include "mpidimpl.h"
#include "mpidu_process_locks.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>
#include <assert.h>

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif
#ifdef HAVE_SYS_IPC_H
#include <sys/ipc.h>
#endif
#ifdef HAVE_SYS_SHM_H
#include <sys/shm.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

/* This value is defined in sys/param.h under Linux but in netdb.h 
   under Solaris */
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif
typedef struct MPIDI_CH3I_Process_s
{
    MPIDI_CH3I_Process_group_t * pg;
}
MPIDI_CH3I_Process_t;

extern MPIDI_CH3I_Process_t MPIDI_CH3I_Process;

#define MPIDI_CH3I_SendQ_enqueue(vc, req)				\
{									\
    /* MT - not thread safe! */						\
    MPIDI_DBG_PRINTF((50, FCNAME, "SendQ_enqueue vc=0x%08x req=0x%08x",	\
	              (unsigned long) vc, req->handle));		\
    req->ch3.next = NULL;						\
    if (vc->shm.sendq_tail != NULL)					\
    {									\
	vc->shm.sendq_tail->ch3.next = req;				\
    }									\
    else								\
    {									\
	vc->shm.sendq_head = req;					\
    }									\
    vc->shm.sendq_tail = req;						\
}

#define MPIDI_CH3I_SendQ_enqueue_head(vc, req)				     \
{									     \
    /* MT - not thread safe! */						     \
    MPIDI_DBG_PRINTF((50, FCNAME, "SendQ_enqueue_head vc=0x%08x req=0x%08x", \
	              (unsigned long) vc, req->handle));		     \
    req->ch3.next = vc->shm.sendq_tail;					     \
    if (vc->shm.sendq_tail == NULL)					     \
    {									     \
	vc->shm.sendq_tail = req;					     \
    }									     \
    vc->shm.sendq_head = req;						     \
}

#define MPIDI_CH3I_SendQ_dequeue(vc)					\
{									\
    /* MT - not thread safe! */						\
    MPIDI_DBG_PRINTF((50, FCNAME, "SendQ_dequeue vc=0x%08x req=0x%08x",	\
	              (unsigned long) vc, vc->shm.sendq_head));		\
    vc->shm.sendq_head = vc->shm.sendq_head->ch3.next;			\
    if (vc->shm.sendq_head == NULL)					\
    {									\
	vc->shm.sendq_tail = NULL;					\
    }									\
}

#define MPIDI_CH3I_SendQ_head(vc) (vc->shm.sendq_head)

#define MPIDI_CH3I_SendQ_empty(vc) (vc->shm.sendq_head == NULL)

/* global variables */
extern void *MPIDI_Shm_addr, *MPIDI_Shm_my_curr_addr;
#ifdef HAVE_SHMGET
extern int MPIDI_Shm_key;
extern int MPIDI_Shm_id;
#elif defined (HAVE_MAPVIEWOFFILE)
extern char MPIDI_Shm_key[MAX_PATH];
extern HANDLE MPIDI_Shm_id;
#else
#error You must have some shared memory variables
#endif
extern unsigned int MPIDI_Shm_size, MPIDI_Shm_my_rem_size;
#ifdef USE_GARBAGE_COLLECTING
extern int MPIDI_Shm_gc_count;
extern MPIDU_Process_lock_t MPIDI_Shm_gc_lock;     /* lock for garbage collection counter */
#endif
extern int MPIDI_bUseShm;
extern int g_nShmWaitSpinCount;

extern MPIDU_Process_lock_t MPIDI_Shm_addr_lock;   /* lock for local shared memory pool size (remaining) */

#if 0
extern MPIDU_Queue *MPIDI_Shm_recv_queues;  /* array of recv queues of all  
                                               processes that share memory */

extern MPIDU_Queue MPIDI_Shm_active_queue;  /* queue of active rendezvous requests that
                                               MPID_SHM_Test must cause progress on */
#endif

#ifdef USE_GARBAGE_COLLECTING
typedef struct MPIDI_Shm_mem_obj_hdr_st {
    struct MPIDI_Shm_mem_obj_hdr_st *next;
    int inuse;
} MPIDI_Shm_mem_obj_hdr;
#else
typedef struct MPIDI_Shm_mem_obj_hdr_st {
    struct MPIDI_Shm_mem_obj_hdr_st *next;
    int index;
} MPIDI_Shm_mem_obj_hdr;
#endif

typedef struct {
    unsigned int size;
    MPIDU_Process_lock_t thr_lock;
    int count;
    int max_count;
    MPIDI_Shm_mem_obj_hdr *ptr;
} MPIDI_Shm_list;

extern MPIDI_Shm_list *MPIDI_Shm_free_list;
#ifdef USE_GARBAGE_COLLECTING
extern MPIDI_Shm_list *MPIDI_Shm_inuse_list;
#endif

#define MPIDI_SHM_SHORT_LIMIT MPIDI_SHORT_MSG_LIMIT
#define MPIDI_SHM_EAGER_LIMIT 10240
extern int g_nShmEagerLimit;
#ifdef HAVE_SHARED_PROCESS_READ
#define MPIDI_SHM_RNDV_LIMIT 10240
extern int g_nShmRndvLimit;
#endif

#define MPIDI_SHM_GC_COUNT_MAX 10
#define MPID_SHMEM_PER_PROCESS 1048576

/* sizes of shared memory segments used by the memory management algorithm */
#define SHM_NSIZES 6
#define SHM_SIZE1 64
#define SHM_SIZE2 256
#define SHM_SIZE3 1024
#define SHM_SIZE4 4096
#define SHM_SIZE5 16384
#define SHM_MAX_SIZE 262144

int MPIDI_CH3I_Progress_init(void);
int MPIDI_CH3I_Progress_finalize(void);
int MPIDI_CH3I_Request_adjust_iov(MPID_Request *, MPIDI_msg_sz_t);
int MPIDI_CH3I_Setup_connections();

void *MPIDI_CH3I_SHM_Alloc(unsigned int size);
void MPIDI_CH3I_SHM_Free(void *address);
void *MPIDI_CH3I_SHM_Get_mem_sync(int nTotalSize, int nRank, int nNproc);
void MPIDI_CH3I_SHM_Release_mem();

int MPIDI_CH3I_SHM_init(void);
int MPIDI_CH3I_SHM_finalize(void);
int MPIDI_CH3I_SHM_set_user_ptr(shm_t shm, void *user_ptr);
int MPIDI_CH3I_SHM_post_read(shm_t shm, void *buf, int len, int (*read_progress_update)(int, void*));
int MPIDI_CH3I_SHM_post_readv(shm_t shm, MPID_IOV *iov, int n, int (*read_progress_update)(int, void*));
int MPIDI_CH3I_SHM_write(shm_t shm, void *buf, int len);
int MPIDI_CH3I_SHM_writev(shm_t shm, MPID_IOV *iov, int n);

#endif /* !defined(MPICH_MPIDI_CH3_IMPL_H_INCLUDED) */
