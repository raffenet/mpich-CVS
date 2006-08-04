/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * WARNING: Functions and macros in this file are for internal use only.  
 * As such, they are only visible to the device and
 * channel.  Do not include them in the MPID macros.
 */

#if !defined(MPICH_MPIDIMPL_H_INCLUDED)
#define MPICH_MPIDIMPL_H_INCLUDED

#if defined(HAVE_ASSERT_H)
#include <assert.h>
#endif

#include "mpiimpl.h"

#if !defined(MPICH_MPIDPRE_H_INCLUDED)
#include "mpidpre.h"
#endif

#if !defined(MPIDI_IOV_DENSITY_MIN)
#   define MPIDI_IOV_DENSITY_MIN (16 * 1024)
#endif

#if defined(HAVE_GETHOSTNAME) && defined(NEEDS_GETHOSTNAME_DECL) && !defined(gethostname)
int gethostname(char *name, size_t len);
# endif

/*S
  MPIDI_Process_t - The information required about this process by the CH3 
  device.

  S*/
typedef struct MPIDI_Process
{
    MPIDI_PG_t * my_pg;
    int my_pg_rank;
    int lpid_counter;
}
MPIDI_Process_t;

extern MPIDI_Process_t MPIDI_Process;

/* FIXME: When we're sure that this works as a file-local variable, remote 
   it */
/* extern volatile int MPIDI_Outstanding_close_ops; */


/*----------------------
  BEGIN DATATYPE SECTION
  ----------------------*/
/* FIXME: We want to avoid even storing information about the builtins
   if we can */
#define MPIDI_Datatype_get_info(count_, datatype_, dt_contig_out_, data_sz_out_, dt_ptr_, dt_true_lb_)\
{									\
    if (HANDLE_GET_KIND(datatype_) == HANDLE_KIND_BUILTIN)		\
    {									\
	(dt_ptr_) = NULL;						\
	(dt_contig_out_) = TRUE;					\
        (dt_true_lb_)    = 0;                                           \
	(data_sz_out_) = (count_) * MPID_Datatype_get_basic_size(datatype_);\
	MPIDI_DBG_PRINTF((15, FCNAME, "basic datatype: dt_contig=%d, dt_sz=%d, data_sz=" MPIDI_MSG_SZ_FMT,\
			  (dt_contig_out_), MPID_Datatype_get_basic_size(datatype_), (data_sz_out_)));\
    }									\
    else								\
    {									\
	MPID_Datatype_get_ptr((datatype_), (dt_ptr_));			\
	(dt_contig_out_) = (dt_ptr_)->is_contig;			\
	(data_sz_out_) = (count_) * (dt_ptr_)->size;			\
        (dt_true_lb_)    = (dt_ptr_)->true_lb;                          \
	MPIDI_DBG_PRINTF((15, FCNAME, "user defined datatype: dt_contig=%d, dt_sz=%d, data_sz=" MPIDI_MSG_SZ_FMT,\
			  (dt_contig_out_), (dt_ptr_)->size, (data_sz_out_)));\
    }									\
}
/*--------------------
  END DATATYPE SECTION
  --------------------*/


/*---------------------
  BEGIN REQUEST SECTION
  ---------------------*/
/* FIXME: This makes request creation expensive.  We need to trim
   this to the basics, with additional setup for special-purpose requests 
   (think base class and inheritance).  For example, do we *really*
   want to set the kind to UNDEFINED? And should the RMA values 
   be set only for RMA requests? */
#define MPIDI_CH3U_Request_create(req_)				\
{								\
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

#define MPIDI_Request_get_msg_type(req_)				\
(((req_)->dev.state & MPIDI_REQUEST_MSG_MASK) >> MPIDI_REQUEST_MSG_SHIFT)

#define MPIDI_Request_set_msg_type(req_, msgtype_)			\
{									\
    (req_)->dev.state &= ~MPIDI_REQUEST_MSG_MASK;			\
    (req_)->dev.state |= ((msgtype_) << MPIDI_REQUEST_MSG_SHIFT) & MPIDI_REQUEST_MSG_MASK;\
}

#define MPIDI_REQUEST_SRBUF_MASK (0x1 << MPIDI_REQUEST_SRBUF_SHIFT)
#define MPIDI_REQUEST_SRBUF_SHIFT 2

#define MPIDI_Request_get_srbuf_flag(req_)					\
(((req_)->dev.state & MPIDI_REQUEST_SRBUF_MASK) >> MPIDI_REQUEST_SRBUF_SHIFT)

#define MPIDI_Request_set_srbuf_flag(req_, flag_)			\
{									\
    (req_)->dev.state &= ~MPIDI_REQUEST_SRBUF_MASK;			\
    (req_)->dev.state |= ((flag_) << MPIDI_REQUEST_SRBUF_SHIFT) & MPIDI_REQUEST_SRBUF_MASK;	\
}

#define MPIDI_REQUEST_SYNC_SEND_MASK (0x1 << MPIDI_REQUEST_SYNC_SEND_SHIFT)
#define MPIDI_REQUEST_SYNC_SEND_SHIFT 3

#define MPIDI_Request_get_sync_send_flag(req_)						\
(((req_)->dev.state & MPIDI_REQUEST_SYNC_SEND_MASK) >> MPIDI_REQUEST_SYNC_SEND_SHIFT)

#define MPIDI_Request_set_sync_send_flag(req_, flag_)			\
{									\
    (req_)->dev.state &= ~MPIDI_REQUEST_SYNC_SEND_MASK;			\
    (req_)->dev.state |= ((flag_) << MPIDI_REQUEST_SYNC_SEND_SHIFT) & MPIDI_REQUEST_SYNC_SEND_MASK;\
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

#define MPIDI_Request_set_type(req_, type_)				\
{									\
    (req_)->dev.state &= ~MPIDI_REQUEST_TYPE_MASK;			\
    (req_)->dev.state |= ((type_) << MPIDI_REQUEST_TYPE_SHIFT) & MPIDI_REQUEST_TYPE_MASK;\
}

/* NOTE: Request updates may require atomic ops (critical sections) if
   a fine-grain thread-sync model is used. */
#define MPIDI_Request_cancel_pending(req_, flag_)	\
{							\
    *(flag_) = (req_)->dev.cancel_pending;		\
    (req_)->dev.cancel_pending = TRUE;			\
}

#define MPIDI_Request_recv_pending(req_, recv_pending_)	\
    {								\
 	*(recv_pending_) = --(req_)->dev.recv_pending_count;	\
    }

/* MPIDI_Request_fetch_and_clear_rts_sreq() - atomically fetch current 
   partner RTS sreq and nullify partner request */
#define MPIDI_Request_fetch_and_clear_rts_sreq(sreq_, rts_sreq_)	\
    {									\
    	*(rts_sreq_) = (sreq_)->partner_request;			\
    	(sreq_)->partner_request = NULL;				\
    }

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
	MPIU_DBG_PrintVCState2(*(vcp_), MPIDI_VC_STATE_ACTIVE);  \
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
/* FIXME: Determine which of these functions should be exported to all of 
   the MPICH routines and which are internal to the device implementation */
typedef int (*MPIDI_PG_Compare_ids_fn_t)(void * id1, void * id2);
typedef int (*MPIDI_PG_Destroy_fn_t)(MPIDI_PG_t * pg);

int MPIDI_PG_Init(MPIDI_PG_Compare_ids_fn_t, MPIDI_PG_Destroy_fn_t);
int MPIDI_PG_Finalize(void);
int MPIDI_PG_Create(int vct_sz, void * pg_id, MPIDI_PG_t ** ppg);
int MPIDI_PG_Destroy(MPIDI_PG_t * pg);
void MPIDI_PG_Add_ref(MPIDI_PG_t * pg);
void MPIDI_PG_Release_ref(MPIDI_PG_t * pg, int * inuse);
int MPIDI_PG_Find(void * id, MPIDI_PG_t ** pgp);
int MPIDI_PG_Id_compare(void *id1, void *id2);
int MPIDI_PG_Get_next(MPIDI_PG_t ** pgp);
int MPIDI_PG_Iterate_reset(void);
/* FIXME: MPIDI_PG_Get_vc is a macro, not a routine */
int MPIDI_PG_Get_vc(MPIDI_PG_t * pg, int rank, MPIDI_VC_t ** vc); 
int MPIDI_PG_Close_VCs( void );

int MPIDI_PG_InitConnKVS( MPIDI_PG_t * );
int MPIDI_PG_GetConnKVSname( char ** );
int MPIDI_PG_InitConnString( MPIDI_PG_t * );
int MPIDI_PG_GetConnString( MPIDI_PG_t *, int, char *, int );
int MPIDI_PG_Dup_vcr( MPIDI_PG_t *, int, MPIDI_VC_t ** );
int MPIDI_PG_Get_size(MPIDI_PG_t * pg);
void MPIDI_PG_IdToNum( MPIDI_PG_t *, int * );

/* CH3_PG_Init allows the channel to pre-initialize the process group */
int MPIDI_CH3_PG_Init( MPIDI_PG_t * );

/* FIXME: It would be simpler if we used MPIU_Object_add_ref etc. uniformly,
   rather than defining separate routines */
#define MPIDI_PG_Add_ref(pg_)			\
{						\
    MPIU_Object_add_ref(pg_);			\
}
#define MPIDI_PG_Release_ref(pg_, inuse_)	\
{						\
    MPIU_Object_release_ref(pg_, inuse_);	\
}
/* FIXME: What is the difference between get_vcr and get_vc? */
#define MPIDI_PG_Get_vc(pg_, rank_, vcp_)		\
{							\
    *(vcp_) = &(pg_)->vct[rank_];			\
    if ((*(vcp_))->state == MPIDI_VC_STATE_INACTIVE)	\
    {							\
	MPIU_DBG_PrintVCState2(*(vcp_), MPIDI_VC_STATE_ACTIVE);  \
	(*(vcp_))->state = MPIDI_VC_STATE_ACTIVE;	\
    }							\
}

#define MPIDI_PG_Get_size(pg_) ((pg_)->size)

#ifdef MPIDI_DEV_IMPLEMENTS_KVS
int MPIDI_PG_To_string(MPIDI_PG_t *pg_ptr, char **str_ptr, int *);
int MPIDI_PG_Create_from_string(const char * str, MPIDI_PG_t ** pg_pptr, 
				int *flag);
#endif
/*-------------------------
  END PROCESS GROUP SECTION
  -------------------------*/


/*--------------------------------
  BEGIN VIRTUAL CONNECTION SECTION
  --------------------------------*/
#define MPIDI_VC_Get_next_lpid(lpid_ptr_)		\
    {							\
    	*(lpid_ptr_) = MPIDI_Process.lpid_counter++;	\
    }

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

/* FIXME: Should this fully initialize the vc_ entry? */
/* FIXME: Make this into a routine (initializing/creating 
   connections are rare and expensive; no need to use a macro.
   In addition, the lpid_counter can then be a static int in the
   file that implements this routine */
#define MPIDI_VC_Init(vc_, pg_, rank_)		\
{						\
    (vc_)->state = MPIDI_VC_STATE_INACTIVE;	\
    MPIU_Object_set_ref((vc_), 0);		\
    (vc_)->handle = MPID_VCONN;                 \
    (vc_)->pg = (pg_);				\
    (vc_)->pg_rank = (rank_);			\
    MPIDI_VC_Get_next_lpid(&(vc_)->lpid);	\
    MPIDI_VC_Init_seqnum_send(vc_);		\
    MPIDI_VC_Init_seqnum_recv(vc_);		\
    MPIU_DBG_PrintVCState(vc_);                 \
}

/* Note: In the current implementation, the mpid_xsend.c routines that
   make use of MPIDI_VC_FAI_send_seqnum are all protected by the 
   SINGLE_CS_ENTER/EXIT macros, so all uses of this macro are 
   alreay within a critical section when needed.  If/when we move to
   a finer-grain model, we'll need to examine whether this requires
   a separate lock. */
#if defined(MPID_USE_SEQUENCE_NUMBERS)
#       define MPIDI_VC_FAI_send_seqnum(vc_, seqnum_out_)	\
        {							\
	    (seqnum_out_) = (vc_)->seqnum_send++;		\
	}
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
#    define MPIDI_CH3U_SRBuf_size (256 * 1024)
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

/* If there is no support for dynamic processes, there will be no
   channel-specific connection state */
#ifdef USE_DBG_LOGGING
#ifdef MPIDI_CH3_HAS_NO_DYNAMIC_PROCESS
#define MPIDI_CH3_VC_GetStateString( _c ) "none"
#else
/* FIXME: This duplicates a value in util/sock/ch3usock.h */
const char *MPIDI_CH3_VC_GetStateString(int);
#endif
#endif

/* These macros simplify and unify the debugging of changes in the
   connection state 

   MPIU_DBG_VCSTATECHANGE(vc,newstate) - use when changing the state
   of a VC

   MPIU_DBG_VCCHSTATECHANGE(vc,newstate) - use when changing the state
   of the channel-specific part of the vc (e.g., vc->ch.state)

   MPIU_DBG_CONNSTATECHANGE(vc,conn,newstate ) - use when changing the
   state of a conn.  vc may be null

   MPIU_DBG_CONNSTATECHANGEMSG(vc,conn,newstate,msg ) - use when changing the
   state of a conn.  vc may be null.  Like CONNSTATECHANGE, but allows
   an additional message

   MPIU_DBG_PKT(conn,pkt,msg) - print out a short description of an
   packet being sent/received on the designated connection, prefixed with
   msg.

*/
#define MPIU_DBG_VCSTATECHANGE(_vc,_newstate) \
     MPIU_DBG_MSG_FMT(CH3_CONNECT,TYPICAL,(MPIU_DBG_FDEST, \
     "vc=%p: Setting state (vc) from %s to %s, vcchstate is %s", \
                  _vc, MPIDI_VC_GetStateString((_vc)->state), \
                  #_newstate, MPIDI_CH3_VC_GetStateString( (_vc)->ch.state )) )

#define MPIU_DBG_VCCHSTATECHANGE(_vc,_newstate) \
     MPIU_DBG_MSG_FMT(CH3_CONNECT,TYPICAL,(MPIU_DBG_FDEST, \
     "vc=%p: Setting state (ch) from %s to %s, vc state is %s", \
                   _vc, MPIDI_CH3_VC_GetStateString((_vc)->ch.state), \
                   #_newstate, MPIDI_VC_GetStateString( (_vc)->state )) )

#define MPIU_DBG_CONNSTATECHANGE(_vc,_conn,_newstate) \
     MPIU_DBG_MSG_FMT(CH3_CONNECT,TYPICAL,(MPIU_DBG_FDEST, \
     "vc=%p,conn=%p: Setting state (conn) from %s to %s, vcstate = %s", \
             _vc, _conn, \
             MPIDI_Conn_GetStateString((_conn)->state), #_newstate, \
             _vc ? MPIDI_VC_GetStateString((_vc)->state) : "<no vc>" ))

#define MPIU_DBG_CONNSTATECHANGE_MSG(_vc,_conn,_newstate,_msg) \
     MPIU_DBG_MSG_FMT(CH3_CONNECT,TYPICAL,(MPIU_DBG_FDEST, \
     "vc=%p,conn=%p: Setting conn state from %s to %s, vcstate = %s %s", \
             _vc, _conn, \
             MPIDI_Conn_GetStateString((_conn)->state), #_newstate, \
             _vc ? MPIDI_VC_GetStateString((_vc)->state) : "<no vc>", _msg ))
#define MPIU_DBG_VCUSE(_vc,_msg) \
     MPIU_DBG_MSG_FMT(CH3_CONNECT,TYPICAL,(MPIU_DBG_FDEST,\
      "vc=%p,conn=%p: Using vc for %s", _vc, (_vc)->ch.conn, _msg ))
#define MPIU_DBG_PKT(_conn,_pkt,_msg) \
     MPIU_DBG_MSG_FMT(CH3,TYPICAL,(MPIU_DBG_FDEST,\
     "conn=%p: %s %s", _conn, _msg, MPIDI_Pkt_GetDescString( _pkt ) ))

const char *MPIDI_Pkt_GetDescString( MPIDI_CH3_Pkt_t *pkt );

/* FIXME: Switch this to use the common debug code */
void MPIDI_dbg_printf(int, char *, char *, ...);
void MPIDI_err_printf(char *, char *, ...);

/* FIXME: This does not belong here */
#ifdef USE_MPIU_DBG_PRINT_VC
extern char *MPIU_DBG_parent_str;
#endif

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

/* FIXME: What are these for?  Why not just use #A? */
#define MPIDI_QUOTE(A) MPIDI_QUOTE2(A)
#define MPIDI_QUOTE2(A) #A

#ifdef MPICH_DBG_OUTPUT
    void MPIDI_DBG_Print_packet(MPIDI_CH3_Pkt_t *pkt);
#else
#   define MPIDI_DBG_Print_packet(a)
#endif

/* Given a state, return the string for this state (VC's and connections) */
const char * MPIDI_VC_GetStateString(int);
/*--------------------------
  END DEBUGGING TOOL SECTION
  --------------------------*/


/* Prototypes for internal device routines */
int MPIDI_Isend_self(const void *, int, MPI_Datatype, int, int, MPID_Comm *, 
		     int, int, MPID_Request **);

/*--------------------------
  BEGIN MPI PORT SECTION 
  --------------------------*/
/* These are the default functions */
int MPIDI_Comm_connect(const char *, MPID_Info *, int, MPID_Comm *, MPID_Comm **);
int MPIDI_Comm_accept(const char *, MPID_Info *, int, MPID_Comm *, MPID_Comm **);

int MPIDI_Comm_spawn_multiple(int, char **, char ***, int *, MPID_Info **, 
			      int, MPID_Comm *, MPID_Comm **, int *);


/* This structure defines a module that handles the routines that 
   work with MPI port names */
typedef struct MPIDI_Port_Ops {
    int (*OpenPort)( MPID_Info *, char * );
    int (*ClosePort)( const char * );
    int (*CommAccept)( const char *, MPID_Info *, int, MPID_Comm *, 
		       MPID_Comm ** );
    int (*CommConnect)( const char *, MPID_Info *, int, MPID_Comm *, 
			MPID_Comm ** );
} MPIDI_PortFns;
#define MPIDI_PORTFNS_VERSION 1
int MPIDI_CH3_PortFnsInit( MPIDI_PortFns * );

/* Utility routines provided in src/ch3u_port.c for working with connection
   queues */
int MPIDI_CH3I_Acceptq_enqueue(MPIDI_VC_t * vc);
int MPIDI_CH3I_Acceptq_dequeue(MPIDI_VC_t ** vc, int port_name_tag);
int MPIDI_CH3I_Acceptq_init(void);
/*--------------------------
  END MPI PORT SECTION 
  --------------------------*/

/* part of mpid_vc.c, this routine completes any pending operations 
   on a communicator */
int MPIDI_CH3U_Comm_FinishPending( MPID_Comm * );


#define MPIDI_MAX_KVS_VALUE_LEN    4096

/* ------------------------------------------------------------------------- */
/* mpirma.h (in src/mpi/rma?) */
/* ------------------------------------------------------------------------- */

/* This structure defines a module that handles the routines that 
   work with MPI-2 RMA ops */
typedef struct MPIDI_RMA_Ops {
    int (*Win_create)(void *, MPI_Aint, int, MPID_Info *, MPID_Comm *,
		      MPID_Win **, struct MPIDI_RMA_Ops *);
    int (*Win_free)(MPID_Win **);
    int (*Put)(void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, 
		MPID_Win *);
    int (*Get)(void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, 
		MPID_Win *);
    int (*Accumulate)(void *, int, MPI_Datatype, int, MPI_Aint, int, 
		       MPI_Datatype, MPI_Op, MPID_Win *);
    int (*Win_fence)(int, MPID_Win *);
    int (*Win_post)(MPID_Group *, int, MPID_Win *);
    int (*Win_start)(MPID_Group *, int, MPID_Win *);
    int (*Win_complete)(MPID_Win *);
    int (*Win_wait)(MPID_Win *);
    int (*Win_lock)(int, int, int, MPID_Win *);
    int (*Win_unlock)(int, MPID_Win *);
    void * (*Alloc_mem)(size_t, MPID_Info *);
    int (*Free_mem)(void *);
} MPIDI_RMAFns;
#define MPIDI_RMAFNS_VERSION 1
int MPIDI_CH3_RMAFnsInit( MPIDI_RMAFns * );

#define MPIDI_RMA_PUT 23
#define MPIDI_RMA_GET 24
#define MPIDI_RMA_ACCUMULATE 25
#define MPIDI_RMA_LOCK 26
#define MPIDI_RMA_DATATYPE_BASIC 50
#define MPIDI_RMA_DATATYPE_DERIVED 51

#define MPID_LOCK_NONE 0

int MPIDI_Win_create(void *, MPI_Aint, int, MPID_Info *, MPID_Comm *,
                    MPID_Win **, MPIDI_RMAFns *);
int MPIDI_Win_fence(int, MPID_Win *);
int MPIDI_Put(void *, int, MPI_Datatype, int, MPI_Aint, int,
            MPI_Datatype, MPID_Win *); 
int MPIDI_Get(void *, int, MPI_Datatype, int, MPI_Aint, int,
            MPI_Datatype, MPID_Win *);
int MPIDI_Accumulate(void *, int, MPI_Datatype, int, MPI_Aint, int, 
		   MPI_Datatype,  MPI_Op, MPID_Win *);
int MPIDI_Win_free(MPID_Win **); 
int MPIDI_Win_wait(MPID_Win *win_ptr);
int MPIDI_Win_complete(MPID_Win *win_ptr);
int MPIDI_Win_post(MPID_Group *group_ptr, int assert, MPID_Win *win_ptr);
int MPIDI_Win_start(MPID_Group *group_ptr, int assert, MPID_Win *win_ptr);
int MPIDI_Win_lock(int lock_type, int dest, int assert, MPID_Win *win_ptr);
int MPIDI_Win_unlock(int dest, MPID_Win *win_ptr);
void *MPIDI_Alloc_mem(size_t size, MPID_Info *info_ptr);
int MPIDI_Free_mem(void *ptr);

/* optional channel-specific */
void *MPIDI_CH3_Alloc_mem(size_t size, MPID_Info *info_ptr);
int MPIDI_CH3_Win_create(void *base, MPI_Aint size, int disp_unit, MPID_Info *info, 
                    MPID_Comm *comm_ptr, MPID_Win **win_ptr, MPIDI_RMAFns *RMAFns);
int MPIDI_CH3_Free_mem(void *ptr);
void MPIDI_CH3_Cleanup_mem(void);
int MPIDI_CH3_Win_free(MPID_Win **win_ptr);
int MPIDI_CH3_Put(void *origin_addr, int origin_count, MPI_Datatype
            origin_datatype, int target_rank, MPI_Aint target_disp,
            int target_count, MPI_Datatype target_datatype, MPID_Win *win_ptr);
int MPIDI_CH3_Get(void *origin_addr, int origin_count, MPI_Datatype
            origin_datatype, int target_rank, MPI_Aint target_disp,
            int target_count, MPI_Datatype target_datatype, MPID_Win *win_ptr);
int MPIDI_CH3_Accumulate(void *origin_addr, int origin_count, MPI_Datatype
                    origin_datatype, int target_rank, MPI_Aint target_disp,
                    int target_count, MPI_Datatype target_datatype, MPI_Op op,
                    MPID_Win *win_ptr);
int MPIDI_CH3_Win_fence(int assert, MPID_Win *win_ptr);
int MPIDI_CH3_Win_lock(int lock_type, int dest, int assert, MPID_Win *win_ptr);
int MPIDI_CH3_Win_unlock(int dest, MPID_Win *win_ptr);
int MPIDI_CH3_Win_wait(MPID_Win *win_ptr);
int MPIDI_CH3_Win_complete(MPID_Win *win_ptr);
int MPIDI_CH3_Win_post(MPID_Group *group_ptr, int assert, MPID_Win *win_ptr);
int MPIDI_CH3_Win_start(MPID_Group *group_ptr, int assert, MPID_Win *win_ptr);

/* internal */
int MPIDI_CH3I_Release_lock(MPID_Win * win_ptr);
int MPIDI_CH3I_Try_acquire_win_lock(MPID_Win * win_ptr, int requested_lock);
int MPIDI_CH3I_Send_lock_granted_pkt(MPIDI_VC_t * vc, int source_win_ptr);
int MPIDI_CH3I_Send_pt_rma_done_pkt(MPIDI_VC_t * vc, int source_win_ptr);


#define MPIDI_CH3I_DATATYPE_IS_PREDEFINED(type, predefined) \
    if ((HANDLE_GET_KIND(type) == HANDLE_KIND_BUILTIN) || \
        (type == MPI_FLOAT_INT) || (type == MPI_DOUBLE_INT) || \
        (type == MPI_LONG_INT) || (type == MPI_SHORT_INT) || \
	(type == MPI_LONG_DOUBLE_INT)) \
        predefined = 1; \
    else predefined = 0;


int MPIDI_CH3I_Progress_finalize(void);

/* Function that may be used to provide buisness card info */
int MPIDI_CH3I_BCInit( char **bc_val_p, int *val_max_sz_p);
/* Function to free the storage allocated by MPIDI_CH3I_BCInit */
int MPIDI_CH3I_BCFree( char *publish_bc );

/* Inform the process group of our connection information string (business
   card) */
int MPIDI_PG_SetConnInfo( int rank, const char *connString );

/* NOTE: Channel function prototypes are in mpidi_ch3_post.h since some of the 
   macros require their declarations. */

/* Access the business card (used in mpid_port) */
int MPIDI_CH3I_Get_business_card(char *value, int length);

/* Perform channel-specific initialization of a virtural connection */
int MPIDI_CH3_VC_Init( MPIDI_VC_t *);

/* FIXME: These should be defined only when these particular utility
   packages are used.  Best would be to keep these prototypes in the
   related util/xxx directories, and either copy them into an include
   directory used only for builds or add (yet another) include path */
/* from util/sock */
int MPIDI_VC_InitSock( MPIDI_VC_t *);
int MPIDI_CH3I_Connect_to_root_sock(const char *, MPIDI_VC_t **);
int MPIDI_CH3U_Get_business_card_sock(char **, int *);


int MPIDI_CH3I_VC_post_sockconnect(MPIDI_VC_t * );

/* FIXME: Where should this go? */

/* Used internally to broadcast process groups belonging to peercomm to
 all processes in comm*/
int MPID_PG_BCast( MPID_Comm *peercomm_p, MPID_Comm *comm_p, int root );

/* from util/shm */
int MPIDI_CH3I_Connect_to_root_sshm(const char *, MPIDI_VC_t **);
int MPIDI_VC_InitShm( MPIDI_VC_t *vc );

/* from util/sock */
#ifdef HAVE_UTIL_SOCK
int MPIDU_Sock_get_conninfo_from_bc( const char *bc, 
				     char *host_description, int maxlen,
				     int *port, MPIDU_Sock_ifaddr_t *ifaddr, 
				     int *hasIfaddr );

/* These two routines from util/sock initialize and shutdown the 
   socket used to establish connections.  */
int MPIDU_CH3I_SetupListener( MPIDU_Sock_set_t );
int MPIDU_CH3I_ShutdownListener( void );

#endif

/* Channel defintitions */
/*E
  MPIDI_CH3_iStartMsg - A non-blocking request to send a CH3 packet.  A r
  equest object is allocated only if the send could not be completed 
  immediately.

  Input Parameters:
+ vc - virtual connection to send the message over
. pkt - pointer to a MPIDI_CH3_Pkt_t structure containing the substructure to 
  be sent
- pkt_sz - size of the packet substucture

  Output Parameters:
. sreq_ptr - send request or NULL if the send completed immediately

  Return value:
  An mpi error code.
  
  NOTE:
  The packet structure may be allocated on the stack.

  IMPLEMETORS:
  If the send can not be completed immediately, the CH3 packet structure must 
  be stored internally until the request is complete.
  
  If the send completes immediately, the channel implementation shold return 
  NULL and must not call MPIDI_CH3U_Handle_send_req().
E*/
int MPIDI_CH3_iStartMsg(MPIDI_VC_t * vc, void * pkt, MPIDI_msg_sz_t pkt_sz, 
			MPID_Request **sreq_ptr);


/*E
  MPIDI_CH3_iStartMsgv - A non-blocking request to send a CH3 packet and 
  associated data.  A request object is allocated only if
  the send could not be completed immediately.

  Input Parameters:
+ vc - virtual connection to send the message over
. iov - a vector of a structure contains a buffer pointer and length
- iov_n - number of elements in the vector

  Output Parameters:
. sreq_ptr - send request or NULL if the send completed immediately

  Return value:
  An mpi error code.
  
  NOTE:
  The first element in the vector must point to the packet structure.   The 
  packet structure and the vector may be allocated on
  the stack.

  IMPLEMENTORS:
  If the send can not be completed immediately, the CH3 packet structure and 
  the vector must be stored internally until the
  request is complete.
  
  If the send completes immediately, the channel implementation shold return 
  NULL and must not call MPIDI_CH3U_Handle_send_req().
E*/
int MPIDI_CH3_iStartMsgv(MPIDI_VC_t * vc, MPID_IOV * iov, int iov_n, 
			 MPID_Request **sreq_ptr);


/*E
  MPIDI_CH3_iSend - A non-blocking request to send a CH3 packet using an 
  existing request object.  When the send is complete
  the channel implementation will call MPIDI_CH3U_Handle_send_req().

  Input Parameters:
+ vc - virtual connection over which to send the CH3 packet
. sreq - pointer to the send request object
. pkt - pointer to a MPIDI_CH3_Pkt_t structure containing the substructure to 
  be sent
- pkt_sz - size of the packet substucture

  Return value:
  An mpi error code.
  
  NOTE:
  The packet structure may be allocated on the stack.

  IMPLEMETORS:
  If the send can not be completed immediately, the packet structure must be 
  stored internally until the request is complete.

  If the send completes immediately, the channel implementation still must 
  call MPIDI_CH3U_Handle_send_req().
E*/
int MPIDI_CH3_iSend(MPIDI_VC_t * vc, MPID_Request * sreq, void * pkt, 
		    MPIDI_msg_sz_t pkt_sz);


/*E
  MPIDI_CH3_iSendv - A non-blocking request to send a CH3 packet and 
  associated data using an existing request object.  When
  the send is complete the channel implementation will call 
  MPIDI_CH3U_Handle_send_req().

  Input Parameters:
+ vc - virtual connection over which to send the CH3 packet and data
. sreq - pointer to the send request object
. iov - a vector of a structure contains a buffer pointer and length
- iov_n - number of elements in the vector

  Return value:
  An mpi error code.
  
  NOTE:
  The first element in the vector must point to the packet structure.   The packet structure and the vector may be allocated on
  the stack.

  IMPLEMENTORS:
  If the send can not be completed immediately, the packet structure and the vector must be stored internally until the request is
  complete.

  If the send completes immediately, the channel implementation still must call MPIDI_CH3U_Handle_send_req().
E*/
int MPIDI_CH3_iSendv(MPIDI_VC_t * vc, MPID_Request * sreq, MPID_IOV * iov, int iov_n);

/*E
  MPIDI_CH3_Connection_terminate - terminate the underlying connection associated with the specified VC

  Input Parameters:
. vc - virtual connection

  Return value:
  An MPI error code
E*/
int MPIDI_CH3_Connection_terminate(MPIDI_VC_t * vc);

/* MPIDI_CH3_Connect_to_root (really connect to peer) - channel routine
   for connecting to a process through a port, used in implementing
   MPID_Comm_connect and accept */
int MPIDI_CH3_Connect_to_root(const char *, MPIDI_VC_t **);

/* BEGIN EXPERIMENTAL BLOCK */

/* The following functions enable RDMA capabilities in the CH3 device.
 * These functions may change in future releases.
 * There usage is protected in the code by #ifdef MPIDI_CH3_CHANNEL_RNDV
 */

/*E
  MPIDI_CH3U_Handle_recv_rndv_pkt - This function is used by RDMA enabled channels to handle a rts packet.

  Input Parameters:
+ vc - virtual connection over which the packet was received
- pkt - pointer to the CH3 packet

  Output Parameters:
+ rreqp - request pointer
- foundp - found

  Return value:
  An mpi error code.

  Notes:
  This is the handler function to be called when the channel receives a rndv rts packet.
  After this function is called the channel is returned a request and a found flag.  The channel may set any channel
  specific fields in the request at this time.  Then the channel should call MPIDI_CH3U_Post_data_receive() and 
  MPIDI_CH3_iStartRndvTransfer() if the found flag is set.
E*/
int MPIDI_CH3U_Handle_recv_rndv_pkt(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt, MPID_Request ** rreqp, int *foundp);

/*E
  MPIDI_CH3_iStartRndvMsg - This function is used to initiate a rendezvous
  send.

  NOTE: An "rts packet" is provided which must be passed to
  handle_recv_rndv_pkt on the remote side.  The first iov is also provided
  so the channel can register buffers, etc., if neccessary.

  Input Parameters:
+ vc - virtual connection over which the rendezvous will be performed
. sreq - pointer to the send request object
- rts_pkt - CH3 packet to be delivered to CH3 on remote side

  Return value:
  An mpi error code.

  IMPLEMENTORS:
E*/
int MPIDI_CH3_iStartRndvMsg (MPIDI_VC_t * vc, MPID_Request * sreq, MPIDI_CH3_Pkt_t * rts_pkt);

/*E
  MPIDI_CH3_iStartRndvTransfer - This function is used to indicate that a previous
  rendezvous rts has been matched and data transfer can commence.

  Input Parameters:
+ vc - virtual connection over which the rendezvous will be performed
- rreq - pointer to the receive request object

  Return value:
  An mpi error code.

  IMPLEMENTORS:
E*/
int MPIDI_CH3_iStartRndvTransfer (MPIDI_VC_t * vc, MPID_Request * rreq);
/* END EXPERIMENTAL BLOCK */

/*
 * Channel utility prototypes
 */
int MPIDI_CH3U_Recvq_FU(int, int, int, MPI_Status * );
MPID_Request * MPIDI_CH3U_Recvq_FDU(MPI_Request, MPIDI_Message_match *);
MPID_Request * MPIDI_CH3U_Recvq_FDU_or_AEP(int, int, int, int * found);
int MPIDI_CH3U_Recvq_DP(MPID_Request * rreq);
MPID_Request * MPIDI_CH3U_Recvq_FDP(MPIDI_Message_match * match);
MPID_Request * MPIDI_CH3U_Recvq_FDP_or_AEU(MPIDI_Message_match * match, int * found);

#if 0
/* FIXME: These are macros! Why do they have prototypes */
void MPIDI_CH3U_Request_complete(MPID_Request * req);
void MPIDI_CH3U_Request_increment_cc(MPID_Request * req, int * was_incomplete);
void MPIDI_CH3U_Request_decrement_cc(MPID_Request * req, int * incomplete);
#endif

int MPIDI_CH3U_Request_load_send_iov(MPID_Request * const sreq, MPID_IOV * const iov, int * const iov_n);
int MPIDI_CH3U_Request_load_recv_iov(MPID_Request * const rreq);
int MPIDI_CH3U_Request_unpack_uebuf(MPID_Request * rreq);
int MPIDI_CH3U_Request_unpack_srbuf(MPID_Request * rreq);

void MPIDI_CH3U_Buffer_copy(const void * const sbuf, int scount, MPI_Datatype sdt, int * smpi_errno,
			    void * const rbuf, int rcount, MPI_Datatype rdt, MPIDI_msg_sz_t * rdata_sz, int * rmpi_errno);
int MPIDI_CH3U_Post_data_receive(int found, MPID_Request ** rreqp);



/* FIXME: Move these prototypes into header files in the appropriate 
   util directories  */
/* added by brad.  upcalls for MPIDI_CH3_Init that contain code which could be executed by two or more channels */
int MPIDI_CH3U_Init_sock(int has_parent, MPIDI_PG_t * pg_p, int pg_rank,
                         char **bc_val_p, int *val_max_sz_p);                         
int MPIDI_CH3U_Init_sshm(int has_parent, MPIDI_PG_t * pg_p, int pg_rank,
                         char **bc_val_p, int *val_max_sz_p);

int MPIDI_SHM_InitRWProc( pid_t, int * );
int MPIDI_SHM_AttachProc( pid_t );
int MPIDI_SHM_DetachProc( pid_t );
int MPIDI_SHM_ReadProcessMemory( int, int, const char *, char *, size_t );

/* added by brad.  business card related global and functions */
/* FIXME: Make these part of the channel support headers */
#define MAX_HOST_DESCRIPTION_LEN 256
int MPIDI_CH3U_Get_business_card_sock(char **bc_val_p, int *val_max_sz_p);
int MPIDI_CH3U_Get_business_card_sshm(char **bc_val_p, int *val_max_sz_p);
int MPIDI_CH3I_Get_business_card(char *value, int length);

/* added by brad.  finalization related upcalls */
int MPIDI_CH3U_Finalize_sshm(void);

/*E
  MPIDI_CH3_Cancel_send - Attempt to cancel a send request by removing the 
  request from the local send queue.

  Input Parameters:
+ vc - virtual connection over which to send the data 
- sreq - pointer to the send request object

  Output Parameters:
. cancelled - TRUE if the send request was successful.  FALSE otherwise.

  Return value:
  An mpi error code.
  
  IMPLEMENTORS:
  The send request may not be removed from the send queue if one or more bytes 
  of the message have already been sent.
E*/
int MPIDI_CH3_Cancel_send(MPIDI_VC_t * vc, MPID_Request * sreq, int *cancelled);
/*
 * Channel upcall prototypes
 */

/*E
  MPIDI_CH3U_Handle_recv_pkt- Handle a freshly received CH3 packet.

  Input Parameters:
+ vc - virtual connection over which the packet was received
- pkt - pointer to the CH3 packet

  Output Parameter:
. rreqp - receive request defining data to be received; may be NULL

  NOTE:
  Multiple threads may not simultaneously call this routine with the same 
  virtual connection.  This constraint eliminates the
  need to lock the VC and thus improves performance.  If simultaneous upcalls 
  for a single VC are a possible, then the calling
  routine must serialize the calls (perhaps by locking the VC).  Special 
  consideration may need to be given to packet ordering
  if the channel has made guarantees about ordering.
E*/
int MPIDI_CH3U_Handle_recv_pkt(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt, MPID_Request ** rreqp);

/*E
  MPIDI_CH3U_Handle_recv_req - Process a receive request for which all of the data has been received (and copied) into the
  buffers described by the request's IOV.

  Input Parameters:
+ vc - virtual connection over which the data was received
- rreq - pointer to the receive request object

  Output Parameter:
. complete - data transfer for the request has completed
E*/
int MPIDI_CH3U_Handle_recv_req(MPIDI_VC_t * vc, MPID_Request * rreq, int * complete);


/*E
  MPIDI_CH3U_Handle_send_req - Process a send request for which all of the data described the request's IOV has been completely
  buffered and/or sent.

  Input Parameters:
+ vc - virtual connection over which the data was sent
- sreq - pointer to the send request object

  Output Parameter:
. complete - data transfer for the request has completed
E*/
int MPIDI_CH3U_Handle_send_req(MPIDI_VC_t * vc, MPID_Request * sreq, int * complete);

int MPIDI_CH3U_Handle_connection(MPIDI_VC_t * vc, MPIDI_VC_Event_t event);

int MPIDI_CH3U_VC_SendClose( MPIDI_VC_t *vc, int rank );
int MPIDI_CH3U_VC_WaitForClose( void );


/*E
  MPIDI_CH3_Init - Initialize the channel implementation.

  Input Parameters:
+ has_parent - boolean value that is true if this MPI job was spawned by another set of MPI processes
. pg_ptr - the new process group representing MPI_COMM_WORLD
- pg_rank - my rank in the process group

  Return value:
  A MPI error code.

  Notes:
  MPID_Init has called 'PMI_Init' and created the process group structure 
  before this routine is called.
E*/
int MPIDI_CH3_Init(int has_parent, MPIDI_PG_t *pg_ptr, int pg_rank );

/*E
  MPIDI_CH3_Finalize - Shutdown the channel implementation.

  Return value:
  A MPI error class.
E*/
int MPIDI_CH3_Finalize(void);

/* Routines in support of ch3 */

/* Implement the send side of a rendevous send */
int MPIDI_CH3_RndvSend( MPID_Request **sreq_p, const void * buf, int count, 
			MPI_Datatype datatype, int dt_contig, int data_sz, 
			int rank, 
			int tag, MPID_Comm * comm, int context_offset );

/* Here are the packet handlers */
int MPIDI_CH3_PktHandler_EagerSend( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
				   MPID_Request ** );
int MPIDI_CH3_PktHandler_ReadySend( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
				    MPID_Request ** );
int MPIDI_CH3_PktHandler_EagerSyncSend( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
					MPID_Request ** );
int MPIDI_CH3_PktHandler_EagerSyncAck( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
				       MPID_Request ** );
int MPIDI_CH3_PktHandler_RndvReqToSend( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
					MPID_Request ** );
int MPIDI_CH3_PktHandler_RndvClrToSend( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
					MPID_Request ** );
int MPIDI_CH3_PktHandler_RndvSend( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
				   MPID_Request ** );
int MPIDI_CH3_PktHandler_CancelSendReq( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
					MPID_Request ** );
int MPIDI_CH3_PktHandler_CancelSendResp( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
					 MPID_Request ** );
int MPIDI_CH3_PktHandler_Put( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
			      MPID_Request ** );
int MPIDI_CH3_PktHandler_Accumulate( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
				     MPID_Request ** );
int MPIDI_CH3_PktHandler_Get( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
			      MPID_Request ** );
int MPIDI_CH3_PktHandler_GetResp( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
				 MPID_Request ** );
int MPIDI_CH3_PktHandler_Lock( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
			      MPID_Request ** );
int MPIDI_CH3_PktHandler_LockGranted( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
				      MPID_Request ** );
int MPIDI_CH3_PktHandler_PtRMADone( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
				    MPID_Request ** );
int MPIDI_CH3_PktHandler_LockPutUnlock( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
					MPID_Request ** );
int MPIDI_CH3_PktHandler_LockAccumUnlock( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
					  MPID_Request ** );
int MPIDI_CH3_PktHandler_LockGetUnlock( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
					MPID_Request ** );
int MPIDI_CH3_PktHandler_FlowCntlUpdate( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt,
					 MPID_Request ** );
int MPIDI_CH3_PktHandler_Close( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
				MPID_Request ** );
int MPIDI_CH3_PktHandler_EndCH3( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *,
				 MPID_Request ** );

int MPIDI_CH3_PktHandler_CancelSendReq( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
					MPID_Request ** );
int MPIDI_CH3_PktHandler_CancelSendResp( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, 
					 MPID_Request ** );
#ifdef MPICH_DBG_OUTPUT
int MPIDI_CH3_PktPrint_CancelSendReq( FILE *, MPIDI_CH3_Pkt_t * );
int MPIDI_CH3_PktPrint_CancelSendResp( FILE *, MPIDI_CH3_Pkt_t * );
int MPIDI_CH3_PktPrint_EagerSend( FILE *, MPIDI_CH3_Pkt_t * );
int MPIDI_CH3_PktPrint_ReadySend( FILE *, MPIDI_CH3_Pkt_t * );
int MPIDI_CH3_PktPrint_RndvReqToSend( FILE *, MPIDI_CH3_Pkt_t * );
int MPIDI_CH3_PktPrint_RndvClrToSend( FILE *, MPIDI_CH3_Pkt_t * );
int MPIDI_CH3_PktPrint_RndvSend( FILE *, MPIDI_CH3_Pkt_t * );
#endif

/* Routines to create packets (used in implementing MPI communications */
int MPIDI_CH3_EagerNoncontigSend( MPID_Request **, int,  const void *, int, 
				  MPI_Datatype, int, int, int, MPID_Comm *, 
				  int );
int MPIDI_CH3_EagerContigSend( MPID_Request **, int, const void *, int, int, 
			       int, MPID_Comm *, int );
int MPIDI_CH3_EagerContigShortSend( MPID_Request **, int, const void *, int, 
				    int, int, MPID_Comm *, int );
int MPIDI_CH3_EagerContigIsend( MPID_Request **, int, const void *, int, int, 
				int, MPID_Comm *, int );


int MPIDI_CH3_RndvSend( MPID_Request **, const void *, int, MPI_Datatype, 
			int, int, int, int, MPID_Comm *, int );

int MPIDI_CH3_EagerSyncNoncontigSend( MPID_Request **, const void *, int, 
				      MPI_Datatype, int, int, MPI_Aint,
				      int, int, MPID_Comm *, int );
int MPIDI_CH3_EagerSyncZero(MPID_Request **, int, int, MPID_Comm *, int );

/* Routines to ack packets, called in the receive routines when a 
   message is matched */
int MPIDI_CH3_EagerSyncAck( MPIDI_VC_t *, MPID_Request * );
int MPIDI_CH3_RecvFromSelf( MPID_Request *, void *, int, MPI_Datatype );
int MPIDI_CH3_RecvRndv( MPIDI_VC_t *, MPID_Request * );

#endif /* !defined(MPICH_MPIDIMPL_H_INCLUDED) */
