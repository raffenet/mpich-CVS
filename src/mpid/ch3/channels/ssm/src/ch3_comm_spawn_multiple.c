/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"
 
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

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_COMM_SPAWN_MULTIPLE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_COMM_SPAWN_MULTIPLE);
    if (comm_ptr->rank == root) {
        info_keyval_sizes = (int *) MPIU_Malloc(count * sizeof(int));
        /* TEMPORARILY set all user-provided info to NULL. PMI is not
           using it anyway. */
        for (i=0; i<count; i++) info_keyval_sizes[i] = 0;
        info_keyval_vectors = NULL;

        mpi_errno = MPIDI_CH3_Open_port(port_name);
        if (mpi_errno != MPI_SUCCESS) goto fn_exit;
        
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

        if (mpi_errno != 0)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_spawn_multiple", "**pmi_spawn_multiple %d", mpi_errno);
            goto fn_exit;
        }

        MPIU_Free(info_keyval_sizes);
    }

    mpi_errno = MPIDI_CH3_Comm_accept(port_name, root, comm_ptr, intercomm); 

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_COMM_SPAWN_MULTIPLE);
    return mpi_errno;
}




#ifdef OOOLD
int MPIDI_CH3_Comm_spawn(const char *command, const char *argv[],
                         const int maxprocs, MPI_Info info, const int root,
                         MPID_Comm *comm, MPID_Comm *intercomm,
                         int array_of_errcodes[])
{
    int mpi_errno = MPI_SUCCESS;
    char *kvsname;
    int kvsnamelen;
    MPIDI_CH3I_Process_group_t * pg;
    MPIDI_VC * vc_table;
    int p, rc, i;
    char *key, *val;
    int key_max_sz, val_max_sz;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_COMM_SPAWN);
    
    MPIDI_DBG_PRINTF((10, FCNAME, "entering"));
    /* printf( "before call to PMI_Spawn, maxprocs=%d, intercomm=0x%x, comm=0x%x\n",
       maxprocs, intercomm, comm ); fflush(stdout); */

    kvsnamelen = PMI_KVS_Get_name_length_max();
    kvsname = MPIU_Malloc(kvsnamelen);
    assert(kvsname != NULL);

    if (comm->rank == root) {

        /* send context_id and comm_size to the children using PMI */  
        key_max_sz = PMI_KVS_Get_key_length_max();
        key = MPIU_Malloc(key_max_sz);
        assert(key != NULL);
        val_max_sz = PMI_KVS_Get_value_length_max();
        val = MPIU_Malloc(val_max_sz);
        assert(val != NULL);
        
        rc = PMI_KVS_Get_my_name(kvsname);
        assert(rc == 0);

        rc = MPIU_Snprintf(key, key_max_sz, "Intercomm-context-id");
        assert(rc > -1 && rc < key_max_sz);
        rc = MPIU_Snprintf(val, val_max_sz, "%d", intercomm->context_id);
        assert(rc > -1 && rc < key_max_sz);
        rc = PMI_KVS_Put(kvsname, key, val);
        assert(rc == 0);

#ifdef DEBUG
        MPIU_dbg_printf("Parent: kvs %s, key %s, val %s\n", kvsname, key, val);
        fflush(stdout);
#endif
        rc = MPIU_Snprintf(key, key_max_sz, "Comm-size");
        assert(rc > -1 && rc < key_max_sz);
        rc = MPIU_Snprintf(val, val_max_sz, "%d", comm->local_size);
        assert(rc > -1 && rc < key_max_sz);
        rc = PMI_KVS_Put(kvsname, key, val);
        assert(rc == 0);
        
        rc = PMI_KVS_Commit(kvsname);
        assert(rc == 0);

        /* get the business cards of other processes into the root's
           cache so that they get sent over to the children. Temporary hack. */

        for (i=0; i<comm->local_size; i++) {
            if (i != root) {
                rc = MPIU_Snprintf(key, key_max_sz, "P%d-businesscard", i);
                assert(rc > -1 && rc < key_max_sz);
                rc = PMI_KVS_Get(kvsname, key, val);
                rc = PMI_KVS_Put(kvsname, key, val);
                assert(rc == 0);
            }
        }

        MPIU_Free(val);
        MPIU_Free(key);

/*	rc = PMI_Spawn(command, argv, maxprocs, kvsname, kvsnamelen );
 */

/* Note: The use of PMI_Spawn_multiple in this way is because it is
   currently implemented that way in MPD. It is only temporary. */

/* Commented out temporarily */
/*        rc = PMI_Spawn_multiple(1, &command, &argv, &maxprocs,
                                (const void *) kvsname, array_of_errcodes,
                                &same_domain, (const void *) kvsnamelen);
	assert(rc == 0);
*/
        rc = NMPI_Bcast(kvsname, kvsnamelen, MPI_CHAR, root, comm->handle);
        assert(rc == 0);

        /* since maxprocs may be provided only on the root, we need to
           bcast that as well */
        rc = NMPI_Bcast((int *) &maxprocs, 1, MPI_INT, root, comm->handle);
        assert(rc == 0);
    }
    else {
	/* get some information as needed from root */
        rc = NMPI_Bcast(kvsname, kvsnamelen, MPI_CHAR, root, comm->handle);
        assert(rc == 0);
        rc = NMPI_Bcast((int *) &maxprocs, 1, MPI_INT, root, comm->handle);
        assert(rc == 0);

        key_max_sz = PMI_KVS_Get_key_length_max();
        key = MPIU_Malloc(key_max_sz);
        assert(key != NULL);
        val_max_sz = PMI_KVS_Get_value_length_max();
        val = MPIU_Malloc(val_max_sz);
        assert(val != NULL);

        for (i=0; i<maxprocs; i++) {
            rc = MPIU_Snprintf(key, key_max_sz, "P%d-businesscard", i);
            assert(rc > -1 && rc < key_max_sz);
            rc = PMI_KVS_Get(kvsname, key, val);
            assert(rc == 0);
#ifdef DEBUG
            MPIU_dbg_printf("Parent: child rank %d b card %s\n", i, val);
            fflush(stdout);
#endif
        }
        MPIU_Free(key);
        MPIU_Free(val);

    } 

#ifdef DEBUG
    MPIU_dbg_printf("spawned kvsname %s\n", kvsname);
    fflush(stdout);
#endif

    /* printf( "after call to PMI_Spawn, maxprocs=%d, intercomm=0x%x, comm=0x%x\n",
       maxprocs, intercomm, comm ); fflush(stdout); */
    
    /* Fill in new intercomm */
    intercomm->attributes   = NULL;
    intercomm->remote_size  = maxprocs;
    intercomm->local_size   = comm->local_size;
    intercomm->rank         = comm->rank;
    intercomm->local_group  = NULL;
    intercomm->remote_group = NULL;
    intercomm->comm_kind    = MPID_INTERCOMM;
    intercomm->local_comm   = NULL;
    intercomm->is_low_group = 0;
    intercomm->coll_fns     = NULL;

    /* Point local vcr, vcrt at those of incoming intracommunicator */
    intercomm->local_vcrt = comm->vcrt;
    MPID_VCRT_Add_ref(comm->vcrt);
    intercomm->local_vcr  = comm->vcr;

    /* Allocate process group data structure for remote group and populate */
    pg = MPIU_Malloc(sizeof(MPIDI_CH3I_Process_group_t));
    assert(pg != NULL);
    pg->size = maxprocs;
    pg->kvs_name = MPIU_Malloc(kvsnamelen);
    assert(pg->kvs_name != NULL);
    MPIU_Strncpy(pg->kvs_name, kvsname, kvsnamelen);
    pg->ref_count = 0;  

    /* Allocate and initialize the VC table associated with the remote group */
    vc_table = MPIU_Malloc(sizeof(MPIDI_VC) * pg->size);
    assert(vc_table != NULL);
    pg->ref_count += pg->size;
    for (p = 0; p < pg->size; p++)
    {
	MPIDI_CH3U_VC_init(&vc_table[p], p);
	vc_table[p].sc.pg = pg;
	vc_table[p].sc.pg_rank = p;
	vc_table[p].sc.sendq_head = NULL;
	vc_table[p].sc.sendq_tail = NULL;
	vc_table[p].sc.state = MPIDI_CH3I_VC_STATE_UNCONNECTED;
	vc_table[p].sc.sock = SOCK_INVALID_SOCK;
	vc_table[p].sc.conn = NULL;
    }
    pg->vc_table = vc_table;

    /* Set up VC reference table */
    rc = MPID_VCRT_Create(intercomm->remote_size, &intercomm->vcrt);
    assert(rc == MPI_SUCCESS);
    rc = MPID_VCRT_Get_ptr(intercomm->vcrt, &intercomm->vcr);
    assert(rc == MPI_SUCCESS);
    for (p = 0; p < pg->size; p++) {
	MPID_VCR_Dup(&vc_table[p], &intercomm->vcr[p]);
    }

    MPIU_Free(kvsname);

    /*  WHERE DO ALL THE MALLOCS IN THIS FILE GET FREED? */

    /* MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_COMM_SPAWN); */
    MPIDI_DBG_PRINTF((10, FCNAME, "exiting"));
    return mpi_errno;
}

#endif
