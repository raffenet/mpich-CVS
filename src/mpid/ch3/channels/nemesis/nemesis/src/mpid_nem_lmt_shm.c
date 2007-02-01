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

typedef struct lmt_shm_queue
{
    int (* progress)(MPIDI_VC_t *vc, MPID_Request *req, int *done);
    MPIDI_VC_t *vc;
    MPID_Request *req;
    struct lmt_shm_queue *next;
    struct lmt_shm_queue *prev;
} lmt_shm_queue_t;

static struct {lmt_shm_queue_t *head;} lmt_shm_progress_q = {NULL};

#define LMT_SHM_L_EMPTY() GENERIC_L_EMPTY(lmt_shm_progress_q)
#define LMT_SHM_L_HEAD() GENERIC_L_HEAD(lmt_shm_progress_q)
#define LMT_SHM_L_REMOVE(ep) GENERIC_L_REMOVE(&lmt_shm_progress_q, ep, next, prev)
#define LMT_SHM_L_ADD(ep) GENERIC_L_ADD(&lmt_shm_progress_q, ep, next, prev)

/* copy buffer in shared memory */
#define MPID_NEM_COPY_BUF_LEN (64 * 1024)

typedef union
{
    int val;
    char padding[MPID_NEM_CACHE_LINE_LEN];
} MPID_nem_cacheline_int_t;

typedef struct MPID_nem_copy_buf
{
    MPID_nem_cacheline_int_t sender_present; /* is the sender currently in the lmt progress function for this buffer */
    MPID_nem_cacheline_int_t receiver_present; /* is the receiver currently in the lmt progress function for this buffer */
    MPID_nem_cacheline_int_t flag[2];
    char buf[2][MPID_NEM_COPY_BUF_LEN];
} MPID_nem_copy_buf_t;
/* copy buffer flag values */
#define BUF_EMPTY 0
#define BUF_FULL  1
#define BUF_DONE  2


static int lmt_shm_send_progress(MPIDI_VC_t *vc, MPID_Request *req, int *done);
static int lmt_shm_recv_progress(MPIDI_VC_t *vc, MPID_Request *req, int *done);
static int MPID_nem_allocate_shm_region(volatile MPID_nem_copy_buf_t **buf_p, char *handle[]);
static int MPID_nem_attach_shm_region(volatile MPID_nem_copy_buf_t **buf_p, const char handle[]);
static int MPID_nem_detach_shm_region(volatile MPID_nem_copy_buf_t *buf, char handle[]);
static int MPID_nem_lmt_shm_start_recv(MPIDI_VC_t *vc, MPID_Request *req);


/* number of iterations to wait for the other side to process a buffer */
#define NUM_BUSY_POLLS 1000

#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_shm_pre_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_lmt_shm_pre_send(MPIDI_VC_t *vc, MPID_Request *req, MPID_IOV *cookie)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LMT_SHM_PRE_SEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LMT_SHM_PRE_SEND);

    cookie->MPID_IOV_BUF = NULL;
    cookie->MPID_IOV_LEN = 0;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LMT_SHM_PRE_SEND);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_shm_pre_recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_lmt_shm_pre_recv(MPIDI_VC_t *vc, MPID_Request *req, MPID_IOV s_cookie, MPID_IOV *r_cookie, int *send_cts)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LMT_SHM_PRE_RECV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LMT_SHM_PRE_RECV);

    *send_cts = 1;

    mpi_errno = MPID_nem_allocate_shm_region(&vc->ch.lmt_copy_buf, &vc->ch.lmt_copy_buf_handle);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    
    vc->ch.lmt_copy_buf->sender_present.val = 0;
    vc->ch.lmt_copy_buf->receiver_present.val = 0;
    
    vc->ch.lmt_copy_buf->flag[0].val = BUF_EMPTY;
    vc->ch.lmt_copy_buf->flag[1].val = BUF_EMPTY;

    r_cookie->MPID_IOV_BUF = vc->ch.lmt_copy_buf_handle;
    r_cookie->MPID_IOV_LEN = (int)strlen (vc->ch.lmt_copy_buf_handle) + 1;
        
    mpi_errno = MPID_nem_lmt_shm_start_recv(vc, req);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LMT_SHM_PRE_RECV);
    return mpi_errno;
 fn_fail:
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
    MPIU_CHKPMEM_DECL(2);
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LMT_SHM_START_SEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LMT_SHM_START_SEND);

    
    MPIU_CHKPMEM_MALLOC (vc->ch.lmt_copy_buf_handle, char *, r_cookie.MPID_IOV_LEN, mpi_errno, "copy buf handle");
    MPID_NEM_MEMCPY(vc->ch.lmt_copy_buf_handle, r_cookie.MPID_IOV_BUF, r_cookie.MPID_IOV_LEN);

    mpi_errno = MPID_nem_attach_shm_region(&vc->ch.lmt_copy_buf, vc->ch.lmt_copy_buf_handle);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    
    MPIDI_Datatype_get_info(req->dev.user_count, req->dev.datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    MPID_Segment_init(req->dev.user_buf, req->dev.user_count, req->dev.datatype, &req->dev.segment, 0);
    req->dev.segment_first = 0;
    req->ch.lmt_buf_num = 0;
    req->ch.lmt_data_sz = data_sz;

    mpi_errno = lmt_shm_send_progress(vc, req, &done);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    if (!done)
    {
        /* lmt send didn't finish, enqueue it to be completed later */
        lmt_shm_queue_t *e;

        MPIU_CHKPMEM_MALLOC (e, lmt_shm_queue_t *, sizeof (lmt_shm_queue_t), mpi_errno, "lmt progress queue element");
        e->progress = lmt_shm_send_progress;
        e->vc = vc;
        e->req = req;
        LMT_SHM_L_ADD(e);
        MPID_nem_lmt_shm_pending = TRUE;
    }

    MPIU_CHKPMEM_COMMIT();
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LMT_SHM_START_SEND);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
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
    const MPIDI_msg_sz_t data_sz = req->ch.lmt_data_sz;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_LMT_SHM_SEND_PROGRESS);

    MPIDI_FUNC_ENTER(MPID_STATE_LMT_SHM_SEND_PROGRESS);

    copy_buf->sender_present.val = TRUE;
    
    buf_num = req->ch.lmt_buf_num;
    first = req->dev.segment_first;
    
    do
    {
        /* If the buffer is full, wait.  If the receiver is actively
           working on this transfer, yield the processor and keep
           waiting, otherwise wait for a bounded amount of time. */
        i = 0;
        while (copy_buf->flag[buf_num].val == BUF_FULL)
        {
            if (i == NUM_BUSY_POLLS)
            {
                if (copy_buf->receiver_present.val)
                    SCHED_YIELD();
                else
                {
                    req->dev.segment_first = first;
                    req->ch.lmt_buf_num = buf_num;
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
    mpi_errno = MPID_nem_detach_shm_region(copy_buf, vc->ch.lmt_copy_buf_handle);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    
 fn_exit:
    copy_buf->sender_present.val = FALSE;
    MPIDI_FUNC_EXIT(MPID_STATE_LMT_SHM_SEND_PROGRESS);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_shm_start_recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPID_nem_lmt_shm_start_recv(MPIDI_VC_t *vc, MPID_Request *req)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_msg_sz_t data_sz = req->ch.lmt_data_sz;
    volatile MPID_nem_copy_buf_t * const copy_buf = vc->ch.lmt_copy_buf;
    int first;
    int last;
    int buf_num;
    int done = FALSE;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LMT_SHM_START_RECV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LMT_SHM_START_RECV);

    
    MPID_Segment_init(req->dev.user_buf, req->dev.user_count, req->dev.datatype, &req->dev.segment, 0);

    req->dev.segment_first = 0;    
    req->ch.lmt_buf_num = 0;

    mpi_errno = lmt_shm_recv_progress(vc, req, &done);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    if (!done)
    {
        /* lmt send didn't finish, enqueue it to be completed later */
        lmt_shm_queue_t *e;

        MPIU_CHKPMEM_MALLOC (e, lmt_shm_queue_t *, sizeof (lmt_shm_queue_t), mpi_errno, "lmt progress queue element");
        e->progress = lmt_shm_recv_progress;
        e->vc = vc;
        e->req = req;
        LMT_SHM_L_ADD(e);
        MPID_nem_lmt_shm_pending = TRUE;
    }

    MPIU_CHKPMEM_COMMIT();
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LMT_SHM_START_RECV);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
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
    const MPIDI_msg_sz_t data_sz = req->ch.lmt_data_sz;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_LMT_SHM_RECV_PROGRESS);

    MPIDI_FUNC_ENTER(MPID_STATE_LMT_SHM_RECV_PROGRESS);

    copy_buf->receiver_present.val = TRUE;
    
    buf_num = req->ch.lmt_buf_num;
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
                    SCHED_YIELD();
                else
                {
                    req->dev.segment_first = first;
                    req->ch.lmt_buf_num = buf_num;
                    *done = FALSE;
                    goto fn_exit;
                }
            }
                
            ++i;
        }

        /* receiver should never find a buffer with the DONE flag */
        MPIU_Assert(copy_buf->flag[buf_num].val != BUF_DONE);

        last = (data_sz - first <= MPID_NEM_COPY_BUF_LEN) ? data_sz : first + MPID_NEM_COPY_BUF_LEN;
	MPID_Segment_unpack (&req->dev.segment, first, &last, (void *)copy_buf->buf[buf_num]); /* cast away volatile */
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

    
    *done = TRUE;
    MPIDI_CH3U_Request_complete(req);
    mpi_errno = MPID_nem_detach_shm_region(vc->ch.lmt_copy_buf, vc->ch.lmt_copy_buf_handle);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    
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
#define FUNCNAME MPID_nem_lmt_shm_post_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_lmt_shm_post_send(MPIDI_VC_t *vc, MPID_Request *req)
{
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_shm_post_recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_lmt_shm_post_recv(MPIDI_VC_t *vc, MPID_Request *req)
{
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_shm_progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_lmt_shm_progress()
{
    int mpi_errno = MPI_SUCCESS;
    lmt_shm_queue_t *e;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LMT_SHM_PROGRESS);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LMT_SHM_PROGRESS);

    e = LMT_SHM_L_HEAD();

    while (e)
    {
        int done = FALSE;
        lmt_shm_queue_t *f;

        mpi_errno = e->progress(e->vc, e->req, &done);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        if (done)
        {
            f = e;
            e = e->next;
            LMT_SHM_L_REMOVE(f);
            MPIU_Free(f);
        }
        else
            e = e->next;
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
