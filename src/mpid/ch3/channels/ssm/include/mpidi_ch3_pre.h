/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED)
#define MPICH_MPIDI_CH3_PRE_H_INCLUDED

#include "sock.h"
#ifdef USE_WINCONF_H
#include "winmpidi_ch3i_ssm_conf.h"
#include "winmpidi_ch3_conf.h"
#else
#include "mpidi_ch3i_ssm_conf.h"
#include "mpidi_ch3_conf.h"
#endif

#if defined (HAVE_SHM_OPEN) && defined (HAVE_MMAP)
#define USE_POSIX_SHM
#elif defined (HAVE_SHMGET) && defined (HAVE_SHMAT) && defined (HAVE_SHMCTL) && defined (HAVE_SHMDT)
#define USE_SYSV_SHM
#elif defined (HAVE_WINDOWS_H)
#define USE_WINDOWS_SHM
#else
#error No shared memory subsystem defined
#endif

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif

typedef struct MPIDI_CH3I_BootstrapQ_struct * MPIDI_CH3I_BootstrapQ;

typedef struct MPIDI_Process_group_s
{
    volatile int ref_count;
    char * kvs_name;
    int size;
    struct MPIDI_VC * vc_table;
    int nShmEagerLimit;
#ifdef HAVE_SHARED_PROCESS_READ
    int nShmRndvLimit;
#endif
    int nShmWaitSpinCount;
    int nShmWaitYieldCount;
    MPIDI_CH3I_BootstrapQ bootstrapQ;
    char shm_hostname[MAXHOSTNAMELEN];
}
MPIDI_CH3I_Process_group_t;

#define MPIDI_CH3_PKT_ENUM			\
MPIDI_CH3I_PKT_SC_OPEN_REQ,			\
MPIDI_CH3I_PKT_SC_OPEN_RESP,			\
MPIDI_CH3I_PKT_SC_CLOSE

#define MPIDI_CH3_PKT_DEFS													  \
typedef struct															  \
{																  \
    MPIDI_CH3_Pkt_type_t type;													  \
    /* FIXME - We need a little security here to avoid having a random port scan crash the process.  Perhaps a "secret" value for \
       each process could be published in the key-val space and subsequently sent in the open pkt. */				  \
																  \
    /* FIXME - We need some notion of a global process group ID so that we can tell the remote process which process is		  \
       connecting to it */													  \
    int pg_id;															  \
    int pg_rank;														  \
}																  \
MPIDI_CH3I_Pkt_sc_open_req_t;													  \
																  \
typedef struct															  \
{																  \
    MPIDI_CH3_Pkt_type_t type;													  \
    int ack;															  \
}																  \
MPIDI_CH3I_Pkt_sc_open_resp_t;													  \
																  \
typedef struct															  \
{																  \
    MPIDI_CH3_Pkt_type_t type;													  \
}																  \
MPIDI_CH3I_Pkt_sc_close_t;

#define MPIDI_CH3_PKT_DECL			\
MPIDI_CH3I_Pkt_sc_open_req_t sc_open_req;	\
MPIDI_CH3I_Pkt_sc_open_resp_t sc_open_resp;	\
MPIDI_CH3I_Pkt_sc_close_t sc_close;

typedef enum MPIDI_CH3I_VC_state
{
    MPIDI_CH3I_VC_STATE_UNCONNECTED,
    MPIDI_CH3I_VC_STATE_CONNECTING,
    MPIDI_CH3I_VC_STATE_CONNECTED,
    MPIDI_CH3I_VC_STATE_FAILED
}
MPIDI_CH3I_VC_state_t;

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

typedef struct MPIDI_CH3I_Shmem_block_request_result
{
    int error;
    void *addr;
    unsigned int size;
#ifdef USE_POSIX_SHM
    char key[100];
    int id;
#elif defined (USE_SYSV_SHM)
    int key;
    int id;
#elif defined (USE_WINDOWS_SHM)
    char key[MAX_PATH];
    HANDLE id;
#else
#error *** No shared memory mapping variables specified ***
#endif
} MPIDI_CH3I_Shmem_block_request_result;

typedef struct MPIDI_CH3I_VC
{
    MPIDI_CH3I_Process_group_t * pg;
    int pg_rank;
    struct MPIDI_CH3I_SHM_Queue_t * shm, * read_shmq, * write_shmq;
    struct MPID_Request * sendq_head;
    struct MPID_Request * sendq_tail;
    struct MPID_Request * send_active;
    struct MPID_Request * recv_active;
    struct MPID_Request * req;
    MPIDI_CH3I_VC_state_t state;
    sock_t sock;
    struct MPIDI_CH3I_Connection * conn;
    BOOL bShm;
    MPIDI_CH3I_Shmem_block_request_result shm_write_queue_info, shm_read_queue_info;
    int shm_reading_pkt;
    int shm_state;
    MPIDI_CH3I_SHM_Buffer_t read;
#ifdef HAVE_SHARED_PROCESS_READ
#ifdef HAVE_WINDOWS_H
    HANDLE hSharedProcessHandle;
#else
    int nSharedProcessID;
    int nSharedProcessFileDescriptor;
#endif
#endif
    struct MPIDI_VC *shm_next_reader, *shm_next_writer;
} MPIDI_CH3I_VC;

#define MPIDI_CH3_VC_DECL MPIDI_CH3I_VC ssm;


/*
 * MPIDI_CH3_CA_ENUM (additions to MPIDI_CA_t)
 *
 * MPIDI_CH3I_CA_HANDLE_PKT - The completion of a packet request (send or receive) needs to be handled.
 */
#define MPIDI_CH3_CA_ENUM			\
MPIDI_CH3I_CA_HANDLE_PKT,			\
MPIDI_CH3I_CA_END_SSM_CHANNEL


/*
 * MPIDI_CH3_REQUEST_DECL (additions to MPID_Request)
 */
#define MPIDI_CH3_REQUEST_DECL									\
struct MPIDI_CH3I_Request									\
{												\
    /* iov_offset points to the current head element in the IOV */				\
    int iov_offset;										\
												\
    /*  pkt is used to temporarily store a packet header associated with this request */	\
    MPIDI_CH3_Pkt_t pkt;									\
} ssm;

#define MPID_STATE_LIST_CH3 \
MPID_STATE_MPIDI_CH3_CANCEL_SEND, \
MPID_STATE_MPIDI_CH3_COMM_SPAWN, \
MPID_STATE_MPIDI_CH3_FINALIZE, \
MPID_STATE_MPIDI_CH3_INIT, \
MPID_STATE_MPIDI_CH3_IREAD, \
MPID_STATE_MPIDI_CH3_ISEND, \
MPID_STATE_MPIDI_CH3_ISENDV, \
MPID_STATE_MPIDI_CH3_ISTARTMSG, \
MPID_STATE_MPIDI_CH3_ISTARTMSGV, \
MPID_STATE_MPIDI_CH3_IWRITE, \
MPID_STATE_MPIDI_CH3_PROGRESS_INIT, \
MPID_STATE_MPIDI_CH3_PROGRESS_FINALIZE, \
MPID_STATE_MPIDI_CH3_PROGRESS_START, \
MPID_STATE_MPIDI_CH3_PROGRESS_END, \
MPID_STATE_MPIDI_CH3_PROGRESS, \
MPID_STATE_MPIDI_CH3_PROGRESS_POKE, \
MPID_STATE_MPIDI_CH3_REQUEST_CREATE, \
MPID_STATE_MPIDI_CH3_REQUEST_ADD_REF, \
MPID_STATE_MPIDI_CH3_REQUEST_RELEASE_REF, \
MPID_STATE_MPIDI_CH3_REQUEST_DESTROY, \
MPID_STATE_MPIDI_CH3I_LISTENER_GET_PORT, \
MPID_STATE_MPIDI_CH3I_PROGRESS_FINALIZE, \
MPID_STATE_MPIDI_CH3I_PROGRESS_INIT, \
MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT, \
MPID_STATE_MPIDI_CH3I_VC_POST_READ, \
MPID_STATE_MPIDI_CH3I_VC_POST_WRITE, \
MPID_STATE_MPIDI_CH3I_GET_BUSINESS_CARD, \
MPID_STATE_MPIDI_CH3U_BUFFER_COPY, \
MPID_STATE_CONNECTION_ALLOC, \
MPID_STATE_CONNECTION_FREE, \
MPID_STATE_CONNECTION_POST_SENDQ_REQ, \
MPID_STATE_CONNECTION_POST_SEND_PKT, \
MPID_STATE_CONNECTION_POST_RECV_PKT, \
MPID_STATE_CONNECTION_SEND_FAIL, \
MPID_STATE_CONNECTION_RECV_FAIL, \
MPID_STATE_UPDATE_REQUEST, \
MPID_STATE_MPIDI_CH3I_SHM_POST_READ, \
MPID_STATE_MPIDI_CH3I_SHM_POST_READV, \
MPID_STATE_MPIDI_CH3I_SHM_WRITE, \
MPID_STATE_MPIDI_CH3I_SHM_WRITEV, \
MPID_STATE_MPIDI_CH3I_SHM_READ, \
MPID_STATE_MPIDI_CH3I_SHM_READV, \
MPID_STATE_MPIDU_COMPARE_SWAP, \
MPID_STATE_MPIDU_PROCESS_LOCK_INIT, \
MPID_STATE_MPIDU_PROCESS_LOCK_FREE, \
MPID_STATE_MPIDU_PROCESS_LOCK, \
MPID_STATE_MPIDU_PROCESS_UNLOCK, \
MPID_STATE_MPIDU_PROCESS_LOCK_BUSY_WAIT, \
MPID_STATE_MPIDI_CH3I_SHM_ALLOC, \
MPID_STATE_MPIDI_CH3I_SHM_FREE, \
MPID_STATE_MPIDI_CH3I_SHM_GET_MEM, \
MPID_STATE_MPIDI_CH3I_SHM_GET_MEM_SYNC, \
MPID_STATE_MPIDI_CH3I_SHM_ATTACH_TO_MEM, \
MPID_STATE_MPIDI_CH3I_SHM_RELEASE_MEM, \
MPID_STATE_MPIDI_CH3I_SHM_WAIT, \
MPID_STATE_SHMI_READ_UNEX, \
MPID_STATE_SHMI_READV_UNEX, \
MPID_STATE_MPIDU_YIELD, \
MPID_STATE_MPIDU_SLEEP_YIELD, \
MPID_STATE_MEMCPY, \
MPID_STATE_GET_NEXT_BOOTSTRAP_MSG, \
MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE, \
MPID_STATE_MPIDI_CH3I_BOOTSRAPQ_TOSTRING, \
MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_DESTROY, \
MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH, \
MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_DETACH, \
MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_SEND_MSG, \
MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_RECV_MSG, \
MPID_STATE_HANDLE_SOCK_OP, \
MPID_STATE_HANDLE_SHM_READ, \
MPID_STATE_HANDLE_SHM_WRITTEN, \
MPID_STATE_MPIDI_CH3I_REQUEST_ADJUST_IOV, \
MPID_STATE_MPIDI_COMM_SPAWN, \
SOCK_STATE_LIST

#endif /* !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED) */
