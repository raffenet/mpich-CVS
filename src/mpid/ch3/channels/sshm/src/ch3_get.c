/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/*
 * MPIDI_CH3_Get()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Get
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Get(void *origin_addr, int origin_count, MPI_Datatype
            origin_datatype, int target_rank, MPI_Aint target_disp,
            int target_count, MPI_Datatype target_datatype, MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    void *target_addr;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_GET);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_GET);

    target_addr = (char *) win_ptr->shm_structs[target_rank].addr + 
                  (long) win_ptr->offsets[target_rank] +
                  win_ptr->disp_units[target_rank] * target_disp;

    mpi_errno = MPIR_Localcopy (target_addr, target_count, target_datatype,
                                origin_addr, origin_count, origin_datatype);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno)
    {
        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
    }
    /* --END ERROR HANDLING-- */


    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_GET);
    return mpi_errno;
}




