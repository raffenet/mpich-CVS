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
#if 0    /* brad : all accomplished through upcall */
    int rc;
    MPIDI_CH3I_Alloc_mem_list_t *next_ptr, *curr_ptr;

    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));

    /* Shutdown the progress engine */
    mpi_errno = MPIDI_CH3I_Progress_finalize();
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**finalize_progress", 0);
    }

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

    /* if the user had called MPI_Alloc_mem but forgot to call MPI_Free_mem, deallocate any shared
       memory that was allocated */

    /* brad : this should be a separate routine for cleaning up memory, that way and it can be
     *          called elsewhere.  the (new) feature MPIDI_CH3_IMPLEMENTS_CLEANUP_MEM could be
     *          used to tell of this.  we'll need to add an MPID_Cleanup_mem procedure.  it has
     *          also been suggested to put all memory-related functions in a single file, possibly
     *          with the appropriate static's having scope only there.
     */
    curr_ptr = MPIDI_CH3I_Alloc_mem_list_head;
    while (curr_ptr != NULL) {
        /* deallocate shared memory */
        MPIDI_CH3I_SHM_Unlink_and_detach_mem(curr_ptr->shm_struct);  /* brad : func is sshm only */
            
        next_ptr = curr_ptr->next;

        MPIU_Free(curr_ptr->shm_struct);
        MPIU_Free(curr_ptr);

        curr_ptr = next_ptr;
    }
    MPIDI_CH3I_Alloc_mem_list_head = NULL;
#endif
    /* brad : this functionality now, in theory,  could be used elsewhere */
    MPIDI_CH3_Cleanup_mem();

#if 0  /* brad : also accomplished in upcall */

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
