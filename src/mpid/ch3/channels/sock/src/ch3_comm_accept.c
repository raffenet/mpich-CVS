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
#define NUMPGS 50       /* initial value for max. no. of process
                           groups (for allocating some data structures) */

    int p, j, key_max_sz, val_max_sz, mpi_errno=MPI_SUCCESS;
    int i, bizcards_len, rank, kvs_namelen, recv_ints[2],
        send_ints[3];
    int remote_comm_size=0, pgid_len, tmp_n_local_pgs;
    MPID_Comm *tmp_comm, *intercomm, *commself_ptr, *kvscomm_ptr;
    MPI_Comm kvscomm;
    char *key, *val, *bizcards=NULL, *bizcard_ptr;
    MPIDI_CH3I_Process_group_t **remote_pgs_array, *pg, *new_pg;
    MPIDI_VC *vc_table, *vc;
    int n_local_pgs, *local_pg_sizes, n_remote_pgs, *remote_pg_sizes;
    int sendtag=0, recvtag=0, local_comm_size, kvscomm_rank, pg_no;
    char **local_pg_ids, **remote_pg_ids;
    MPI_Status status;
    typedef struct pg_info {
        int pg_no;
        int rank_in_pg;
    } pg_info;  /* used to communicate pg info of each process in the
                   communictor */
    pg_info *local_procs_pg_info, *remote_procs_pg_info;

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
    if (mpi_errno != PMI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_id_length_max", "**pmi_get_id_length_max %d", mpi_errno);
	goto fn_exit;
    }

    mpi_errno = PMI_KVS_Get_name_length_max(&kvs_namelen);
    if (mpi_errno != PMI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get_name_length_max", "**pmi_kvs_get_name_length_max %d", mpi_errno);
	return mpi_errno;
    }

    mpi_errno = PMI_KVS_Get_key_length_max(&key_max_sz);
    if (mpi_errno != PMI_SUCCESS)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get_key_length_max", "**pmi_kvs_get_key_length_max %d", mpi_errno);
        goto fn_exit;
    }
    key = (char *) MPIU_Malloc(key_max_sz);
    if (key == NULL)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        goto fn_exit;
    }
    
    mpi_errno = PMI_KVS_Get_value_length_max(&val_max_sz);
    if (mpi_errno != PMI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get_value_length_max", "**pmi_kvs_get_value_length_max %d", mpi_errno);
	goto fn_exit;
    }
    val = (char *) MPIU_Malloc(val_max_sz);
    if (val == NULL)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        goto fn_exit;
    }

    /* Allocate memory to store process-group info of local processes */

    local_pg_ids = (char **) MPIU_Malloc(NUMPGS * sizeof(char *));
    if (local_pg_ids == NULL)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        goto fn_exit;
    }
        
    for (i=0; i<NUMPGS; i++) {
        local_pg_ids[i] = (char *) MPIU_Malloc(pgid_len);
        if (local_pg_ids[i] == NULL)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
            goto fn_exit;
        }
    }

    local_pg_sizes = (int *) MPIU_Malloc(NUMPGS * sizeof(int));
    if (local_pg_sizes == NULL)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        goto fn_exit;
    }
    
    local_comm_size = comm_ptr->local_size;
    
    local_procs_pg_info = (pg_info *) MPIU_Malloc(local_comm_size *
                                                  sizeof(pg_info));  
    if (local_procs_pg_info == NULL)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        goto fn_exit;
    }
    
    /* Go through each rank in comm_ptr and find out how many
       distinct process groups are there. For each processs group,
       we need to know the size of the group, the pg_id, and the
       ranks in the pg that are part of this communicator. Store
       all this info in a way that can be communicated to the
       remote root. All processes (not just the root) calculate this
       info because they need it later. */
    
    n_local_pgs = 1;
    MPIU_Strncpy(local_pg_ids[0], comm_ptr->vcr[0]->ch.pg->pg_id, pgid_len);
    local_pg_sizes[0] = comm_ptr->vcr[0]->ch.pg->size;
    local_procs_pg_info[0].pg_no = 0;
    local_procs_pg_info[0].rank_in_pg = comm_ptr->vcr[0]->ch.pg_rank;
    
    for (i=1; i<local_comm_size; i++) {
        for (j=0; j<n_local_pgs; j++) {
            if (strcmp(comm_ptr->vcr[i]->ch.pg->pg_id,
                       local_pg_ids[j]) == 0)
                break;
        }
        if (j == n_local_pgs) {
            /* found new pg */
            MPIU_Strncpy(local_pg_ids[j],
                         comm_ptr->vcr[i]->ch.pg->pg_id, pgid_len);  
            local_pg_sizes[j] = comm_ptr->vcr[i]->ch.pg->size;
            n_local_pgs++;
            if (n_local_pgs == NUMPGS) {
                /* FIXME - Reached the limit. Either return error
                   code or realloc memory for data structures that
                   are of size NUMPGS. Abort for now. */
                MPID_Abort(NULL, mpi_errno, 13);
            }
            
        }
        local_procs_pg_info[i].pg_no = j;
        local_procs_pg_info[i].rank_in_pg = comm_ptr->vcr[i]->ch.pg_rank;
    }
    

    /* Create the new intercommunicator here. We need to send the
       context id to the other side. */
    mpi_errno = MPIR_Comm_create(comm_ptr, newcomm);
    if (mpi_errno != MPI_SUCCESS) goto fn_exit;

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
        if (mpi_errno != MPI_SUCCESS) goto fn_exit;
        sendtag++;
        recvtag++;
    }

    /* broadcast the received info to local processes */
    mpi_errno = MPIR_Bcast(recv_ints, 2, MPI_INT, root, comm_ptr);
    if (mpi_errno) goto fn_exit;

    n_remote_pgs = recv_ints[0];
    remote_comm_size = recv_ints[1];

    /* All processes allocate memory to store remote process group info */

    remote_pg_ids = (char **) MPIU_Malloc(n_remote_pgs * sizeof(char *));
    if (remote_pg_ids == NULL)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        goto fn_exit;
    }
    
    for (i=0; i<n_remote_pgs; i++) {
        remote_pg_ids[i] = (char *) MPIU_Malloc(pgid_len);
        if (remote_pg_ids[i] == NULL)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
            goto fn_exit;
        }
    }
    
    remote_pg_sizes = (int *) MPIU_Malloc(n_remote_pgs * sizeof(int));
    if (remote_pg_sizes == NULL)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        goto fn_exit;
    }
    
    remote_procs_pg_info = (pg_info *)
        MPIU_Malloc(remote_comm_size * sizeof(pg_info)); 
    if (remote_procs_pg_info == NULL)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        goto fn_exit;
    }
    
    if (rank == root) {
        /* Exchange with the remote root the following:
           local_pg_sizes, local_procs_pg_info, local_pgs_ids. */

        mpi_errno = MPIC_Sendrecv(local_pg_sizes, n_local_pgs, MPI_INT, 0,
                                  sendtag, remote_pg_sizes,
                                  n_remote_pgs, MPI_INT, 
                                  0, recvtag, tmp_comm->handle,
                                  MPI_STATUS_IGNORE);
        if (mpi_errno != MPI_SUCCESS) goto fn_exit;
        sendtag++;
        recvtag++;

        mpi_errno = MPIC_Sendrecv(local_procs_pg_info,
                                  2*local_comm_size, MPI_INT, 0, 
                                  sendtag, remote_procs_pg_info,
                                  2*remote_comm_size, MPI_INT, 
                                  0, recvtag, tmp_comm->handle,
                                  MPI_STATUS_IGNORE);
        if (mpi_errno != MPI_SUCCESS) goto fn_exit;
        sendtag++;
        recvtag++;

        for (i=0; i<n_remote_pgs; i++) {
            mpi_errno = MPIC_Recv(remote_pg_ids[i], pgid_len, MPI_CHAR,
                                  0, recvtag, tmp_comm->handle,
                                  MPI_STATUS_IGNORE); 
            if (mpi_errno != MPI_SUCCESS) goto fn_exit;
            recvtag++;
        }

        for (i=0; i<n_local_pgs; i++) {
            mpi_errno = MPIC_Send(local_pg_ids[i],
                                  strlen(local_pg_ids[i])+1, MPI_CHAR,
                                  0, sendtag, tmp_comm->handle);
            if (mpi_errno != MPI_SUCCESS) goto fn_exit;
            sendtag++;
        }
    }

    /* broadcast the received info to local processes */

    mpi_errno = MPIR_Bcast(remote_pg_sizes, n_remote_pgs, MPI_INT,
                           root, comm_ptr); 
    if (mpi_errno) goto fn_exit;

    mpi_errno = MPIR_Bcast(remote_procs_pg_info, 2*remote_comm_size,
                           MPI_INT, root, comm_ptr); 
    if (mpi_errno) goto fn_exit;

    for (i=0; i<n_remote_pgs; i++) {
        mpi_errno = MPIR_Bcast(remote_pg_ids[i], pgid_len, MPI_CHAR,
                               root, comm_ptr);
        if (mpi_errno != MPI_SUCCESS) goto fn_exit;
    }

    /* Allocate process groups corresponding to the remote_pgs and
       link them in to the list of pgs. */

    /* For all newly created remote_pgs, we need to create a new
       KVS. Since processes in the same process group can share a KVS, 
       we make one process (the lowest ranked process) in each
       "local" process group create new KVSes and bcast the names to
       other processes in that group. MPI_Comm_split is amazingly well
       suited to create the communicators needed for this. */

    if (n_local_pgs != 1) {
        mpi_errno = NMPI_Comm_split(comm_ptr->handle,
                                    local_procs_pg_info[rank].pg_no, 0,
                                    &kvscomm);
        if (mpi_errno) goto fn_exit;
        MPID_Comm_get_ptr( kvscomm, kvscomm_ptr );
    }
    else {
        kvscomm_ptr = comm_ptr;
        kvscomm = comm_ptr->handle;
    }

    kvscomm_rank = kvscomm_ptr->rank;


    /* Allocate remote_pgs and keep track of them in a
       remote_pgs_array */
    remote_pgs_array = MPIU_Malloc(n_remote_pgs *
                                   sizeof(MPIDI_CH3I_Process_group_t *)); 
    if (remote_pgs_array == NULL)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        goto fn_exit;
    }

    for (i=0; i<n_remote_pgs; i++) {

        /* Allocate process group data structure and populate */
        new_pg = MPIU_Malloc(sizeof(MPIDI_CH3I_Process_group_t));
        if (new_pg == NULL)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
            goto fn_exit;
        }

        remote_pgs_array[i] = new_pg;

        new_pg->size = remote_pg_sizes[i];

        new_pg->kvs_name = MPIU_Malloc(kvs_namelen + 1);
        if (new_pg->kvs_name == NULL)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
            goto fn_exit;
        }

        if (kvscomm_rank == 0) {
            mpi_errno = PMI_KVS_Create(new_pg->kvs_name, kvs_namelen);
            if (mpi_errno != 0)
            {
                mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_create", "**pmi_kvs_create %d", mpi_errno);
                goto fn_exit;
            }
        }

        mpi_errno = MPIR_Bcast(new_pg->kvs_name, kvs_namelen, MPI_CHAR,
                               0, kvscomm_ptr);
        if (mpi_errno != MPI_SUCCESS) goto fn_exit;

        new_pg->pg_id = MPIU_Malloc(pgid_len + 1);
        if (new_pg->pg_id == NULL)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
            goto fn_exit;
        }

        MPIU_Strncpy(new_pg->pg_id, remote_pg_ids[i], pgid_len);

        new_pg->ref_count = 1;
        new_pg->next = NULL;

        /* Link this pg in to the list of pgs. The new pg (represented
           by the new pg_id) should not already exist. If it does,
           flag an error. */

        pg = MPIDI_CH3I_Process.pg;
        while (pg != NULL) {
            if (strcmp(pg->pg_id, new_pg->pg_id) == 0) {
                /* pg already exists! Need to return error code
                   here. Abort for now. */
                MPID_Abort(NULL, mpi_errno, 13);
            }
            if (pg->next == NULL) {
                pg->next = new_pg;
                break;
            }
            pg = pg->next;
        }
    }

    if (rank == root) {

        /* Recv the business cards of the processes on the remote side */
        bizcards = (char *) MPIU_Malloc(remote_comm_size * val_max_sz);
        if (bizcards == NULL)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
            goto fn_exit;
        }
        
        /* recv the business cards from the remote root */
        mpi_errno = MPIC_Recv(bizcards, remote_comm_size*val_max_sz, MPI_CHAR,
                              0, recvtag, tmp_comm->handle, &status);
        if (mpi_errno != MPI_SUCCESS) goto fn_exit;
        recvtag++;

        bizcards_len = status.count;

        /* send the business cards to the root (lowest ranked) process
           of each local pg */

        tmp_n_local_pgs = 0;
        for (i=0; i<local_comm_size; i++) {
            for (j=0; j<tmp_n_local_pgs; j++) {
                if (strcmp(comm_ptr->vcr[i]->ch.pg->pg_id,
                           local_pg_ids[j]) == 0)
                    break;
            }
            if (j == tmp_n_local_pgs) {
                /* found root of new pg */
                if (i != rank) { /* don't need to send to myself */
                    mpi_errno = MPIC_Send(bizcards, bizcards_len, MPI_CHAR,
                                          i, 127, comm_ptr->handle);
                    if (mpi_errno != MPI_SUCCESS) goto fn_exit;
                }
                tmp_n_local_pgs++;
            }
        }
    }

    else if (kvscomm_rank == 0) { /* roots of local pgs other than
                                     root of comm_ptr */
        
        bizcards = (char *) MPIU_Malloc(remote_comm_size * val_max_sz);
        if (bizcards == NULL)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
            goto fn_exit;
        }

        mpi_errno = MPIC_Recv(bizcards, remote_comm_size * val_max_sz,
                              MPI_CHAR, root, 127, comm_ptr->handle,
                              MPI_STATUS_IGNORE); 
        if (mpi_errno != MPI_SUCCESS) goto fn_exit;
    }

    if (kvscomm_rank == 0) {

        /* Extract the business cards and store them in the kvs */

        /* put the business cards into the kvs for the remote
           processes */

        bizcard_ptr = bizcards;
        for (i=0; i<remote_comm_size; i++) {
            mpi_errno = MPIU_Snprintf(key, key_max_sz, "P%d-businesscard", remote_procs_pg_info[i].rank_in_pg);
            if (mpi_errno < 0 || mpi_errno > key_max_sz)
            {
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**snprintf", "**snprintf %d", mpi_errno);
                goto fn_exit;
            }

/*            printf("Child: Put %d's biz card %s\n", i, bizcard_ptr);
            fflush(stdout);
*/

            pg_no = remote_procs_pg_info[i].pg_no;
            mpi_errno = PMI_KVS_Put(remote_pgs_array[pg_no]->kvs_name, key, bizcard_ptr);
            if (mpi_errno != 0)
            {
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", mpi_errno);
                goto fn_exit;
            }
            bizcard_ptr += strlen(bizcard_ptr) + 1;
        }

        for (j=0; j<n_remote_pgs; j++) {
            mpi_errno = PMI_KVS_Commit(remote_pgs_array[j]->kvs_name);
            if (mpi_errno != 0)
            {
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_commit", "**pmi_kvs_commit %d", mpi_errno);
                goto fn_exit;
            }
        }
    }

    mpi_errno = PMI_Barrier();
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", mpi_errno);
	return mpi_errno;
    }

    if (rank == root) {
        /* now send the business cards of the processes on this side
           to the other side. Allocate a larger bizcards buffer if
           necessary. */
        if (local_comm_size > remote_comm_size) {
            MPIU_Free(bizcards);
            bizcards = (char *) MPIU_Malloc(local_comm_size * val_max_sz);
            if (bizcards == NULL)
            {
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
                goto fn_exit;
            }
        }
        /* get the business cards into val and store them compactly in
           bizcards for communication */
        bizcards_len = 0;
        bizcard_ptr = bizcards;
        for (i=0; i<local_comm_size; i++) {
            
            mpi_errno = MPIU_Snprintf(key, key_max_sz,
                                      "P%d-businesscard",
                                      comm_ptr->vcr[i]->ch.pg_rank); 
            if (mpi_errno < 0 || mpi_errno > key_max_sz)
            {
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**snprintf", "**snprintf %d", mpi_errno);
                return mpi_errno;
            }
            mpi_errno = PMI_KVS_Get(comm_ptr->vcr[i]->ch.pg->kvs_name,
                                    key, val, val_max_sz); 
            if (mpi_errno != 0)
            {
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get %d", mpi_errno);
                return mpi_errno;
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
        if (mpi_errno != MPI_SUCCESS) goto fn_exit;
        sendtag++;

        /* All communication with remote root done. Release the
           communicator. */
        MPIR_Comm_release(tmp_comm);
    }


    /* release the kvscomm if it ws created */
    if (n_local_pgs != 1)
        MPIR_Comm_release(kvscomm_ptr);

    /* Allocate and initialize the VC table associated with the remote
       groups */ 

    for (i=0; i<n_remote_pgs; i++) {
        vc_table = MPIU_Malloc(sizeof(MPIDI_VC) * remote_pgs_array[i]->size);
        if (vc_table == NULL)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
            goto fn_exit;
        }
        remote_pgs_array[i]->ref_count += remote_pgs_array[i]->size;
        for (p = 0; p < remote_pgs_array[i]->size; p++)
        {
            MPIDI_CH3U_VC_init(&vc_table[p], p);
            vc_table[p].ch.pg = remote_pgs_array[i];
            vc_table[p].ch.pg_rank = p;
            vc_table[p].ch.sendq_head = NULL;
            vc_table[p].ch.sendq_tail = NULL;
            vc_table[p].ch.state = MPIDI_CH3I_VC_STATE_UNCONNECTED;
            vc_table[p].ch.sock = MPIDU_SOCK_INVALID_SOCK;
            vc_table[p].ch.conn = NULL;
        }
        remote_pgs_array[i]->vc_table = vc_table;
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
        pg_no = remote_procs_pg_info[i].pg_no;
        vc_table = remote_pgs_array[pg_no]->vc_table;
        MPID_VCR_Dup(&vc_table[remote_procs_pg_info[i].rank_in_pg], &intercomm->vcr[i]);
    }

    MPIU_Free(key);
    MPIU_Free(val);

#ifdef FOO
    printf("Reached here, rank %d\n", rank);
    fflush(stdout);
    MPI_Barrier(MPI_COMM_WORLD);
#endif

    for (i=0; i<NUMPGS; i++)
        MPIU_Free(local_pg_ids[i]);
    MPIU_Free(local_pg_ids);
    MPIU_Free(local_pg_sizes);
    MPIU_Free(local_procs_pg_info);

    for (i=0; i<n_remote_pgs; i++)
        MPIU_Free(remote_pg_ids[i]);
    MPIU_Free(remote_pg_ids);
    MPIU_Free(remote_pg_sizes);
    MPIU_Free(remote_procs_pg_info);

    MPIU_Free(remote_pgs_array);

    if (bizcards) MPIU_Free(bizcards);

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_COMM_ACCEPT);
    return mpi_errno;
}
