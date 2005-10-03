/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"
#include "ibu.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Init(int * has_args, int * has_env, int * has_parent, MPIDI_PG_t *pg, int pg_rank,
		   char **publish_bc_p, char **bc_key_p, char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno = PMI_SUCCESS;
    int pg_size;
    int port;
    char * key = NULL;
    char * val = NULL;
    int key_max_sz;
    int val_max_sz;

    MPIU_DBG_PRINTF(("entering ch3_init.\n"));
    pmi_errno = PMI_Get_size(&pg_size);
    if (pmi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_size", "**pmi_get_size %d", pmi_errno);
	return mpi_errno;
    }

    /* Call dbg_init as soon as the rank is available */
    MPIU_dbg_init(pg_rank);
    /*MPIU_Timer_init(pg_rank, pg_size);*/

    pmi_errno = PMI_KVS_Get_key_length_max(&key_max_sz);
    if (pmi_errno != PMI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %d", pmi_errno);
	return mpi_errno;
    }
    key_max_sz++;
    key = MPIU_Malloc(key_max_sz);
    if (key == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }
    pmi_errno = PMI_KVS_Get_value_length_max(&val_max_sz);
    if (pmi_errno != PMI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %d", pmi_errno);
	return mpi_errno;
    }
    val_max_sz++;
    val = MPIU_Malloc(val_max_sz);
    if (val == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }
    
    /* initialize the infinband functions */
    mpi_errno = ibu_init();
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_ibu", 0);
	return mpi_errno;
    }
    /* create a completion set for this process */
    mpi_errno = ibu_create_set(&MPIDI_CH3I_Process.set);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_ibu_set", 0);
	return mpi_errno;
    }
    /* get and put the local id for this process in the PMI database */
    port = ibu_get_lid();

    mpi_errno = MPIU_Snprintf(key, key_max_sz, "P%d-lid", pg_rank);
    if (mpi_errno < 0 || mpi_errno > key_max_sz)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**snprintf", "**snprintf %d", mpi_errno);
	return mpi_errno;
    }
    mpi_errno = MPIU_Snprintf(val, val_max_sz, "%d", port);
    if (mpi_errno < 0 || mpi_errno > val_max_sz)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**snprintf", "**snprintf %d", mpi_errno);
	return mpi_errno;
    }
    mpi_errno = MPI_SUCCESS; /* reset errno after successful snprintf calls */
    pmi_errno = PMI_KVS_Put(pg_p->ch.kvs_name, key, val);
    if (pmi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", pmi_errno);
	return mpi_errno;
    }

    MPIU_DBG_PRINTF(("Published lid=%d\n", port));
    
    pmi_errno = PMI_KVS_Commit(pg_p->ch.kvs_name);
    if (pmi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_commit", "**pmi_kvs_commit %d", pmi_errno);
	return mpi_errno;
    }
    pmi_errno = PMI_Barrier();
    if (pmi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);
	return mpi_errno;
    }

    /* XXX - has_args and has_env need to come from PMI eventually... */
    *has_args = TRUE;
    *has_env = TRUE;

    /* for now, connect all the processes at init time */
    MPIDI_DBG_PRINTF((65, "ch3_init", "calling setup_connections.\n"));fflush(stdout);
    mpi_errno = MPIDI_CH3I_Setup_connections(pg_p, pg_rank);
    MPIDI_DBG_PRINTF((65, "ch3_init", "connections formed, exiting\n"));fflush(stdout);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**setup_connections", 0);
	goto fn_fail;
    }

 fn_exit:
    if (val != NULL)
    {
	MPIU_Free(val);
    }
    if (key != NULL)
    {
	MPIU_Free(key);
    }

    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
