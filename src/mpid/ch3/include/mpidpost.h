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
MPID_Request * MPIDI_CH3_iStartMsgv(MPIDI_VC *, struct iovec *, int);
void MPIDI_CH3_iSendv(MPIDI_VC *, MPID_Request *, struct iovec *, int);
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

/*
 * Channel utility prototypes
 */
MPID_Request * MPIDI_CH3U_Request_FU(int, int, int);
MPID_Request * MPIDI_CH3U_Request_FDU(MPI_Request, MPIDI_Message_match *);
MPID_Request * MPIDI_CH3U_Request_FDU_or_AEP(int, int, int, int *);
int MPIDI_CH3U_Request_FDP(MPID_Request *);
MPID_Request * MPIDI_CH3U_Request_FDP_or_AEU(MPIDI_Message_match *, int *);
int MPIDI_CH3U_Request_adjust_iov(MPID_Request *, int);
int MPIDI_CH3U_Handle_recv_pkt(MPIDI_VC *, MPIDI_CH3_Pkt_t *);
int MPIDI_CH3U_Handle_recv_req(MPIDI_VC *, MPID_Request *);
int MPIDI_CH3U_Handle_send_req(MPIDI_VC *, MPID_Request *);
void MPIDI_CH3U_Request_copy_tmp_data(MPID_Request *);

/*
 * Macros defining both device and channel level request management routines
 */
#define MPIDI_CH3_Request_add_ref(req)				\
{								\
    assert(HANDLE_GET_MPI_KIND(req->handle) == MPID_REQUEST);	\
    MPIU_Object_add_ref(req);					\
}

#define MPIDI_CH3_Request_release_ref(req, req_ref_count)	\
{								\
    assert(HANDLE_GET_MPI_KIND(req->handle) == MPID_REQUEST);	\
    MPIU_Object_release_ref(req, req_ref_count);		\
    assert(req->ref_count >= 0);				\
}

#define MPID_Request_create() (MPIDI_CH3_Request_create())

#define MPID_Request_set_complete(req) {*req->cc_ptr = 0;}

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

/*
 * Macros defining device level progress engine routines
 */
#define MPID_Progress_start()
#define MPID_Progress_end()
#define MPID_Progress_test() (MPIDI_CH3_Progress(FALSE))
#define MPID_Progress_wait() {MPIDI_CH3_Progress(TRUE);}
#define MPID_Progress_poke() {MPIDI_CH3_Progress_poke();}

/* Include definitions from the channel which require items defined by this
   file (mpidimpl.h) or the file it includes (mpiimpl.h).  NOTE: This include
   requires the device to copy mpidi_ch3_post.h to the src/include directory in
   the build tree. */
#include "mpidi_ch3_post.h"

#endif /* !defined(MPICH_MPIDPOST_H_INCLUDED) */

