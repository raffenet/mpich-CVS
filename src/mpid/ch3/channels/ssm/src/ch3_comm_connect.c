/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"

/*
 * MPIDI_CH3_Comm_connect()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Comm_connect
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Comm_connect(char *port_name, int root, MPID_Comm *comm_ptr, MPID_Comm **newcomm)
{
#if 0
    int p, key_max_sz, val_max_sz, mpi_errno=MPI_SUCCESS;
    int i, bizcards_len, rank, kvs_namelen, recv_ints[3],
        send_ints[2], remote_pg_size, comm_size, context_id;
    int remote_comm_size=0;
    MPID_Comm *tmp_comm, *intercomm, *commself_ptr;
    char *remote_kvsname, *key, *val, *bizcards, *bizcard_ptr;
    MPIDI_CH3I_Process_group_t * remote_pg;
    MPIDI_VC *vc_table, *vc;
    int *local_pg_ranks, *remote_pg_ranks;
#else
    int mpi_errno = MPI_SUCCESS;
#endif

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_COMM_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_COMM_CONNECT);
#if 0
/* Algorithm: First create a connection (vc) between this root and the
   root on the accept side. Using this vc, create a temporary
   intercomm between the two roots. Use MPI functions to communicate
   the other information needed to create the real intercommunicator
   between the processes on the two sides. Then free the
   intercommunicator between the roots. */

    rank = comm_ptr->rank;

    /* need a new kvs to store the business cards of the processes
       on the other side. The root creates the kvs (later below) and
       broadcasts the name to the other processes in comm_ptr. */
    kvs_namelen = PMI_KVS_Get_name_length_max();
    remote_kvsname = (char *) MPIU_Malloc(kvs_namelen); 
    if (remote_kvsname == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	goto fn_exit;
    }

    /* Allocate new pg structure to store information about the remote group */
    remote_pg = MPIU_Malloc(sizeof(MPIDI_CH3I_Process_group_t));
    if (remote_pg == NULL)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        goto fn_exit;
    }
    /* FIXME - Where does this new pg get freed? */

    val_max_sz = PMI_KVS_Get_value_length_max();
    val = (char *) MPIU_Malloc(val_max_sz);
    if (val == NULL)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        goto fn_exit;
    }

    if (rank == root) {
        /* connect to the root on the other side and return a new
           vc. Function implemented in ch3_progress.c */
        MPIDI_CH3I_Connect_to_root(port_name, &vc);

        /* Use this vc to create a temporary intercommunicator
           between the two roots*/ 
        MPID_Comm_get_ptr( MPI_COMM_SELF, commself_ptr );
        mpi_errno = MPIR_Comm_create(commself_ptr, &tmp_comm);
        if (mpi_errno != MPI_SUCCESS) goto fn_exit;

        /* fill in all the fields of tmp_comm. */ 

        tmp_comm->context_id = 4999;  /* FIXME - we probably need a
                                         unique context_id. */  
        tmp_comm->remote_size = 1;
        
        /* Fill in new intercomm */
        tmp_comm->attributes   = NULL;
        tmp_comm->local_size   = 1;
        tmp_comm->rank         = 0;
        tmp_comm->local_group  = NULL;
        tmp_comm->remote_group = NULL;
        tmp_comm->comm_kind    = MPID_INTERCOMM;
        tmp_comm->local_comm   = NULL;
        tmp_comm->is_low_group = 1;
        tmp_comm->coll_fns     = NULL;
        
        /* No pg structure needed since vc has already been set up
           (connection has been established). */

        /* Point local vcr, vcrt at those of commself_ptr */
        tmp_comm->local_vcrt = commself_ptr->vcrt;
        MPID_VCRT_Add_ref(commself_ptr->vcrt);
        tmp_comm->local_vcr  = commself_ptr->vcr;
        
        /* Set up VC reference table */
        mpi_errno = MPID_VCRT_Create(tmp_comm->remote_size, &tmp_comm->vcrt);
        if (mpi_errno != MPI_SUCCESS)
        {
            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_vcrt", 0);
            goto fn_exit;
        }
        mpi_errno = MPID_VCRT_Get_ptr(tmp_comm->vcrt, &tmp_comm->vcr);
        if (mpi_errno != MPI_SUCCESS)
        {
            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_getptr", 0);
            goto fn_exit;
        }

        MPID_VCR_Dup(vc, tmp_comm->vcr);
        
        /* tmp_comm is now established; can communicate with the root on
           the other side. */

        /* First send the comm_size and pg_size to the root on the
           other side and receive the remote_comm_size,
           remote_pg_size, and context_id of the new communicator from
           that root. */ 

        comm_size = comm_ptr->local_size;
        send_ints[0] = comm_size;
        send_ints[1] = MPIDI_CH3I_Process.pg->size;

        mpi_errno = MPIC_Send(send_ints, 2, MPI_INT, 0, 100,
                              tmp_comm->handle);
        if (mpi_errno != MPI_SUCCESS) goto fn_exit;
        
        mpi_errno = MPIC_Recv(recv_ints, 3, MPI_INT, 0, 101,
                              tmp_comm->handle, MPI_STATUS_IGNORE);
        if (mpi_errno != MPI_SUCCESS) goto fn_exit;
        
        remote_comm_size = recv_ints[0];
        remote_pg_size = recv_ints[1];
        context_id = recv_ints[2];

        /* Create a new business card containing the address of the pg
           for the remote group. Receive the business cards thus
           created by all other processes on this side and then
           forward them to the root on the remote side. If we knew the
           sizes of all the business cards, we could have used
           MPI_Gather instead of a loop of MPI_Recvs. */

        bizcards = (char *) MPIU_Malloc(comm_size * val_max_sz);
        if (bizcards == NULL)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
            goto fn_exit;
        }
        
        /* get the business cards into val and store them compactly in
           bizcards for communication */
        bizcards_len = 0;
        bizcard_ptr = bizcards;
        for (i=0; i<comm_size; i++) {
            if (i == root) {
                mpi_errno = MPIDI_CH3I_Get_business_card(val, val_max_sz, remote_pg);
                if (mpi_errno != MPI_SUCCESS)
                {
                    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_buscard", 0);
                    goto fn_exit;
                }
            }
            else {
                mpi_errno = MPIC_Recv(val, val_max_sz, MPI_CHAR, i, 52,
                                      comm_ptr->handle, MPI_STATUS_IGNORE); 
                if (mpi_errno) goto fn_exit;
            }
            
            MPIU_Strncpy(bizcard_ptr, val, val_max_sz);
            
/*            printf("Child %d biz card %s\n", i, bizcard_ptr);
            fflush(stdout);
*/
            bizcard_ptr += strlen(val) + 1;
            bizcards_len += strlen(val) + 1;
        }

        /* send the business cards to the remote root */
        mpi_errno = MPIC_Send(bizcards, bizcards_len, MPI_CHAR,
                              0, 102, tmp_comm->handle); 
        if (mpi_errno != MPI_SUCCESS) goto fn_exit;
        
        /* now send the rank_in_pg of each of the processes in
           comm_ptr. This is necessary because comm_ptr may be a
           subset or a permutation of processes in local process group */

        local_pg_ranks = (int *) MPIU_Malloc(comm_size*sizeof(int));
        if (local_pg_ranks == NULL)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
            goto fn_exit;
        }

        for (i=0; i<comm_size; i++) 
            local_pg_ranks[i] = comm_ptr->vcr[i]->ssm.pg_rank;

        mpi_errno = MPIC_Send(local_pg_ranks, comm_size, MPI_INT,
                              0, 103, tmp_comm->handle); 
        if (mpi_errno != MPI_SUCCESS) goto fn_exit;
        
        MPIU_Free(local_pg_ranks);


        /* now receive the business cards of the processes on the
           other side. Allocate a larger bizcards buffer if
           necessary. */
        if (remote_comm_size > comm_size) {
            MPIU_Free(bizcards);
            bizcards = (char *) MPIU_Malloc(remote_comm_size * val_max_sz);
            if (bizcards == NULL)
            {
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
                goto fn_exit;
            }
        }
        
        /* recv the business cards */
        mpi_errno = MPIC_Recv(bizcards, remote_comm_size*val_max_sz, MPI_CHAR,
                              0, 104, tmp_comm->handle, MPI_STATUS_IGNORE); 
        if (mpi_errno != MPI_SUCCESS) goto fn_exit;

        /* recv the rank in process group for each process on the
           remote side */
        remote_pg_ranks = (int *) MPIU_Malloc(remote_comm_size*sizeof(int));
        if (remote_pg_ranks == NULL)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
            goto fn_exit;
        }

        mpi_errno = MPIC_Recv(remote_pg_ranks, remote_comm_size, MPI_INT,
                              0, 105, tmp_comm->handle, MPI_STATUS_IGNORE); 
        if (mpi_errno != MPI_SUCCESS) goto fn_exit;

        /* Extract the business cards and store them in a new
           kvs. First create a new kvs for the purpose. */
        mpi_errno = PMI_KVS_Create(remote_kvsname);
        if (mpi_errno != 0)
        {
            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_create", "**pmi_kvs_create %d", mpi_errno);
            goto fn_exit;
        }
        /* FIXME - Where does this new kvs get freed? */

        /* put the business cards into the kvs for the remote
           processes */
        key_max_sz = PMI_KVS_Get_key_length_max();
        key = (char *) MPIU_Malloc(key_max_sz);
        if (key == NULL)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
            goto fn_exit;
        }

        bizcard_ptr = bizcards;
        for (i=0; i<remote_comm_size; i++) {
            mpi_errno = MPIU_Snprintf(key, key_max_sz, "P%d-businesscard", remote_pg_ranks[i]);
            if (mpi_errno < 0 || mpi_errno > key_max_sz)
            {
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**snprintf", "**snprintf %d", mpi_errno);
                goto fn_exit;
            }

/*            printf("Child: Put %d's biz card %s\n", i, bizcard_ptr);
            fflush(stdout);
*/
            mpi_errno = PMI_KVS_Put(remote_kvsname, key, bizcard_ptr);
            if (mpi_errno != 0)
            {
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", mpi_errno);
                goto fn_exit;
            }
            
            bizcard_ptr += strlen(bizcard_ptr) + 1;
        }
        
        mpi_errno = PMI_KVS_Commit(remote_kvsname);
        if (mpi_errno != 0)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_commit", "**pmi_kvs_commit %d", mpi_errno);
            goto fn_exit;
        }

        /* All communication with remote root done. Release the
           communicator. */

        /* FIXME - Try to reusethe established vc for the real communicator. */

        MPIR_Comm_release(tmp_comm);
        MPIU_Free(bizcards);
        MPIU_Free(key);
        MPIU_Free(val);

        /* broadcast the remote_kvsname, remote_comm_size,
           remote_pg_size, context_id of the intercommunicator, and
           remote_pg_ranks to other processes. */

        /* Calling MPIR_Bcast instead of MPI_Bcast because when
           comm_connect is called in CH3_Init by a newly spawned child
           process, MPI_Init has not yet completed and MPI_Bcast gives
           an error saying MPI not initialized. */
        mpi_errno = MPIR_Bcast(remote_kvsname, kvs_namelen, MPI_CHAR,
                               root, comm_ptr);
        if (mpi_errno) goto fn_exit;

        mpi_errno = MPIR_Bcast(recv_ints, 3, MPI_INT, root, comm_ptr);
        if (mpi_errno) goto fn_exit;

        mpi_errno = MPIR_Bcast(remote_pg_ranks, remote_comm_size, MPI_INT, root, comm_ptr);
        if (mpi_errno) goto fn_exit;
    }

    else {
        /* non-root nodes */

        /* Create a new business card containing the address of the pg
           for the remote group. Send the business card to the local
           root who will then forward all the business cards to the
           remote root. If we knew the sizes of all the business
           cards, we could have used MPI_Gather instead of MPI_Send. */

        mpi_errno = MPIDI_CH3I_Get_business_card(val, val_max_sz, remote_pg);
        if (mpi_errno != MPI_SUCCESS)
        {
            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_buscard", 0);
            goto fn_exit;
        }

        mpi_errno = MPIC_Send(val, strlen(val)+1, MPI_CHAR, root, 52,
                              comm_ptr->handle); 
        if (mpi_errno) goto fn_exit;

        /* recv the remote_kvsname, remote_comm_size, and intercomm
           context_id from the root. */

        /* Calling MPIR_Bcast instead of MPI_Bcast because when
           comm_connect is called in CH3_Init by a newly spawned child
           process, MPI_Init has not yet completed and MPI_Bcast gives
           an error saying MPI not initialized. */

        mpi_errno = MPIR_Bcast(remote_kvsname, kvs_namelen, MPI_CHAR,
                               root, comm_ptr);
        if (mpi_errno) goto fn_exit;

        mpi_errno = MPIR_Bcast(recv_ints, 3, MPI_INT, root, comm_ptr);
        if (mpi_errno) goto fn_exit;
        remote_comm_size = recv_ints[0];
        remote_pg_size = recv_ints[1];
        context_id = recv_ints[2];

        remote_pg_ranks = (int *) MPIU_Malloc(remote_comm_size*sizeof(int));
        if (remote_pg_ranks == NULL)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
            goto fn_exit;
        }
        mpi_errno = MPIR_Bcast(remote_pg_ranks, remote_comm_size, MPI_INT, root, comm_ptr);
        if (mpi_errno) goto fn_exit;
    }


    /* create and fill in the new intercommunicator */
    mpi_errno = MPIR_Comm_create(comm_ptr, newcomm);
    if (mpi_errno) goto fn_exit;
    
    intercomm = *newcomm;

    intercomm->remote_size = remote_comm_size;
    intercomm->context_id = context_id;
    intercomm->attributes   = NULL;
    intercomm->local_size   = comm_ptr->local_size;
    intercomm->rank         = comm_ptr->rank;
    intercomm->local_group  = NULL;
    intercomm->remote_group = NULL;
    intercomm->comm_kind    = MPID_INTERCOMM;
    intercomm->local_comm   = NULL;
    intercomm->is_low_group = 1;
    intercomm->coll_fns     = NULL;

    /* Point local vcr, vcrt at those of incoming intracommunicator */
    intercomm->local_vcrt = comm_ptr->vcrt;
    MPID_VCRT_Add_ref(comm_ptr->vcrt);
    intercomm->local_vcr  = comm_ptr->vcr;

    /* Fill the pg structure allocated way above */
    remote_pg->size = remote_pg_size;
    remote_pg->kvs_name = MPIU_Malloc(kvs_namelen);
    if (remote_pg->kvs_name == NULL)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        goto fn_exit;
    }
    MPIU_Strncpy(remote_pg->kvs_name, remote_kvsname, kvs_namelen);
    remote_pg->ref_count = 0;
    
    /* Allocate and initialize the VC table associated with the remote group */
    vc_table = MPIU_Malloc(sizeof(MPIDI_VC) * remote_pg->size);
    if (vc_table == NULL)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        goto fn_exit;
    }
    remote_pg->ref_count += remote_pg->size;
    for (p = 0; p < remote_pg->size; p++)
    {
        MPIDI_CH3U_VC_init(&vc_table[p], p);
        vc_table[p].ssm.pg = remote_pg;
        vc_table[p].ssm.pg_rank = p;
        vc_table[p].ssm.sendq_head = NULL;
        vc_table[p].ssm.sendq_tail = NULL;
        vc_table[p].ssm.state = MPIDI_CH3I_VC_STATE_UNCONNECTED;
        vc_table[p].ssm.sock = SOCK_INVALID_SOCK;
        vc_table[p].ssm.conn = NULL;
    }
    remote_pg->vc_table = vc_table;
    
    /* Set up VC reference table */
    mpi_errno = MPID_VCRT_Create(intercomm->remote_size, &intercomm->vcrt);
    if (mpi_errno != MPI_SUCCESS)
    {
        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_vcrt", 0);
        goto fn_exit;
    }
    mpi_errno = MPID_VCRT_Get_ptr(intercomm->vcrt, &intercomm->vcr);
    if (mpi_errno != MPI_SUCCESS)
    {
        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_getptr", 0);
        goto fn_exit;
    }
    for (i=0; i < intercomm->remote_size; i++) {
        MPID_VCR_Dup(&vc_table[remote_pg_ranks[i]], &intercomm->vcr[i]);
    }

    MPIU_Free(remote_kvsname);
    MPIU_Free(remote_pg_ranks);

 fn_exit: 
#endif
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_COMM_CONNECT);
    return mpi_errno;
}


#ifdef USE_OOOLD
int MPIDI_CH3_Comm_connect(char *port_name, int root, MPID_Comm *comm_ptr, MPID_Comm **newcomm)
{
    int p, key_max_sz, val_max_sz, mpi_errno=MPI_SUCCESS;
    int i, bizcards_len, rank, kvs_namelen, recv_ints[2], comm_size;
    int remote_comm_size=0;
    MPID_Comm *tmp_comm, *intercomm, *commself_ptr;
    char *remote_kvsname, *key, *val, *bizcards, *bizcard_ptr;
    MPIDI_CH3I_Process_group_t * pg;
    MPIDI_VC * vc_table;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_COMM_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_COMM_CONNECT);

/* Algorithm: First create an intercommunicator between this root and
   the root on the accept side. Use this intercomm. to use MPI
   functions to communicate the other information needed to create the
   real intercommunicator between the processes on this side and the
   accept side. Then free the intercommunicator between the roots. */

    rank = comm_ptr->rank;

    kvs_namelen = PMI_KVS_Get_name_length_max();

    /* create a new kvs to store the business cards of the processes
       on the other side. */
    remote_kvsname = (char *) MPIU_Malloc(kvs_namelen); 
    if (remote_kvsname == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }

    if (rank == root) {
        /* root creates the kvs and broadcasts it to others */
        mpi_errno = PMI_KVS_Create(remote_kvsname);
        if (mpi_errno != 0)
        {
            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_create", "**pmi_kvs_create %d", mpi_errno);
            return mpi_errno;
        }
        /* FIXME - Where does this new kvs get freed? */


        /* now create the temporary intercommunicator between the two roots*/
        MPID_Comm_get_ptr( MPI_COMM_SELF, commself_ptr );
        mpi_errno = MPIR_Comm_create(commself_ptr, &tmp_comm);
        if (mpi_errno != MPI_SUCCESS) return mpi_errno;

        /* Put the port_name of the remote root as a business card into
           remote_kvsname so that this root can do an MPI_Send to it
           using tmp_comm.  */ 
        mpi_errno = PMI_KVS_Put(remote_kvsname, "P0-businesscard", port_name);
        if (mpi_errno != 0)
        {
            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", mpi_errno);
            return mpi_errno;
        }
        
        /* fill in all the fields of tmp_comm. */

        tmp_comm->context_id = 4999;  /* FIXME - need unique context_id. */
        tmp_comm->remote_size = 1;
        
        /* Fill in new intercomm */
        tmp_comm->attributes   = NULL;
        tmp_comm->local_size   = 1;
        tmp_comm->rank         = 0;
        tmp_comm->local_group  = NULL;
        tmp_comm->remote_group = NULL;
        tmp_comm->comm_kind    = MPID_INTERCOMM;
        tmp_comm->local_comm   = NULL;
        tmp_comm->is_low_group = 1;
        tmp_comm->coll_fns     = NULL;
        
        /* Point local vcr, vcrt at those of commself_ptr */
        tmp_comm->local_vcrt = commself_ptr->vcrt;
        MPID_VCRT_Add_ref(commself_ptr->vcrt);
        tmp_comm->local_vcr  = commself_ptr->vcr;
        
        /* Allocate process group data structure for remote group and populate */
        pg = MPIU_Malloc(sizeof(MPIDI_CH3I_Process_group_t));
        if (pg == NULL)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
            return mpi_errno;
        }
        pg->size = tmp_comm->remote_size;
        pg->kvs_name = MPIU_Malloc(kvs_namelen);
        if (pg->kvs_name == NULL)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
            return mpi_errno;
        }
        MPIU_Strncpy(pg->kvs_name, remote_kvsname, kvs_namelen);
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
            vc_table[p].ssm.pg = pg;
            vc_table[p].ssm.pg_rank = p;
            vc_table[p].ssm.sendq_head = NULL;
            vc_table[p].ssm.sendq_tail = NULL;
            vc_table[p].ssm.state = MPIDI_CH3I_VC_STATE_UNCONNECTED;
            vc_table[p].ssm.sock = SOCK_INVALID_SOCK;
            vc_table[p].ssm.conn = NULL;
        }
        pg->vc_table = vc_table;
        
        /* Set up VC reference table */
        mpi_errno = MPID_VCRT_Create(tmp_comm->remote_size, &tmp_comm->vcrt);
        if (mpi_errno != MPI_SUCCESS)
        {
            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_vcrt", 0);
            return mpi_errno;
        }
        mpi_errno = MPID_VCRT_Get_ptr(tmp_comm->vcrt, &tmp_comm->vcr);
        if (mpi_errno != MPI_SUCCESS)
        {
            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_getptr", 0);
            return mpi_errno;
        }
        for (p = 0; p < pg->size; p++) {
            MPID_VCR_Dup(&vc_table[p], &tmp_comm->vcr[p]);
        }
        
        /* tmp_comm is now established; can communicate with the root on
           the other side. Send comm_size and the business cards of
           all processes. */ 

        comm_size = comm_ptr->local_size;
        mpi_errno = MPIC_Send(&comm_size, 1, MPI_INT, 0, 100,
                              tmp_comm->handle);
        if (mpi_errno != MPI_SUCCESS) return mpi_errno;
        
        val_max_sz = PMI_KVS_Get_value_length_max();
        key_max_sz = PMI_KVS_Get_key_length_max();
        
        key = (char *) MPIU_Malloc(key_max_sz);
        if (key == NULL)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
            return mpi_errno;
        }
        val = (char *) MPIU_Malloc(val_max_sz);
        if (val == NULL)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
            return mpi_errno;
        }
        bizcards = (char *) MPIU_Malloc(comm_size * val_max_sz);
        if (bizcards == NULL)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
            return mpi_errno;
        }
        
        /* get the business cards into val and store them compactly in
           bizcards for communication */
        bizcards_len = 0;
        bizcard_ptr = bizcards;
        for (i=0; i<comm_size; i++) {

            /* find rank of i in process group */
            p = comm_ptr->vcr[i]->ssm.pg_rank;

            mpi_errno = MPIU_Snprintf(key, key_max_sz, "P%d-businesscard", p);
            if (mpi_errno < 0 || mpi_errno > key_max_sz)
            {
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**snprintf", "**snprintf %d", mpi_errno);
                return mpi_errno;
            }
            
            mpi_errno = PMI_KVS_Get(comm_ptr->vcr[i]->ssm.pg->kvs_name,
                                    key, val); 
            if (mpi_errno != 0)
            {
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get %d", mpi_errno);
                return mpi_errno;
            }
            
            MPIU_Strncpy(bizcard_ptr, val, val_max_sz);
            
	    /* I Assume that this is temporary debugging output - WDG */
            /* printf("Child's biz card %s\n", bizcard_ptr);
	       fflush(stdout); */

            bizcard_ptr += strlen(val) + 1;
            bizcards_len += strlen(val) + 1;
        }
        
        /* send the business cards */
        mpi_errno = MPIC_Send(bizcards, bizcards_len, MPI_CHAR,
                              0, 101, tmp_comm->handle); 
        if (mpi_errno != MPI_SUCCESS) return mpi_errno;
        
        /* recv the remote comm_size and the context_id to use for
           newcomm */
        mpi_errno = MPIC_Recv(recv_ints, 2, MPI_INT, 0, 102,
                              tmp_comm->handle, MPI_STATUS_IGNORE);
        if (mpi_errno != MPI_SUCCESS) return mpi_errno;
        
        remote_comm_size = recv_ints[0];
        
        /* now receive the business cards of the processes on the
           other side. Allocate a larger bizcards buffer if
           necessary. */
        if (remote_comm_size > comm_size) {
            MPIU_Free(bizcards);
            bizcards = (char *) MPIU_Malloc(remote_comm_size * val_max_sz);
            if (bizcards == NULL)
            {
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
                return mpi_errno;
            }
        }
        
        /* recv the business cards */
        mpi_errno = MPIC_Recv(bizcards, remote_comm_size*val_max_sz, MPI_CHAR,
                              0, 103, tmp_comm->handle, MPI_STATUS_IGNORE); 
        if (mpi_errno != MPI_SUCCESS) return mpi_errno;

        /* put the business cards into the kvs for the remote
           processes */
        bizcard_ptr = bizcards;
        for (i=0; i<remote_comm_size; i++) {
            mpi_errno = MPIU_Snprintf(key, key_max_sz, "P%d-businesscard", i);
            if (mpi_errno < 0 || mpi_errno > key_max_sz)
            {
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**snprintf", "**snprintf %d", mpi_errno);
                return mpi_errno;
            }

            printf("Child: Put %d's biz card %s\n", i, bizcard_ptr);
            fflush(stdout);

            mpi_errno = PMI_KVS_Put(remote_kvsname, key, bizcard_ptr);
            if (mpi_errno != 0)
            {
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", mpi_errno);
                return mpi_errno;
            }
            
            bizcard_ptr += strlen(bizcard_ptr) + 1;
        }
        
        mpi_errno = PMI_KVS_Commit(remote_kvsname);
        if (mpi_errno != 0)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_commit", "**pmi_kvs_commit %d", mpi_errno);
            return mpi_errno;
        }

        /* broadcast the remote_kvsname, remote_comm_size, and
           context_id of the intercommunicator to other processes. */

        /* Calling MPIR_Bcast instead of MPI_Bcast because when
           comm_connect is called in CH3_Init by a newly spawned child
           process, MPI_Init has not yet completed and MPI_Bcast gives
           an error saying MPI not initialized. */
        mpi_errno = MPIR_Bcast(remote_kvsname, kvs_namelen, MPI_CHAR,
                               root, comm_ptr);
        if (mpi_errno) return mpi_errno;

        mpi_errno = MPIR_Bcast(recv_ints, 2, MPI_INT, root, comm_ptr);
        if (mpi_errno) return mpi_errno;

        MPIR_Comm_release(tmp_comm);
        MPIU_Free(bizcards);
        MPIU_Free(key);
        MPIU_Free(val);

    }

    else {
        /* non-root nodes */
        /* recv the remote_kvsname, remote_comm_size, and intercomm
           context_id from the root. */

        /* Calling MPIR_Bcast instead of MPI_Bcast because when
           comm_connect is called in CH3_Init by a newly spawned child
           process, MPI_Init has not yet completed and MPI_Bcast gives
           an error saying MPI not initialized. */

        mpi_errno = MPIR_Bcast(remote_kvsname, kvs_namelen, MPI_CHAR,
                               root, comm_ptr);
        if (mpi_errno) return mpi_errno;

        mpi_errno = MPIR_Bcast(recv_ints, 2, MPI_INT, root, comm_ptr);
        if (mpi_errno) return mpi_errno;
    }


    /* create and fill in the new intercommunicator */
    mpi_errno = MPIR_Comm_create(comm_ptr, newcomm);
    if (mpi_errno) return mpi_errno;
    
    intercomm = *newcomm;

    intercomm->remote_size = recv_ints[0];
    intercomm->context_id = recv_ints[1];
    intercomm->attributes   = NULL;
    intercomm->local_size   = comm_ptr->local_size;
    intercomm->rank         = comm_ptr->rank;
    intercomm->local_group  = NULL;
    intercomm->remote_group = NULL;
    intercomm->comm_kind    = MPID_INTERCOMM;
    intercomm->local_comm   = NULL;
    intercomm->is_low_group = 1;
    intercomm->coll_fns     = NULL;

    /* Point local vcr, vcrt at those of incoming intracommunicator */
    intercomm->local_vcrt = comm_ptr->vcrt;
    MPID_VCRT_Add_ref(comm_ptr->vcrt);
    intercomm->local_vcr  = comm_ptr->vcr;

    /* Allocate process group data structure for remote group and populate */
    pg = MPIU_Malloc(sizeof(MPIDI_CH3I_Process_group_t));
    if (pg == NULL)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        return mpi_errno;
    }
    pg->size = intercomm->remote_size;
    pg->kvs_name = MPIU_Malloc(kvs_namelen);
    if (pg->kvs_name == NULL)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        return mpi_errno;
    }
    MPIU_Strncpy(pg->kvs_name, remote_kvsname, kvs_namelen);
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
        vc_table[p].ssm.pg = pg;
        vc_table[p].ssm.pg_rank = p;
        vc_table[p].ssm.sendq_head = NULL;
        vc_table[p].ssm.sendq_tail = NULL;
        vc_table[p].ssm.state = MPIDI_CH3I_VC_STATE_UNCONNECTED;
        vc_table[p].ssm.sock = SOCK_INVALID_SOCK;
        vc_table[p].ssm.conn = NULL;
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

    MPIU_Free(remote_kvsname);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_COMM_CONNECT);
    return mpi_errno;
}
#endif
