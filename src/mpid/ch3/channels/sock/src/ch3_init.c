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


MPIDI_CH3I_Process_t MPIDI_CH3I_Process;
/* int MPIDI_CH3I_Port_name_tag=0; brad : now in upcall for all channels using sockets */

static int MPIDI_CH3I_PG_Compare_ids(void * id1, void * id2);
static int MPIDI_CH3I_PG_Destroy(MPIDI_PG_t * pg, void * id);


/*
 *  MPIDI_CH3_Init  - makes socket specific initializations.  Most of this functionality
 *                      is in the MPIDI_CH3U_Init_sock upcall because the same tasks need
 *                      to be done for the ssh (sock + shm) channel.  As a result, it
 *                      must except a lot more arguements.
 */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Init(int * has_args, int * has_env, int * has_parent, MPIDI_PG_t ** pg_p, int * pg_rank_p,
                    char **publish_bc_p, char **bc_key_p, char **bc_val_p, int *val_max_sz_p)
{
    /* brad : none of these are used any more.  generic PMI code in mpid_init.c and also upcalls
    *           to ch3u_init_sock.c accomplish the same thing
    */
#if 0
    MPIDI_PG_t * pg = NULL;
    int pg_rank;
    int pg_size;
    char * pg_id = NULL;
    int pg_id_sz;
    int kvs_name_sz;
    int key_max_sz;
    int val_max_sz;
    int appnum;
    char * key = NULL;
    char * val = NULL;
    int p;
    int pmi_errno = PMI_SUCCESS;
#endif
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_CH3_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_CH3_INIT);

    /* brad : these have moved into MPIDI_CH3I_PMI_Init now */
#if 0
    MPIDI_CH3I_Process.parent_port_name = NULL;
    MPIDI_CH3I_Process.acceptq_head = NULL;
#   if (USE_THREAD_IMPL == MPICH_THREAD_IMPL_NOT_IMPLEMENTED)
    {
	MPID_Thread_lock_init(&MPIDI_CH3I_Process.acceptq_mutex);
    }
#   endif
#endif

    /* initialize aspects specific to sockets  */
    mpi_errno = MPIDI_CH3U_Init_sock(has_args, has_env, has_parent, pg_p, pg_rank_p,
                               publish_bc_p, bc_key_p, bc_val_p, val_max_sz_p);
    if (mpi_errno != MPI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }    

    /* brad : this code is all independent and now in MPIDI_CH3I_PMI_Init.  the upcall
     *         above gets the code specific to sockets (ssm uses it too).
     */    
#if 0
    /*
     * Intial the process manangement interface (PMI), and get rank and size information about our process group
     */
    pmi_errno = PMI_Init(has_parent);
    if (pmi_errno != PMI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_init",
					 "**pmi_init %d", pmi_errno);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
    
    pmi_errno = PMI_Get_rank(&pg_rank);
    if (pmi_errno != PMI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_rank",
					 "**pmi_get_rank %d", pmi_errno);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
    
    pmi_errno = PMI_Get_size(&pg_size);
    if (pmi_errno != 0)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_size",
					 "**pmi_get_size %d", pmi_errno);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }

    pmi_errno = PMI_Get_appnum(&appnum);
    if (pmi_errno != PMI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_appnum",
					 "**pmi_get_appnum %d", pmi_errno);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
    if (appnum != -1)
    {
	MPIR_Process.attrs.appnum = appnum;
    }
    
    /*
     * Get the process group id
     */
    pmi_errno = PMI_Get_id_length_max(&pg_id_sz);
    if (pmi_errno != PMI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**pmi_get_id_length_max", "**pmi_get_id_length_max %d", pmi_errno);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }

    pg_id = MPIU_Malloc(pg_id_sz + 1);
    if (pg_id == NULL)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", NULL);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
    
    pmi_errno = PMI_Get_id(pg_id, pg_id_sz);
    if (pmi_errno != PMI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_id",
					 "**pmi_get_id %d", pmi_errno);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }


    /*
     * Initialize the process group tracking subsystem
     */
    mpi_errno = MPIDI_PG_Init(MPIDI_CH3I_PG_Compare_ids, MPIDI_CH3I_PG_Destroy);
    if (mpi_errno != MPI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**dev|pg_init", NULL);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
    
    /*
     * Create a new structure to track the process group
     */
    mpi_errno = MPIDI_PG_Create(pg_size, pg_id, &pg);
    if (mpi_errno != MPI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**dev|pg_create", NULL);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
    pg->ch.kvs_name = NULL;
    
    /*
     * Get the name of the key-value space (KVS)
     */
    pmi_errno = PMI_KVS_Get_name_length_max(&kvs_name_sz);
    if (pmi_errno != PMI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**pmi_kvs_get_name_length_max", "**pmi_kvs_get_name_length_max %d", pmi_errno);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
    
    pg->ch.kvs_name = MPIU_Malloc(kvs_name_sz + 1);
    if (pg->ch.kvs_name == NULL)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", NULL);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
    
    pmi_errno = PMI_KVS_Get_my_name(pg->ch.kvs_name, kvs_name_sz);
    if (pmi_errno != PMI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**pmi_kvs_get_my_name", "**pmi_kvs_get_my_name %d", pmi_errno);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }

    /*   brad : the following is now taken care of in MPIDI_CH3U_Init_sock although
     *           in time, this VC for-loop executes AFTER the code still in this method...
     *           question : does MPIDI_CH3I_Progress_init() depend on these fields?
     */

    /*
     * Initialize the VCs associated with this process group (and thus MPI_COMM_WORLD)
     */

    for (p = 0; p < pg_size; p++)
    {
	pg->vct[p].ch.sendq_head = NULL;
	pg->vct[p].ch.sendq_tail = NULL;
	pg->vct[p].ch.state = MPIDI_CH3I_VC_STATE_UNCONNECTED;
	pg->vct[p].ch.sock = MPIDU_SOCK_INVALID_SOCK;
	pg->vct[p].ch.conn = NULL;
    }

    /*
     *  brad : the following is now taken care of in MPIDI_CH3_PMI_Init
     */

    
    /*
     * Initialize Progress Engine.  This must occur before the business card is requested because part of progress engine
     * initialization is setting up the listener socket.  The port of the listener socket needs to be included in the business
     * card.
     */
    mpi_errno = MPIDI_CH3I_Progress_init();
    if (mpi_errno != MPI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**init_progress", NULL);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
    
    /*
     * Publish the contact information (a.k.a. business card) for this process into the PMI keyval space associated with this
     * process group.
     */
    pmi_errno = PMI_KVS_Get_key_length_max(&key_max_sz);
    if (pmi_errno != PMI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**pmi_kvs_get_key_length_max", "**pmi_kvs_get_key_length_max %d", pmi_errno);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
    
    key = MPIU_Malloc(key_max_sz);
    if (key == NULL)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", NULL);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
    
    pmi_errno = PMI_KVS_Get_value_length_max(&val_max_sz);
    if (pmi_errno != PMI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**pmi_kvs_get_value_length_max", "**pmi_kvs_get_value_length_max %d", pmi_errno);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
    
    val = MPIU_Malloc(val_max_sz);
    if (val == NULL)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", NULL);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }

    mpi_errno = MPIU_Snprintf(key, key_max_sz, "P%d-businesscard", pg_rank);
    if (mpi_errno < 0 || mpi_errno > key_max_sz)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**snprintf",
					 "**snprintf %d", mpi_errno);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }

    /* brad :  the following code has migrated to MPIDI_CH3U_Init_sock */

    /*  TODO brad : must implement this the same in ssm as in sock.  where should the code go? upcalls
     *        to mpid/src/ch3u_get_business_card.c perhaps? */
    mpi_errno = MPIDI_CH3I_Get_business_card(val, val_max_sz);
    if (mpi_errno != MPI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_buscard", NULL);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }

    pmi_errno = PMI_KVS_Put(pg->ch.kvs_name, key, val);
    if (pmi_errno != PMI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_put",
					 "**pmi_kvs_put %d", pmi_errno);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }

#   if defined(MPICH_DBG_OUTPUT)
    {
	MPIU_dbg_printf("[%d] Business card: <%s>\n", pg_rank, val);
	fflush(stdout);
    }
#   endif

    pmi_errno = PMI_KVS_Commit(pg->ch.kvs_name);
    if (pmi_errno != PMI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_commit",
					 "**pmi_kvs_commit %d", pmi_errno);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }

    pmi_errno = PMI_Barrier();
    if (pmi_errno != PMI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_barrier",
					 "**pmi_barrier %d", pmi_errno);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }

    /* brad : if was here originally */
#if 0
    {
	for (p = 0; p < pg_size; p++)
	{
	    mpi_errno = MPIU_Snprintf(key, key_max_sz, "P%d-businesscard", p);
	    if (mpi_errno < 0 || mpi_errno >= key_max_sz)
	    {
		/* --BEGIN ERROR HANDLING-- */
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
						 "**nomem", NULL);
		goto fn_fail;
		/* --END ERROR HANDLING-- */
	    }
	    pmi_errno = PMI_KVS_Get(pg->ch.kvs_name, key, val, val_max_sz);
	    if (pmi_errno != PMI_SUCCESS)
	    {
		/* --BEGIN ERROR HANDLING-- */
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
						 "**pmi_kvs_get", "**pmi_kvs_get %d", pmi_errno);
		goto fn_fail;
		/* --END ERROR HANDLING-- */
	    }

	    MPIU_dbg_printf("[%d] businesscard=%s\n", pg_rank, val);
	}
    }
    /* brad : endif here originally */
#endif

    /* brad : the following was here previously */
    /* FIXME: has_args and has_env need to come from PMI eventually... */
    *has_args = TRUE;
    *has_env = TRUE;

    *pg_p = pg;
    *pg_rank_p = pg_rank;

    /* brad : a close to my if 0 */
#endif
  fn_exit:

    /* brad : these are handled (or their equivalent bc_*) in MPID_Init now */
#if 0
    if (val != NULL)
    { 
	MPIU_Free(val);
    }
    if (key != NULL)
    { 
	MPIU_Free(key);
    }
#endif
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_CH3_INIT);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    /* brad : this is handled in MPIDI_CH3I_Init_sock now */
#if 0    
    if (pg != NULL)
    {
	/* MPIDI_CH3I_PG_Destroy(), which is called by MPIDI_PG_Destroy(), frees pg->ch.kvs_name */
	MPIDI_PG_Destroy(pg);
    }
#endif

    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/* these were identical in each channel so were moved up to mpid_init.c */
#if 0
static int MPIDI_CH3I_PG_Compare_ids(void * id1, void * id2)
{
    return (strcmp((char *) id1, (char *) id2) == 0) ? TRUE : FALSE;
}


static int MPIDI_CH3I_PG_Destroy(MPIDI_PG_t * pg, void * id)
{
    if (pg->ch.kvs_name != NULL)
    {
	MPIU_Free(pg->ch.kvs_name);
    }

    if (id != NULL)
    { 
	MPIU_Free(id);
    }
    
    return MPI_SUCCESS;
}
#endif
