/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

MPIDI_CH3I_Process_t MPIDI_CH3I_Process;

/* XXX - all calls to assert() need to be turned into real error checking and
   return meaningful errors */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Init(int * has_args, int * has_env, int * has_parent)
{
    int mpi_errno;
    MPIDI_CH3I_Process_group_t * pg;
    int pg_rank;
    int pg_size;
    MPIDI_VC * vc_table;
    MPID_Comm * comm, *intercomm, *commworld;
    int p;
    char * key;
    char * val;
    int key_max_sz;
    int val_max_sz;
    
    MPIDI_CH3I_Process.acceptq_head = NULL;
    MPIDI_CH3I_Process.acceptq_tail = NULL;
    MPID_Thread_lock_init(&MPIDI_CH3I_Process.acceptq_mutex);

    /*
     * Extract process group related information from PMI and initialize structures that track the process group connections,
     * MPI_COMM_WORLD, and MPI_COMM_SELF
     */
    mpi_errno = PMI_Init(has_parent);
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_init", "**pmi_init %d", mpi_errno);
	return mpi_errno;
    }
    mpi_errno = PMI_Get_rank(&pg_rank);
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_rank", "**pmi_get_rank %d", mpi_errno);
	return mpi_errno;
    }
    mpi_errno = PMI_Get_size(&pg_size);
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_size", "**pmi_get_size %d", mpi_errno);
	return mpi_errno;
    }

    /* Allocate process group data structure and populate */
    pg = MPIU_Malloc(sizeof(MPIDI_CH3I_Process_group_t));
    if (pg == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }
    pg->size = pg_size;
    pg->kvs_name = MPIU_Malloc(PMI_KVS_Get_name_length_max() + 1);
    if (pg->kvs_name == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }
    mpi_errno = PMI_KVS_Get_my_name(pg->kvs_name);
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get_my_name", "**pmi_kvs_get_my_name %d", mpi_errno);
	return mpi_errno;
    }
    pg->ref_count = 1;
    MPIDI_CH3I_Process.pg = pg;
    
    /* Allocate and initialize the VC table associated with this process group (and thus COMM_WORLD) */
    vc_table = MPIU_Malloc(sizeof(MPIDI_VC) * pg_size);
    if (vc_table == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }
    pg->ref_count += pg_size;
    for (p = 0; p < pg_size; p++)
    {
	MPIDI_CH3U_VC_init(&vc_table[p], p);
	vc_table[p].sc.pg = pg;
	vc_table[p].sc.pg_rank = p;
	vc_table[p].sc.sendq_head = NULL;
	vc_table[p].sc.sendq_tail = NULL;
	vc_table[p].sc.state = MPIDI_CH3I_VC_STATE_UNCONNECTED;
	vc_table[p].sc.sock = MPIDU_SOCK_INVALID_SOCK;
	vc_table[p].sc.conn = NULL;
    }
    pg->vc_table = vc_table;
    
    /* Initialize MPI_COMM_WORLD object */
    comm = MPIR_Process.comm_world;

    comm->rank = pg_rank;
    comm->remote_size = comm->local_size = pg_size;
    mpi_errno = MPID_VCRT_Create(comm->remote_size, &comm->vcrt);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_vcrt", 0);
	return mpi_errno;
    }
    mpi_errno = MPID_VCRT_Get_ptr(comm->vcrt, &comm->vcr);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_getptr", 0);
	return mpi_errno;
    }
    for (p = 0; p < pg_size; p++)
    {
	MPID_VCR_Dup(&vc_table[p], &comm->vcr[p]);
    }
    
    /* Initialize MPI_COMM_SELF object */
    comm = MPIR_Process.comm_self;
    comm->rank = 0;
    comm->remote_size = comm->local_size = 1;
    mpi_errno = MPID_VCRT_Create(comm->remote_size, &comm->vcrt);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_vcrt", 0);
	return mpi_errno;
    }
    mpi_errno = MPID_VCRT_Get_ptr(comm->vcrt, &comm->vcr);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_getptr", 0);
	return mpi_errno;
    }
    MPID_VCR_Dup(&vc_table[pg_rank], &comm->vcr[0]);

    /*
     * Initialize Progress Engine (and setup listener socket)
     */
    mpi_errno = MPIDI_CH3I_Progress_init();
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_progress", 0);
	return mpi_errno;
    }
    
    /*
     * Publish the contact info for this process in the PMI keyval space
     */
    key_max_sz = PMI_KVS_Get_key_length_max();
    key = MPIU_Malloc(key_max_sz);
    if (key == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }
    val_max_sz = PMI_KVS_Get_value_length_max();
    val = MPIU_Malloc(val_max_sz);
    if (val == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }

    mpi_errno = MPIU_Snprintf(key, key_max_sz, "P%d-businesscard", pg_rank);
    if (mpi_errno < 0 || mpi_errno > key_max_sz)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**snprintf", "**snprintf %d", mpi_errno);
	return mpi_errno;
    }
    mpi_errno = MPIDI_CH3I_Get_business_card(val, val_max_sz, pg);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_buscard", 0);
	return mpi_errno;
    }
    mpi_errno = PMI_KVS_Put(pg->kvs_name, key, val);
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", mpi_errno);
	return mpi_errno;
    }

#   if defined(DEBUG)
    {
	/*dbg_printf("[%d] Published hostname=%s port=%d\n", pg_rank, hostname, port);*/
	MPIU_dbg_printf("[%d] Business card: <%s>\n", pg_rank, val);
	fflush(stdout);
    }
#   endif

    mpi_errno = PMI_KVS_Commit(pg->kvs_name);
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_commit", "**pmi_kvs_commit %d", mpi_errno);
	return mpi_errno;
    }

    mpi_errno = PMI_Barrier();
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", mpi_errno);
	return mpi_errno;
    }

#   if defined(DEBUG)
    {
	for (p = 0; p < pg_size; p++)
	{
	    mpi_errno = MPIU_Snprintf(key, key_max_sz, "P%d-businesscard", p);
	    assert(mpi_errno > -1 && mpi_errno < key_max_sz);
	    mpi_errno = PMI_KVS_Get(pg->kvs_name, key, val);
	    assert(mpi_errno == 0);

	    MPIU_dbg_printf("[%d] businesscard=%s\n", pg_rank, val);
	    fflush(stdout);
	}
    }
#   endif
    
    /* XXX - has_args and has_env need to come from PMI eventually... */
    *has_args = TRUE;
    *has_env = TRUE;

    if (*has_parent) {
        /* This process was spawned. Create intercommunicator with parents. */

        if (pg_rank == 0) {
            /* get the port name of the root of the parents */
            mpi_errno = PMI_KVS_Get(pg->kvs_name, "PARENT_ROOT_PORT_NAME", val);
            if (mpi_errno != 0)
            {
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get %d", mpi_errno);
                return mpi_errno;
            }
        }

        /* do a connect with the root */
        MPID_Comm_get_ptr(MPI_COMM_WORLD, commworld);
        mpi_errno = MPIDI_CH3_Comm_connect(val, 0, commworld, &intercomm);
        if (mpi_errno != MPI_SUCCESS) return mpi_errno;

        MPIR_Process.comm_parent = intercomm;

        /* TODO: Check that this intercommunicator gets freed in
           MPI_Finalize if not already freed.  */
    }

    MPIU_Free(val);
    MPIU_Free(key);

    return MPI_SUCCESS;
}




#ifdef OOOLD
int MPIDI_CH3_Init(int * has_args, int * has_env, int * has_parent)
{
    int mpi_errno;
    MPIDI_CH3I_Process_group_t * pg;
    
    int pg_rank;
    int pg_size;
    MPIDI_VC * vc_table;
    MPID_Comm * comm, *commworld, *intercomm;
    int p;
#ifdef FOO
    int i;
#endif
	
    char * key;
    char * val;
    int key_max_sz;
    int val_max_sz;
    char *parent_kvsname;
    
    /*
     * Extract process group related information from PMI and initialize structures that track the process group connections,
     * MPI_COMM_WORLD, and MPI_COMM_SELF
     */
    mpi_errno = PMI_Init(has_parent);
#ifdef DEBUG
    MPIU_dbg_printf("HAS PARENT %d\n", *has_parent);
    fflush(stdout);
#endif
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_init", "**pmi_init %d", mpi_errno);
	return mpi_errno;
    }
    mpi_errno = PMI_Get_rank(&pg_rank);
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_rank", "**pmi_get_rank %d", mpi_errno);
	return mpi_errno;
    }
    mpi_errno = PMI_Get_size(&pg_size);
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_size", "**pmi_get_size %d", mpi_errno);
	return mpi_errno;
    }

    /* Allocate process group data structure and populate */
    pg = MPIU_Malloc(sizeof(MPIDI_CH3I_Process_group_t));
    if (pg == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }
    pg->size = pg_size;
    pg->kvs_name = MPIU_Malloc(PMI_KVS_Get_name_length_max() + 1);
    if (pg->kvs_name == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }
    mpi_errno = PMI_KVS_Get_my_name(pg->kvs_name);
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get_my_name", "**pmi_kvs_get_my_name %d", mpi_errno);
	return mpi_errno;
    }
    pg->ref_count = 1;
    MPIDI_CH3I_Process.pg = pg;
    
    /* Allocate and initialize the VC table associated with this process group (and thus COMM_WORLD) */
    vc_table = MPIU_Malloc(sizeof(MPIDI_VC) * pg_size);
    if (vc_table == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }
    pg->ref_count += pg_size;
    for (p = 0; p < pg_size; p++)
    {
	MPIDI_CH3U_VC_init(&vc_table[p], p);
	vc_table[p].sc.pg = pg;
	vc_table[p].sc.pg_rank = p;
	vc_table[p].sc.sendq_head = NULL;
	vc_table[p].sc.sendq_tail = NULL;
	vc_table[p].sc.state = MPIDI_CH3I_VC_STATE_UNCONNECTED;
	vc_table[p].sc.sock = MPIDU_SOCK_INVALID_SOCK;
	vc_table[p].sc.conn = NULL;
    }
    pg->vc_table = vc_table;
    
    /* Initialize MPI_COMM_WORLD object */
    comm = MPIR_Process.comm_world;
    comm->rank = pg_rank;
    comm->remote_size = comm->local_size = pg_size;
    mpi_errno = MPID_VCRT_Create(comm->remote_size, &comm->vcrt);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_vcrt", 0);
	return mpi_errno;
    }
    mpi_errno = MPID_VCRT_Get_ptr(comm->vcrt, &comm->vcr);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_getptr", 0);
	return mpi_errno;
    }
    for (p = 0; p < pg_size; p++)
    {
	MPID_VCR_Dup(&vc_table[p], &comm->vcr[p]);
    }
    
    /* Initialize MPI_COMM_SELF object */
    comm = MPIR_Process.comm_self;
    comm->rank = 0;
    comm->remote_size = comm->local_size = 1;
    mpi_errno = MPID_VCRT_Create(comm->remote_size, &comm->vcrt);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_vcrt", 0);
	return mpi_errno;
    }
    mpi_errno = MPID_VCRT_Get_ptr(comm->vcrt, &comm->vcr);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_getptr", 0);
	return mpi_errno;
    }
    MPID_VCR_Dup(&vc_table[pg_rank], &comm->vcr[0]);

    /*
     * Initialize Progress Engine (and setup listener socket)
     */
    mpi_errno = MPIDI_CH3I_Progress_init();
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_progress", 0);
	return mpi_errno;
    }
    
    /*
     * Publish the contact info for this process in the PMI keyval space
     */
    key_max_sz = PMI_KVS_Get_key_length_max();
    key = MPIU_Malloc(key_max_sz);
    if (key == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }
    val_max_sz = PMI_KVS_Get_value_length_max();
    val = MPIU_Malloc(val_max_sz);
    if (val == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }

    mpi_errno = MPIU_Snprintf(key, key_max_sz, "P%d-businesscard", pg_rank);
    if (mpi_errno < 0 || mpi_errno > key_max_sz)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**snprintf", "**snprintf %d", mpi_errno);
	return mpi_errno;
    }
    mpi_errno = MPIDI_CH3I_Get_business_card(val, val_max_sz);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_buscard", 0);
	return mpi_errno;
    }
    mpi_errno = PMI_KVS_Put(pg->kvs_name, key, val);
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", mpi_errno);
	return mpi_errno;
    }

#   if defined(DEBUG)
    {
	/*dbg_printf("[%d] Published hostname=%s port=%d\n", pg_rank, hostname, port);*/
	MPIU_dbg_printf("[%d] Business card: <%s>\n", pg_rank, val);
	fflush(stdout);
    }
#   endif

#ifdef FOO
    if (pg_rank == 0) {
        /* get the business cards of other processes into the root's
           cache so that they get sent over to the parents. Temporary hack. */
    
        for (i=1; i<pg_size; i++) {
            mpi_errno = MPIU_Snprintf(key, key_max_sz, "P%d-businesscard", i);
            assert(mpi_errno > -1 && mpi_errno < key_max_sz);
            mpi_errno = -1; 
            while (mpi_errno != 0) {
                mpi_errno = PMI_KVS_Get(pg->kvs_name, key, val);
                usleep(1000);
            }
            mpi_errno = PMI_KVS_Put(pg->kvs_name, key, val);
            MPIU_dbg_printf("Child: rank %d b card %s\n", i, val);
            fflush(stdout);
        }
    }
#endif

    mpi_errno = PMI_KVS_Commit(pg->kvs_name);
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_commit", "**pmi_kvs_commit %d", mpi_errno);
	return mpi_errno;
    }

    mpi_errno = PMI_Barrier();
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", mpi_errno);
	return mpi_errno;
    }

#   if defined(DEBUG)
    {
	for (p = 0; p < pg_size; p++)
	{
	    mpi_errno = MPIU_Snprintf(key, key_max_sz, "P%d-businesscard", p);
	    assert(mpi_errno > -1 && mpi_errno < key_max_sz);
	    mpi_errno = PMI_KVS_Get(pg->kvs_name, key, val);
	    assert(mpi_errno == 0);

	    MPIU_dbg_printf("[%d] businesscard=%s\n", pg_rank, val);
	    fflush(stdout);
	}
    }
#   endif
    
    /* XXX - has_args and has_env need to come from PMI eventually... */
    *has_args = TRUE;
    *has_env = TRUE;

    if (*has_parent) {
        /* This process was spawned. Create intercommunicator with parents. */

        parent_kvsname = MPIU_Malloc(val_max_sz);
        assert(parent_kvsname != NULL);
        
        mpi_errno = MPIU_Snprintf(key, key_max_sz, "PMI_PARENT_KVSNAME");
	if (mpi_errno < 0 || mpi_errno > key_max_sz)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**snprintf", "**snprintf %d", mpi_errno);
	    return mpi_errno;
	}
        mpi_errno = PMI_KVS_Get(pg->kvs_name, key, parent_kvsname);
	if (mpi_errno != 0)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get %d", mpi_errno);
	    return mpi_errno;
	}

        MPID_Comm_get_ptr( MPI_COMM_WORLD, commworld );
        mpi_errno = MPIR_Comm_create( commworld, &intercomm );
	if (mpi_errno != 0)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_comm_create", "**init_comm_create %d", mpi_errno);
	    return mpi_errno;
	}
        MPIR_Process.comm_parent = intercomm;

        /* get the context_id, comm_size, and kvsname of parents */  
        
        mpi_errno = MPIU_Snprintf(key, key_max_sz, "Intercomm-context-id");
	if (mpi_errno < 0 || mpi_errno > key_max_sz)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**snprintf", "**snprintf %d", mpi_errno);
	    return mpi_errno;
	}
        mpi_errno = PMI_KVS_Get(parent_kvsname, key, val);
	if (mpi_errno != 0)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get %d", mpi_errno);
	    return mpi_errno;
	}
        intercomm->context_id = atoi(val);
#ifdef DEBUG
        MPIU_dbg_printf("child: context_id %d\n", atoi(val));
        fflush(stdout);
#endif
        mpi_errno = MPIU_Snprintf(key, key_max_sz, "Comm-size");
	if (mpi_errno < 0 || mpi_errno > key_max_sz)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**snprintf", "**snprintf %d", mpi_errno);
	    return mpi_errno;
	}
        mpi_errno = PMI_KVS_Get(parent_kvsname, key, val);
	if (mpi_errno != 0)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get %d", mpi_errno);
	    return mpi_errno;
	}
        intercomm->remote_size = atoi(val);
#ifdef DEBUG
        MPIU_dbg_printf("child: remote_size %d\n", atoi(val));
        fflush(stdout);
#endif
        /* Fill in new intercomm */
        intercomm->attributes   = NULL;
        intercomm->local_size   = pg_size;
        intercomm->rank         = pg_rank;
        intercomm->local_group  = NULL;
        intercomm->remote_group = NULL;
        intercomm->comm_kind    = MPID_INTERCOMM;
        intercomm->local_comm   = NULL;
        intercomm->is_low_group = 1;
        intercomm->coll_fns     = NULL;

        /* Point local vcr, vcrt at those of comm_world */
        intercomm->local_vcrt = commworld->vcrt;
        MPID_VCRT_Add_ref(commworld->vcrt);
        intercomm->local_vcr  = commworld->vcr;

        /* Allocate process group data structure for remote group and populate */
        pg = MPIU_Malloc(sizeof(MPIDI_CH3I_Process_group_t));
	if (pg == NULL)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	    return mpi_errno;
	}
        pg->size = intercomm->remote_size;
        pg->kvs_name = MPIU_Malloc(PMI_KVS_Get_name_length_max());
	if (pg->kvs_name == NULL)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	    return mpi_errno;
	}
        MPIU_Strncpy(pg->kvs_name, parent_kvsname, PMI_KVS_Get_name_length_max());
        pg->ref_count = 0;
        
        /* Allocate and initialize the VC table associated with the remote group */
        vc_table = MPIU_Malloc(sizeof(MPIDI_VC) * pg->size);
	if (vc_table == NULL)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	    return mpi_errno;
	}
        pg->ref_count += pg->size;
        for (p = 0; p < pg->size; p++)
        {
            MPIDI_CH3U_VC_init(&vc_table[p], p);
            vc_table[p].sc.pg = pg;
            vc_table[p].sc.pg_rank = p;
            vc_table[p].sc.sendq_head = NULL;
            vc_table[p].sc.sendq_tail = NULL;
            vc_table[p].sc.state = MPIDI_CH3I_VC_STATE_UNCONNECTED;
            vc_table[p].sc.sock = MPIDU_SOCK_INVALID_SOCK;
            vc_table[p].sc.conn = NULL;
        }
        pg->vc_table = vc_table;
        
        /* Set up VC reference table */
        mpi_errno = MPID_VCRT_Create(intercomm->remote_size, &intercomm->vcrt);
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_vcrt", 0);
	    return mpi_errno;
	}
        mpi_errno = MPID_VCRT_Get_ptr(intercomm->vcrt, &intercomm->vcr);
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_getptr", 0);
	    return mpi_errno;
	}
        for (p = 0; p < pg->size; p++) {
            MPID_VCR_Dup(&vc_table[p], &intercomm->vcr[p]);
        }

        MPIU_Free(parent_kvsname);
        
        /* WHERE DOES THE NEW INTERCOMM GET FREED? */
    }

    MPIU_Free(val);
    MPIU_Free(key);

    return MPI_SUCCESS;
}
#endif
