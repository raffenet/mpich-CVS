/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/* FIXME: Why is this here?  Who else is going to implement spawn-multiple? */
#ifdef MPIDI_DEV_IMPLEMENTS_COMM_SPAWN_MULTIPLE

/* 
 * We require support for the PMI calls.  If a channel cannot support
 * a PMI call, it should provide a stub and return an error code.
 */
   

#include "pmi.h"

/* FIXME: We can avoid this is we define PMI as using MPI info values */
static void free_pmi_keyvals(PMI_keyval_t **kv, int size, int *counts)
{
    int i,j;

    for (i=0; i<size; i++)
    {
	for (j=0; j<counts[i]; j++)
	{
	    if (kv[i][j].key != NULL)
		MPIU_Free(kv[i][j].key);
	    if (kv[i][j].val != NULL)
		MPIU_Free(kv[i][j].val);
	}
	if (kv[i] != NULL)
	{
	    MPIU_Free(kv[i]);
	}
    }
    if (kv != NULL)
	MPIU_Free(kv);
}

/*
 * MPIDI_CH3_Comm_spawn_multiple()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_Comm_spawn_multiple
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Comm_spawn_multiple(int count, char **commands, 
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
    char key[MPI_MAX_INFO_KEY];
    int vallen, flag;
    int *pmi_errcodes;
    int total_num_processes;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_COMM_SPAWN_MULTIPLE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_COMM_SPAWN_MULTIPLE);
    
    MPIR_Nest_incr();

    if (comm_ptr->rank == root)
    {
	/* FIXME: This is *really* awkward.  We should either
	   Fix on MPI-style info data structures for PMI (avoid unnecessary
	   duplication) or add an MPIU_Info_getall(...) that creates
	   the necessary arrays of key/value pairs */

	/* convert the infos into PMI keyvals */
        info_keyval_sizes = (int *) MPIU_Malloc(count * sizeof(int));
	info_keyval_vectors = (PMI_keyval_t**) MPIU_Malloc(count * sizeof(PMI_keyval_t*));

        for (i=0; i<count; i++)
	{
	    if (info_ptrs != NULL)
	    {
		icount = 0;
		if (info_ptrs[i] != NULL)
		{
		    mpi_errno = NMPI_Info_get_nkeys(info_ptrs[i]->handle, &icount);
		    if (mpi_errno != MPI_SUCCESS) {
			MPIU_ERR_POP(mpi_errno);
		    }
		}
		info_keyval_sizes[i] = icount;
		if (icount > 0)
		{
		    info_keyval_vectors[i] = (PMI_keyval_t*) MPIU_Malloc(icount * sizeof(PMI_keyval_t));
		}
		else
		{
		    info_keyval_vectors[i] = NULL;
		}
		iter = info_ptrs[i];
		for (j=0; j<icount; j++)
		{
		    mpi_errno = NMPI_Info_get_nthkey(info_ptrs[i]->handle, j, key);
		    if (mpi_errno != MPI_SUCCESS) {
			MPIU_ERR_POP(mpi_errno);
		    }
		    mpi_errno = NMPI_Info_get_valuelen(info_ptrs[i]->handle, key, &vallen, &flag);
		    if (mpi_errno != MPI_SUCCESS) {
			MPIU_ERR_POP(mpi_errno);
		    }
		    if (!flag) {
			MPIU_ERR_POP(mpi_errno);
		    }

		    info_keyval_vectors[i][j].key = MPIU_Strdup(key);
		    info_keyval_vectors[i][j].val = MPIU_Malloc((vallen + 1)* sizeof(char));
		    mpi_errno = NMPI_Info_get(info_ptrs[i]->handle, key, vallen+1, info_keyval_vectors[i][j].val, &flag);
		    if (mpi_errno != MPI_SUCCESS) {
			MPIU_ERR_POP(mpi_errno);
		    }
		    if (!flag) {
			MPIU_ERR_POP(mpi_errno);
		    }
		    MPIU_DBG_PRINTF(("key: <%s>, value: <%s>\n", info_keyval_vectors[i][j].key, info_keyval_vectors[i][j].val));
		}
	    }
	    else
	    {
		info_keyval_sizes[i] = 0;
		info_keyval_vectors[i] = NULL;
	    }
	}

	/* create an array for the pmi error codes */
	total_num_processes = 0;
	for (i=0; i<count; i++)
	{
	    total_num_processes += maxprocs[i];
	}
	pmi_errcodes = (int*)MPIU_Malloc(sizeof(int) * total_num_processes);
	/* --BEGIN ERROR HANDLING-- */
	if (pmi_errcodes == NULL)
	{
	    free_pmi_keyvals(info_keyval_vectors, count, info_keyval_sizes);
	    MPIU_Free(info_keyval_sizes);
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	    goto fn_exit;
	}
	/* --END ERROR HANDLING-- */

	/* Open a port for the spawned processes to connect to */
        mpi_errno = MPID_Open_port(NULL, port_name);
	/* --BEGIN ERROR HANDLING-- */
        if (mpi_errno != MPI_SUCCESS)
	{
	    free_pmi_keyvals(info_keyval_vectors, count, info_keyval_sizes);
	    MPIU_Free(info_keyval_sizes);
	    MPIU_Free(pmi_errcodes);
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	    goto fn_exit;
	}
	/* --END ERROR HANDLING-- */

        preput_keyval_vector.key = "PARENT_ROOT_PORT_NAME";
        preput_keyval_vector.val = port_name;

	/* Spawn the processes */
        mpi_errno = PMI_Spawn_multiple(count, (const char **)
                                       commands, 
                                       (const char ***) argvs,
                                       maxprocs, info_keyval_sizes,
                                       (const PMI_keyval_t **)
                                       info_keyval_vectors, 1, 
                                       &preput_keyval_vector,
                                       pmi_errcodes);
	/* --BEGIN ERROR HANDLING-- */
        if (mpi_errno != PMI_SUCCESS)
        {
	    free_pmi_keyvals(info_keyval_vectors, count, info_keyval_sizes);
	    MPIU_Free(info_keyval_sizes);
	    MPIU_Free(pmi_errcodes);
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_spawn_multiple", "**pmi_spawn_multiple %d", mpi_errno);
            goto fn_exit;
        }
	/* --END ERROR HANDLING-- */
	if (errcodes != MPI_ERRCODES_IGNORE)
	{ 
	    for (i=0; i<total_num_processes; i++)
	    {
		/* FIXME: translate the pmi error codes here */
		errcodes[i] = pmi_errcodes[i];
	    }
	}
	
	free_pmi_keyvals(info_keyval_vectors, count, info_keyval_sizes);
        MPIU_Free(info_keyval_sizes);
	MPIU_Free(pmi_errcodes);
    }

    mpi_errno = MPID_Comm_accept(port_name, NULL, root, comm_ptr, intercomm); 
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }

 fn_exit:
    MPIR_Nest_decr();
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_COMM_SPAWN_MULTIPLE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#endif
