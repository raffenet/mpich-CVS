/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"

extern MPIDI_CH3I_Process_t MPIDI_CH3I_Process;

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Finalize()
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));

    /* Free resources allocated in CH3_Init() */
    MPID_VCRT_Release (MPIR_Process.comm_self->vcrt);
    MPID_VCRT_Release (MPIR_Process.comm_world->vcrt);
    MPIU_Free (MPIDI_CH3I_Process.pg->vc_table);
    MPIU_Free (MPIDI_CH3I_Process.pg->kvs_name);
    MPIU_Free (MPIDI_CH3I_Process.pg);
    
    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));

    PMI_Finalize ();
    printf_d ("MPIDI_CH3_Finalize\n");
    
    return mpi_errno;
}
