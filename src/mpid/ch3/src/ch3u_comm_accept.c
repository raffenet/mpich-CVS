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
/* brad : later this will be the upcall to the  _sock version of this */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Comm_accept
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Comm_accept(char *port_name, int root, MPID_Comm *comm_ptr, MPID_Comm **newcomm)
{
    int mpi_errno=MPI_SUCCESS;
#ifdef MPIDI_CH3_USES_SOCK
    
#define NUMPGS 50       /* initial value for max. no. of process
                           groups (for allocating some data structures) */

    int p, j, val_max_sz;
    int i, bizcards_len, rank, kvs_namelen, recv_ints[2],
        send_ints[3];
    int remote_comm_size=0, pgid_len;
    MPID_Comm *tmp_comm = NULL, *intercomm, *commself_ptr;
    char *bizcards=NULL, *bizcard_ptr;
    MPIDI_PG_t ** remote_pgs_array;
    MPIDI_PG_t * new_pg;
    MPIDI_VC_t * vc, *new_vc = NULL;
    int n_local_pgs=1, *local_pg_sizes=NULL, n_remote_pgs, *remote_pg_sizes;
    int sendtag=0, recvtag=0, local_comm_size, pg_no;
    char **local_pg_ids=NULL, **remote_pg_ids;
    MPI_Status status;
    typedef struct pg_info {
        int pg_no;
        int rank_in_pg;
    } pg_info;  /* used to communicate pg info of each process in the
                   communictor */
    pg_info *local_procs_pg_info=NULL, *remote_procs_pg_info;
    MPIU_CHKLMEM_DECL(NUMPGS*3+10); 
    MPIU_CHKPMEM_DECL(NUMPGS);

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_COMM_ACCEPT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_COMM_ACCEPT);

/* Algorithm: First dequeue the vc from the accept queue (it was
   enqueued by the progress engine in response to a connect request
   from the root on the connect side). Use this vc to create an
   intercommunicator between this root and the root on the connect
   side. Use this intercomm. to communicate the other information
   needed to create the real intercommunicator between the processes
   on the two sides. Then free the intercommunicator between the
   roots. Most of the complexity is because there can be multiple
   process groups on each side.*/ 

    mpi_errno = PMI_Get_id_length_max(&pgid_len);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != PMI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_id_length_max", "**pmi_get_id_length_max %d", mpi_errno);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    /* Create the new intercommunicator here. We need to send the
       context id to the other side. */
    mpi_errno = MPIR_Comm_create(comm_ptr, newcomm);
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

    rank = comm_ptr->rank;
    local_comm_size = comm_ptr->local_size;
    
    if (rank == root) {
	MPID_Progress_state progress_state;
	int port_name_tag;

	/* extract the tag from the port_name */
	mpi_errno = MPIU_Str_get_int_arg(port_name, MPIDI_CH3I_PORT_NAME_TAG_KEY, &port_name_tag);
	if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
	
        /* dequeue the accept queue to see if a connection with the
           root on the connect side has been formed in the progress
           engine (the connection is returned in the form of a vc). If
           not, poke the progress engine. */

/*         new_vc = NULL; brad : not present in post-merge code */
	MPID_Progress_start(&progress_state);
        for(;;)
	{
            MPIDI_CH3I_Acceptq_dequeue(&new_vc, port_name_tag);  /* brad : this is implemented in essm/ssm/sshm
                                                   *         but this is this the ONLY place this
                                                   *         is called...
                                                   */
            if (new_vc != NULL)
	    {
		break;
	    }
	    
	    mpi_errno = MPID_Progress_wait(&progress_state);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno) { 
		MPID_Progress_end(&progress_state);
		MPIU_ERR_POP(mpi_errno); 
	    }
	    /* --END ERROR HANDLING-- */
        }
	MPID_Progress_end(&progress_state);

        /* create and fill in the temporary intercommunicator between
           the two roots */ 
        MPID_Comm_get_ptr( MPI_COMM_SELF, commself_ptr );
        mpi_errno = MPIR_Comm_create(commself_ptr, &tmp_comm);
	if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
       
        /* fill in all the fields of tmp_comm. */

        tmp_comm->context_id = 4095;  /* FIXME - we probably need a
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
	if (mpi_errno) {
	    MPIU_ERR_SETFATALANDJUMP(mpi_errno,MPI_ERR_OTHER,"**init_vcrt");
	}
        mpi_errno = MPID_VCRT_Get_ptr(tmp_comm->vcrt, &tmp_comm->vcr);
        if (mpi_errno) {
	    MPIU_ERR_SETFATALANDJUMP(mpi_errno,MPI_ERR_OTHER,"**init_getptr");
	}

        MPID_VCR_Dup(new_vc, tmp_comm->vcr);

        /* tmp_comm is now established; can communicate with the root on
           the other side. */

        /* Go through each rank in comm_ptr and find out how many
           distinct process groups are there. For each processs group,
           we need to know the size of the group, the pg_id, and the
           ranks in the pg that are part of this communicator. Store
           all this info in a way that can be communicated to the
           remote root. */
    
        /* Allocate memory to store process-group info of local processes */
	MPIU_CHKLMEM_MALLOC(local_pg_ids,char**,NUMPGS*sizeof(char*),
			    mpi_errno,"local_pg_ids");
        
        for (i=0; i<NUMPGS; i++)
        {
	    MPIU_CHKLMEM_MALLOC(local_pg_ids[i],char*,pgid_len,
				mpi_errno,"local_pg_ids[i]");
        }
        
	MPIU_CHKLMEM_MALLOC(local_pg_sizes,int*,NUMPGS*sizeof(int),
			    mpi_errno,"local_pg_sizes");
	MPIU_CHKLMEM_MALLOC(local_procs_pg_info,pg_info*,
			    local_comm_size*sizeof(pg_info),
			    mpi_errno,"local_procs_pg_info");
        
        n_local_pgs = 1;
        MPIU_Strncpy(local_pg_ids[0], comm_ptr->vcr[0]->pg->id, pgid_len);
        local_pg_sizes[0] = comm_ptr->vcr[0]->pg->size;
        local_procs_pg_info[0].pg_no = 0;
        local_procs_pg_info[0].rank_in_pg = comm_ptr->vcr[0]->pg_rank;
        
        for (i=1; i<local_comm_size; i++) {
            for (j=0; j<n_local_pgs; j++) {
                if (strcmp(comm_ptr->vcr[i]->pg->id,
                           local_pg_ids[j]) == 0)
                    break;
            }
            if (j == n_local_pgs) {
                /* found new pg */
                MPIU_Strncpy(local_pg_ids[j], comm_ptr->vcr[i]->pg->id, pgid_len);  
                local_pg_sizes[j] = comm_ptr->vcr[i]->pg->size;
                n_local_pgs++;
                if (n_local_pgs == NUMPGS)
                {
                    /* FIXME - Reached the limit. Either return error
                       code or realloc memory for data structures that
                       are of size NUMPGS. Abort for now. */
		    MPIU_ERR_SETFATALANDJUMP(mpi_errno,MPI_ERR_INTERN,"**ch3|sock|pg_limit");
                }
            }
            local_procs_pg_info[i].pg_no = j;
            local_procs_pg_info[i].rank_in_pg = comm_ptr->vcr[i]->pg_rank;
        }
    
        /* Send the remote root: n_local_pgs, local_comm_size,
           context_id for newcomm  
           Recv from the remote root: n_remote_pgs, remote_comm_size */

        send_ints[0] = n_local_pgs;
        send_ints[1] = local_comm_size;
        send_ints[2] = (*newcomm)->context_id;

        mpi_errno = MPIC_Sendrecv(send_ints, 3, MPI_INT, 0,
                                  sendtag, recv_ints, 2, MPI_INT,
                                  0, recvtag, tmp_comm->handle,
                                  MPI_STATUS_IGNORE);
	if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
        sendtag++;
        recvtag++;
    }

    /* broadcast the received info to local processes */
    mpi_errno = MPIR_Bcast(recv_ints, 2, MPI_INT, root, comm_ptr);
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

    n_remote_pgs = recv_ints[0];
    remote_comm_size = recv_ints[1];

    /* All processes allocate memory to store remote process group info */
    MPIU_CHKLMEM_MALLOC(remote_pg_ids,char**,n_remote_pgs*sizeof(char*),
			mpi_errno,"remote_pg_ids");
    
    for (i=0; i<n_remote_pgs; i++) {
	/*
	MPIU_CHKLMEM_MALLOC(remote_pg_ids[i],char*,pgid_len,
			    mpi_errno,"remote_pg_ids[i]");
	*/
	remote_pg_ids[i] = (char*)MPIU_Malloc(pgid_len);
	if (!remote_pg_ids[i]) {
	    MPIU_ERR_SETFATALANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem");
	}
    }

    MPIU_CHKLMEM_MALLOC(remote_pg_sizes,int*,n_remote_pgs*sizeof(int),
			mpi_errno,"remote_pg_sizes");
    MPIU_CHKLMEM_MALLOC(remote_procs_pg_info,pg_info*,
			remote_comm_size*sizeof(pg_info),
			mpi_errno,"remote_procs_pg_info");
    
    if (rank == root) {
        /* Exchange with the remote root the following:
           local_pg_sizes, local_procs_pg_info, local_pgs_ids. */

        mpi_errno = MPIC_Sendrecv(local_pg_sizes, n_local_pgs, MPI_INT, 0,
                                  sendtag, remote_pg_sizes,
                                  n_remote_pgs, MPI_INT, 
                                  0, recvtag, tmp_comm->handle,
                                  MPI_STATUS_IGNORE);
	if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
        sendtag++;
        recvtag++;

        mpi_errno = MPIC_Sendrecv(local_procs_pg_info,
                                  2*local_comm_size, MPI_INT, 0, 
                                  sendtag, remote_procs_pg_info,
                                  2*remote_comm_size, MPI_INT, 
                                  0, recvtag, tmp_comm->handle,
                                  MPI_STATUS_IGNORE);
	if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
        sendtag++;
        recvtag++;

        for (i=0; i<n_remote_pgs; i++) {
            mpi_errno = MPIC_Recv(remote_pg_ids[i], pgid_len, MPI_CHAR,
                                  0, recvtag, tmp_comm->handle,
                                  MPI_STATUS_IGNORE); 
	    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
            recvtag++;
        }

        for (i=0; i<n_local_pgs; i++) {
            mpi_errno = MPIC_Send(local_pg_ids[i],
                                  (int)strlen(local_pg_ids[i])+1, MPI_CHAR,
                                  0, sendtag, tmp_comm->handle);
	    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
            sendtag++;
        }
    }

    /* broadcast the received info to local processes */
    mpi_errno = MPIR_Bcast(remote_pg_sizes, n_remote_pgs, MPI_INT,
                           root, comm_ptr); 
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

    mpi_errno = MPIR_Bcast(remote_procs_pg_info, 2*remote_comm_size,
                           MPI_INT, root, comm_ptr); 
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

    for (i=0; i<n_remote_pgs; i++) {
        mpi_errno = MPIR_Bcast(remote_pg_ids[i], pgid_len, MPI_CHAR,
                               root, comm_ptr);
	if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    }

    /* Allocate process groups corresponding to the remote_pgs and
       link them in to the list of pgs. */

    /* Allocate remote_pgs and keep track of them in a
       remote_pgs_array */
    MPIU_CHKLMEM_MALLOC(remote_pgs_array,MPIDI_PG_t**,
			n_remote_pgs*sizeof(MPIDI_PG_t*),
			mpi_errno, "remote_pgs_array");

    mpi_errno = PMI_KVS_Get_name_length_max(&kvs_namelen);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != PMI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get_name_length_max", "**pmi_kvs_get_name_length_max %d", mpi_errno);
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    for (i=0; i<n_remote_pgs; i++)
    {
	/* If the remote_pg already exists, don't create a new one */

	mpi_errno = MPIDI_PG_Find(remote_pg_ids[i], &new_pg);
	if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

	if (new_pg == NULL)
	{ 
            /* Allocate process group data structure and populate */
            mpi_errno = MPIDI_PG_Create(remote_pg_sizes[i], remote_pg_ids[i], &new_pg);
	    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
#ifdef MPIDI_CH3_USES_SSHM
            /* brad : extra pg ref needed for shared memory modules because the shm_XXXXing_list's
             *         need to be walked though in the later stages of finalize to free queue_info's.
             */
            MPIDI_PG_Add_ref(new_pg);
#endif
            new_pg->size = remote_pg_sizes[i];

	    MPIU_CHKPMEM_MALLOC(new_pg->ch.kvs_name,char *,kvs_namelen+1,
				mpi_errno,"new_pg->ch.kvs_name");

            /* Set the kvsname to blank since it will not be used */
/*             new_pg->ch.kvs_name[0] = '\0'; */
            /* brad : since this is not used, I'm going to store the pg->id of the INTRAcommunicator
             *         on this side of the INTERcommunicator.  this (together with the new Search
             *         bizcard function) lets us obtain the appropriate rank and thus VC on this side
             *         of the INTERcommunicator.  This helps us use shared memory between spawned
             *         processes.
             */
            /* strcmp(ch.kvs_name, id) == 0 for INTRA and != 0 for INTER */            
/*             new_pg->ch.kvs_name[0] = '\0'; */
            MPIU_Strncpy(new_pg->ch.kvs_name, comm_ptr->vcr[0]->pg->id, kvs_namelen);            

            /* Allocate and initialize the VC table associated with the remote pg */

            for (p = 0; p < new_pg->size; p++)
            {
                MPIDI_PG_Get_vcr(new_pg, p, &vc);
	    
                vc->ch.sendq_head = NULL;
                vc->ch.sendq_tail = NULL;
                vc->ch.state = MPIDI_CH3I_VC_STATE_UNCONNECTED;
                vc->ch.sock = MPIDU_SOCK_INVALID_SOCK;
                vc->ch.conn = NULL;
            }
        }
        else { 
            /* free the memory allocated for remote_pg_ids[i] */
            MPIU_Free(remote_pg_ids[i]);
            /* do a sanity check on the process group that was found */

            /* --BEGIN ERROR HANDLING-- */
            if (new_pg->size != remote_pg_sizes[i]) {
                mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN, "**ch3|sock|pgsize", "**ch3|sock|pgsize %d %d", new_pg->size, remote_pg_sizes[i]);
                 goto fn_exit;
            }
            /* --END ERROR HANDLING-- */
        }

        remote_pgs_array[i] = new_pg;
    }

    /* Allocate memory for business cards */

    mpi_errno = PMI_KVS_Get_value_length_max(&val_max_sz);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != PMI_SUCCESS)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get_value_length_max", "**pmi_kvs_get_value_length_max %d", mpi_errno);
        goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    MPIU_CHKLMEM_MALLOC(bizcards,char*,
		MPIDU_MAX(local_comm_size,remote_comm_size) * val_max_sz,
			mpi_errno, "bizcards");

    if (rank == root)
    {
       /* Recv the business cards of the processes on the remote side from the remote root */
        mpi_errno = MPIC_Recv(bizcards, remote_comm_size*val_max_sz, MPI_CHAR,
                              0, recvtag, tmp_comm->handle, &status);
	if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
        recvtag++;

        bizcards_len = status.count;
    }

    /* broadcast the business cards to all local procs. first broadcast the length
       because the non-root nodes need to know it to call bcast. */
    
    mpi_errno = MPIR_Bcast(&bizcards_len, 1, MPI_INT, root, comm_ptr);
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    
    mpi_errno = MPIR_Bcast(bizcards, bizcards_len, MPI_CHAR, root, comm_ptr);
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
        
    /* Extract the business cards and store them in the cache */

    bizcard_ptr = bizcards;
    for (i=0; i<remote_comm_size; i++) {
        pg_no = remote_procs_pg_info[i].pg_no;

        mpi_errno = MPIDI_CH3I_Add_to_bizcard_cache(remote_pgs_array[pg_no]->id, 
                                                    remote_pgs_array[pg_no]->size,
                                                    remote_procs_pg_info[i].rank_in_pg, 
                                                    bizcard_ptr);
	if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

        bizcard_ptr += strlen(bizcard_ptr) + 1;
    }

    if (rank == root) {
        /* now send the business cards of the processes on this side
           to the remote root. */

        int key_max_sz, found;
        char *key, *val;

        mpi_errno = PMI_KVS_Get_key_length_max(&key_max_sz);
        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno != PMI_SUCCESS)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get_key_length_max", "**pmi_kvs_get_key_length_max %d", mpi_errno);
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */
	MPIU_CHKLMEM_MALLOC(key,char*,key_max_sz,mpi_errno,"key");
        MPIU_CHKLMEM_MALLOC(val,char*,val_max_sz,mpi_errno,"val");

        /* get the business cards into val and store them compactly in
           bizcards for communication */
        bizcards_len = 0;
        bizcard_ptr = bizcards;
        for (i=0; i<local_comm_size; i++)
	{
            /* first lookup bizcard cache to see if bizcard is there. */
            mpi_errno = MPIDI_CH3I_Lookup_bizcard_cache(comm_ptr->vcr[i]->pg->id, 
                                                        comm_ptr->vcr[i]->pg_rank, val, 
                                                        val_max_sz, &found);
	    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

            if (!found) {
                /* get it from the KVS space */
                mpi_errno = MPIU_Snprintf(key, key_max_sz, "P%d-businesscard", comm_ptr->vcr[i]->pg_rank); 
                /* --BEGIN ERROR HANDLING-- */
                if (mpi_errno < 0 || mpi_errno > key_max_sz)
                {
                    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
                                                     "**snprintf", "**snprintf %d", mpi_errno);
                    return mpi_errno;
                }
                /* --END ERROR HANDLING-- */
                mpi_errno = PMI_KVS_Get(comm_ptr->vcr[i]->pg->ch.kvs_name, key, val, val_max_sz); 
                /* --BEGIN ERROR HANDLING-- */
                if (mpi_errno != 0)
                {
                    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
                                                     "**pmi_kvs_get", "**pmi_kvs_get %d", mpi_errno);
                    return mpi_errno;
                }
                /* --END ERROR HANDLING-- */

                /* brad : should this result be added to the local bizcache? */
            }

            MPIU_Strncpy(bizcard_ptr, val, val_max_sz);
            
/*            printf("Child %d biz card %s\n", i, bizcard_ptr);
              fflush(stdout);
*/
            bizcard_ptr += strlen(val) + 1;
            bizcards_len += (int)strlen(val) + 1;
        }

        /* send the business cards to the remote root */
        mpi_errno = MPIC_Send(bizcards, bizcards_len, MPI_CHAR,
                              0, sendtag, tmp_comm->handle); 
	if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
        sendtag++;
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

    /* Set up VC reference table */
    mpi_errno = MPID_VCRT_Create(intercomm->remote_size, &intercomm->vcrt);
    if (mpi_errno) {
	MPIU_ERR_SETFATALANDJUMP(mpi_errno,MPI_ERR_OTHER,"**init_vcrt");
    }
    mpi_errno = MPID_VCRT_Get_ptr(intercomm->vcrt, &intercomm->vcr);
    if (mpi_errno) {
	MPIU_ERR_SETFATALANDJUMP(mpi_errno,MPI_ERR_OTHER,"**init_getptr");
    }

    for (i=0; i < intercomm->remote_size; i++)
    {
	MPIDI_PG_Get_vcr(remote_pgs_array[remote_procs_pg_info[i].pg_no], remote_procs_pg_info[i].rank_in_pg, &vc);
        MPID_VCR_Dup(vc, &intercomm->vcr[i]);

    }

    mpi_errno = MPIR_Barrier(comm_ptr);
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

    /* synchronize with remote root */
    if (rank == root) {
        mpi_errno = MPIC_Sendrecv(&i, 0, MPI_INT, 0,
                                  sendtag, &j, 0, MPI_INT,
                                  0, recvtag, tmp_comm->handle,
                                  MPI_STATUS_IGNORE);
	if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

        /* All communication with remote root done. Release the communicator. */
        MPIR_Comm_release(tmp_comm);
    }

    mpi_errno = MPIR_Barrier(comm_ptr);
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

    /* Free new_vc. It was explicitly allocated in ch3_progress.c and returned by 
       MPIDI_CH3I_Acceptq_dequeue. */
    if (rank == root) {
        MPID_Progress_state progress_state;

        if (new_vc->state != MPIDI_VC_STATE_INACTIVE) {
            MPID_Progress_start(&progress_state);
            while (new_vc->state != MPIDI_VC_STATE_INACTIVE) {
                mpi_errno = MPID_Progress_wait(&progress_state);
                /* --BEGIN ERROR HANDLING-- */
                if (mpi_errno != MPI_SUCCESS)
                {
                    MPID_Progress_end(&progress_state);
                    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
                    break;
                }
                /* --END ERROR HANDLING-- */
            }
            MPID_Progress_end(&progress_state);
        }
	
	/* new_vc was allocated in a separate code */
        MPIU_Free(new_vc);
    }
    MPIU_CHKPMEM_COMMIT();

fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_COMM_ACCEPT);
    return mpi_errno;
fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
#else
    return mpi_errno;
#endif    
}
