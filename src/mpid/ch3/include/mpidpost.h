/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDPOST_H_INCLUDED)
#define MPICH_MPIDPOST_H_INCLUDED

#define MPID_Request_free(req) {MPIDI_CH3_Request_free(req);}

#define MPID_Progress_start()
#define MPID_Progress_end()
#define MPID_Progress_test() (MPIDI_CH3_Progress(0))
#define MPID_Progress_wait() {MPIDI_CH3_Progress(1);}
#define MPID_Progress_poke() {MPIDI_CH3_Progress_poke();}

/* Channel API prototypes */
int MPIDI_CH3_Init(int *, int *, int *);
int MPIDI_CH3_Finalize(void);
void MPIDI_CH3_InitParent(MPID_Comm *);

MPID_Request * MPIDI_CH3_iStartMsg(MPIDI_VC *, void *, int);
MPID_Request * MPIDI_CH3_iStartMsgv(MPIDI_VC *, struct iovec *, int);
void MPIDI_CH3_iSendv(MPIDI_VC *, MPID_Request *, struct iovec *, int);
void MPIDI_CH3_iSend(MPIDI_VC *, MPID_Request *, void *, int);
void MPIDI_CH3_iWrite(MPIDI_VC *, MPID_Request *);
void MPIDI_CH3_iRead(MPIDI_VC *, MPID_Request *);

MPID_Request * MPIDI_CH3_Request_new();
void MPIDI_CH3_Request_free(MPID_Request *);

void MPIDI_CH3_Progress_start(void);
void MPIDI_CH3_Progress_end(void);
int MPIDI_CH3_Progress(int);
void MPIDI_CH3_Progress_poke(void);

/* Channel utility prototypes */
MPID_Request * MPIDI_CH3U_Request_FUOAP(
    int source, int tag, int context_id, int * found);
MPID_Request * MPIDI_CH3U_Request_FPOAU(
    MPIDI_Message_match * match, int * found);

/* Include definitions from the channel which require items defined by this
   file (mpidimpl.h) or the file it includes (mpiimpl.h).  NOTE: This include
   requires the device to copy mpidi_ch3_post.h to the src/include directory in
   the build tree. */
#include "mpidi_ch3_post.h"

#endif /* !defined(MPICH_MPIDPOST_H_INCLUDED) */

