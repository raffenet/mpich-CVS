/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Win_free
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_free(MPID_Win **win_ptr)
{
    int mpi_errno=MPI_SUCCESS, total_pt_rma_puts_accs, i, *recvcnts, comm_size;
    MPID_Comm *comm_ptr;
#ifndef MPICH_SINGLE_THREADED
    int err;
#endif

    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_FREE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_FREE);

    MPIR_Nest_incr();

    /* set up the recvcnts array for the reduce scatter to check if all
       passive target rma operations are done */
    MPID_Comm_get_ptr( (*win_ptr)->comm, comm_ptr );
    comm_size = comm_ptr->local_size;

    recvcnts = (int *) MPIU_Malloc(comm_size * sizeof(int));
    /* --BEGIN ERROR HANDLING-- */
    if (!recvcnts)
    {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        goto fn_exit;
    }
    /* --END ERROR HANDLING-- */
    
    for (i=0; i<comm_size; i++)
    {
        recvcnts[i] = 1;
    }
    
    mpi_errno = NMPI_Reduce_scatter((*win_ptr)->pt_rma_puts_accs, 
                                    &total_pt_rma_puts_accs, recvcnts, 
                                    MPI_INT, MPI_SUM, (*win_ptr)->comm);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "The reduce_scatter to send out the data to all the nodes in the fence failed");
        goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    MPIU_Free(recvcnts);

    if (total_pt_rma_puts_accs != (*win_ptr)->my_pt_rma_puts_accs)
    {
	MPID_Progress_state progress_state;
	
        /* poke the progress engine until the two are equal */
        MPID_Progress_start(&progress_state);
	while (total_pt_rma_puts_accs != (*win_ptr)->my_pt_rma_puts_accs)
        {
            mpi_errno = MPID_Progress_wait(&progress_state);
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno != MPI_SUCCESS)
            {
		MPID_Progress_end(&progress_state);
                mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
						 "**fail", "**fail %s", "making progress on the rma messages failed");
                goto fn_exit;
            }
            /* --END ERROR HANDLING-- */
        }
	MPID_Progress_end(&progress_state);
    }



    MPIDI_Passive_target_thread_exit_flag = 1;

#ifdef FOOOOOOOOOOO

#ifndef MPICH_SINGLE_THREADED
#ifdef HAVE_PTHREAD_H
    pthread_join((*win_ptr)->passive_target_thread_id, (void **) &err);
#elif defined(HAVE_WINTHREADS)
    if (WaitForSingleObject((*win_ptr)->passive_target_thread_id, INFINITE) == WAIT_OBJECT_0)
	err = GetExitCodeThread((*win_ptr)->passive_target_thread_id, &err);
    else
	err = GetLastError();
#else
#error Error: No thread package specified.
#endif
    mpi_errno = err;
#endif

#endif

    NMPI_Comm_free(&((*win_ptr)->comm));

    MPIR_Nest_decr();

    MPIU_Free((*win_ptr)->base_addrs);
    MPIU_Free((*win_ptr)->disp_units);
    MPIU_Free((*win_ptr)->all_win_handles);
    MPIU_Free((*win_ptr)->pt_rma_puts_accs);
 
    /* check whether refcount needs to be decremented here as in group_free */
    MPIU_Handle_obj_free( &MPID_Win_mem, *win_ptr );
 
 fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FREE);
    return mpi_errno;
}
