/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "ch3i_progress.h"

int MPIDI_CH3I_shm_read_active = 0;
int MPIDI_CH3I_shm_write_active = 0;
int MPIDI_CH3I_sock_read_active = 0;
int MPIDI_CH3I_sock_write_active = 0;
int MPIDI_CH3I_active_flag = 0;

#ifdef USE_FIXED_SPIN_WAITS

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Progress(int is_blocking)
{
    int mpi_errno = MPI_SUCCESS;
    int rc;
    int register count, bShmProgressMade;
    sock_event_t event;
    unsigned completions = MPIDI_CH3I_progress_completions;
    MPIDI_CH3I_Shmem_queue_info info;
    int num_bytes;
    MPIDI_VC *vc_ptr;
    static int spin_count = 1;
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
		handle_shm_read(vc_ptr, num_bytes);
		bShmProgressMade = TRUE;
	    }
	    else
	    {
		if (rc != SHM_WAIT_TIMEOUT)
		{
		    /*MPIDI_err_printf("MPIDI_CH3_Progress", "MPIDI_CH3I_SHM_read_progress returned error %d\n", rc);*/
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shm_read_progress", 0);
		    goto fn_exit;
		}
	    }
	}

	if (MPIDI_CH3I_Process.shm_writing_list)
	{
	    vc_ptr = MPIDI_CH3I_Process.shm_writing_list;
	    while (vc_ptr)
	    {
		if (vc_ptr->ssm.send_active != NULL)
		{
		    if (MPIDI_CH3I_SHM_write_progress(vc_ptr) != 0)
		    {
			bShmProgressMade = TRUE;
		    }
		}
		vc_ptr = vc_ptr->ssm.shm_next_writer;
	    }
	}

	if (bShmProgressMade)
	{
	    spin_count = 1;
#ifdef USE_SLEEP_YIELD
	    MPIDI_Sleep_yield_count = 0;
#endif
	    continue;
	}

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
	    spin_count = 1;
#endif
	}
	spin_count++;

	if (spin_count > (MPIDI_CH3I_Process.pg->nShmWaitSpinCount >> 1) )
	{
	    /* make progress on the sockets */

	    rc = sock_wait(sock_set, 0, &event);
	    if (rc == SOCK_SUCCESS)
	    {
		rc = handle_sock_op(&event);
		if (rc != MPI_SUCCESS)
		{
		    mpi_errno = MPIR_Err_create_code(rc, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**progress_handle_sock_op", 0);
		    goto fn_exit;
		}
	    }
	    else
	    {
		if (rc != SOCK_ERR_TIMEOUT)
		{
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**progress_sock_wait", 0);
		    goto fn_exit;
		}
		MPIDU_Yield();
	    }
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
	    assert(num_bytes == 0 || num_bytes == sizeof(info));
	    if (num_bytes)
	    {
		vc_ptr = &MPIDI_CH3I_Process.pg->vc_table[info.pg_rank];
		rc = MPIDI_CH3I_SHM_Attach_to_mem(&info.info, &vc_ptr->ssm.shm_read_queue_info);
		if (rc != MPI_SUCCESS)
		{
		    mpi_errno = MPIR_Err_create_code(rc, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**attach_to_mem", "**attach_to_mem %d", vc_ptr->ssm.shm_read_queue_info.error);
		    goto fn_exit;
		}
		MPIU_DBG_PRINTF(("attached to queue from process %d\n", info.pg_rank));
		/*vc_ptr->ssm.state = MPIDI_CH3I_VC_STATE_CONNECTED;*/ /* we are read connected but not write connected */
		vc_ptr->ssm.bShm = TRUE;
		vc_ptr->ssm.read_shmq = vc_ptr->ssm.shm_read_queue_info.addr;/*info.info.addr;*/
		MPIU_DBG_PRINTF(("read_shmq = %p\n", vc_ptr->ssm.read_shmq));
		vc_ptr->ssm.shm_reading_pkt = TRUE;
		/* add this VC to the global list to be shm_waited on */
		vc_ptr->ssm.shm_next_reader = MPIDI_CH3I_Process.shm_reading_list;
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

#endif

#ifdef USE_ADAPTIVE_PROGRESS

#define MPIDI_CH3I_UPDATE_ITERATIONS    10
#define MPID_SINGLE_ACTIVE_FACTOR      100

#undef FUNCNAME
#define FUNCNAME MPID_CH3I_Message_queue_progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_CH3I_Message_queue_progress()
{
    MPIDI_CH3I_Shmem_queue_info info;
    int num_bytes;
    MPIDI_VC *vc_ptr;
    int rc, mpi_errno;

    /* check for new shmem queue connection requests */
    /*printf("<%dR>", MPIR_Process.comm_world->rank);fflush(stdout);*/
    rc = MPIDI_CH3I_BootstrapQ_recv_msg(
	MPIDI_CH3I_Process.pg->bootstrapQ, &info, sizeof(info), 
	&num_bytes, FALSE);
    if (rc != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(rc, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**boot_recv", 0);
	return mpi_errno;
    }
    assert(num_bytes == 0 || num_bytes == sizeof(info));
    if (num_bytes)
    {
	vc_ptr = &MPIDI_CH3I_Process.pg->vc_table[info.pg_rank];
	rc = MPIDI_CH3I_SHM_Attach_to_mem(
	    &info.info, &vc_ptr->ssm.shm_read_queue_info);
	if (rc != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**MPIDI_CH3I_SHM_Attach_to_mem", "**MPIDI_CH3I_SHM_Attach_to_mem %d", vc_ptr->ssm.shm_read_queue_info.error); /*"MPIDI_CH3I_SHM_Attach_to_mem failed, error %d\n", vc_ptr->ssm.shm_read_queue_info.error);*/
	    return mpi_errno;
	}
	MPIU_DBG_PRINTF(("attached to queue from process %d\n", info.pg_rank));
	/*vc_ptr->ssm.state = MPIDI_CH3I_VC_STATE_CONNECTED;*/ /* we are read connected but not write connected */
	vc_ptr->ssm.bShm = TRUE;
	vc_ptr->ssm.read_shmq = vc_ptr->ssm.shm_read_queue_info.addr;
	MPIU_DBG_PRINTF(("read_shmq = %p\n", vc_ptr->ssm.read_shmq));
	vc_ptr->ssm.shm_reading_pkt = TRUE;
	/* add this VC to the global list to be shm_waited on */
	vc_ptr->ssm.shm_next_reader = MPIDI_CH3I_Process.shm_reading_list;
	MPIDI_CH3I_Process.shm_reading_list = vc_ptr;
    }
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Progress_test
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Progress_test()
{
    /* This function has a problem that is #if 0'd out.
     * The commented out code causes test to only probe the message queue for connection attempts
     * every MPIDI_CH3I_MSGQ_ITERATIONS iterations.  This can delay shm connection formation for 
     * codes that call test infrequently.
     * But the uncommented code also has the problem that the overhead of checking the message queue
     * is incurred with every call to test.
     */
    int mpi_errno = MPI_SUCCESS;
    int rc;
    sock_event_t event;
    int num_bytes;
    MPIDI_VC *vc_ptr;
#if 0
    static int msgqIter = 0;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS_TEST);
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_YIELD);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS_TEST);


    if (MPIDI_CH3I_Process.shm_reading_list)
    {
	rc = MPIDI_CH3I_SHM_read_progress(
	    MPIDI_CH3I_Process.shm_reading_list,
	    0, &vc_ptr, &num_bytes);
	if (rc == MPI_SUCCESS)
	{
	    MPIDI_DBG_PRINTF((50, FCNAME, "MPIDI_CH3I_SHM_read_progress reported %d bytes read", num_bytes));
	    handle_shm_read(vc_ptr, num_bytes);
	}
    }

    if (MPIDI_CH3I_Process.shm_writing_list)
    {
	vc_ptr = MPIDI_CH3I_Process.shm_writing_list;
	while (vc_ptr)
	{
	    if (vc_ptr->ssm.send_active != NULL)
	    {
		MPIDI_CH3I_SHM_write_progress(vc_ptr);
	    }
	    vc_ptr = vc_ptr->ssm.shm_next_writer;
	}
    }

    rc = sock_wait(sock_set, 0, &event);
    if (rc == SOCK_SUCCESS)
    {
	rc = handle_sock_op(&event);
	if (rc != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(rc, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**handle_sock_op", 0);
	    return mpi_errno;
	}
	/*active = active | MPID_CH3I_SOCK_BIT;*/
	MPIDI_CH3I_active_flag |= MPID_CH3I_SOCK_BIT;
    }

#if 0
    if (msgqIter++ == MPIDI_CH3I_MSGQ_ITERATIONS)
    {
	msgqIter = 0;
	/*printf("[%d] calling message queue progress from test.\n", MPIR_Process.comm_world->rank);fflush(stdout);*/
	MPID_CH3I_Message_queue_progress();
    }
#else
    MPID_CH3I_Message_queue_progress();
#endif

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS_TEST);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Progress_wait
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Progress_wait()
{
    int mpi_errno = MPI_SUCCESS;
    int rc;
    sock_event_t event;
    unsigned completions = MPIDI_CH3I_progress_completions;
    int num_bytes;
    MPIDI_VC *vc_ptr;

    /*static int active = 0;*/
    static int updateIter = 0;
    static int msgqIter = 0;
    MPID_CPU_Tick_t start, end;

    static MPID_CPU_Tick_t shmTicks = 0;
    static int shmIter = 0;
    static int shmReps = 1;
    static int shmTotalReps = 0;
    static int spin_count = 1;

    static MPID_CPU_Tick_t sockTicks = 0;
    static int sockIter = 0;
    static int sockReps = 1;
    static int sockTotalReps = 0;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS);
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_YIELD);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS);

    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));
    do
    {
	/* make progress on the shared memory queues */

	if (sockIter != 0)
	    goto skip_shm_loop;

	MPID_CPU_TICK(&start);
	for (; shmIter<shmReps; shmIter++, shmTotalReps++)
	{
	    if (MPIDI_CH3I_Process.shm_reading_list)
	    {
		rc = MPIDI_CH3I_SHM_read_progress(
		    MPIDI_CH3I_Process.shm_reading_list,
		    0, &vc_ptr, &num_bytes);
		if (rc == MPI_SUCCESS)
		{
		    MPIDI_DBG_PRINTF((50, FCNAME, "MPIDI_CH3I_SHM_read_progress reported %d bytes read", num_bytes));
		    handle_shm_read(vc_ptr, num_bytes);
		}
		else
		{
		    if (rc != SHM_WAIT_TIMEOUT)
		    {
			MPIDI_err_printf("MPIDI_CH3_Progress", "MPIDI_CH3I_SHM_read_progress returned error %d\n", rc);
			goto fn_exit;
		    }
		    /*
		    if (rc == SHM_WAIT_TIMEOUT_WHILE_ACTIVE)
		    {
			active = active | MPID_CH3I_SHM_BIT;
		    }
		    */
		}
		if (MPIDI_CH3I_shm_read_active)
		    spin_count = 1;
		if (completions != MPIDI_CH3I_progress_completions)
		{
		    MPIDI_CH3I_active_flag |= MPID_CH3I_SHM_BIT;
		    /*
		    active = active | MPID_CH3I_SHM_BIT;
		    spin_count = 1;
		    */
		    shmIter++;
		    shmTotalReps++;
		    goto after_shm_loop;
		}
	    }

	    if (MPIDI_CH3I_Process.shm_writing_list)
	    {
		vc_ptr = MPIDI_CH3I_Process.shm_writing_list;
		while (vc_ptr)
		{
		    if (vc_ptr->ssm.send_active != NULL)
		    {
			if (MPIDI_CH3I_SHM_write_progress(vc_ptr) != 0)
			{
			    /*active = active | MPID_CH3I_SHM_BIT;*/
			    if (completions != MPIDI_CH3I_progress_completions)
			    {
				MPIDI_CH3I_active_flag |= MPID_CH3I_SHM_BIT;
				spin_count = 1;
				shmIter++;
				shmTotalReps++;
				goto after_shm_loop;
			    }
			}
		    }
		    vc_ptr = vc_ptr->ssm.shm_next_writer;
		}
	    }
/*
	    if (spin_count++ >= MPIDI_CH3I_Process.pg->nShmWaitSpinCount)
	    {
		MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_YIELD);
		MPIDU_Yield();
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_YIELD);
		spin_count = 1;
	    }
*/
	    MPIDU_Yield();
	}
after_shm_loop:
	if (shmIter == shmReps)
	{
	    shmIter = 0;
	}
	MPID_CPU_TICK(&end);
	shmTicks += end - start;

	if (shmIter != 0)
	    goto skip_sock_loop;

skip_shm_loop:
	MPID_CPU_TICK(&start);
	for (; sockIter<sockReps; sockIter++, sockTotalReps++)
	{
	    /* make progress on the sockets */

	    rc = sock_wait(sock_set, 0, &event);
	    if (rc == SOCK_SUCCESS)
	    {
		rc = handle_sock_op(&event);
		if (rc != MPI_SUCCESS)
		{
		    mpi_errno = MPIR_Err_create_code(rc, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**handle_sock_op", 0);
		    return mpi_errno;
		}
		/*active = active | MPID_CH3I_SOCK_BIT;*/
		MPIDI_CH3I_active_flag |= MPID_CH3I_SOCK_BIT;
	    }
	    else
	    {
		if (rc != SOCK_ERR_TIMEOUT)
		{
		    goto fn_exit;
		}
		MPIDU_Yield();
	    }
	    if (completions != MPIDI_CH3I_progress_completions)
	    {
		sockIter++;
		sockTotalReps++;
		goto after_sock_loop;
	    }
	}
after_sock_loop:
	if (sockIter == sockReps)
	{ 
	    sockIter = 0;
	    updateIter++;
	}
	MPID_CPU_TICK(&end);
	sockTicks += end - start;

skip_sock_loop:
	if (updateIter == MPIDI_CH3I_UPDATE_ITERATIONS)
	{
	    updateIter = 0;
	    switch (MPIDI_CH3I_active_flag)
	    {
	    case MPID_CH3I_SHM_BIT:
		/* only shared memory has been active */
		/* give shm MPID_SINGLE_ACTIVE_FACTOR cycles for every 1 sock cycle */
		shmReps = (int) (
		    ( sockTicks * (MPID_CPU_Tick_t)MPID_SINGLE_ACTIVE_FACTOR * (MPID_CPU_Tick_t)shmTotalReps ) 
		    / (MPID_CPU_Tick_t)sockTotalReps 
		    / shmTicks);
		if (shmReps < 1)
		    shmReps = 1;
		sockReps = 1;
		/*MPIU_DBG_PRINTF(("(SHM_BIT: shmReps = %d, sockReps = %d)", shmReps, sockReps));*/
		break;
	    case MPID_CH3I_SOCK_BIT:
		/* only sockets have been active */
		/* give sock MPID_SINGLE_ACTIVE_FACTOR cycles for every 1 shm cycle */
		sockReps = (int) ( 
		    ( shmTicks * (MPID_CPU_Tick_t)MPID_SINGLE_ACTIVE_FACTOR * (MPID_CPU_Tick_t)sockTotalReps )
		    / (MPID_CPU_Tick_t)shmTotalReps
		    / sockTicks );
		if (sockReps < 1)
		    sockReps = 1;
		shmReps = 1;
		/*MPIU_DBG_PRINTF(("(SOCK_BIT: shmReps = %d, sockReps = %d)", shmReps, sockReps));*/
		break;
	    case MPID_CH3I_SHM_BIT + MPID_CH3I_SOCK_BIT:
		/* both channels have been active */
		/* give each channel 50% of the spin cycles */
		shmReps = (int)
		    (
		     (
		      (MPID_CPU_Tick_t)((shmTicks + sockTicks) >> 1)
		      * (MPID_CPU_Tick_t)shmTotalReps
		     ) / shmTicks
		    ) / MPIDI_CH3I_UPDATE_ITERATIONS;
		if (shmReps < 1)
		    shmReps = 1;
		sockReps = (int)
		    (
		     (
		      (MPID_CPU_Tick_t)((shmTicks + sockTicks) >> 1)
		      * (MPID_CPU_Tick_t)sockTotalReps
		     ) / sockTicks
		    ) / MPIDI_CH3I_UPDATE_ITERATIONS;
		if (sockReps < 1)
		    sockReps = 1;
		/*MPIU_DBG_PRINTF(("(BOTH_BITS: shmReps = %d, sockReps = %d)", shmReps, sockReps));*/
		break;
	    default:
		/* neither channel has been active */
		/* leave the state in its previous state */
		/*printf(".");*/
		break;
	    }
	    MPIDI_CH3I_active_flag = 0;
	    shmTotalReps = 0;
	    shmTicks = 0;
	    shmIter = 0;
	    sockTotalReps = 0;
	    sockTicks = 0;
	    sockIter = 0;
	}

	if (msgqIter++ == MPIDI_CH3I_MSGQ_ITERATIONS)
	{
	    msgqIter = 0;
	    /*printf("[%d] calling message queue progress\n", MPIR_Process.comm_world->rank);fflush(stdout);*/
	    MPID_CH3I_Message_queue_progress();
	}
    }
    while (completions == MPIDI_CH3I_progress_completions);

fn_exit:
    MPIDI_DBG_PRINTF((50, FCNAME, "exiting, count=%d", MPIDI_CH3I_progress_completions - completions));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS);
    return mpi_errno;
}

#endif /* USE_ADAPTIVE_PROGRESS */

#ifdef USE_FIXED_ACTIVE_PROGRESS

#define MPIDI_CH3I_UPDATE_ITERATIONS    10
#define MPID_SINGLE_ACTIVE_FACTOR      100

#undef FUNCNAME
#define FUNCNAME MPID_CH3I_Message_queue_progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_CH3I_Message_queue_progress()
{
    MPIDI_CH3I_Shmem_queue_info info;
    int num_bytes;
    MPIDI_VC *vc_ptr;
    int rc, mpi_errno;

    /* check for new shmem queue connection requests */
    rc = MPIDI_CH3I_BootstrapQ_recv_msg(
	MPIDI_CH3I_Process.pg->bootstrapQ, &info, sizeof(info), 
	&num_bytes, FALSE);
    if (rc != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(rc, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**boot_recv", 0);
	return mpi_errno;
    }
    assert(num_bytes == 0 || num_bytes == sizeof(info));
    if (num_bytes)
    {
	vc_ptr = &MPIDI_CH3I_Process.pg->vc_table[info.pg_rank];
	rc = MPIDI_CH3I_SHM_Attach_to_mem(
	    &info.info, &vc_ptr->ssm.shm_read_queue_info);
	if (rc != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**MPIDI_CH3I_SHM_Attach_to_mem", "**MPIDI_CH3I_SHM_Attach_to_mem %d", vc_ptr->ssm.shm_read_queue_info.error); /*"MPIDI_CH3I_SHM_Attach_to_mem failed, error %d\n", vc_ptr->ssm.shm_read_queue_info.error);*/
	    return mpi_errno;
	}
	MPIU_DBG_PRINTF(("attached to queue from process %d\n", info.pg_rank));
	/*vc_ptr->ssm.state = MPIDI_CH3I_VC_STATE_CONNECTED;*/ /* we are read connected but not write connected */
	vc_ptr->ssm.bShm = TRUE;
	vc_ptr->ssm.read_shmq = vc_ptr->ssm.shm_read_queue_info.addr;
	MPIU_DBG_PRINTF(("read_shmq = %p\n", vc_ptr->ssm.read_shmq));
	vc_ptr->ssm.shm_reading_pkt = TRUE;
	/* add this VC to the global list to be shm_waited on */
	vc_ptr->ssm.shm_next_reader = MPIDI_CH3I_Process.shm_reading_list;
	MPIDI_CH3I_Process.shm_reading_list = vc_ptr;
    }
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Progress(int is_blocking)
{
    int mpi_errno = MPI_SUCCESS;
    int rc;
    sock_event_t event;
    unsigned completions = MPIDI_CH3I_progress_completions;
    int num_bytes;
    MPIDI_VC *vc_ptr;

    /*static int active = 0;*/
    static int updateIter = 0;
    static int msgqIter = 0;
    MPID_CPU_Tick_t start, end;

    static MPID_CPU_Tick_t shmTicks = 0;
    static int shmIter = 0;
    static int shmReps = 1;
    static int shmTotalReps = 0;
    static int spin_count = 1;

    static MPID_CPU_Tick_t sockTicks = 0;
    static int sockIter = 0;
    static int sockReps = 1;
    static int sockTotalReps = 0;

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

	if (sockIter != 0)
	    goto skip_shm_loop;

	MPID_CPU_TICK(&start);
	for (; shmIter<shmReps; shmIter++, shmTotalReps++)
	{
	    if (MPIDI_CH3I_Process.shm_reading_list)
	    {
		rc = MPIDI_CH3I_SHM_read_progress(
		    MPIDI_CH3I_Process.shm_reading_list,
		    0, &vc_ptr, &num_bytes);
		if (rc == MPI_SUCCESS)
		{
		    MPIDI_DBG_PRINTF((50, FCNAME, "MPIDI_CH3I_SHM_read_progress reported %d bytes read", num_bytes));
		    handle_shm_read(vc_ptr, num_bytes);
		}
		else
		{
		    if (rc != SHM_WAIT_TIMEOUT)
		    {
			MPIDI_err_printf("MPIDI_CH3_Progress", "MPIDI_CH3I_SHM_read_progress returned error %d\n", rc);
			goto fn_exit;
		    }
		    /*
		    if (rc == SHM_WAIT_TIMEOUT_WHILE_ACTIVE)
		    {
			active = active | MPID_CH3I_SHM_BIT;
		    }
		    */
		}
		if (MPIDI_CH3I_shm_read_active)
		    spin_count = 1;
		if (completions != MPIDI_CH3I_progress_completions)
		{
		    MPIDI_CH3I_active_flag |= MPID_CH3I_SHM_BIT;
		    /*
		    active = active | MPID_CH3I_SHM_BIT;
		    spin_count = 1;
		    */
		    shmIter++;
		    shmTotalReps++;
		    goto after_shm_loop;
		}
	    }

	    if (MPIDI_CH3I_Process.shm_writing_list)
	    {
		vc_ptr = MPIDI_CH3I_Process.shm_writing_list;
		while (vc_ptr)
		{
		    if (vc_ptr->ssm.send_active != NULL)
		    {
			if (MPIDI_CH3I_SHM_write_progress(vc_ptr) != 0)
			{
			    /*active = active | MPID_CH3I_SHM_BIT;*/
			    if (completions != MPIDI_CH3I_progress_completions)
			    {
				MPIDI_CH3I_active_flag |= MPID_CH3I_SHM_BIT;
				spin_count = 1;
				shmIter++;
				shmTotalReps++;
				goto after_shm_loop;
			    }
			}
		    }
		    vc_ptr = vc_ptr->ssm.shm_next_writer;
		}
	    }
	    if (spin_count++ >= MPIDI_CH3I_Process.pg->nShmWaitSpinCount)
	    {
		MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_YIELD);
		MPIDU_Yield();
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_YIELD);
		spin_count = 1;
	    }
	}
after_shm_loop:
	if (shmIter == shmReps)
	{
	    shmIter = 0;
	}
	MPID_CPU_TICK(&end);
	shmTicks += end - start;

	if (shmIter != 0)
	    goto skip_sock_loop;

skip_shm_loop:
	MPID_CPU_TICK(&start);
	for (; sockIter<sockReps; sockIter++, sockTotalReps++)
	{
	    /* make progress on the sockets */

	    rc = sock_wait(sock_set, 0, &event);
	    if (rc == SOCK_SUCCESS)
	    {
		rc = handle_sock_op(&event);
		if (rc != MPI_SUCCESS)
		{
		    mpi_errno = MPIR_Err_create_code(rc, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**handle_sock_op", 0);
		    return mpi_errno;
		}
		/*active = active | MPID_CH3I_SOCK_BIT;*/
		MPIDI_CH3I_active_flag |= MPID_CH3I_SOCK_BIT;
	    }
	    else
	    {
		if (rc != SOCK_ERR_TIMEOUT)
		{
		    goto fn_exit;
		}
		MPIDU_Yield();
	    }
	    if (completions != MPIDI_CH3I_progress_completions)
	    {
		sockIter++;
		sockTotalReps++;
		goto after_sock_loop;
	    }
	}
after_sock_loop:
	if (sockIter == sockReps)
	{ 
	    sockIter = 0;
	    updateIter++;
	}
	MPID_CPU_TICK(&end);
	sockTicks += end - start;

skip_sock_loop:
	if (updateIter == MPIDI_CH3I_UPDATE_ITERATIONS)
	{
	    updateIter = 0;
	    switch (MPIDI_CH3I_active_flag)
	    {
	    case MPID_CH3I_SHM_BIT:
		/* only shared memory has been active */
		/* give shm MPID_SINGLE_ACTIVE_FACTOR cycles for every 1 sock cycle */
		shmReps = (int) (
		    ( sockTicks * (MPID_CPU_Tick_t)MPID_SINGLE_ACTIVE_FACTOR * (MPID_CPU_Tick_t)shmTotalReps ) 
		    / (MPID_CPU_Tick_t)sockTotalReps 
		    / shmTicks);
		if (shmReps < 1)
		    shmReps = 1;
		sockReps = 1;
		/*MPIU_DBG_PRINTF(("(SHM_BIT: shmReps = %d, sockReps = %d)", shmReps, sockReps));*/
		break;
	    case MPID_CH3I_SOCK_BIT:
		/* only sockets have been active */
		/* give sock MPID_SINGLE_ACTIVE_FACTOR cycles for every 1 shm cycle */
		sockReps = (int) ( 
		    ( shmTicks * (MPID_CPU_Tick_t)MPID_SINGLE_ACTIVE_FACTOR * (MPID_CPU_Tick_t)sockTotalReps )
		    / (MPID_CPU_Tick_t)shmTotalReps
		    / sockTicks );
		if (sockReps < 1)
		    sockReps = 1;
		shmReps = 1;
		/*MPIU_DBG_PRINTF(("(SOCK_BIT: shmReps = %d, sockReps = %d)", shmReps, sockReps));*/
		break;
	    case MPID_CH3I_SHM_BIT + MPID_CH3I_SOCK_BIT:
		/* both channels have been active */
		/* give each channel 50% of the spin cycles */
		shmReps = (int)
		    (
		     (
		      (MPID_CPU_Tick_t)((shmTicks + sockTicks) >> 1)
		      * (MPID_CPU_Tick_t)shmTotalReps
		     ) / shmTicks
		    ) / MPIDI_CH3I_UPDATE_ITERATIONS;
		if (shmReps < 1)
		    shmReps = 1;
		sockReps = (int)
		    (
		     (
		      (MPID_CPU_Tick_t)((shmTicks + sockTicks) >> 1)
		      * (MPID_CPU_Tick_t)sockTotalReps
		     ) / sockTicks
		    ) / MPIDI_CH3I_UPDATE_ITERATIONS;
		if (sockReps < 1)
		    sockReps = 1;
		/*MPIU_DBG_PRINTF(("(BOTH_BITS: shmReps = %d, sockReps = %d)", shmReps, sockReps));*/
		break;
	    default:
		/* neither channel has been active */
		/* leave the state in its previous state */
		/*printf(".");*/
		break;
	    }
	    MPIDI_CH3I_active_flag = 0;
	    shmTotalReps = 0;
	    shmTicks = 0;
	    shmIter = 0;
	    sockTotalReps = 0;
	    sockTicks = 0;
	    sockIter = 0;
	}

	if (msgqIter++ == MPIDI_CH3I_MSGQ_ITERATIONS)
	{
	    msgqIter = 0;
	    MPID_CH3I_Message_queue_progress();
	}
    }
    while (completions == MPIDI_CH3I_progress_completions && is_blocking);

fn_exit:
#ifdef MPICH_DBG_OUTPUT
    if (is_blocking)
    {
	MPIDI_DBG_PRINTF((50, FCNAME, "exiting, count=%d", MPIDI_CH3I_progress_completions - completions));
    }
    else
    {
	if (MPIDI_CH3I_progress_completions - completions > 0)
	{
	    MPIDI_DBG_PRINTF((50, FCNAME, "exiting (non-blocking), count=%d", MPIDI_CH3I_progress_completions - completions));
	}
    }
#endif
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS);
    /*return MPIDI_CH3I_progress_completions - completions;*/
    return mpi_errno;
}

#endif

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

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress_init()
{
    int rc;
    sock_t sock;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_INIT);

    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));

    rc = sock_init();
    if (rc != SOCK_SUCCESS)
    {
	mpi_errno = MPIDI_CH3I_sock_errno_to_mpi_errno(rc, FCNAME);
	/*MPID_Abort(NULL, mpi_errno);*/
	goto fn_exit;
    }

    /* create sock set */
    rc = sock_create_set(&sock_set);
    if (rc != SOCK_SUCCESS)
    {
	mpi_errno = MPIDI_CH3I_sock_errno_to_mpi_errno(rc, FCNAME);
	/*MPID_Abort(NULL, mpi_errno);*/
	goto fn_exit;
    }

    /* establish non-blocking listener */
    listener_conn = connection_alloc();
    if (listener_conn == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }
    listener_conn->sock = NULL;
    listener_conn->vc = NULL;
    listener_conn->state = CONN_STATE_LISTENING;
    listener_conn->send_active = NULL;
    listener_conn->recv_active = NULL;

    rc = sock_listen(sock_set, listener_conn, &listener_port, &sock);
    if (rc != SOCK_SUCCESS)
    {
	mpi_errno = MPIDI_CH3I_sock_errno_to_mpi_errno(rc, FCNAME);
	/*MPID_Abort(NULL, mpi_errno);*/
	goto fn_exit;
    }

    listener_conn->sock = sock;

fn_exit:
    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
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
	NMPI_Barrier(MPI_COMM_WORLD); /* FIXME: this barrier may not be necessary */
	shutting_down = TRUE;
	NMPI_Barrier(MPI_COMM_WORLD);
    }
    MPIR_Nest_decr();

    /* Shut down the listener */
    mpi_errno = sock_post_close(listener_conn->sock);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pfinal_sockclose", 0);
    }

    /* XXX: Wait for listener sock to close */

    /* FIXME: Cleanly shutdown other socks and MPIU_Free connection structures. (close protocol?) */

    sock_destroy_set(sock_set);
    sock_finalize();

    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS_FINALIZE);
    return mpi_errno;
}
