/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

volatile unsigned int MPIDI_CH3I_progress_completions = 0;

/*
 * MPIDI_CH3I_Request_adjust_iov()
 *
 * Adjust the iovec in the request by the supplied number of bytes.  If the iovec has been consumed, return true; otherwise return
 * false.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Request_adjust_iov
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Request_adjust_iov(MPID_Request * req, MPIDI_msg_sz_t nb)
{
    int offset = req->ch.iov_offset;
    const int count = req->dev.iov_count;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_REQUEST_ADJUST_IOV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_REQUEST_ADJUST_IOV);
    
    while (offset < count)
    {
	if (req->dev.iov[offset].MPID_IOV_LEN <= (unsigned int)nb)
	{
	    nb -= req->dev.iov[offset].MPID_IOV_LEN;
	    offset++;
	}
	else
	{
	    req->dev.iov[offset].MPID_IOV_BUF = (char *) req->dev.iov[offset].MPID_IOV_BUF + nb;
	    req->dev.iov[offset].MPID_IOV_LEN -= nb;
	    req->ch.iov_offset = offset;
	    MPIDI_DBG_PRINTF((60, FCNAME, "adjust_iov returning FALSE"));
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_REQUEST_ADJUST_IOV);
	    return FALSE;
	}
    }
    
    req->ch.iov_offset = offset;

    MPIDI_DBG_PRINTF((60, FCNAME, "adjust_iov returning TRUE"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_REQUEST_ADJUST_IOV);
    return TRUE;
}

/*
#define post_pkt_recv(vc) \
{ \
    MPIDI_STATE_DECL(MPID_STATE_POST_PKT_RECV); \
    MPIDI_FUNC_ENTER(MPID_STATE_POST_PKT_RECV); \
    vc->ch.req->dev.iov[0].MPID_IOV_BUF = (void *)&vc->ch.req->ch.pkt; \
    vc->ch.req->dev.iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t); \
    vc->ch.req->dev.iov_count = 1; \
    vc->ch.req->ch.iov_offset = 0; \
    vc->ch.req->dev.ca = MPIDI_CH3I_CA_HANDLE_PKT; \
    vc->ch.recv_active = vc->ch.req; \
    MPIDI_CH3I_SHM_post_read(vc, &vc->ch.req->ch.pkt, sizeof(MPIDI_CH3_Pkt_t), NULL); \
    MPIDI_FUNC_EXIT(MPID_STATE_POST_PKT_RECV); \
}
*/
/* Because packets are interpreted in-place in the shm queue, posting a pkt read only requires setting a flag */
#define post_pkt_recv(vc) vc->ch.shm_reading_pkt = TRUE

#undef FUNCNAME
#define FUNCNAME handle_read
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int handle_read(MPIDI_VC *vc, int nb)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request * req;
    int complete;
    MPIDI_STATE_DECL(MPID_STATE_HANDLE_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_HANDLE_READ);
    
    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));

    req = vc->ch.recv_active;
    if (req == NULL)
    {
	MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
	MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_READ);
	return MPI_SUCCESS;
    }

    if (nb > 0)
    {
	if (MPIDI_CH3I_Request_adjust_iov(req, nb))
	{
	    /* Read operation complete */
	    mpi_errno = MPIDI_CH3U_Handle_recv_req(vc, req, &complete);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	    }
	    if (complete)
	    {
		post_pkt_recv(vc);
	    }
	    else
	    {
		vc->ch.recv_active = req;
		mpi_errno = MPIDI_CH3I_SHM_post_readv(vc, req->dev.iov, req->dev.iov_count, NULL);
	    }
	}
    }
    else
    {
	MPIDI_DBG_PRINTF((65, FCNAME, "Read args were iov=%x, count=%d",
	    req->dev.iov, req->dev.iov_count));
    }
    
    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_READ);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME handle_written
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int handle_written(MPIDI_VC * vc)
{
    int mpi_errno = MPI_SUCCESS;
    int nb;
    int complete;
    MPIDI_STATE_DECL(MPID_STATE_HANDLE_WRITTEN);

    MPIDI_FUNC_ENTER(MPID_STATE_HANDLE_WRITTEN);
    
    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));
    while (vc->ch.send_active != NULL)
    {
	MPID_Request * req = vc->ch.send_active;

	/*MPIDI_DBG_PRINTF((60, FCNAME, "calling shm_writev"));*/
	mpi_errno = MPIDI_CH3I_SHM_writev(vc, req->dev.iov + req->ch.iov_offset, req->dev.iov_count - req->ch.iov_offset, &nb);
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**handle_written", 0);
	    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_WRITTEN);
	    return mpi_errno;
	}
	MPIDI_DBG_PRINTF((60, FCNAME, "shm_writev returned %d", nb));

	if (nb > 0)
	{
	    if (MPIDI_CH3I_Request_adjust_iov(req, nb))
	    {
		/* Write operation complete */
		mpi_errno = MPIDI_CH3U_Handle_send_req(vc, req, &complete);
		if (mpi_errno != MPI_SUCCESS)
		{
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
		    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_WRITTEN);
		    return mpi_errno;
		}
		if (complete)
		{
		    MPIDI_CH3I_SendQ_dequeue(vc);
		}
		vc->ch.send_active = MPIDI_CH3I_SendQ_head(vc);
	    }
	}
	else
	{
	    MPIDI_DBG_PRINTF((65, FCNAME, "shm_post_writev returned %d bytes", nb));
	    break;
	}
    }

    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));

    MPIDI_FUNC_EXIT(MPID_STATE_HANDLE_WRITTEN);
    return mpi_errno;
}

void MPIDI_CH3_Progress_start()
{
    /* MT - This function is empty for the single-threaded implementation */
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress(int is_blocking)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIDI_VC *vc_ptr;
    int num_bytes;
    shm_wait_t wait_result;
#ifdef MPICH_DBG_OUTPUT
    unsigned register count;
#endif
    int spin_count = 1;
    unsigned completions = MPIDI_CH3I_progress_completions;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS);
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_YIELD);
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SLEEP_YIELD);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS);

    MPIDI_DBG_PRINTF((50, FCNAME, "entering, blocking=%s", is_blocking ? "true" : "false"));
    do
    {
	mpi_errno = MPIDI_CH3I_SHM_wait(MPIDI_CH3I_Process.vc, 0, &vc_ptr, &num_bytes, &wait_result);
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shm_wait", 0);
	    goto fn_exit;
	}
	switch (wait_result)
	{
	case SHM_WAIT_TIMEOUT:
	    if (spin_count >= MPIDI_CH3I_Process.pg->nShmWaitSpinCount)
	    {
#ifdef USE_SLEEP_YIELD
		if (spin_count >= MPIDI_CH3I_Process.pg->nShmWaitYieldCount)
		{
		    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SLEEP_YIELD);
		    MPIDU_Sleep_yield();
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SLEEP_YIELD);
		}
		else
		{
		    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_YIELD);
		    MPIDU_Yield();
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_YIELD);
		}
#else
		MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_YIELD);
		MPIDU_Yield();
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_YIELD);
#endif
	    }
	    spin_count++;
	    break;
	case SHM_WAIT_READ:
	    MPIDI_DBG_PRINTF((50, FCNAME, "MPIDI_CH3I_SHM_wait reported %d bytes read", num_bytes));
	    spin_count = 1;
#ifdef USE_SLEEP_YIELD
	    MPIDI_Sleep_yield_count = 0;
#endif
	    mpi_errno = handle_read(vc_ptr, num_bytes);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**progress", 0);
		goto fn_exit;
		/*
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS);
		return mpi_errno;
		*/
	    }
	    break;
	case SHM_WAIT_WRITE:
	    MPIDI_DBG_PRINTF((50, FCNAME, "MPIDI_CH3I_SHM_wait reported %d bytes written", num_bytes));
	    spin_count = 1;
#ifdef USE_SLEEP_YIELD
	    MPIDI_Sleep_yield_count = 0;
#endif
	    mpi_errno = handle_written(vc_ptr);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**progress", 0);
		goto fn_exit;
		/*
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS);
		return mpi_errno;
		*/
	    }
	    break;
	default:
	    /*MPIDI_err_printf(FCNAME, "MPIDI_CH3I_SHM_wait returned an unknown operation code\n");*/
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shm_op", "**shm_op %d", wait_result);
	    goto fn_exit;
	    /*
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS);
	    return mpi_errno;
	    */
	    /*
	    MPID_Abort(MPIR_Process.comm_world, MPI_SUCCESS, 13);
	    break;
	    */
	}

	/* pound on the write queues since shm_wait currently does not return SHM_WAIT_WRITE */
	for (i=0; i<MPIDI_CH3I_Process.vc->ch.pg->size; i++)
	{
	    if (MPIDI_CH3I_Process.pg->vc_table[i].ch.send_active != NULL)
	    {
		mpi_errno = handle_written(&MPIDI_CH3I_Process.pg->vc_table[i]);
		if (mpi_errno != MPI_SUCCESS)
		{
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**progress", 0);
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS);
		    return mpi_errno;
		}
	    }
	}
    }
    while (completions == MPIDI_CH3I_progress_completions && is_blocking);

fn_exit:
#ifdef MPICH_DBG_OUTPUT
    count = MPIDI_CH3I_progress_completions - completions;
    if (is_blocking)
    {
	MPIDI_DBG_PRINTF((50, FCNAME, "exiting, count=%d", count));
    }
    else
    {
	if (count > 0)
	{
	    MPIDI_DBG_PRINTF((50, FCNAME, "exiting (non-blocking), count=%d", count));
	}
    }
#endif
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Progress_poke
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Progress_poke()
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS_POKE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS_POKE);
    mpi_errno = MPIDI_CH3I_Progress(0);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS_POKE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Progress_end
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3_Progress_end()
{
    /* MT - This function is empty for the single-threaded implementation */
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress_init()
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS_INIT);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS_INIT);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress_finalize()
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS_FINALIZE);

    /*
     * Wait for any pending communication to complete.  This prevents another process from hanging if it performs a send and then
     * attempts to cancel it.
     */
    MPIR_Nest_incr();
    {
	mpi_errno = NMPI_Barrier(MPI_COMM_WORLD);
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**progress_finalize", 0);
	}
    }
    MPIR_Nest_decr();
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS_FINALIZE);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS_FINALIZE);
    return mpi_errno;
}

