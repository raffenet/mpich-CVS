/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * WARNING: Functions and macros in this file are for internal use only.  As such, they are only visible to the device and
 * channel.  Do not include then in the MPID macros.
 */

/* XXX - move these to mpiimpl.h??? */
/* 
 * Note: Never define the feature set in a header file, since this changes
 * the language accepted by the C compiler and the contents of the headers
 * seen by the C preprocessor.  Defining any of these renders the work of
 * configure irrelevant.
 */
#if 0
#if !defined(_XOPEN_SOURCE)
#define _XOPEN_SOURCE
#endif
#if !defined(_BSD_SOURCE)
#define _BSD_SOURCE
#endif
#endif

#if !defined(MPICH_MPIDIMPL_H_INCLUDED)
#define MPICH_MPIDIMPL_H_INCLUDED

#if defined(HAVE_ASSERT_H)
#include <assert.h>
#endif

#include "mpiimpl.h"


#if !defined(MPIDI_IOV_DENSITY_MIN)
#   define MPIDI_IOV_DENSITY_MIN 128
#endif


typedef struct MPIDI_Process
{
    MPIDI_PG_t * my_pg;
    int my_pg_rank;
    MPID_Request * recvq_posted_head;
    MPID_Request * recvq_posted_tail;
    MPID_Request * recvq_unexpected_head;
    MPID_Request * recvq_unexpected_tail;
    int lpid_counter;
    char * processor_name;
    int warnings_enabled;
}
MPIDI_Process_t;

extern MPIDI_Process_t MPIDI_Process;

extern volatile int MPIDI_Outstanding_close_ops;


/*----------------------
  BEGIN DATATYPE SECTION
  ----------------------*/
#define MPIDI_Datatype_get_info(count_, datatype_, dt_contig_out_, data_sz_out_, dt_ptr_, dt_true_lb_)			\
{															\
    if (HANDLE_GET_KIND(datatype_) == HANDLE_KIND_BUILTIN)								\
    {															\
	(dt_ptr_) = NULL;												\
	(dt_contig_out_) = TRUE;											\
        (dt_true_lb_)    = 0;                                                                                           \
	(data_sz_out_) = (count_) * MPID_Datatype_get_basic_size(datatype_);						\
	MPIDI_DBG_PRINTF((15, FCNAME, "basic datatype: dt_contig=%d, dt_sz=%d, data_sz=" MPIDI_MSG_SZ_FMT,		\
			  (dt_contig_out_), MPID_Datatype_get_basic_size(datatype_), (data_sz_out_)));			\
    }															\
    else														\
    {															\
	MPID_Datatype_get_ptr((datatype_), (dt_ptr_));									\
	(dt_contig_out_) = (dt_ptr_)->is_contig;									\
	(data_sz_out_) = (count_) * (dt_ptr_)->size;									\
        (dt_true_lb_)    = (dt_ptr_)->true_lb;                                                                          \
	MPIDI_DBG_PRINTF((15, FCNAME, "user defined datatype: dt_contig=%d, dt_sz=%d, data_sz=" MPIDI_MSG_SZ_FMT,	\
			  (dt_contig_out_), (dt_ptr_)->size, (data_sz_out_)));						\
    }															\
}
/*--------------------
  END DATATYPE SECTION
  --------------------*/


/*---------------------
  BEGIN REQUEST SECTION
  ---------------------*/
#define MPIDI_CH3U_Request_create(req_)				\
{								\
    MPID_Request_construct(req_);				\
    MPIU_Object_set_ref((req_), 1);				\
    (req_)->kind = MPID_REQUEST_UNDEFINED;			\
    (req_)->cc = 1;						\
    (req_)->cc_ptr = &(req_)->cc;				\
    (req_)->status.MPI_SOURCE = MPI_UNDEFINED;			\
    (req_)->status.MPI_TAG = MPI_UNDEFINED;			\
    (req_)->status.MPI_ERROR = MPI_SUCCESS;			\
    (req_)->status.count = 0;					\
    (req_)->status.cancelled = FALSE;				\
    (req_)->comm = NULL;					\
    (req_)->dev.datatype_ptr = NULL;				\
    MPIDI_Request_state_init((req_));				\
    (req_)->dev.cancel_pending = FALSE;				\
    (req_)->dev.target_win_handle = MPI_WIN_NULL;               \
    (req_)->dev.source_win_handle = MPI_WIN_NULL;               \
    (req_)->dev.single_op_opt = 0;                              \
    (req_)->dev.lock_queue_entry = NULL;                        \
    (req_)->dev.dtype_info = NULL;				\
    (req_)->dev.dataloop = NULL;				\
    (req_)->dev.rdma_iov_count = 0;				\
    (req_)->dev.rdma_iov_offset = 0;				\
}

#define MPIDI_CH3U_Request_destroy(req_)			\
{								\
    if ((req_)->comm != NULL)					\
    {								\
	MPIR_Comm_release((req_)->comm);			\
    }								\
								\
    if ((req_)->dev.datatype_ptr != NULL)			\
    {								\
	MPID_Datatype_release((req_)->dev.datatype_ptr);	\
    }								\
								\
    if (MPIDI_Request_get_srbuf_flag(req_))			\
    {								\
	MPIDI_CH3U_SRBuf_free(req_);				\
    }								\
								\
    MPID_Request_destruct(req_);				\
}

#define MPIDI_CH3U_Request_complete(req_)			\
{								\
    int incomplete__;						\
								\
    MPIDI_CH3U_Request_decrement_cc((req_), &incomplete__);	\
    if (!incomplete__)						\
    {								\
	MPID_Request_release(req_);				\
	MPIDI_CH3_Progress_signal_completion();			\
    }								\
}

#define MPIDI_Request_create_sreq(sreq_, mpi_errno_, FAIL_)			\
{										\
    (sreq_) = MPIDI_CH3_Request_create();					\
    if ((sreq_) == NULL)							\
    {										\
	MPIDI_DBG_PRINTF((15, FCNAME, "send request allocation failed"));	\
	(mpi_errno_) = MPIR_ERR_MEMALLOCFAILED;					\
	FAIL_;									\
    }										\
    										\
    MPIU_Object_set_ref((sreq_), 2);						\
    (sreq_)->kind = MPID_REQUEST_SEND;						\
    (sreq_)->comm = comm;							\
    MPIR_Comm_add_ref(comm);							\
    (sreq_)->dev.match.rank = rank;						\
    (sreq_)->dev.match.tag = tag;						\
    (sreq_)->dev.match.context_id = comm->context_id + context_offset;		\
    (sreq_)->dev.user_buf = (void *) buf;					\
    (sreq_)->dev.user_count = count;						\
    (sreq_)->dev.datatype = datatype;						\
}

#define MPIDI_Request_create_psreq(sreq_, mpi_errno_, FAIL_)			\
{										\
    (sreq_) = MPIDI_CH3_Request_create();					\
    if ((sreq_) == NULL)							\
    {										\
	MPIDI_DBG_PRINTF((15, FCNAME, "send request allocation failed"));	\
	(mpi_errno_) = MPIR_ERR_MEMALLOCFAILED;					\
	FAIL_;									\
    }										\
										\
    MPIU_Object_set_ref((sreq_), 1);						\
    (sreq_)->kind = MPID_PREQUEST_SEND;						\
    (sreq_)->comm = comm;							\
    MPIR_Comm_add_ref(comm);							\
    (sreq_)->dev.match.rank = rank;						\
    (sreq_)->dev.match.tag = tag;						\
    (sreq_)->dev.match.context_id = comm->context_id + context_offset;		\
    (sreq_)->dev.user_buf = (void *) buf;					\
    (sreq_)->dev.user_count = count;						\
    (sreq_)->dev.datatype = datatype;						\
    (sreq_)->partner_request = NULL;						\
}

/* Masks and flags for channel device state in an MPID_Request */
#define MPIDI_Request_state_init(req_)		\
{						\
    (req_)->dev.state = 0;			\
}

#define MPIDI_REQUEST_MSG_MASK (0x3 << MPIDI_REQUEST_MSG_SHIFT)
#define MPIDI_REQUEST_MSG_SHIFT 0
#define MPIDI_REQUEST_NO_MSG 0
#define MPIDI_REQUEST_EAGER_MSG 1
#define MPIDI_REQUEST_RNDV_MSG 2
#define MPIDI_REQUEST_SELF_MSG 3

#define MPIDI_Request_get_msg_type(req_)					\
(((req_)->dev.state & MPIDI_REQUEST_MSG_MASK) >> MPIDI_REQUEST_MSG_SHIFT)

#define MPIDI_Request_set_msg_type(req_, msgtype_)						\
{												\
    (req_)->dev.state &= ~MPIDI_REQUEST_MSG_MASK;						\
    (req_)->dev.state |= ((msgtype_) << MPIDI_REQUEST_MSG_SHIFT) & MPIDI_REQUEST_MSG_MASK;	\
}

#define MPIDI_REQUEST_SRBUF_MASK (0x1 << MPIDI_REQUEST_SRBUF_SHIFT)
#define MPIDI_REQUEST_SRBUF_SHIFT 2

#define MPIDI_Request_get_srbuf_flag(req_)					\
(((req_)->dev.state & MPIDI_REQUEST_SRBUF_MASK) >> MPIDI_REQUEST_SRBUF_SHIFT)

#define MPIDI_Request_set_srbuf_flag(req_, flag_)						\
{												\
    (req_)->dev.state &= ~MPIDI_REQUEST_SRBUF_MASK;						\
    (req_)->dev.state |= ((flag_) << MPIDI_REQUEST_SRBUF_SHIFT) & MPIDI_REQUEST_SRBUF_MASK;	\
}

#define MPIDI_REQUEST_SYNC_SEND_MASK (0x1 << MPIDI_REQUEST_SYNC_SEND_SHIFT)
#define MPIDI_REQUEST_SYNC_SEND_SHIFT 3

#define MPIDI_Request_get_sync_send_flag(req_)						\
(((req_)->dev.state & MPIDI_REQUEST_SYNC_SEND_MASK) >> MPIDI_REQUEST_SYNC_SEND_SHIFT)

#define MPIDI_Request_set_sync_send_flag(req_, flag_)							\
{													\
    (req_)->dev.state &= ~MPIDI_REQUEST_SYNC_SEND_MASK;							\
    (req_)->dev.state |= ((flag_) << MPIDI_REQUEST_SYNC_SEND_SHIFT) & MPIDI_REQUEST_SYNC_SEND_MASK;	\
}

#define MPIDI_REQUEST_TYPE_MASK (0xF << MPIDI_REQUEST_TYPE_SHIFT)
#define MPIDI_REQUEST_TYPE_SHIFT 4
#define MPIDI_REQUEST_TYPE_RECV 0
#define MPIDI_REQUEST_TYPE_SEND 1
#define MPIDI_REQUEST_TYPE_RSEND 2
#define MPIDI_REQUEST_TYPE_SSEND 3
/* We need a BSEND type for persistent bsends (see mpid_startall.c) */
#define MPIDI_REQUEST_TYPE_BSEND 4
#define MPIDI_REQUEST_TYPE_PUT_RESP 5
#define MPIDI_REQUEST_TYPE_GET_RESP 6
#define MPIDI_REQUEST_TYPE_ACCUM_RESP 7
#define MPIDI_REQUEST_TYPE_PUT_RESP_DERIVED_DT 8
#define MPIDI_REQUEST_TYPE_GET_RESP_DERIVED_DT 9
#define MPIDI_REQUEST_TYPE_ACCUM_RESP_DERIVED_DT 10
#define MPIDI_REQUEST_TYPE_PT_SINGLE_PUT 11
#define MPIDI_REQUEST_TYPE_PT_SINGLE_ACCUM 12


#define MPIDI_Request_get_type(req_)						\
(((req_)->dev.state & MPIDI_REQUEST_TYPE_MASK) >> MPIDI_REQUEST_TYPE_SHIFT)

#define MPIDI_Request_set_type(req_, type_)							\
{												\
    (req_)->dev.state &= ~MPIDI_REQUEST_TYPE_MASK;						\
    (req_)->dev.state |= ((type_) << MPIDI_REQUEST_TYPE_SHIFT) & MPIDI_REQUEST_TYPE_MASK;	\
}

#if defined(MPICH_SINGLE_THREADED)
#define MPIDI_Request_cancel_pending(req_, flag_)	\
{							\
    *(flag_) = (req_)->dev.cancel_pending;		\
    (req_)->dev.cancel_pending = TRUE;			\
}
#else
/* MT: to make this code lock free, an atomic exchange can be used. */ 
#define MPIDI_Request_cancel_pending(req_, flag_)	\
{							\
    MPID_Request_thread_lock(req_);			\
    {							\
	*(flag_) = (req_)->dev.cancel_pending;		\
	(req_)->dev.cancel_pending = TRUE;		\
    }							\
    MPID_Request_thread_unlock(req_);			\
}
#endif

#if defined(MPICH_SINGLE_THREADED)
#   define MPIDI_Request_recv_pending(req_, recv_pending_)	\
    {								\
 	*(recv_pending_) = --(req_)->dev.recv_pending_count;	\
    }
#elif defined(USE_ATOMIC_UPDATES)
#   define MPIDI_Request_recv_pending(req_, recv_pending_)			\
    {										\
    	int recv_pending__;							\
										\
    	MPID_Atomic_decr_flag(&(req_)->dev.recv_pending_count, recv_pending__);	\
    	*(recv_pending_) = recv_pending__;					\
    }
#else
#   define MPIDI_Request_recv_pending(req_, recv_pending_)		\
    {									\
    	MPID_Request_thread_lock(req_);					\
    	{								\
    	    *(recv_pending_) = --(req_)->dev.recv_pending_count;	\
    	}								\
    	MPID_Request_thread_unlock(req_);				\
    }
#endif

/* MPIDI_Request_fetch_and_clear_rts_sreq() - atomically fetch current partner RTS sreq and nullify partner request */
#if defined(MPICH_SINGLE_THREADED)
#   define MPIDI_Request_fetch_and_clear_rts_sreq(sreq_, rts_sreq_)	\
    {									\
    	*(rts_sreq_) = (sreq_)->partner_request;			\
    	(sreq_)->partner_request = NULL;				\
    }
#else
    /* MT: to make this code lock free, an atomic exchange can be used. */
#   define MPIDI_Request_fetch_and_clear_rts_sreq(sreq_, rts_sreq_)	\
    {									\
    	MPID_Request_thread_lock(sreq_);				\
    	{								\
    	    *(rts_sreq_) = (sreq_)->partner_request;			\
    	    (sreq_)->partner_request = NULL;				\
    	}								\
    	MPID_Request_thread_unlock(sreq_);				\
    }
#endif

#if defined(MPID_USE_SEQUENCE_NUMBERS)
#   define MPIDI_Request_set_seqnum(req_, seqnum_)	\
    {							\
    	(req_)->dev.seqnum = (seqnum_);			\
    }
#else
#   define MPIDI_Request_set_seqnum(req_, seqnum_)
#endif
/*-------------------
  END REQUEST SECTION
  -------------------*/


/*------------------
  BEGIN COMM SECTION
  ------------------*/
#define MPIDI_Comm_get_vc(comm_, rank_, vcp_)		\
{							\
    *(vcp_) = (comm_)->vcr[(rank_)];			\
    if ((*(vcp_))->state == MPIDI_VC_STATE_INACTIVE)	\
    {							\
	(*(vcp_))->state = MPIDI_VC_STATE_ACTIVE;	\
    }							\
}
/*----------------
  END COMM SECTION
  ----------------*/


/*--------------------
  BEGIN PACKET SECTION
  --------------------*/
#if !defined(MPICH_DEBUG_MEMINIT)
#   define MPIDI_Pkt_init(pkt_, type_)		\
    {						\
	(pkt_)->type = (type_);			\
    }
#else
#   define MPIDI_Pkt_init(pkt_, type_)				\
    {								\
	memset((void *) (pkt_), 0xfc, sizeof(MPIDI_CH3_Pkt_t));	\
	(pkt_)->type = (type_);					\
    }
#endif

#if defined(MPID_USE_SEQUENCE_NUMBERS)
#   define MPIDI_Pkt_set_seqnum(pkt_, seqnum_)	\
    {						\
    	(pkt_)->seqnum = (seqnum_);		\
    }
#else
#   define MPIDI_Pkt_set_seqnum(pkt_, seqnum_)
#endif
/*------------------
  END PACKET SECTION
  ------------------*/


/*---------------------------
  BEGIN PROCESS GROUP SECTION
  ---------------------------*/
typedef int (*MPIDI_PG_Compare_ids_fn_t)(void * id1, void * id2);
typedef int (*MPIDI_PG_Destroy_fn_t)(MPIDI_PG_t * pg, void * id);

int MPIDI_PG_Init(MPIDI_PG_Compare_ids_fn_t compare_ids_fn, MPIDI_PG_Destroy_fn_t destroy_fn);
int MPIDI_PG_Finalize(void);
int MPIDI_PG_Create(int vct_sz, void * pg_id, MPIDI_PG_t ** ppg);
int MPIDI_PG_Destroy(MPIDI_PG_t * pg);
void MPIDI_PG_Add_ref(MPIDI_PG_t * pg);
void MPIDI_PG_Release_ref(MPIDI_PG_t * pg, int * inuse);
int MPIDI_PG_Find(void * id, MPIDI_PG_t ** pgp);
int MPIDI_PG_Get_next(MPIDI_PG_t ** pgp);
int MPIDI_PG_Get_vc(MPIDI_PG_t * pg, int rank, MPIDI_VC_t ** vc);
int MPIDI_PG_Get_size(MPIDI_PG_t * pg);

#define MPIDI_PG_Add_ref(pg_)			\
{						\
    MPIU_Object_add_ref(pg_);			\
}
#define MPIDI_PG_Release_ref(pg_, inuse_)	\
{						\
    MPIU_Object_release_ref(pg_, inuse_);	\
}
#define MPIDI_PG_Get_vc(pg_, rank_, vcp_)		\
{							\
    *(vcp_) = &(pg_)->vct[rank_];			\
    if ((*(vcp_))->state == MPIDI_VC_STATE_INACTIVE)	\
    {							\
	(*(vcp_))->state = MPIDI_VC_STATE_ACTIVE;	\
    }							\
}
#define MPIDI_PG_Get_vcr(pg_, rank_, vcp_)		\
{							\
    *(vcp_) = &(pg_)->vct[rank_];			\
}
#define MPIDI_PG_Get_size(pg_) ((pg_)->size)
/*-------------------------
  END PROCESS GROUP SECTION
  -------------------------*/


/*--------------------------------
  BEGIN VIRTUAL CONNECTION SECTION
  --------------------------------*/
#if defined(MPICH_SINGLE_THREADED)
#   define MPIDI_VC_Get_next_lpid(lpid_ptr_)		\
    {							\
    	*(lpid_ptr_) = MPIDI_Process.lpid_counter++;	\
    }
#else
#   error thread safe MPIDI_CH3U_Get_next_lpid() not implemented
#endif

#if defined(MPID_USE_SEQUENCE_NUMBERS)
#   define MPIDI_VC_Init_seqnum_send(vc_)	\
    {						\
    	(vc_)->seqnum_send = 0;			\
    }
#else
#   define MPIDI_VC_Init_seqnum_send(vc_)
#endif

#if defined(MPIDI_CH3_MSGS_UNORDERED)
#   define MPIDI_VC_Init_seqnum_recv(vc_);	\
    {						\
    	(vc_)->seqnum_recv = 0;			\
    	(vc_)->msg_reorder_queue = NULL;	\
    }
#else
#   define MPIDI_VC_Init_seqnum_recv(vc_);
#endif

#define MPIDI_VC_Init(vc_, pg_, rank_)		\
{						\
    (vc_)->state = MPIDI_VC_STATE_INACTIVE;	\
    MPIU_Object_set_ref((vc_), 0);		\
    (vc_)->pg = (pg_);				\
    (vc_)->pg_rank = (rank_);			\
    MPIDI_VC_Get_next_lpid(&(vc_)->lpid);	\
    MPIDI_VC_Init_seqnum_send(vc_);		\
    MPIDI_VC_Init_seqnum_recv(vc_);		\
}

#if defined(MPID_USE_SEQUENCE_NUMBERS)
#   if defined(MPICH_SINGLE_THREADED)
#       define MPIDI_VC_FAI_send_seqnum(vc_, seqnum_out_)	\
        {							\
	    (seqnum_out_) = (vc_)->seqnum_send++;		\
	}
#   elif defined(USE_ATOMIC_UPDATES)
#       define MPIDI_VC_FAI_send_seqnum(vc_, seqnum_out_)			\
	{									\
	    MPID_Atomic_fetch_and_incr(&(vc_)->seqnum_send, (seqnum_out_));	\
	}
#   else
        /* FIXME: a VC specific mutex could be used if contention is a problem. */
#	define MPIDI_VC_FAI_send_seqnum(vc_, seqnum_out_)	\
	{							\
	    MPID_Common_thread_lock();				\
	    {							\
		(seqnum_out_) = (vc_)->seqnum_send++;		\
	    }							\
	    MPID_Common_thread_unlock();			\
	}
#    endif
#else
#    define MPIDI_VC_FAI_send_seqnum(vc_, seqnum_out_)
#endif
/*------------------------------
  END VIRTUAL CONNECTION SECTION
  ------------------------------*/


/*---------------------------------
  BEGIN SEND/RECEIVE BUFFER SECTION
  ---------------------------------*/
#if !defined(MPIDI_CH3U_SRBuf_size)
#    define MPIDI_CH3U_SRBuf_size (16384)
#endif

#if !defined(MPIDI_CH3U_SRBuf_alloc)
#   define MPIDI_CH3U_SRBuf_alloc(req_, size_)				\
    {									\
 	(req_)->dev.tmpbuf = MPIU_Malloc(MPIDI_CH3U_SRBuf_size);	\
 	if ((req_)->dev.tmpbuf != NULL)					\
 	{								\
 	    (req_)->dev.tmpbuf_sz = MPIDI_CH3U_SRBuf_size;		\
 	    MPIDI_Request_set_srbuf_flag((req_), TRUE);			\
 	}								\
 	else								\
 	{								\
 	    (req_)->dev.tmpbuf_sz = 0;					\
 	}								\
    }
#endif

#if !defined(MPIDI_CH3U_SRBuf_free)
#   define MPIDI_CH3U_SRBuf_free(req_)				\
    {								\
    	MPIU_Assert(MPIDI_Request_get_srbuf_flag(req_));	\
    	MPIDI_Request_set_srbuf_flag((req_), FALSE);		\
    	MPIU_Free((req_)->dev.tmpbuf);				\
    }
#endif
/*-------------------------------
  END SEND/RECEIVE BUFFER SECTION
  -------------------------------*/


/*----------------------------
  BEGIN DEBUGGING TOOL SECTION
  ----------------------------*/
void MPIDI_dbg_printf(int, char *, char *, ...);
void MPIDI_err_printf(char *, char *, ...);

#if defined(MPICH_DBG_OUTPUT)
#define MPIDI_DBG_PRINTF(e_)				\
{                                               	\
    if (MPIUI_dbg_state != MPIU_DBG_STATE_NONE)		\
    {							\
	MPIDI_dbg_printf e_;				\
    }							\
}
#else
#   define MPIDI_DBG_PRINTF(e)
#endif

#define MPIDI_ERR_PRINTF(e) MPIDI_err_printf e

#if defined(HAVE_CPP_VARARGS)
#   define MPIDI_dbg_printf(level, func, fmt, args...)							\
    {													\
    	MPIU_dbglog_printf("[%d] %s(): " fmt "\n", MPIR_Process.comm_world->rank, func, ## args);	\
    }
#   define MPIDI_err_printf(func, fmt, args...)									\
    {														\
    	MPIU_Error_printf("[%d] ERROR - %s(): " fmt "\n", MPIR_Process.comm_world->rank, func, ## args);	\
    	fflush(stdout);												\
    }
#endif

#define MPIDI_QUOTE(A) MPIDI_QUOTE2(A)
#define MPIDI_QUOTE2(A) #A

#ifdef MPICH_DBG_OUTPUT
    void MPIDI_DBG_Print_packet(MPIDI_CH3_Pkt_t *pkt);
#else
#   define MPIDI_DBG_Print_packet(a)
#endif

const char * MPIDI_VC_Get_state_description(int state);
/*--------------------------
  END DEBUGGING TOOL SECTION
  --------------------------*/


/* Prototypes for internal device routines */
int MPIDI_Isend_self(const void *, int, MPI_Datatype, int, int, MPID_Comm *, int, int, MPID_Request **);


/* ------------------------------------------------------------------------- */
/* mpirma.h (in src/mpi/rma?) */
/* ------------------------------------------------------------------------- */

#define MPIDI_RMA_PUT 23
#define MPIDI_RMA_GET 24
#define MPIDI_RMA_ACCUMULATE 25
#define MPIDI_RMA_LOCK 26
#define MPIDI_RMA_DATATYPE_BASIC 50
#define MPIDI_RMA_DATATYPE_DERIVED 51

#define MPID_LOCK_NONE 0

int MPIDI_CH3I_Send_rma_msg(MPIDI_RMA_ops * rma_op, MPID_Win * win_ptr, MPI_Win source_win_handle, MPI_Win target_win_handle, 
                            MPIDI_RMA_dtype_info * dtype_info, void ** dataloop, MPID_Request ** request);

int MPIDI_CH3I_Recv_rma_msg(MPIDI_RMA_ops * rma_op, MPID_Win * win_ptr, MPI_Win source_win_handle, MPI_Win target_win_handle, 
                            MPIDI_RMA_dtype_info * dtype_info, void ** dataloop, MPID_Request ** request); 

int MPIDI_CH3I_Release_lock(MPID_Win * win_ptr);

int MPIDI_CH3I_Try_acquire_win_lock(MPID_Win * win_ptr, int requested_lock);

int MPIDI_CH3I_Send_lock_granted_pkt(MPIDI_VC_t * vc, int source_win_ptr);

int MPIDI_CH3I_Send_pt_rma_done_pkt(MPIDI_VC_t * vc, int source_win_ptr);

/* NOTE: Channel function prototypes are in mpidi_ch3_post.h since some of the macros require their declarations. */

#endif /* !defined(MPICH_MPIDIMPL_H_INCLUDED) */
