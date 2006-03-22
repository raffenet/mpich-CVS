/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_POST_H_INCLUDED)
#define MPICH_MPIDI_CH3_POST_H_INCLUDED

/* #define MPIDI_CH3_EAGER_MAX_MSG_SIZE (1500 - sizeof(MPIDI_CH3_Pkt_t)) */
#define MPIDI_CH3_EAGER_MAX_MSG_SIZE   (128*1024)

/*
 * Channel level request management macros
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

/*
 * MPIDI_CH3_Progress_signal_completion() is used to notify the progress
 * engine that a completion has occurred.  The multi-threaded version will need
 * to wake up any (and all) threads blocking in MPIDI_CH3_Progress().
 */
extern volatile unsigned int MPIDI_CH3I_progress_completions;

#if !defined(MPICH_IS_THREADED)
#define MPIDI_CH3_Progress_signal_completion()	\
{						\
    MPIDI_CH3I_progress_completions++;		\
}
#else
#error Multi-threaded MPIDI_CH3_Progress_notify_completion() not implemented.
/* XXX - this routine need to wake up any threads blocking in the progress
   engine */
#endif

#if !defined(MPICH_IS_THREADED)
#define MPIDI_CH3_Progress_start(state)
#define MPIDI_CH3_Progress_end(state)
#endif

enum {
    MPIDI_CH3_start_packet_handler_id = 128,
    MPIDI_CH3_continue_packet_handler_id = 129,
    MPIDI_CH3_CTS_packet_handler_id = 130,
    MPIDI_CH3_reload_IOV_or_done_handler_id = 131,
    MPIDI_CH3_reload_IOV_reply_handler_id = 132
};


int MPIDI_CH3I_Progress(int blocking);
MPID_Request *MPIDI_CH3_Progress_poke_with_matching(int,int,MPID_Comm *comm,int,int *,void *,int, MPI_Datatype, MPI_Status *);
MPID_Request *MPIDI_CH3_Progress_ipoke_with_matching(int,int,MPID_Comm *comm,int,int *,void *,int, MPI_Datatype, MPI_Status *);
#define MPIDI_CH3_Progress_test() MPIDI_CH3I_Progress(FALSE)
#define MPIDI_CH3_Progress_wait(state) MPIDI_CH3I_Progress(TRUE)

int MPIDI_CH3I_Posted_recv_enqueued (MPID_Request *rreq);
int MPIDI_CH3I_Posted_recv_dequeued (MPID_Request *rreq);

/*
 * Enable optional functionality
 */
#define MPIDI_CH3_Comm_Spawn MPIDI_CH3_Comm_Spawn

#endif /* !defined(MPICH_MPIDI_CH3_POST_H_INCLUDED) */
