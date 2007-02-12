/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem_impl.h"
#include "mpid_nem_datatypes.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>

#ifdef ENABLE_NO_SCHED_YIELD
#define SCHED_YIELD() do { } while(0)
#else
#include <sched.h>
#define SCHED_YIELD() sched_yield()
#endif

int MPID_nem_lmt_shm_pending = FALSE;

/* Progress queue */

typedef struct lmt_shm_prog_element
{
    MPIDI_VC_t *vc;
    struct lmt_shm_prog_element *next;
    struct lmt_shm_prog_element *prev;
} lmt_shm_prog_element_t;

static struct {lmt_shm_prog_element_t *head;} lmt_shm_progress_q = {NULL};

#define LMT_SHM_L_EMPTY() GENERIC_L_EMPTY(lmt_shm_progress_q)
#define LMT_SHM_L_HEAD() GENERIC_L_HEAD(lmt_shm_progress_q)
#define LMT_SHM_L_REMOVE(ep) GENERIC_L_REMOVE(&lmt_shm_progress_q, ep, next, prev)
#define LMT_SHM_L_ADD(ep) GENERIC_L_ADD(&lmt_shm_progress_q, ep, next, prev)

typedef struct MPID_nem_lmt_shm_wait_element
{
    int (* progress)(MPIDI_VC_t *vc, MPID_Request *req, int *done);
    MPID_Request *req;
    struct MPID_nem_lmt_shm_wait_element *next;
} MPID_nem_lmt_shm_wait_element_t;

#define LMT_SHM_Q_EMPTY(qp) GENERIC_Q_EMPTY(qp)
#define LMT_SHM_Q_HEAD(qp) GENERIC_Q_HEAD(qp)
#define LMT_SHM_Q_ENQUEUE(qp, ep) GENERIC_Q_ENQUEUE(qp, ep, next)
#define LMT_SHM_Q_DEQUEUE(qp, epp) GENERIC_Q_DEQUEUE(qp, epp, next)
#define LMT_SHM_Q_SEARCH_REMOVE(qp, req_id, epp) GENERIC_Q_SEARCH_REMOVE(qp, _e->req->handle == (req_id), epp, \
                                                                         MPID_nem_lmt_shm_wait_element_t, next)
/* copy buffer in shared memory */
#define MPID_NEM_COPY_BUF_LEN (64 * 1024)

typedef union
{
    int val;
    char padding[MPID_NEM_CACHE_LINE_LEN];
} MPID_nem_cacheline_int_t;

typedef union
{
    struct
    {
        int rank;
        int remote_req_id;
    } val;
    char padding[MPID_NEM_CACHE_LINE_LEN];
} MPID_nem_cacheline_owner_info_t;

typedef struct MPID_nem_copy_buf
{
    MPID_nem_cacheline_owner_info_t owner_info;
/*     MPID_nem_cacheline_int_t reader_rank; /\* is the copy buffer in use?  Sender sets this to TRUE, receiver sets this to FALSE. *\/ */
    MPID_nem_cacheline_int_t sender_present; /* is the sender currently in the lmt progress function for this buffer */
    MPID_nem_cacheline_int_t receiver_present; /* is the receiver currently in the lmt progress function for this buffer */
    MPID_nem_cacheline_int_t flag[2];
    char buf[2][MPID_NEM_COPY_BUF_LEN];
} MPID_nem_copy_buf_t;
/* copy buffer flag values */
#define BUF_EMPTY 0
#define BUF_FULL  1
#define BUF_DONE  2

#define NO_OWNER -1


static inline int lmt_shm_progress_vc(MPIDI_VC_t *vc, int *done);
static int lmt_shm_send_progress(MPIDI_VC_t *vc, MPID_Request *req, int *done);
static int lmt_shm_recv_progress(MPIDI_VC_t *vc, MPID_Request *req, int *done);
static int MPID_nem_allocate_shm_region(volatile MPID_nem_copy_buf_t **buf_p, char *handle[]);
static int MPID_nem_attach_shm_region(volatile MPID_nem_copy_buf_t **buf_p, const char handle[]);
static int MPID_nem_detach_shm_region(volatile MPID_nem_copy_buf_t *buf, char handle[]);
static int MPID_nem_delete_shm_region(volatile MPID_nem_copy_buf_t *buf, char handle[]);

/* number of iterations to wait for the other side to process a buffer */
#define NUM_BUSY_POLLS 1000

#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_shm_initiate_lmt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_lmt_shm_initiate_lmt(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, MPID_Request *req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LMT_SHM_INITIATE_LMT);
    MPIDI_msg_sz_t data_sz;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPID_Datatype * dt_ptr;
    MPID_nem_pkt_lmt_rts_t * const rts_pkt = &pkt->lmt_rts;

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LMT_SHM_INITIATE_LMT);

    MPID_nem_lmt_send_RTS(vc, rts_pkt, NULL, 0);

    MPIDI_Datatype_get_info(req->dev.user_count, req->dev.datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    req->ch.lmt_data_sz = data_sz;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LMT_SHM_INITIATE_LMT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_shm_start_recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_lmt_shm_start_recv(MPIDI_VC_t *vc, MPID_Request *req, MPID_IOV s_cookie)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_IOV r_cookie;
    const MPIDI_msg_sz_t data_sz = req->ch.lmt_data_sz;
    volatile MPID_nem_copy_buf_t * const copy_buf = vc->ch.lmt_copy_buf;
    int first;
    int last;
    int buf_num;
    int done = FALSE;
    MPIU_CHKPMEM_DECL(2);
    MPID_nem_lmt_shm_wait_element_t *e;
    int queue_initially_empty;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LMT_SHM_START_RECV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LMT_SHM_START_RECV);

    if (vc->ch.lmt_copy_buf == NULL)
    {
        mpi_errno = MPID_nem_allocate_shm_region(&vc->ch.lmt_copy_buf, &vc->ch.lmt_copy_buf_handle);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        vc->ch.lmt_copy_buf->sender_present.val   = 0;
        vc->ch.lmt_copy_buf->receiver_present.val = 0;
        
        vc->ch.lmt_copy_buf->flag[0].val = BUF_EMPTY;
        vc->ch.lmt_copy_buf->flag[1].val = BUF_EMPTY;

        vc->ch.lmt_copy_buf->owner_info.val.rank          = NO_OWNER;
        vc->ch.lmt_copy_buf->owner_info.val.remote_req_id = MPI_REQUEST_NULL;
    }

    /* send CTS with handle for copy buffer */
    MPID_nem_lmt_send_CTS(vc, req, vc->ch.lmt_copy_buf_handle, (int)strlen(vc->ch.lmt_copy_buf_handle) + 1);
    
    queue_initially_empty = LMT_SHM_Q_EMPTY(vc->ch.lmt_queue) && vc->ch.lmt_active_lmt == NULL;

    MPIU_CHKPMEM_MALLOC (e, MPID_nem_lmt_shm_wait_element_t *, sizeof (MPID_nem_lmt_shm_wait_element_t), mpi_errno, "lmt wait queue element");
    e->progress = lmt_shm_recv_progress;
    e->req = req;
    LMT_SHM_Q_ENQUEUE(&vc->ch.lmt_queue, e); /* MT: not thread safe */
    
    /* make progress on that vc */
    mpi_errno = lmt_shm_progress_vc(vc, &done);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* MT: not thread safe, another thread may have enqueued another
       lmt after we did, and added this vc to the progress list.  In
       that case we would be adding the vc twice. */
    if (!done && queue_initially_empty)
    {
        /* lmt send didn't finish, enqueue it to be completed later */
        lmt_shm_prog_element_t *pe;
            
        MPIU_CHKPMEM_MALLOC (pe, lmt_shm_prog_element_t *, sizeof (lmt_shm_prog_element_t), mpi_errno, "lmt progress queue element");
        pe->vc = vc;
        LMT_SHM_L_ADD(pe);
        MPID_nem_lmt_shm_pending = TRUE;
        MPIU_Assert(!vc->ch.lmt_enqueued);
        vc->ch.lmt_enqueued = TRUE;
    }    

    MPIU_Assert(LMT_SHM_Q_EMPTY(vc->ch.lmt_queue) || !LMT_SHM_L_EMPTY());
    
    MPIU_CHKPMEM_COMMIT();
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LMT_SHM_START_RECV);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_shm_start_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_lmt_shm_start_send(MPIDI_VC_t *vc, MPID_Request *req, MPID_IOV r_cookie)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_msg_sz_t data_sz;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPID_Datatype * dt_ptr;
    int done = FALSE;
    int queue_initially_empty;
    MPID_nem_lmt_shm_wait_element_t *e;
    MPIU_CHKPMEM_DECL(3);
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LMT_SHM_START_SEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LMT_SHM_START_SEND);

    if (vc->ch.lmt_copy_buf == NULL)
    {
        MPIU_CHKPMEM_MALLOC (vc->ch.lmt_copy_buf_handle, char *, r_cookie.MPID_IOV_LEN, mpi_errno, "copy buf handle");
        MPID_NEM_MEMCPY(vc->ch.lmt_copy_buf_handle, r_cookie.MPID_IOV_BUF, r_cookie.MPID_IOV_LEN);

        mpi_errno = MPID_nem_attach_shm_region(&vc->ch.lmt_copy_buf, vc->ch.lmt_copy_buf_handle);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    else if (strncmp(vc->ch.lmt_copy_buf_handle, r_cookie.MPID_IOV_BUF, r_cookie.MPID_IOV_LEN) < 0)
    {
        /* Each side allocated its own buffer, lexicographically lower valued buffer handle is deleted */

        mpi_errno = MPID_nem_delete_shm_region(vc->ch.lmt_copy_buf, vc->ch.lmt_copy_buf_handle);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        vc->ch.lmt_copy_buf = NULL;

        MPIU_CHKPMEM_MALLOC (vc->ch.lmt_copy_buf_handle, char *, r_cookie.MPID_IOV_LEN, mpi_errno, "copy buf handle");
        MPID_NEM_MEMCPY(vc->ch.lmt_copy_buf_handle, r_cookie.MPID_IOV_BUF, r_cookie.MPID_IOV_LEN);

        mpi_errno = MPID_nem_attach_shm_region(&vc->ch.lmt_copy_buf, vc->ch.lmt_copy_buf_handle);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        LMT_SHM_Q_ENQUEUE(&vc->ch.lmt_queue, vc->ch.lmt_active_lmt);
        vc->ch.lmt_active_lmt = NULL;
    }

    queue_initially_empty = LMT_SHM_Q_EMPTY(vc->ch.lmt_queue) && vc->ch.lmt_active_lmt == NULL;

    MPIU_CHKPMEM_MALLOC (e, MPID_nem_lmt_shm_wait_element_t *, sizeof (MPID_nem_lmt_shm_wait_element_t), mpi_errno, "lmt wait queue element");
    e->progress = lmt_shm_send_progress;
    e->req = req;
    LMT_SHM_Q_ENQUEUE(&vc->ch.lmt_queue, e); /* MT: not thread safe */

    /* make progress on that vc */
    mpi_errno = lmt_shm_progress_vc(vc, &done);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* MT: not thread safe, another thread may have enqueued another
       lmt after we did, and added this vc to the progress list.  In
       that case we would be adding the vc twice. */
    if (!done && queue_initially_empty)
    {
        /* lmt send didn't finish, enqueue it to be completed later */
        lmt_shm_prog_element_t *pe;
            
        MPIU_CHKPMEM_MALLOC (pe, lmt_shm_prog_element_t *, sizeof (lmt_shm_prog_element_t), mpi_errno, "lmt progress queue element");
        pe->vc = vc;
        LMT_SHM_L_ADD(pe);
        MPID_nem_lmt_shm_pending = TRUE;
        MPIU_Assert(!vc->ch.lmt_enqueued);
        vc->ch.lmt_enqueued = TRUE;
   }

    MPIU_Assert(LMT_SHM_Q_EMPTY(vc->ch.lmt_queue) || !LMT_SHM_L_EMPTY());
    

 fn_exit:
    MPIU_CHKPMEM_COMMIT();
 fn_return:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LMT_SHM_START_SEND);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_return;
}

#undef FUNCNAME
#define FUNCNAME get_next_req
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int get_next_req(MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    volatile MPID_nem_copy_buf_t * const copy_buf = vc->ch.lmt_copy_buf;
    int prev_owner_rank;
    MPID_Request *req;
    MPIDI_STATE_DECL(MPID_STATE_GET_NEXT_REQ);

    MPIDI_FUNC_ENTER(MPID_STATE_GET_NEXT_REQ);

    prev_owner_rank = MPID_NEM_CAS_INT(&copy_buf->owner_info.val.rank, NO_OWNER, MPIDI_Process.my_pg_rank);

    if (prev_owner_rank == MPIDI_Process.my_pg_rank)
    {
        /* last lmt is not complete (receiver still receiving */
        goto fn_exit;
    }

    if (prev_owner_rank == NO_OWNER)
    {
        /* successfully grabbed idle copy buf */
        copy_buf->flag[0].val = BUF_EMPTY;
        copy_buf->flag[1].val = BUF_EMPTY;

        LMT_SHM_Q_DEQUEUE(&vc->ch.lmt_queue, &vc->ch.lmt_active_lmt);
        copy_buf->owner_info.val.remote_req_id = vc->ch.lmt_active_lmt->req->ch.lmt_req_id;
    }
    else
    {
        /* copy buf is owned by the remote side */
        /* remote side chooses next transfer */
        int i = 0;
        int req_id;
        
        while (copy_buf->owner_info.val.remote_req_id == MPI_REQUEST_NULL)
        {
            if (i == NUM_BUSY_POLLS)
            {
                SCHED_YIELD();
                i = 0;
            }
            ++i;
        }    

        LMT_SHM_Q_SEARCH_REMOVE(&vc->ch.lmt_queue, copy_buf->owner_info.val.remote_req_id, &vc->ch.lmt_active_lmt);

        if (vc->ch.lmt_active_lmt == NULL)
            /* request not found  */
            goto fn_exit;
    }

    req = vc->ch.lmt_active_lmt->req;
    MPID_Segment_init(req->dev.user_buf, req->dev.user_count, req->dev.datatype, &req->dev.segment, 0);
    req->dev.segment_first = 0;
    vc->ch.lmt_buf_num = 0;

    MPIU_Assert((vc->ch.lmt_copy_buf->owner_info.val.rank == MPIDI_Process.my_pg_rank &&
                 vc->ch.lmt_copy_buf->owner_info.val.remote_req_id == vc->ch.lmt_active_lmt->req->ch.lmt_req_id) ||
                (vc->ch.lmt_copy_buf->owner_info.val.rank == vc->pg_rank &&
                 vc->ch.lmt_copy_buf->owner_info.val.remote_req_id == vc->ch.lmt_active_lmt->req->handle));
    
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_GET_NEXT_REQ);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME lmt_shm_send_progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int lmt_shm_send_progress(MPIDI_VC_t *vc, MPID_Request *req, int *done)
{
    int mpi_errno = MPI_SUCCESS;
    volatile MPID_nem_copy_buf_t * const copy_buf = vc->ch.lmt_copy_buf;
    MPIDI_msg_sz_t first;
    MPIDI_msg_sz_t last;
    int buf_num;
    MPIDI_msg_sz_t data_sz;
    MPID_nem_lmt_shm_wait_element_t *e;
    MPIDI_STATE_DECL(MPID_STATE_LMT_SHM_SEND_PROGRESS);

    MPIDI_FUNC_ENTER(MPID_STATE_LMT_SHM_SEND_PROGRESS);

    MPIU_Assert((vc->ch.lmt_copy_buf->owner_info.val.rank == MPIDI_Process.my_pg_rank &&
                 vc->ch.lmt_copy_buf->owner_info.val.remote_req_id == vc->ch.lmt_active_lmt->req->ch.lmt_req_id) ||
                (vc->ch.lmt_copy_buf->owner_info.val.rank == vc->pg_rank &&
                 vc->ch.lmt_copy_buf->owner_info.val.remote_req_id == vc->ch.lmt_active_lmt->req->handle));

    copy_buf->sender_present.val = TRUE;    

    MPIU_Assert(req == vc->ch.lmt_active_lmt->req);
/*     MPIU_Assert(MPIDI_Request_get_type(req) == MPIDI_REQUEST_TYPE_SEND); */
    
    data_sz = req->ch.lmt_data_sz;
    buf_num = vc->ch.lmt_buf_num;
    first = req->dev.segment_first;
    
    do
    {
        int i;
        /* If the buffer is full, wait.  If the receiver is actively
           working on this transfer, yield the processor and keep
           waiting, otherwise wait for a bounded amount of time. */
        i = 0;
        while (copy_buf->flag[buf_num].val == BUF_FULL)
        {
            if (i == NUM_BUSY_POLLS)
            {
                if (copy_buf->receiver_present.val)
                {
                    SCHED_YIELD();
                    i = 0;
                }
                else
                {
                    req->dev.segment_first = first;
                    vc->ch.lmt_buf_num = buf_num;
                    *done = FALSE;
                    goto fn_exit;
                }
            }
                
            ++i;
        }

        /* quit if receiver indicates that its done receiving */
        if (copy_buf->flag[buf_num].val == BUF_DONE)
            break;

        /* we have a free buffer, fill it */
        last = (data_sz - first <= MPID_NEM_COPY_BUF_LEN) ? data_sz : first + MPID_NEM_COPY_BUF_LEN;
	MPID_Segment_pack(&req->dev.segment, first, &last, (void *)copy_buf->buf[buf_num]); /* cast away volatile */
        MPID_NEM_WRITE_BARRIER();
        copy_buf->flag[buf_num].val = BUF_FULL;
        MPID_NEM_WRITE_BARRIER();

        first = last;
        buf_num = 1 - buf_num;
    }
    while (last < data_sz);

    *done = TRUE;
    MPIDI_CH3U_Request_complete(req);   
       
 fn_exit:
    copy_buf->sender_present.val = FALSE;
    MPIDI_FUNC_EXIT(MPID_STATE_LMT_SHM_SEND_PROGRESS);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME lmt_shm_recv_progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int lmt_shm_recv_progress(MPIDI_VC_t *vc, MPID_Request *req, int *done)
{
    int mpi_errno = MPI_SUCCESS;
    volatile MPID_nem_copy_buf_t * const copy_buf = vc->ch.lmt_copy_buf;
    MPIDI_msg_sz_t first;
    MPIDI_msg_sz_t last;
    int buf_num;
    MPIDI_msg_sz_t data_sz;
    int i;
    MPID_nem_lmt_shm_wait_element_t *e;
    MPIDI_STATE_DECL(MPID_STATE_LMT_SHM_RECV_PROGRESS);

    MPIDI_FUNC_ENTER(MPID_STATE_LMT_SHM_RECV_PROGRESS);

    MPIU_Assert((vc->ch.lmt_copy_buf->owner_info.val.rank == MPIDI_Process.my_pg_rank &&
                 vc->ch.lmt_copy_buf->owner_info.val.remote_req_id == vc->ch.lmt_active_lmt->req->ch.lmt_req_id) ||
                (vc->ch.lmt_copy_buf->owner_info.val.rank == vc->pg_rank &&
                 vc->ch.lmt_copy_buf->owner_info.val.remote_req_id == vc->ch.lmt_active_lmt->req->handle));

    copy_buf->receiver_present.val = TRUE;

    data_sz = req->ch.lmt_data_sz;
    buf_num = vc->ch.lmt_buf_num;
    first = req->dev.segment_first;

    do
    {
        /* If the buffer is full, wait.  If the sender is actively
           working on this transfer, yield the processor and keep
           waiting, otherwise wait for a bounded amount of time. */
        i = 0;
        while (copy_buf->flag[buf_num].val == BUF_EMPTY)
        {
            if (i == NUM_BUSY_POLLS)
            {
                if (copy_buf->sender_present.val)
                {
                    SCHED_YIELD();
                    i = 0;
                }
                else
                {
                    req->dev.segment_first = first;
                    vc->ch.lmt_buf_num = buf_num;
                    *done = FALSE;
                    goto fn_exit;
                }
            }
                
            ++i;
        }

        /* receiver should never find a buffer with the DONE flag */
        MPIU_Assert(copy_buf->flag[buf_num].val != BUF_DONE);

        last = (data_sz - first <= MPID_NEM_COPY_BUF_LEN) ? data_sz : first + MPID_NEM_COPY_BUF_LEN;
	MPID_Segment_unpack(&req->dev.segment, first, &last, (void *)copy_buf->buf[buf_num]); /* cast away volatile */
        MPID_NEM_READ_BARRIER();
        if (last < data_sz)
            copy_buf->flag[buf_num].val = BUF_EMPTY;
        else
            copy_buf->flag[buf_num].val = BUF_DONE;
        MPID_NEM_WRITE_BARRIER();

        first = last;
        buf_num = 1 - buf_num;
    }
    while (last < data_sz); 

    copy_buf->flag[0].val                  = BUF_EMPTY;
    copy_buf->flag[1].val                  = BUF_EMPTY;
    copy_buf->owner_info.val.remote_req_id = MPI_REQUEST_NULL;
    MPID_NEM_WRITE_BARRIER();
    copy_buf->owner_info.val.rank          = NO_OWNER;

    *done = TRUE;
    MPIDI_CH3U_Request_complete(req);
    
 fn_exit:
    copy_buf->receiver_present.val = FALSE;
    MPIDI_FUNC_EXIT(MPID_STATE_LMT_SHM_RECV_PROGRESS);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_shm_handle_cookie
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_lmt_shm_handle_cookie(MPIDI_VC_t *vc, MPID_Request *req, MPID_IOV cookie)
{
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_shm_done_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_lmt_shm_done_send(MPIDI_VC_t *vc, MPID_Request *req)
{
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_shm_done_recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_lmt_shm_done_recv(MPIDI_VC_t *vc, MPID_Request *req)
{
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_shm_progress_vc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int lmt_shm_progress_vc(MPIDI_VC_t *vc, int *done)
{
    int mpi_errno = MPI_SUCCESS;
    int done_req = FALSE;
    MPID_nem_lmt_shm_wait_element_t *we;
    MPIDI_STATE_DECL(MPID_STATE_LMT_SHM_PROGRESS_VC);

    MPIDI_FUNC_ENTER(MPID_STATE_LMT_SHM_PROGRESS_VC);

    *done = FALSE;

    if (vc->ch.lmt_active_lmt == NULL)
    {
        mpi_errno = get_next_req(vc);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        
        if (vc->ch.lmt_active_lmt == NULL)
        {
            /* couldn't find an appropriate request, try again later */
            goto fn_exit;
        }
    }
        
    we = vc->ch.lmt_active_lmt;
    mpi_errno = we->progress(vc, we->req, &done_req);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    if (done_req)
    {
        MPIU_Free(vc->ch.lmt_active_lmt);
        vc->ch.lmt_active_lmt = NULL;
        
        if (LMT_SHM_Q_EMPTY(vc->ch.lmt_queue))
            *done = TRUE;    
    }
    
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_LMT_SHM_PROGRESS_VC);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_shm_progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_lmt_shm_progress()
{
    int mpi_errno = MPI_SUCCESS;
    lmt_shm_prog_element_t *pe;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LMT_SHM_PROGRESS);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LMT_SHM_PROGRESS);

    pe = LMT_SHM_L_HEAD();

    while (pe)
    {
        int done = FALSE;

        mpi_errno = lmt_shm_progress_vc(pe->vc, &done);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        
        if (done)
        {
            lmt_shm_prog_element_t *f;
            MPIU_Assert(LMT_SHM_Q_EMPTY(pe->vc->ch.lmt_queue));
            MPIU_Assert(pe->vc->ch.lmt_active_lmt == NULL);
            MPIU_Assert(pe->vc->ch.lmt_enqueued);
            pe->vc->ch.lmt_enqueued = FALSE;

            f = pe;
            pe = pe->next;
            LMT_SHM_L_REMOVE(f);
            MPIU_Free(f);
        }
        else
            pe = pe->next;
    }

    if (LMT_SHM_L_EMPTY())
        MPID_nem_lmt_shm_pending = FALSE;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LMT_SHM_PROGRESS);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_allocate_shm_region
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPID_nem_allocate_shm_region(volatile MPID_nem_copy_buf_t **buf_p, char *handle[])
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ALLOCATE_SHM_REGION);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ALLOCATE_SHM_REGION);

    if (*buf_p)
    {
        /* we're already attached */
        goto fn_exit;
    }

    mpi_errno = MPID_nem_allocate_shared_memory((char **)buf_p, sizeof (MPID_nem_copy_buf_t), handle);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ALLOCATE_SHM_REGION);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_attach_shm_region
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPID_nem_attach_shm_region(volatile MPID_nem_copy_buf_t **buf_p, const char handle[])
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ATTACH_SHM_REGION);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ATTACH_SHM_REGION);
    
    if(*buf_p)
    {
        /* we're already attached */
        goto fn_exit;
    }

    mpi_errno = MPID_nem_attach_shared_memory((char **)buf_p, sizeof (MPID_nem_copy_buf_t), handle);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    mpi_errno = MPID_nem_remove_shared_memory(handle);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ATTACH_SHM_REGION);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_detach_shm_region
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPID_nem_detach_shm_region(volatile MPID_nem_copy_buf_t *buf, char handle[])
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    struct shmid_ds ds;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_DETACH_SHM_REGION);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_DETACH_SHM_REGION);

    /* for now never detach */
    goto fn_exit;

    MPIU_Free(handle);

    mpi_errno = MPID_nem_detach_shared_memory ((char *)buf, sizeof (MPID_nem_copy_buf_t));
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_DETACH_SHM_REGION);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_delete_shm_region
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPID_nem_delete_shm_region(volatile MPID_nem_copy_buf_t *buf, char handle[])
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    struct shmid_ds ds;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_DELETE_SHM_REGION);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_DELETE_SHM_REGION);

    mpi_errno = MPID_nem_remove_shared_memory(handle);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    MPIU_Free(handle);

    mpi_errno = MPID_nem_detach_shared_memory ((char *)buf, sizeof (MPID_nem_copy_buf_t));
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_DELETE_SHM_REGION);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
