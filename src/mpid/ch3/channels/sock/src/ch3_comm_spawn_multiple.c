/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"
 
static void free_pmi_keyvals(PMI_keyval_t **kv, int size, int *counts)
{
    int i,j;

    for (i=0; i<size; i++)
    {
	for (j=0; j<counts[i]; j++)
	{
	    MPIU_Free(kv[i][j].key);
	    MPIU_Free(kv[i][j].val);
	}
	if (kv[i] != NULL)
	{
	    MPIU_Free(kv[i]);
	}
    }
    MPIU_Free(kv);
}

/*
 * MPIDI_CH3_Comm_spawn_multiple()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Comm_spawn_multiple
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Comm_spawn_multiple(int count, char **commands, 
                                  char ***argvs, int *maxprocs, 
                                  MPID_Info **info_ptrs, int root,
                                  MPID_Comm *comm_ptr, MPID_Comm
                                  **intercomm, int *errcodes) 
{
    char port_name[MPI_MAX_PORT_NAME];
    int *info_keyval_sizes, i, mpi_errno=MPI_SUCCESS;
    PMI_keyval_t **info_keyval_vectors, preput_keyval_vector;
    int icount, j;
    MPID_Info *iter;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_COMM_SPAWN_MULTIPLE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_COMM_SPAWN_MULTIPLE);

    if (comm_ptr->rank == root)
    {
	/* convert the infos into PMI keyvals */
        info_keyval_sizes = (int *) MPIU_Malloc(count * sizeof(int));
	info_keyval_vectors = (PMI_keyval_t**) MPIU_Malloc(count * sizeof(PMI_keyval_t*));
        for (i=0; i<count; i++)
	{
	    if (info_ptrs != NULL)
	    {
		icount = 0;
		iter = info_ptrs[i];
		while (iter)
		{
		    icount++;
		    iter = iter->next;
		}
		info_keyval_sizes[i] = icount;
		info_keyval_vectors[i] = (PMI_keyval_t*) MPIU_Malloc(icount * sizeof(PMI_keyval_t));
		iter = info_ptrs[i];
		for (j=0; j<icount; j++)
		{
		    info_keyval_vectors[i][j].key = MPIU_Strdup(iter->key);
		    info_keyval_vectors[i][j].val = MPIU_Strdup(iter->value);
		    iter = iter->next;
		}
	    }
	    else
	    {
		info_keyval_sizes[i] = 0;
		info_keyval_vectors[i] = NULL;
	    }
	}

        mpi_errno = MPIDI_CH3_Open_port(port_name);
        if (mpi_errno != MPI_SUCCESS)
	{
	    free_pmi_keyvals(info_keyval_vectors, count, info_keyval_sizes);
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	    goto fn_exit;
	}
        
        preput_keyval_vector.key = "PARENT_ROOT_PORT_NAME";
        preput_keyval_vector.val = port_name;

        mpi_errno = PMI_Spawn_multiple(count, (const char **)
                                       commands, 
                                       (const char ***) argvs,
                                       maxprocs, info_keyval_sizes,
                                       (const PMI_keyval_t **)
                                       info_keyval_vectors, 1, 
                                       &preput_keyval_vector,
                                       errcodes);

        if (mpi_errno != PMI_SUCCESS)
        {
	    free_pmi_keyvals(info_keyval_vectors, count, info_keyval_sizes);
	    MPIU_Free(info_keyval_sizes);
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_spawn_multiple", "**pmi_spawn_multiple %d", mpi_errno);
            goto fn_exit;
        }

	free_pmi_keyvals(info_keyval_vectors, count, info_keyval_sizes);
        MPIU_Free(info_keyval_sizes);
    }

    mpi_errno = MPIDI_CH3_Comm_accept(port_name, root, comm_ptr, intercomm); 

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_COMM_SPAWN_MULTIPLE);
    return mpi_errno;
}
