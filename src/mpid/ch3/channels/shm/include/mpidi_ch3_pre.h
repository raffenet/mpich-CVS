/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED)
#define MPICH_MPIDI_CH3_PRE_H_INCLUDED

#include "mpidi_ch3i_shm_conf.h"
#include "mpid_locksconf.h"

/*#define MPICH_DBG_OUTPUT*/

#if defined (HAVE_SHM_OPEN) && defined (HAVE_MMAP)
#define USE_POSIX_SHM
#elif defined (HAVE_SHMGET) && defined (HAVE_SHMAT) && defined (HAVE_SHMCTL) && defined (HAVE_SHMDT)
#define USE_SYSV_SHM
#elif defined (HAVE_WINDOWS_H)
#define USE_WINDOWS_SHM
#else
#error No shared memory subsystem defined
#endif

#define MPIDI_MAX_SHM_NAME_LENGTH 100

#define SHM_SUCCESS    0
#define SHM_FAIL      -1

#ifdef HAVE_SHARED_PROCESS_READ
typedef struct MPIDI_CH3I_Shared_process
{
    int nRank;
#ifdef HAVE_WINDOWS_H
    DWORD nPid;
#else
    int nPid;
#endif
    BOOL bFinished;
} MPIDI_CH3I_Shared_process_t;
#endif

typedef struct MPIDI_Process_group_s
{
    volatile int ref_count;
    char * kvs_name;
    int nShmEagerLimit;
#ifdef HAVE_SHARED_PROCESS_READ
    int nShmRndvLimit;
    MPIDI_CH3I_Shared_process_t *pSHP;
#ifdef HAVE_WINDOWS_H
    HANDLE *pSharedProcessHandles;
#else
    int *pSharedProcessIDs;
    int *pSharedProcessFileDescriptors;
#endif
#endif
    void *addr;
#ifdef USE_POSIX_SHM
    char key[MPIDI_MAX_SHM_NAME_LENGTH];
    int id;
#elif defined (USE_SYSV_SHM)
    int key;
    int id;
#elif defined (USE_WINDOWS_SHM)
    char key[MPIDI_MAX_SHM_NAME_LENGTH];
    HANDLE id;
#else
#error *** No shared memory mapping variables specified ***
#endif
    int nShmWaitSpinCount;
    int nShmWaitYieldCount;
}
MPIDI_CH3I_Process_group_t;

#define MPIDI_CH3_PG_DECL MPIDI_CH3I_Process_group_t ch;

#ifdef USE_ALIGNED_PACKET_SIZE
typedef struct MPIDI_CH3_Pkt_max_size_aligned
{
    char pad[32];
} MPIDI_CH3_Pkt_max_size_aligned_t;

#define MPIDI_CH3_PKT_DECL MPIDI_CH3_Pkt_max_size_aligned_t dummy;
#else
#define MPIDI_CH3_PKT_DECL
#endif

#define MPIDI_CH3_PKT_ENUM
#define MPIDI_CH3_PKT_DEFS

/* This structure requires the iovec structure macros to be defined */
typedef struct MPIDI_CH3I_SHM_Buffer_t
{
    int use_iov;
    unsigned int num_bytes;
    void *buffer;
    unsigned int bufflen;
#ifdef USE_SHM_IOV_COPY
    MPID_IOV iov[MPID_IOV_LIMIT];
#else
    MPID_IOV *iov;
#endif
    int iovlen;
    int index;
    int total;
} MPIDI_CH3I_SHM_Buffer_t;

typedef struct MPIDI_CH3I_SHM_Unex_read_s
{
    struct MPIDI_CH3I_SHM_Packet_t *pkt_ptr;
    unsigned char *buf;
    unsigned int length;
    int src;
    struct MPIDI_CH3I_SHM_Unex_read_s *next;
} MPIDI_CH3I_SHM_Unex_read_t;

typedef struct MPIDI_CH3I_VC
{
    struct MPIDI_CH3I_SHM_Queue_t * shm, * read_shmq, * write_shmq;
    struct MPID_Request * sendq_head;
    struct MPID_Request * sendq_tail;
    struct MPID_Request * send_active;
    struct MPID_Request * recv_active;
    struct MPID_Request * req;
    int shm_reading_pkt;
    int shm_state;
    MPIDI_CH3I_SHM_Buffer_t read;
#ifdef USE_SHM_UNEX
    MPIDI_CH3I_SHM_Unex_read_t *unex_list;
    struct MPIDI_VC_t *unex_finished_next;
#endif
#ifdef HAVE_SHARED_PROCESS_READ
#ifdef HAVE_WINDOWS_H
    HANDLE hSharedProcessHandle;
#else
    int nSharedProcessID;
    int nSharedProcessFileDescriptor;
#endif
#endif
} MPIDI_CH3I_VC;

#define MPIDI_CH3_VC_DECL MPIDI_CH3I_VC ch;

/*
 * MPIDI_CH3_CA_ENUM (additions to MPIDI_CA_t)
 *
 * MPIDI_CH3I_CA_HANDLE_PKT - The completion of a packet request (send or
 * receive) needs to be handled.
 */
#define MPIDI_CH3_CA_ENUM			\
MPIDI_CH3I_CA_HANDLE_PKT,			\
MPIDI_CH3I_CA_END_SHM


/*
 * MPIDI_CH3_REQUEST_DECL (additions to MPID_Request)
 */
#define MPIDI_CH3_REQUEST_DECL						\
struct MPIDI_CH3I_Request						\
{									\
    /* iov_offset points to the current head element in the IOV */	\
    int iov_offset;							\
    									\
    /*  pkt is used to temporarily store a packet header associated	\
       with this request */						\
    MPIDI_CH3_Pkt_t pkt;						\
} ch;

/*
 * MPID_Progress_state - device/channel dependent state to be passed between MPID_Progress_{start,wait,end}
 */
typedef struct MPIDI_CH3I_Progress_state
{
    int completion_count;
} MPIDI_CH3I_Progress_state;

#define MPIDI_CH3_PROGRESS_STATE_DECL MPIDI_CH3I_Progress_state ch;

#endif /* !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED) */
