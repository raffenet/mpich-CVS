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
    int mpi_errno = MPI_SUCCESS;
    int rc;
    /*
    char * key;
    char * val;
    int key_max_sz;
    int val_max_sz;
    */

    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));

    /* Shutdown the progress engine */
    mpi_errno = MPIDI_CH3I_Progress_finalize();
    assert (mpi_errno == MPI_SUCCESS);

    /* put a key in the database to acknowledge that this process has made it to MPI_Finalize */
    /*
    key_max_sz = PMI_KVS_Get_key_length_max();
    key = MPIU_Malloc(key_max_sz);
    assert(key != NULL);
    val_max_sz = PMI_KVS_Get_value_length_max();
    val = MPIU_Malloc(val_max_sz);
    assert(val != NULL);
    */

    /* Free resources allocated in CH3_Init() */
    if (MPIDI_CH3I_Process.pg->size > 1)
	MPIDI_CH3I_SHM_Release_mem(MPIDI_CH3I_Process.pg, TRUE);
    else
	MPIDI_CH3I_SHM_Release_mem(MPIDI_CH3I_Process.pg, FALSE);
    MPID_VCRT_Release(MPIR_Process.comm_self->vcrt);
    MPID_VCRT_Release(MPIR_Process.comm_world->vcrt);
    MPIU_Free(MPIDI_CH3I_Process.pg->vc_table);
    MPIU_Free(MPIDI_CH3I_Process.pg->kvs_name);
    MPIU_Free(MPIDI_CH3I_Process.pg);
    /*
    MPIU_Free(key);
    MPIU_Free(val);
    */

    /* Let PMI know the process is about to exit */
    rc = PMI_Finalize();
    assert(rc == 0);
    if (rc)
    {
	mpi_errno = MPIR_Err_create_code(
	    MPI_ERR_OTHER, "**pmi_finalize", 0 );
    }

    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));
    return mpi_errno;
}
