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
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_GCC_AND_PENTIUM_ASM
#define HAVE_COMPARE_AND_SWAP
static inline char
__attribute__ ((unused))
     compare_and_swap (volatile long int *p, long int oldval, long int newval)
{
  char ret;
  long int readval;

  __asm__ __volatile__ ("lock; cmpxchgl %3, %1; sete %0"
                : "=q" (ret), "=m" (*p), "=a" (readval)
            : "r" (newval), "m" (*p), "a" (oldval));
  return ret;
}
#endif

/* This value is defined in sys/param.h under Linux but in netdb.h 
   under Solaris */
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif

#define MPIDI_CH3I_NUM_PACKETS    16
#define MPIDI_CH3I_PACKET_SIZE   (16*1024)
#define MPIDI_CH3I_PKT_AVAILABLE  0
#define MPIDI_CH3I_PKT_USED       1

typedef struct MPIDI_CH3I_SHM_Packet_t
{
    int avail;
    int num_bytes;
    char data[MPIDI_CH3I_PACKET_SIZE];
} MPIDI_CH3I_SHM_Packet_t;

typedef struct MPIDI_CH3I_SHM_Queue_t
{
    int head_index;
    int tail_index;
    MPIDI_CH3I_SHM_Packet_t packet[MPIDI_CH3I_NUM_PACKETS];
} MPIDI_CH3I_SHM_Queue_t;

typedef struct MPIDI_CH3I_Process_s
{
    MPIDI_CH3I_Process_group_t * pg;
    MPIDI_VC *unex_finished_list, *vc;
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

#define MPIDI_SHM_EAGER_LIMIT 10240
#ifdef HAVE_SHARED_PROCESS_READ
#define MPIDI_SHM_RNDV_LIMIT 10240
#endif

#define MPID_SHMEM_PER_PROCESS 1048576

typedef enum shm_wait_e
{
SHM_WAIT_TIMEOUT,
SHM_WAIT_READ,
SHM_WAIT_WRITE,
SHM_WAIT_ERROR
} shm_wait_t;

int MPIDI_CH3I_Progress_init(void);
int MPIDI_CH3I_Progress_finalize(void);
int MPIDI_CH3I_Request_adjust_iov(MPID_Request *, MPIDI_msg_sz_t);

void *MPIDI_CH3I_SHM_Get_mem_sync(MPIDI_CH3I_Process_group_t *pg, int nTotalSize, int nRank, int nNproc);
void MPIDI_CH3I_SHM_Release_mem(MPIDI_CH3I_Process_group_t *pg);

shm_wait_t MPIDI_CH3I_SHM_wait(MPIDI_VC *vc, int millisecond_timeout, MPIDI_VC **vc_pptr, int *num_bytes_ptr, int *error_ptr);
int MPIDI_CH3I_SHM_post_read(MPIDI_VC *vc, void *buf, int len, int (*read_progress_update)(int, void*));
int MPIDI_CH3I_SHM_post_readv(MPIDI_VC *vc, MPID_IOV *iov, int n, int (*read_progress_update)(int, void*));
int MPIDI_CH3I_SHM_write(MPIDI_VC *vc, void *buf, int len);
int MPIDI_CH3I_SHM_writev(MPIDI_VC *vc, MPID_IOV *iov, int n);
int MPIDI_CH3I_SHM_read(MPIDI_VC *vc, void *buf, int len);
int MPIDI_CH3I_SHM_readv(MPIDI_VC *vc, MPID_IOV *iov, int n);

#endif /* !defined(MPICH_MPIDI_CH3_IMPL_H_INCLUDED) */
