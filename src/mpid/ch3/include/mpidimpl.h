/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDIMPL_H_INCLUDED)
#define MPICH_MPIDIMPL_H_INCLUDED

#include <assert.h>
#include "mpiimpl.h"

#define MPIDI_IOV_DENSITY_MIN 128

typedef struct
{
    MPID_Request * recv_posted_head;
    MPID_Request * recv_posted_tail;
    MPID_Request * recv_unexpected_head;
    MPID_Request * recv_unexpected_tail;
    char * processor_name;
}
MPIDI_Process_t;

extern MPIDI_Process_t MPIDI_Process;

/*
 * VC utility macros
 */
#if defined(MPID_USE_SEQUENCE_NUMBERS)
#define MPIDI_CH3U_VC_init_seqnum_send(_vc)	\
{						\
    (_vc)->seqnum_send = 0;			\
}
#else
#define MPIDI_CH3U_VC_init_seqnum_send(_vc)
#endif

#if defined(MPIDI_CH3_MSGS_UNORDERED)
#define MPIDI_CH3U_VC_init_seqnum_recv(_vc);	\
{						\
    (_vc)->seqnum_recv = 0;			\
    (_vc)->msg_reorder_queue = NULL;		\
}
#else
#define MPIDI_CH3U_VC_init_seqnum_recv(_vc);
#endif

#define MPIDI_CH3U_VC_init(_vc, _lpid)		\
{						\
    MPIU_Object_set_ref((_vc), 0);		\
    (_vc)->lpid = (_lpid);			\
    MPIDI_CH3U_VC_init_seqnum_send(_vc);	\
    MPIDI_CH3U_VC_init_seqnum_recv(_vc);	\
}


/*
 * Datatype Utility Macros (internal - do not use in MPID macros)
 */
#define MPIDI_CH3U_Datatype_get_info(_count, _datatype, _dt_contig_out, _data_sz_out)					\
{															\
    if (HANDLE_GET_KIND(_datatype) == HANDLE_KIND_BUILTIN)								\
    {															\
	(_dt_contig_out) = TRUE;											\
	(_data_sz_out) = (_count) * MPID_Datatype_get_basic_size(_datatype);						\
	MPIDI_DBG_PRINTF((15, FCNAME, "basic datatype: dt_contig=%d, dt_sz=%d, data_sz=" MPIDI_MSG_SZ_FMT,		\
			  (_dt_contig_out), MPID_Datatype_get_basic_size(_datatype), (_data_sz_out)));			\
    }															\
    else														\
    {															\
	MPID_Datatype * __dtp;												\
															\
	MPID_Datatype_get_ptr((_datatype), __dtp);									\
	(_dt_contig_out) = __dtp->is_contig;										\
	(_data_sz_out) = (_count) * __dtp->size;									\
	MPIDI_DBG_PRINTF((15, FCNAME, "user defined datatype: dt_contig=%d, dt_sz=%d, data_sz=" MPIDI_MSG_SZ_FMT,	\
			  (_dt_contig_out), __dtp->size, (_data_sz_out)));						\
    }															\
}

/*
 * Request utility macros (internal - do not use in MPID macros)
 */
#define MPIDI_CH3U_Request_create(_req)				\
{								\
    MPIU_Object_set_ref((_req), 1);				\
    (_req)->cc = 1;						\
    (_req)->cc_ptr = &(_req)->cc;				\
    (_req)->status.MPI_SOURCE = MPI_UNDEFINED;			\
    (_req)->status.MPI_TAG = MPI_UNDEFINED;			\
    (_req)->status.MPI_ERROR = MPI_SUCCESS;			\
    (_req)->status.count = 0;					\
    (_req)->status.cancelled = FALSE;				\
    MPIDI_Request_state_init((_req));				\
    (_req)->comm = NULL;					\
}

#define MPIDI_CH3U_Request_destroy(_req)	\
{						\
    if (MPIDI_Request_get_srbuf_flag(_req))	\
    {						\
	MPIDI_CH3U_SRBuf_free(_req);		\
    }						\
}

#define MPIDI_CH3U_Request_complete(_req)		\
{							\
    int cc;						\
							\
    MPIDI_CH3U_Request_decrement_cc((_req), &cc);	\
    if (cc == 0)					\
    {							\
	MPID_Request_release(_req);			\
	MPIDI_CH3_Progress_signal_completion();		\
    }							\
}

#define MPIDI_CH3M_create_sreq(_sreq, _mpi_errno, _FAIL)			\
{										\
    (_sreq) = MPIDI_CH3_Request_create();					\
    if ((_sreq) == NULL)							\
    {										\
	MPIDI_DBG_PRINTF((15, FCNAME, "send request allocation failed"));	\
	(_mpi_errno) = MPIR_ERR_MEMALLOCFAILED;					\
	_FAIL;									\
    }										\
    										\
    MPIU_Object_set_ref((_sreq), 2);						\
    (_sreq)->kind = MPID_REQUEST_SEND;						\
    (_sreq)->comm = comm;							\
    (_sreq)->ch3.match.rank = rank;						\
    (_sreq)->ch3.match.tag = tag;						\
    (_sreq)->ch3.match.context_id = comm->context_id + context_offset;		\
    (_sreq)->ch3.user_buf = (void *) buf;					\
    (_sreq)->ch3.user_count = count;						\
    (_sreq)->ch3.datatype = datatype;						\
}

#define MPIDI_CH3M_create_psreq(_sreq, _mpi_errno, _FAIL)			\
{										\
    (_sreq) = MPIDI_CH3_Request_create();					\
    if ((_sreq) == NULL)							\
    {										\
	MPIDI_DBG_PRINTF((15, FCNAME, "send request allocation failed"));	\
	(_mpi_errno) = MPIR_ERR_MEMALLOCFAILED;					\
	_FAIL;									\
    }										\
										\
    MPIU_Object_set_ref((_sreq), 1);						\
    (_sreq)->kind = MPID_PREQUEST_SEND;						\
    (_sreq)->comm = comm;							\
    (_sreq)->ch3.match.rank = rank;						\
    (_sreq)->ch3.match.tag = tag;						\
    (_sreq)->ch3.match.context_id = comm->context_id + context_offset;		\
    (_sreq)->ch3.user_buf = (void *) buf;					\
    (_sreq)->ch3.user_count = count;						\
    (_sreq)->ch3.datatype = datatype;						\
    (_sreq)->partner_request = NULL;						\
}

/* Masks and flags for channel device state in an MPID_Request */
#define MPIDI_Request_state_init(_req)		\
{						\
    (_req)->ch3.state = 0;			\
}

#define MPIDI_REQUEST_MSG_MASK (0x3 << MPIDI_REQUEST_MSG_SHIFT)
#define MPIDI_REQUEST_MSG_SHIFT 0
#define MPIDI_REQUEST_NO_MSG 0
#define MPIDI_REQUEST_EAGER_MSG 1
#define MPIDI_REQUEST_RNDV_MSG 2
#define MPIDI_REQUEST_SELF_MSG 3

#define MPIDI_Request_get_msg_type(_req)					\
(((_req)->ch3.state & MPIDI_REQUEST_MSG_MASK) >> MPIDI_REQUEST_MSG_SHIFT)

#define MPIDI_Request_set_msg_type(_req, _msgtype)						\
{												\
    (_req)->ch3.state &= ~MPIDI_REQUEST_MSG_MASK;						\
    (_req)->ch3.state |= ((_msgtype) << MPIDI_REQUEST_MSG_SHIFT) & MPIDI_REQUEST_MSG_MASK;	\
}

#define MPIDI_REQUEST_SRBUF_MASK (0x1 << MPIDI_REQUEST_SRBUF_SHIFT)
#define MPIDI_REQUEST_SRBUF_SHIFT 2

#define MPIDI_Request_get_srbuf_flag(_req)					\
(((_req)->ch3.state & MPIDI_REQUEST_SRBUF_MASK) >> MPIDI_REQUEST_SRBUF_SHIFT)

#define MPIDI_Request_set_srbuf_flag(_req, _flag)						\
{												\
    (_req)->ch3.state &= ~MPIDI_REQUEST_SRBUF_MASK;						\
    (_req)->ch3.state |= ((_flag) << MPIDI_REQUEST_SRBUF_SHIFT) & MPIDI_REQUEST_SRBUF_MASK;	\
}

#define MPIDI_REQUEST_SYNC_SEND_MASK (0x1 << MPIDI_REQUEST_SYNC_SEND_SHIFT)
#define MPIDI_REQUEST_SYNC_SEND_SHIFT 3

#define MPIDI_Request_get_sync_send_flag(_req)						\
(((_req)->ch3.state & MPIDI_REQUEST_SYNC_SEND_MASK) >> MPIDI_REQUEST_SYNC_SEND_SHIFT)

#define MPIDI_Request_set_sync_send_flag(_req, _flag)							\
{													\
    (_req)->ch3.state &= ~MPIDI_REQUEST_SYNC_SEND_MASK;							\
    (_req)->ch3.state |= ((_flag) << MPIDI_REQUEST_SYNC_SEND_SHIFT) & MPIDI_REQUEST_SYNC_SEND_MASK;	\
}

#define MPIDI_REQUEST_TYPE_MASK (0x7 << MPIDI_REQUEST_TYPE_SHIFT)
#define MPIDI_REQUEST_TYPE_SHIFT 4
#define MPIDI_REQUEST_TYPE_RECV 0
#define MPIDI_REQUEST_TYPE_SEND 1
#define MPIDI_REQUEST_TYPE_RSEND 2
#define MPIDI_REQUEST_TYPE_SSEND 3
/* We need a BSEND type for persistent bsends (see mpid_startall.c) */
#define MPIDI_REQUEST_TYPE_BSEND 4
#define MPIDI_Request_get_type(_req)						\
(((_req)->ch3.state & MPIDI_REQUEST_TYPE_MASK) >> MPIDI_REQUEST_TYPE_SHIFT)

#define MPIDI_Request_set_type(_req, _type)							\
{												\
    (_req)->ch3.state &= ~MPIDI_REQUEST_TYPE_MASK;						\
    (_req)->ch3.state |= ((_type) << MPIDI_REQUEST_TYPE_SHIFT) & MPIDI_REQUEST_TYPE_MASK;	\
}

#define MPIDI_REQUEST_CANCEL_MASK (0x1 << MPIDI_REQUEST_CANCEL_SHIFT)
#define MPIDI_REQUEST_CANCEL_SHIFT 7
#if defined(MPICH_SINGLE_THREADED)
#define MPIDI_Request_cancel_pending(_req, _flag)						\
{												\
    *(_flag) = ((_req)->ch3.state & MPIDI_REQUEST_CANCEL_MASK) >> MPIDI_REQUEST_CANCEL_SHIFT;	\
    (_req)->ch3.state |= MPIDI_REQUEST_CANCEL_MASK;						\
}
#else
#error Multi-threaded MPIDI_Request_cancel_pending() not implemented.
#endif

/*
 * Send/Receive buffer macros
 */
#if !defined(MPIDI_CH3U_SRBuf_size)
#define MPIDI_CH3U_SRBuf_size (16384)
#endif

#if !defined(MPIDI_CH3U_SRBuf_alloc)
#define MPIDI_CH3U_SRBuf_alloc(_req, _size)			\
{								\
    (_req)->ch3.tmpbuf = MPIU_Malloc(MPIDI_CH3U_SRBuf_size);	\
    if ((_req)->ch3.tmpbuf != NULL)				\
    {								\
	(_req)->ch3.tmpbuf_sz = MPIDI_CH3U_SRBuf_size;		\
	MPIDI_Request_set_srbuf_flag((_req), TRUE);		\
    }								\
    else							\
    {								\
	(_req)->ch3.tmpbuf_sz = 0;				\
    }								\
}
#endif

#if !defined(MPIDI_CH3U_SRBuf_free)
#define MPIDI_CH3U_SRBuf_free(_req)			\
{							\
    assert(MPIDI_Request_get_srbuf_flag(_req));		\
    MPIDI_Request_set_srbuf_flag((_req), FALSE);	\
    MPIU_Free((_req)->ch3.tmpbuf);			\
}
#endif


/*
 * Sequence number related macros (internal)
 */
#if defined(MPID_USE_SEQUENCE_NUMBERS)
#if defined(MPICH_SINGLE_THREADED)
#define MPIDI_CH3U_VC_FAI_send_seqnum(_vc, _seqnum_out)	\
{							\
    (_seqnum_out) = (_vc)->seqnum_send++;		\
}
#else
#error Multi-threaded MPIDI_Seqnum_fetch_and_inc_send() not implemented.
#endif
#define MPIDI_CH3U_Request_set_seqnum(_req, _seqnum)	\
{							\
    (_req)->ch3.seqnum = (_seqnum);			\
}
#define MPIDI_CH3U_Pkt_set_seqnum(_pkt, _seqnum)	\
{							\
    (_pkt)->seqnum = (_seqnum);				\
}
#else
#define MPIDI_CH3U_VC_FAI_send_seqnum(_vc, _seqnum_out)
#define MPIDI_CH3U_Request_set_seqnum(_req, _seqnum)
#define MPIDI_CH3U_Pkt_set_seqnum(_pkt, _seqnum)
#endif


/*
 * Debugging tools
 */
void MPIDI_dbg_printf(int, char *, char *, ...);
void MPIDI_err_printf(char *, char *, ...);

#if defined(MPICH_DBG_OUTPUT)
#define MPIDI_DBG_PRINTF(_e) MPIDI_dbg_printf _e
#define MPIDI_dbg_printf(level, func, fmt, args...)							\
{													\
    MPIU_DBG_PRINTF(("%d (%d) %s(): " fmt "\n", MPIR_Process.comm_world->rank, level, func, ## args));	\
}
#else
#define MPIDI_DBG_PRINTF(e)
#define MPIDI_dbg_printf(level, func, fmt, args...)
#endif

#define MPIDI_ERR_PRINTF(e) MPIDI_err_printf e
#define MPIDI_err_printf(func, fmt, args...)						\
{											\
    printf("%d ERROR - %s(): " fmt "\n", MPIR_Process.comm_world->rank, func, ## args);	\
    fflush(stdout);									\
}

#define MPIDI_QUOTE(A) MPIDI_QUOTE2(A)
#define MPIDI_QUOTE2(A) #A

/* Prototypes for internal device routines */
int MPIDI_Isend_self(const void *, int, MPI_Datatype, int, int, MPID_Comm *, int, int, MPID_Request **);
int MPIDI_Irecv_self(void *, int, MPI_Datatype, int, int, MPID_Comm *, int, MPID_Request **);


/* Prototypes for collective operations supplied by the device (or channel) */
int MPIDI_Barrier(MPID_Comm *);


/* NOTE: Channel function prototypes are in mpidi_ch3_post.h since some of the macros require their declarations. */

#endif /* !defined(MPICH_MPIDIMPL_H_INCLUDED) */
