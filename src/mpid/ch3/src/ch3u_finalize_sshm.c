/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include "mpidi_ch3_impl.h"
#include "pmi.h"


/*  MPIDI_CH3U_Finalize_sshm - does scalable shared memory specific channel finalization
 */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Finalize_sshm
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Finalize_sshm()
{
    int mpi_errno = MPI_SUCCESS;

#ifdef MPIDI_CH3_USES_SSHM
    MPIDI_PG_t * pg;
    MPIDI_PG_t * pg_next;
    int inuse;

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

    /* brad : used to unlink this within Init but now done in finalize in case someone spawned needs
     *        to attach to this bootstrapQ.
     */
    mpi_errno = MPIDI_CH3I_BootstrapQ_unlink(MPIDI_Process.my_pg->ch.bootstrapQ);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**boot_unlink", 0);
	return mpi_errno;
    }
    
    mpi_errno = MPIDI_CH3I_BootstrapQ_destroy(MPIDI_Process.my_pg->ch.bootstrapQ);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**finalize_boot", 0);
    }
    
    /* brad : added for dynamic processes in ssm.  needed because the vct's can't be freed
     *         earlier since the vc's themselves are still needed here to walk though and
     *         free their member fields.
     */
    MPIDI_PG_Iterate_reset();
    MPIDI_PG_Get_next(&pg);
    MPIDI_PG_Get_next(&pg_next);
    while(pg_next)
    {
        /* the last one is the original and handled in mpid_finalize.c  */
        MPIDI_PG_Release_ref(pg, &inuse);
        if (inuse == 0)
        {
            MPIDI_PG_Destroy(pg);
        }
        pg = pg_next;
        MPIDI_PG_Get_next(&pg_next);        
    }
    
#endif /* MPIDI_CH3_USES_SSHM  */
    return mpi_errno;
}
