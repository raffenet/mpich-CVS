/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDIMPL_H_INCLUDED)
#define MPICH_MPIDIMPL_H_INCLUDED

#include <assert.h>
#include <sys/uio.h>	/* needed for struct iovec */
#if !defined(IOV_MAX)
#define IOV_MAX 16
#endif

#include "mpiimpl.h"

#include "mpidi_ch3_conf.h"

typedef struct
{
    MPID_Request * recv_posted_head;
    MPID_Request * recv_posted_tail;
    MPID_Request * recv_unexpected_head;
    MPID_Request * recv_unexpected_tail;
}
MPIDI_Process_t;

extern MPIDI_Process_t MPIDI_Process;

/* Masks and flags for MPID state in an MPID_Request */
#define MPID_REQUEST_STATE_MSG_MASK 1
#define MPID_REQUEST_STATE_EAGER_MSG 0
#define MPID_REQUEST_STATE_RNDV_MSG 0

/*
 * Debugging tools
 */
void MPIDI_dbg_printf(int, char *, char *, ...);
void MPIDI_err_printf(char *, char *, ...);

#define MPIDI_dbg_printf(level, func, fmt, args...)			\
{									\
    printf("%d (%d) %s(): " ## fmt ## "\n",				\
	   MPIR_Process.comm_world->rank, level, func, ## args);	\
    fflush(stdout);							\
}

#define MPIDI_err_printf(func, fmt, args...)			\
{								\
    printf("%d %s(): " ## fmt ## "\n",				\
	   MPIR_Process.comm_world->rank, func, ## args);	\
    fflush(stdout);						\
}

#define MPIDI_QUOTE(A) MPIDI_QUOTE2(A)
#define MPIDI_QUOTE2(A) #A

/*
 * CH3 Utilities
 */
#define MPIDI_CH3U_Request_decrement_cc(req, cc)	\
{							\
    *cc = --(*req->cc_ptr);				\
}


/* Channel function prototypes are in mpidi_ch3_post.h since some of the macros
   require their declarations. */

#endif /* !defined(MPICH_MPIDIMPL_H_INCLUDED) */
