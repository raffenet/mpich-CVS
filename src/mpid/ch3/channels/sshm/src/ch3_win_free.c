/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/*
 * MPIDI_CH3_Win_free()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Win_free
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Win_free(MPID_Win **win_ptr)
{
    int mpi_errno = MPI_SUCCESS, comm_size, rank, i;
    MPID_Comm *comm_ptr;
    
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_WIN_FREE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_WIN_FREE);

    MPID_Comm_get_ptr( (*win_ptr)->comm, comm_ptr );
    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    MPIR_Nest_incr();

    /* barrier needed so that all passive target rmas directed toward this process are over */
    mpi_errno = NMPI_Barrier((*win_ptr)->comm);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
        goto fn_exit;
    }
    /* --END ERROR HANDLING-- */
    
    mpi_errno = NMPI_Comm_free(&((*win_ptr)->comm));
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
        goto fn_exit;
    }
    /* --END ERROR HANDLING-- */
    
    MPIR_Nest_decr();
    
    MPIU_Free((*win_ptr)->base_addrs);
    MPIU_Free((*win_ptr)->disp_units);
    MPIU_Free((*win_ptr)->all_win_handles);
    MPIU_Free((*win_ptr)->pt_rma_puts_accs);
    
    for (i=0; i<comm_size; i++) {
        if (i != rank) {
            mpi_errno = MPIDI_CH3I_SHM_Release_mem( &((*win_ptr)->shm_structs[i]) );
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno != MPI_SUCCESS)
            {
                mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
                goto fn_exit;
            }
            /* --END ERROR HANDLING-- */
        }
    }
    
    MPIU_Free((*win_ptr)->shm_structs);
    MPIU_Free((*win_ptr)->offsets);

    /* check whether refcount needs to be decremented here as in group_free */
    MPIU_Handle_obj_free( &MPID_Win_mem, *win_ptr );

 fn_exit:    
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_WIN_FREE);
    return mpi_errno;
}
