/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDPOST_H_INCLUDED)
#define MPICH_MPIDPOST_H_INCLUDED

/*
 * Channel API prototypes
 */
int MPIDI_CH3_Init(int *, int *, int *);
int MPIDI_CH3_Finalize(void);
void MPIDI_CH3_InitParent(MPID_Comm *);

MPID_Request * MPIDI_CH3_iStartMsg(MPIDI_VC *, void *, int);
MPID_Request * MPIDI_CH3_iStartMsgv(MPIDI_VC *, MPID_IOV *, int);
void MPIDI_CH3_iSendv(MPIDI_VC *, MPID_Request *, MPID_IOV *, int);
void MPIDI_CH3_iSend(MPIDI_VC *, MPID_Request *, void *, int);
void MPIDI_CH3_iWrite(MPIDI_VC *, MPID_Request *);
void MPIDI_CH3_iRead(MPIDI_VC *, MPID_Request *);

MPID_Request * MPIDI_CH3_Request_create(void);
void MPIDI_CH3_Request_add_ref(MPID_Request *);
int MPIDI_CH3_Request_release_ref(MPID_Request *, int *);
void MPIDI_CH3_Request_destroy(MPID_Request *);

void MPIDI_CH3_Progress_start(void);
void MPIDI_CH3_Progress_end(void);
int MPIDI_CH3_Progress(int);
void MPIDI_CH3_Progress_poke(void);
void MPIDI_CH3_Progress_signal_completion(void);

/*
 * Channel utility prototypes
 */
void MPIDI_CH3U_Handle_recv_pkt(MPIDI_VC *, MPIDI_CH3_Pkt_t *);
void MPIDI_CH3U_Handle_recv_req(MPIDI_VC *, MPID_Request *);
void MPIDI_CH3U_Handle_send_req(MPIDI_VC *, MPID_Request *);
MPID_Request * MPIDI_CH3U_Request_FU(int, int, int);
MPID_Request * MPIDI_CH3U_Request_FDU(MPI_Request, MPIDI_Message_match *);
MPID_Request * MPIDI_CH3U_Request_FDU_or_AEP(int, int, int, int *);
int MPIDI_CH3U_Request_FDP(MPID_Request *);
MPID_Request * MPIDI_CH3U_Request_FDP_or_AEU(MPIDI_Message_match *, int *);
int MPIDI_CH3U_Request_adjust_iov(MPID_Request *, int);
int MPIDI_CH3U_Request_unpack_tmp_buf(MPID_Request *);
void MPIDI_CH3U_Request_decrement_cc(MPID_Request *, int *);

/* Include definitions from the channel which require items defined by this
   file (mpidimpl.h) or the file it includes (mpiimpl.h).  NOTE: This include
   requires the device to copy mpidi_ch3_post.h to the src/include directory in
   the build tree. */
#include "mpidi_ch3_post.h"

/*
 * Request utility macros (public - can be used in MPID macros)
 */
#if defined(MPICH_SINGLE_THREADED)
/* XXX - In the case of a single-threaded shmem channel sharing requests
   between processes, a write barrier must be performed before decrementing the
   completion counter.  This insures that other fields in the req structure are
   updated before the completion is signaled.  How should that be incorporated
   into this code at the device level? */
#define MPIDI_CH3U_Request_decrement_cc(req, flagp)	\
{							\
    *flagp = --*req->cc_ptr;				\
}
#else
/* MT - A write barrier must be performed before decrementing the completion
   counter .  This insures that other fields in the req structure are updated
   before the completion is signaled. */
#error Multi-threaded MPIDI_CH3U_Request_decrement_cc() not implemented.
#endif


/*
 * Device level request management macros
 */
#define MPID_Request_create() (MPIDI_CH3_Request_create())

#define MPID_Request_release(req)				\
{								\
    int ref_count;						\
    								\
    MPIDI_CH3_Request_release_ref(req, &ref_count);		\
    if (ref_count == 0)						\
    {								\
	MPIDI_CH3_Request_destroy(req);				\
    }								\
}

#if defined(MPICH_SINGLE_THREADED)
#define MPID_Request_set_complete(req)		\
{						\
    *req->cc_ptr = 0;				\
    MPIDI_CH3_Progress_signal_completion();	\
}
#else
/* MT - A write barrier must be performed before decrementing the completion
   counter .  This insures that other fields in the req structure are updated
   before the completion is signaled. */
#error Multi-threaded MPID_Request_set_complete() not implemented.
#endif


/*
 * Device level progress engine macros
 *
 * MT - these macros need thread-safety updates
 */
#define MPID_Progress_start()
#define MPID_Progress_end()
#define MPID_Progress_test() (MPIDI_CH3_Progress(FALSE))
#define MPID_Progress_wait() {MPIDI_CH3_Progress(TRUE);}
#define MPID_Progress_poke() {MPIDI_CH3_Progress_poke();}

#endif /* !defined(MPICH_MPIDPOST_H_INCLUDED) */

