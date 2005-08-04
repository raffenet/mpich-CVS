/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Finalize()
{
    int mpi_errno = MPIDI_CH3U_Finalize_sshm();
#if 0 /*brad : below  is common across all channels */    
    int rc;

    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));

    /* Shutdown the progress engine */
    mpi_errno = MPIDI_CH3I_Progress_finalize();
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**finalize_progress", 0);
    }

    /* brad : below is shared memory specific and now accomplished by upcall */
    
    /* Free resources allocated in CH3_Init() */
    while (MPIDI_CH3I_Process.shm_reading_list)
    {
	MPIDI_CH3I_SHM_Release_mem(&MPIDI_CH3I_Process.shm_reading_list->ch.shm_read_queue_info);
	MPIDI_CH3I_Process.shm_reading_list = MPIDI_CH3I_Process.shm_reading_list->ch.shm_next_reader;
    }
    while (MPIDI_CH3I_Process.shm_writing_list)
    {
	MPIDI_CH3I_SHM_Release_mem(&MPIDI_CH3I_Process.shm_writing_list->ch.shm_write_queue_info);
	MPIDI_CH3I_Process.shm_writing_list = MPIDI_CH3I_Process.shm_writing_list->ch.shm_next_writer;
    }
    mpi_errno = MPIDI_CH3I_BootstrapQ_destroy(MPIDI_Process.my_pg->ch.bootstrapQ);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**finalize_boot", 0);
    }

    /* brad : below is done in all channels */
    
    /* Let PMI know the process is about to exit */
    rc = PMI_Finalize();
    if (rc != 0)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_finalize", "**pmi_finalize %d", rc);
    }
#endif
    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));
    return mpi_errno;

}
