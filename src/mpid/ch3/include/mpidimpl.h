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
}
MPIDI_Process_t;

extern MPIDI_Process_t MPIDI_Process;

/*
 * Request utility macros (internal - do not use in MPID macros)
 */
#define MPIDI_CH3U_Request_create(req)				\
{								\
    req->ref_count = 1;						\
    req->cc = 1;						\
    req->cc_ptr = &req->cc;					\
    req->status.MPI_SOURCE = MPI_UNDEFINED;			\
    req->status.MPI_TAG = MPI_UNDEFINED;			\
    req->status.MPI_ERROR = MPI_SUCCESS;			\
    req->status.cancelled = FALSE;				\
    MPIDI_Request_state_init(req);				\
    req->comm = NULL;						\
								\
    /* XXX - initialized only for debugging purposes? */	\
    req->ch3.vc = NULL;						\
    req->ch3.user_buf = NULL;					\
    req->ch3.datatype = MPI_DATATYPE_NULL;			\
    req->ch3.tmpbuf = NULL;					\
}

#define MPIDI_CH3U_Request_destroy(req)			\
{							\
    if (MPIDI_Request_get_srbuf_flag(req))		\
    {							\
	MPIDI_CH3U_SRBuf_free(req);			\
    }							\
}

#define MPIDI_CH3U_Request_complete(req)	\
{						\
    int cc;					\
						\
    MPIDI_CH3U_Request_decrement_cc(req, &cc);	\
    if (cc == 0)				\
    {						\
	MPID_Request_release(req);		\
	MPIDI_CH3_Progress_signal_completion();	\
    }						\
}

/* Masks and flags for channel device state in an MPID_Request */
#define MPIDI_Request_state_init(req)		\
{						\
    req->ch3.state = 0;				\
}

#define MPIDI_REQUEST_MSG_MASK (0x3 << MPIDI_REQUEST_MSG_SHIFT)
#define MPIDI_REQUEST_MSG_SHIFT 0
#define MPIDI_REQUEST_NO_MSG 0
#define MPIDI_REQUEST_EAGER_MSG 1
#define MPIDI_REQUEST_RNDV_MSG 2

#define MPIDI_Request_get_msg_type(req)					\
((req->ch3.state & MPIDI_REQUEST_MSG_MASK) >> MPIDI_REQUEST_MSG_SHIFT)

#define MPIDI_Request_set_msg_type(req, msgtype)		\
{								\
    req->ch3.state &= ~MPIDI_REQUEST_MSG_MASK;			\
    req->ch3.state |= (msgtype << MPIDI_REQUEST_MSG_SHIFT)	\
	& MPIDI_REQUEST_MSG_MASK;				\
}

#define MPIDI_REQUEST_SRBUF_MASK (0x1 << MPIDI_REQUEST_SRBUF_SHIFT)
#define MPIDI_REQUEST_SRBUF_SHIFT 2

#define MPIDI_Request_get_srbuf_flag(req)				     \
((req->ch3.state & MPIDI_REQUEST_SRBUF_MASK) >> MPIDI_REQUEST_SRBUF_SHIFT)

#define MPIDI_Request_set_srbuf_flag(req, flag)			\
{								\
    req->ch3.state &= ~MPIDI_REQUEST_SRBUF_MASK;		\
    req->ch3.state |= (flag << MPIDI_REQUEST_SRBUF_SHIFT)	\
	& MPIDI_REQUEST_SRBUF_MASK;				\
}

#define MPIDI_REQUEST_SYNC_SEND_MASK (0x1 << MPIDI_REQUEST_SYNC_SEND_SHIFT)
#define MPIDI_REQUEST_SYNC_SEND_SHIFT 3

#define MPIDI_Request_get_sync_send_flag(req)		\
((req->ch3.state & MPIDI_REQUEST_SYNC_SEND_MASK) >>	\
 MPIDI_REQUEST_SYNC_SEND_SHIFT)

#define MPIDI_Request_set_sync_send_flag(req, flag)		\
{								\
    req->ch3.state &= ~MPIDI_REQUEST_SYNC_SEND_MASK;		\
    req->ch3.state |= (flag << MPIDI_REQUEST_SYNC_SEND_SHIFT)	\
	& MPIDI_REQUEST_SYNC_SEND_MASK;				\
}

/*
 * Send/Receive buffer macros
 */
#if !defined(MPIDI_CH3U_SRBuf_size)
#define MPIDI_CH3U_SRBuf_size (16384)
#endif

#if !defined(MPIDI_CH3U_SRBuf_alloc)
#define MPIDI_CH3U_SRBuf_alloc(req, size)			\
{								\
    req->ch3.tmpbuf = MPIU_Malloc(MPIDI_CH3U_SRBuf_size);	\
    if (req->ch3.tmpbuf != NULL)				\
    {								\
	req->ch3.tmpbuf_sz = MPIDI_CH3U_SRBuf_size;		\
	MPIDI_Request_set_srbuf_flag(req, TRUE);		\
    }								\
    else							\
    {								\
	req->ch3.tmpbuf_sz = 0;					\
    }								\
}
#endif

#if !defined(MPIDI_CH3U_SRBuf_free)
#define MPIDI_CH3U_SRBuf_free(req)		\
{						\
    assert(MPIDI_Request_get_srbuf_flag(req));	\
    MPIDI_Request_set_srbuf_flag(req, FALSE);	\
    MPIU_Free(req->ch3.tmpbuf);			\
}
#endif


/*
 * Debugging tools
 */
void MPIDI_dbg_printf(int, char *, char *, ...);
void MPIDI_err_printf(char *, char *, ...);

#if defined(MPICH_DBG_OUTPUT)
#define MPIDI_DBG_PRINTF(e) MPIDI_dbg_printf e
#define MPIDI_dbg_printf(level, func, fmt, args...)			\
{									\
    MPIU_DBG_PRINTF(("%d (%d) %s(): " fmt "\n",				\
	       MPIR_Process.comm_world->rank, level, func, ## args));	\
}
#else
#define MPIDI_DBG_PRINTF(e)
#define MPIDI_dbg_printf(level, func, fmt, args...)
#endif

#define MPIDI_ERR_PRINTF(e) MPIDI_err_printf e
#define MPIDI_err_printf(func, fmt, args...)			\
{								\
    printf("%d ERROR - %s(): " fmt "\n",			\
	   MPIR_Process.comm_world->rank, func, ## args);	\
    fflush(stdout);						\
}

#define MPIDI_QUOTE(A) MPIDI_QUOTE2(A)
#define MPIDI_QUOTE2(A) #A


/* Prototypes for collective operations supplied by the device (or channel) */
int MPIDI_Barrier(MPID_Comm *);


/* Channel function prototypes are in mpidi_ch3_post.h since some of the macros
   require their declarations. */

#endif /* !defined(MPICH_MPIDIMPL_H_INCLUDED) */
