/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"

/*
 * MPIDI_CH3_Comm_accept()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Comm_accept
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Comm_accept(char *port_name, int root, MPID_Comm *comm_ptr, MPID_Comm **newcomm)
{
    int p, mpi_errno = MPI_SUCCESS, kvs_namelen;
    int rank, comm_size, remote_comm_size=0, recv_ints[2], id_sz;
    MPID_Comm *tmp_comm, *intercomm, *commself_ptr;
    int i, key_max_sz, val_max_sz, bizcards_len, send_ints[3];
    MPIDI_CH3I_Process_group_t *remote_root_pg, *pg;
    MPIDI_VC *vc_table, *vc;
    char *key, *val, *bizcards, *bizcard_ptr;
    int *local_pg_ranks, *remote_pg_ranks;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_COMM_ACCEPT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_COMM_ACCEPT);

/* Algorithm: First dequeue the vc from the accept queue (it was
   enqueued by the progress engine in response to a connect request
   from the root on the connect side). Use this vc to create an
   intercommunicator between this root and the root on the connect
   side. Use this intercomm. to communicate the other information
   needed to create the real intercommunicator between the processes
   on the two sides. Then free the intercommunicator between the
   roots. */

    /* create the new intercommunicator */
    mpi_errno = MPIR_Comm_create(comm_ptr, newcomm);
    if (mpi_errno != MPI_SUCCESS) goto fn_exit;

    /* Allocate new pg structure to store information about the remote group */
    remote_root_pg = MPIU_Malloc(sizeof(MPIDI_CH3I_Process_group_t));
    if (remote_root_pg == NULL)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        goto fn_exit;
    }
    /* FIXME - Where does this new pg get freed? */

    mpi_errno = PMI_KVS_Get_name_length_max(&kvs_namelen);
    if (mpi_errno != PMI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get_name_length_max", "**pmi_kvs_get_name_length_max %d", mpi_errno);
	return mpi_errno;
    }

    remote_root_pg->kvs_name = MPIU_Malloc(kvs_namelen + 1);
    if (remote_root_pg->kvs_name == NULL)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        goto fn_exit;
    }
    
    mpi_errno = PMI_Get_id_length_max(&id_sz);
    if (mpi_errno != PMI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_id_length_max", "**pmi_get_id_length_max %d", mpi_errno);
	return mpi_errno;
    }
    
    remote_root_pg->pg_id = MPIU_Malloc(id_sz + 1);
    if (remote_root_pg->pg_id == NULL)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        goto fn_exit;
    }
    
    remote_root_pg->ref_count = 1;
    remote_root_pg->next = NULL;

    mpi_errno = PMI_KVS_Get_value_length_max(&val_max_sz);
    if (mpi_errno != PMI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get_value_length_max", "**pmi_kvs_get_value_length_max %d", mpi_errno);
	return mpi_errno;
    }
    val = (char *) MPIU_Malloc(val_max_sz);
    if (val == NULL)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        goto fn_exit;
    }

    rank = comm_ptr->rank;

    if (rank == root) {

        /* dequeue the accept queue to see if a connection with the
           root on the connect side has been formed in the progress
           engine (the connection is returned in the form of a vc). If
           not, poke the progress engine. */

        vc = NULL;
        while (vc == NULL) {
            MPID_Progress_start();

            MPIDI_CH3I_Acceptq_dequeue(&vc);

            if (vc == NULL) {
                mpi_errno = MPID_Progress_wait();
                if (mpi_errno) goto fn_exit;
            }
            else {
                MPID_Progress_end();
                break;
            }
        } 

        /* create and fill in the temporary intercommunicator between
           the two roots */ 
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
        tmp_comm->is_low_group = 0;
        tmp_comm->coll_fns     = NULL;
        
        /* No pg structure needed since vc has already been set up
           (connection has been established). */

        /* Point local vcr, vcrt at those of commself_ptr */
        tmp_comm->local_vcrt = commself_ptr->vcrt;
        MPID_VCRT_Add_ref(commself_ptr->vcrt);
        tmp_comm->local_vcr  = commself_ptr->vcr;

        /* No pg needed since connection has already been formed. 
           FIXME - ensure that the comm_release code does not try to
           free an unallocated pg */
        
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

        /* First recv the remote_comm_size from the root on the other side and
           send the local comm_size, pg_size, and the context_id of the new
           intercommunicator to that root. */ 
        
        mpi_errno = MPIC_Recv(recv_ints, 2, MPI_INT,
                              0, 100, tmp_comm->handle, MPI_STATUS_IGNORE); 
        if (mpi_errno != MPI_SUCCESS) goto fn_exit;

        remote_comm_size = recv_ints[0];
        remote_root_pg->size = recv_ints[1];
     
        /* send the comm_size, pg_size, context_id of newcomm. */
        comm_size = comm_ptr->local_size;
        send_ints[0] = comm_size;
        send_ints[1] = MPIDI_CH3I_Process.pg->size;
        send_ints[2] = (*newcomm)->context_id;
        
        mpi_errno = MPIC_Send(send_ints, 3, MPI_INT, 0, 101, tmp_comm->handle);
        if (mpi_errno != MPI_SUCCESS) goto fn_exit;

        /* receive the remote_root_pg->pg_id */
        mpi_errno = MPIC_Recv(remote_root_pg->pg_id, id_sz, MPI_CHAR,
                              0, 102, tmp_comm->handle, MPI_STATUS_IGNORE); 
        if (mpi_errno != MPI_SUCCESS) goto fn_exit;

        /* broadcast the remote_root_pgid to other processes in
           comm_ptr */
        mpi_errno = MPIR_Bcast(remote_root_pg->pg_id, id_sz, MPI_CHAR,
                               root, comm_ptr);
        if (mpi_errno != MPI_SUCCESS) goto fn_exit;

        /* recv the business cards of the remote processes from the
           remote root */

        bizcards = (char *) MPIU_Malloc(remote_comm_size * val_max_sz);
        if (bizcards == NULL)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
            goto fn_exit;
        }
        
        mpi_errno = MPIC_Recv(bizcards, remote_comm_size*val_max_sz, MPI_CHAR,
                              0, 103, tmp_comm->handle, MPI_STATUS_IGNORE); 
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
                              0, 104, tmp_comm->handle, MPI_STATUS_IGNORE); 
        if (mpi_errno != MPI_SUCCESS) goto fn_exit;

        /* Extract the business cards and store them in a new
           kvs. First create a new kvs for the purpose. */
        mpi_errno = PMI_KVS_Create(remote_root_pg->kvs_name, kvs_namelen);
        if (mpi_errno != PMI_SUCCESS)
        {
            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_create", "**pmi_kvs_create %d", mpi_errno);
            goto fn_exit;
        }
        /* FIXME - Where does this new kvs get freed? */

        mpi_errno = PMI_KVS_Get_key_length_max(&key_max_sz);
	if (mpi_errno != PMI_SUCCESS)
	{
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get_key_length_max", "**pmi_kvs_get_key_length_max %d", mpi_errno);
            return mpi_errno;
	}
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
            
/*            printf("Parent: Put %d's biz card %s\n", i, bizcard_ptr);
            fflush(stdout);
*/
            mpi_errno = PMI_KVS_Put(remote_root_pg->kvs_name, key, bizcard_ptr);
            if (mpi_errno != 0)
            {
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", mpi_errno);
                goto fn_exit;
            }
            
            bizcard_ptr += strlen(bizcard_ptr) + 1;
        }

        mpi_errno = PMI_KVS_Commit(remote_root_pg->kvs_name);
        if (mpi_errno != 0)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_commit", "**pmi_kvs_commit %d", mpi_errno);
            goto fn_exit;
        }

        /* Now we need to send the business cards of the processes on
           this side to the root on the other side. First create a new
           business card containing the pg_id of the pg 
           for the remote group. Receive the business cards thus
           created by all other processes on this side and then
           forward them to the root on the remote side. If we knew the
           sizes of all the business cards, we could have used
           MPI_Gather instead of a loop of MPI_Recvs. */

        /* Allocate a larger bizcards buffer if necessary. */
        if (comm_size > remote_comm_size) {
            MPIU_Free(bizcards);
            bizcards = (char *) MPIU_Malloc(comm_size * val_max_sz);
            if (bizcards == NULL)
            {
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
                goto fn_exit;
            }
        }
        
        /* get the business cards in val and store them compactly in
           bizcards for communication */
        bizcards_len = 0;
        bizcard_ptr = bizcards;
        for (i=0; i<comm_size; i++) {
            if (i == root) {
                mpi_errno = MPIDI_CH3I_Get_business_card(val, val_max_sz, remote_root_pg->pg_id);
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

/*            printf("Parent's biz card %s\n", bizcard_ptr);
            fflush(stdout);
*/            
            bizcard_ptr += strlen(val) + 1;
            bizcards_len += (int)strlen(val) + 1;
        }
        
        /* send the business cards */
        mpi_errno = MPIC_Send(bizcards, bizcards_len, MPI_CHAR,
                              0, 105, tmp_comm->handle); 
        if (mpi_errno != MPI_SUCCESS) goto fn_exit;

        /* now send the rank_in_pg of each of the processes in
           comm_ptr. This is necessary because comm_ptr may be a
           subset or a permutation of the processes in the local
           process group */ 

        local_pg_ranks = (int *) MPIU_Malloc(comm_size*sizeof(int));
        if (local_pg_ranks == NULL)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
            goto fn_exit;
        }

        for (i=0; i<comm_size; i++) 
            local_pg_ranks[i] = comm_ptr->vcr[i]->ch.pg_rank;

        mpi_errno = MPIC_Send(local_pg_ranks, comm_size, MPI_INT,
                              0, 106, tmp_comm->handle); 
        if (mpi_errno != MPI_SUCCESS) goto fn_exit;
        
        MPIU_Free(local_pg_ranks);


        /* All communication with remote root done. Release the
           communicator. */
         
        /* FIXME - Try to reuse the established vc for the real
                   communicator. */

        MPIR_Comm_release(tmp_comm);
        MPIU_Free(bizcards);
        MPIU_Free(key);

        /* Now broadcast to other processes on this (accept) side the
           information they need, namely the kvsname for the remote
           processes, the comm_size and pg_size on the remote side,
           and the remote_pg_ranks. */

        mpi_errno = MPIR_Bcast(remote_root_pg->kvs_name, kvs_namelen, MPI_CHAR,
                               root, comm_ptr);
        if (mpi_errno) goto fn_exit;

        mpi_errno = MPIR_Bcast(recv_ints, 2, MPI_INT, root, comm_ptr);
        if (mpi_errno) goto fn_exit;

        mpi_errno = MPIR_Bcast(remote_pg_ranks, remote_comm_size, MPI_INT, root, comm_ptr);
        if (mpi_errno) goto fn_exit;
    }

    else {
        /* non-root nodes */

        /* receive the pg_id of the remote_root from the root */
        mpi_errno = MPIR_Bcast(remote_root_pg->pg_id, id_sz, MPI_CHAR,
                               root, comm_ptr);
        if (mpi_errno) goto fn_exit;

        /* Create a new business card containing the address of the pg
           for the remote group. Send the business card to the local
           root who will then forward all the business cards to the
           remote root. If we knew the sizes of all the business
           cards, we could have used MPI_Gather instead of MPI_Send. */

        mpi_errno = MPIDI_CH3I_Get_business_card(val, val_max_sz, remote_root_pg->pg_id);
        if (mpi_errno != MPI_SUCCESS)
        {
            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_buscard", 0);
            goto fn_exit;
        }

        mpi_errno = MPIC_Send(val, (int)strlen(val)+1, MPI_CHAR, root, 52,
                              comm_ptr->handle); 
        if (mpi_errno) goto fn_exit;

        /* recv the remote_kvsname and remote_comm_size from the root. */
        mpi_errno = MPIR_Bcast(remote_root_pg->kvs_name, kvs_namelen, MPI_CHAR,
                               root, comm_ptr);
        if (mpi_errno) goto fn_exit;

        mpi_errno = MPIR_Bcast(recv_ints, 2, MPI_INT, root, comm_ptr);
        if (mpi_errno) goto fn_exit;
        remote_comm_size = recv_ints[0];
        remote_root_pg->size = recv_ints[1];

        remote_pg_ranks = (int *) MPIU_Malloc(remote_comm_size*sizeof(int));
        if (remote_pg_ranks == NULL)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
            goto fn_exit;
        }
        mpi_errno = MPIR_Bcast(remote_pg_ranks, remote_comm_size, MPI_INT, root, comm_ptr);
        if (mpi_errno) goto fn_exit;
    }

    MPIU_Free(val);


    /* Link the remote_root_pg to the list of pgs. The pg represented by
       remote_root_pgid should not already exist. If it does, flag an
       error. */

    pg = MPIDI_CH3I_Process.pg;
    while (pg != NULL) {
        if (strcmp(pg->pg_id, remote_root_pg->pg_id) == 0) {
            /* Abort for now. Need to return error code here. */
            /* printf("ACCEPT ERROR: attempting to do a comm_connect to a process in MPI_COMM_WORLD\n");
               fflush(stdout); */
            MPID_Abort(NULL, mpi_errno, 13);
        }
        if (pg->next == NULL) {
            pg->next = remote_root_pg;
            break;
        }
        pg = pg->next;
    }


    /* Now fill in newcomm */
    intercomm = *newcomm;

    intercomm->attributes   = NULL;
    intercomm->remote_size  = remote_comm_size;
    intercomm->local_size   = comm_ptr->local_size;
    intercomm->rank         = comm_ptr->rank;
    intercomm->local_group  = NULL;
    intercomm->remote_group = NULL;
    intercomm->comm_kind    = MPID_INTERCOMM;
    intercomm->local_comm   = NULL;
    intercomm->is_low_group = 0;
    intercomm->coll_fns     = NULL;

    /* Point local vcr, vcrt at those of incoming intracommunicator */
    intercomm->local_vcrt = comm_ptr->vcrt;
    MPID_VCRT_Add_ref(comm_ptr->vcrt);
    intercomm->local_vcr  = comm_ptr->vcr;

    /* Allocate and initialize the VC table associated with the remote group */
    vc_table = MPIU_Malloc(sizeof(MPIDI_VC) * remote_root_pg->size);
    if (vc_table == NULL)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        goto fn_exit;
    }
    remote_root_pg->ref_count += remote_root_pg->size;
    for (p = 0; p < remote_root_pg->size; p++)
    {
	MPIDI_CH3U_VC_init(&vc_table[p], p);
	vc_table[p].ch.pg = remote_root_pg;
	vc_table[p].ch.pg_rank = p;
	vc_table[p].ch.sendq_head = NULL;
	vc_table[p].ch.sendq_tail = NULL;
	vc_table[p].ch.state = MPIDI_CH3I_VC_STATE_UNCONNECTED;
	vc_table[p].ch.sock = MPIDU_SOCK_INVALID_SOCK;
	vc_table[p].ch.conn = NULL;
    }
    remote_root_pg->vc_table = vc_table;

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

    MPIU_Free(remote_pg_ranks);

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_COMM_ACCEPT);
    return mpi_errno;
}
