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

/* Masks and flags for channel device state in an MPID_Request */
#define MPIDI_Request_state_reset(req)		\
{						\
    req->ch3.state = 0;				\
}

#define MPIDI_REQUEST_MSG_MASK 3
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

#define MPIDI_REQUEST_TMPBUF_MASK 1
#define MPIDI_REQUEST_TMPBUF_SHIFT 2

#define MPIDI_Request_get_tmpbuf_flag(req)				     \
((req->ch3.state & MPIDI_REQUEST_TMPBUF_MASK) >> MPIDI_REQUEST_TMPBUF_SHIFT)

#define MPIDI_Request_set_tmpbuf_flag(req, flag)		\
{								\
    req->ch3.state &= ~MPIDI_REQUEST_TMPBUF_MASK;		\
    req->ch3.state |= (flag << MPIDI_REQUEST_TMPBUF_SHIFT)	\
	& MPIDI_REQUEST_TMPBUF_MASK;				\
}


/*
 * Debugging tools
 */
void MPIDI_dbg_printf(int, char *, char *, ...);
void MPIDI_err_printf(char *, char *, ...);

#if defined(MPICH_DBG_OUTPUT)
#define MPIDI_DBG_PRINTF(e) MPIDI_dbg_printf e
#define MPIDI_dbg_printf(level, func, fmt, args...)			\
{									\
    MPIU_DBG_PRINTF(("%d (%d) %s(): " ## fmt ## "\n",			\
	       MPIR_Process.comm_world->rank, level, func, ## args));	\
}
#else
#define MPIDI_DBG_PRINTF(e)
#define MPIDI_dbg_printf(level, func, fmt, args...)
#endif

#define MPIDI_err_printf(func, fmt, args...)			\
{								\
    printf("%d %s(): " ## fmt ## "\n",				\
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
