/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "ch3i_progress.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress(int is_blocking)
{
    int mpi_errno = MPI_SUCCESS;
    int rc;
    int register count, bShmProgressMade;
    unsigned completions = MPIDI_CH3I_progress_completions;
    MPIDI_CH3I_Shmem_queue_info info;
    int num_bytes;
    MPIDI_VC *vc_ptr;
    static int msg_queue_count = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS);
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_YIELD);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS);

#ifdef MPICH_DBG_OUTPUT
    if (is_blocking)
    {
	MPIDI_DBG_PRINTF((50, FCNAME, "entering, blocking=%s", is_blocking ? "true" : "false"));
    }
#endif
    do
    {
	/* make progress on the shared memory queues */

	bShmProgressMade = FALSE;
	if (MPIDI_CH3I_Process.shm_reading_list)
	{
	    rc = MPIDI_CH3I_SHM_read_progress(MPIDI_CH3I_Process.shm_reading_list, 0, &vc_ptr, &num_bytes);
	    if (rc == MPI_SUCCESS)
	    {
		MPIDI_DBG_PRINTF((50, FCNAME, "MPIDI_CH3I_SHM_read_progress reported %d bytes read", num_bytes));
		mpi_errno = handle_shm_read(vc_ptr, num_bytes);
		if (mpi_errno != MPI_SUCCESS)
		{
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**progress", 0);
		    goto fn_exit;
		}
		bShmProgressMade = TRUE;
	    }
	    else
	    {
		if (rc != SHM_WAIT_TIMEOUT)
		{
		    /*MPIDI_err_printf("MPIDI_CH3_Progress", "MPIDI_CH3I_SHM_read_progress returned error %d\n", rc);*/
		    mpi_errno = MPIR_Err_create_code(rc, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**progress", 0);
		    goto fn_exit;
		}
	    }
	}

	if (MPIDI_CH3I_Process.shm_writing_list)
	{
	    vc_ptr = MPIDI_CH3I_Process.shm_writing_list;
	    while (vc_ptr)
	    {
		if (vc_ptr->ch.send_active != NULL)
		{
		    mpi_errno = MPIDI_CH3I_SHM_write_progress(vc_ptr);
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**progress", 0);
			goto fn_exit;
		    }
		    /*bShmProgressMade = TRUE;*/
		}
		vc_ptr = vc_ptr->ch.shm_next_writer;
	    }
	}

	if (!bShmProgressMade)
	{
	    MPIDU_Yield(); /* always yield for now */
	}

	if ((msg_queue_count++ % MPIDI_CH3I_MSGQ_ITERATIONS) == 0)
	{
	    /* check for new shmem queue connection requests */
	    rc = MPIDI_CH3I_BootstrapQ_recv_msg(MPIDI_CH3I_Process.pg->bootstrapQ, &info, sizeof(info), &num_bytes, FALSE);
	    if (rc != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(rc, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**boot_recv", 0);
		goto fn_exit;
	    }
#ifdef MPICH_DBG_OUTPUT
	    /*assert(num_bytes == 0 || num_bytes == sizeof(info));*/
	    if (num_bytes != 0 && num_bytes != sizeof(info))
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**bootqmsg", "**bootqmsg %d", num_bytes);
		goto fn_exit;
	    }
#endif
	    if (num_bytes)
	    {
		vc_ptr = &MPIDI_CH3I_Process.pg->vc_table[info.pg_rank];
		rc = MPIDI_CH3I_SHM_Attach_to_mem(&info.info, &vc_ptr->ch.shm_read_queue_info);
		if (rc != MPI_SUCCESS)
		{
		    mpi_errno = MPIR_Err_create_code(rc, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**attach_to_mem", "**attach_to_mem %d", vc_ptr->ch.shm_read_queue_info.error);
		    goto fn_exit;
		}
		MPIU_DBG_PRINTF(("attached to queue from process %d\n", info.pg_rank));
		/*vc_ptr->ch.state = MPIDI_CH3I_VC_STATE_CONNECTED;*/ /* we are read connected but not write connected */
		vc_ptr->ch.read_shmq = vc_ptr->ch.shm_read_queue_info.addr;/*info.info.addr;*/
		MPIU_DBG_PRINTF(("read_shmq = %p\n", vc_ptr->ch.read_shmq));
		vc_ptr->ch.shm_reading_pkt = TRUE;
		/* add this VC to the global list to be shm_waited on */
		vc_ptr->ch.shm_next_reader = MPIDI_CH3I_Process.shm_reading_list;
		MPIDI_CH3I_Process.shm_reading_list = vc_ptr;
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
    mpi_errno = MPIDI_CH3_Progress_test();
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS_POKE);
    return mpi_errno;
}

#if !defined(MPIDI_CH3_Progress_start)
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Progress_start
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3_Progress_start()
{
    /* MT - This function is empty for the single-threaded implementation */
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS_START);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS_START);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS_START);
}
#endif

#if !defined(MPIDI_CH3_Progress_end)
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Progress_end
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3_Progress_end()
{
    /* MT: This function is empty for the single-threaded implementation */
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS_END);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS_END);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS_END);
}
#endif

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress_init()
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_INIT);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS_INIT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress_finalize()
{
    /* MT: in a multi-threaded environment, finalize() should signal any thread(s) blocking on sock_wait() and wait for those
       threads to complete before destroying the progress engine data structures.  We may need to add a function to the sock
       interface for this. */

    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_FINALIZE);
    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));

    MPIR_Nest_incr();
    {
	NMPI_Barrier(MPI_COMM_WORLD);
    }
    MPIR_Nest_decr();

    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS_FINALIZE);
    return mpi_errno;
}
