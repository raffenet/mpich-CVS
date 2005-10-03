/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

#ifdef MPIDI_DEV_IMPLEMENTS_COMM_ACCEPT

/* Override these macros here if you want to debug this file only.
#define MPIU_DBG_PRINTF(a) printf a ; fflush(stdout)
#define MPICH_DBG_OUTPUT
*/

#undef FUNCNAME
#define FUNCNAME MPIDI_Create_inter_root_communicator
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPIDI_Create_inter_root_communicator(const char *port_name, 
						MPID_Comm **comm_pptr, 
						MPIDI_VC_t **vc_pptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *tmp_comm;
    MPIDI_VC_t *new_vc = NULL;
    MPID_Progress_state progress_state;
    int port_name_tag;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CREATE_INTER_ROOT_COMMUNICATOR);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CREATE_INTER_ROOT_COMMUNICATOR);

    /* extract the tag from the port_name */
    mpi_errno = MPIDI_GetTagFromPort( port_name, &port_name_tag);
    if (mpi_errno != MPIU_STR_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }


    /* dequeue the accept queue to see if a connection with the
       root on the connect side has been formed in the progress
       engine (the connection is returned in the form of a vc). If
       not, poke the progress engine. */

    MPID_Progress_start(&progress_state);
    for(;;)
    {
	MPIDI_CH3I_Acceptq_dequeue(&new_vc, port_name_tag);
	if (new_vc != NULL)
	{
	    break;
	}

	mpi_errno = MPID_Progress_wait(&progress_state);
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno)
	{
	    MPID_Progress_end(&progress_state);
	    MPIU_ERR_POP(mpi_errno);
	}
	/* --END ERROR HANDLING-- */
    }
    MPID_Progress_end(&progress_state);

    mpi_errno = MPIDI_CH3I_Initialize_tmp_comm(&tmp_comm, new_vc, 0);

    /* If the VC creates non-duplex connections then the acceptor will
     * need to connect back to form the other half of the connection. */
#ifdef MPIDI_CH3_HAS_CONN_ACCEPT_HOOK
    mpi_errno = MPIDI_CH3_Complete_unidirectional_connection2( new_vc )
#endif

    *comm_pptr = tmp_comm;
    *vc_pptr = new_vc;

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CREATE_INTER_ROOT_COMMUNICATOR);
    return mpi_errno;

fn_fail:
    goto fn_exit;
}

/*
 * MPIDI_Comm_accept()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_Comm_accept
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Comm_accept(const char *port_name, MPID_Info *info, int root, 
		      MPID_Comm *comm_ptr, MPID_Comm **newcomm)
{
    int mpi_errno=MPI_SUCCESS;
    int p, j;
    int i, rank, recv_ints[2], send_ints[3];
    int remote_comm_size=0;
    MPID_Comm *tmp_comm = NULL, *intercomm;
    MPIDI_VC_t * vc, *new_vc = NULL;
    int n_local_pgs=1, n_remote_pgs;
    int sendtag=100, recvtag=100, local_comm_size;
    typedef struct pg_translation
    {
	int pg_index;
	int pg_rank;
    } pg_translation;
    pg_translation *local_translation = NULL, *remote_translation = NULL;
    typedef struct pg_node
    {
	int index;
	char *pg_id;
	char *str;
	struct pg_node *next;
    } pg_node;
    pg_node *pg_list = NULL, *pg_iter, *pg_trailer;
    int cur_index;
    MPIDI_PG_t **remote_pg = NULL;
    int flag;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_COMM_ACCEPT);
    MPIU_CHKLMEM_DECL(3);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_COMM_ACCEPT);

/* Algorithm: First dequeue the vc from the accept queue (it was
   enqueued by the progress engine in response to a connect request
   from the root on the connect side). Use this vc to create an
   intercommunicator between this root and the root on the connect
   side. Use this intercomm. to communicate the other information
   needed to create the real intercommunicator between the processes
   on the two sides. Then free the intercommunicator between the
   roots. Most of the complexity is because there can be multiple
   process groups on each side.*/ 


    /* Create the new intercommunicator here. We need to send the
       context id to the other side. */
    mpi_errno = MPIR_Comm_create(comm_ptr, newcomm);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }

    rank = comm_ptr->rank;
    local_comm_size = comm_ptr->local_size;
    
    if (rank == root)
    {
	/* Establish a communicator to communicate with the root on the other side. */
	mpi_errno = MPIDI_Create_inter_root_communicator(port_name, &tmp_comm, &new_vc);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_POP(mpi_errno);
	}

	/* Make an array to translate local ranks to process group index and 
	   rank */
	MPIU_CHKLMEM_MALLOC(local_translation,pg_translation*,
			    local_comm_size*sizeof(pg_translation),
			    mpi_errno,"local_translation");

	/* Make a list of the local communicator's process groups and encode 
	   them in strings to be sent to the other side.
	   The encoded string for each process group contains the process 
	   group id, size and all its KVS values */
	cur_index = 0;
	pg_list = (pg_node*)MPIU_Malloc(sizeof(pg_node));
	/* --BEGIN ERROR HANDLING-- */
	if (pg_list == NULL)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
            goto fn_exit;
	}
	/* --END ERROR HANDLING-- */
	pg_list->pg_id = MPIU_Strdup(comm_ptr->vcr[0]->pg->id);
	pg_list->index = cur_index++;
	pg_list->next = NULL;
	mpi_errno = MPIDI_PG_To_string(comm_ptr->vcr[0]->pg, &pg_list->str);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_POP(mpi_errno);
	}

	local_translation[0].pg_index = 0;
	local_translation[0].pg_rank = comm_ptr->vcr[0]->pg_rank;
	pg_iter = pg_list;
	for (i=1; i<local_comm_size; i++)
	{
	    pg_iter = pg_list;
	    pg_trailer = pg_list;
	    while (pg_iter != NULL)
	    {
		if (MPIDI_PG_Id_compare(comm_ptr->vcr[i]->pg->id, pg_iter->pg_id))
		/*if (strcmp(comm_ptr->vcr[i]->pg->id, pg_iter->pg_id) == 0)*/
		{
		    local_translation[i].pg_index = pg_iter->index;
		    local_translation[i].pg_rank = comm_ptr->vcr[i]->pg_rank;
		    break;
		}
		if (pg_trailer != pg_iter)
		    pg_trailer = pg_trailer->next;
		pg_iter = pg_iter->next;
	    }
	    if (pg_iter == NULL)
	    {
		pg_iter = (pg_node*)MPIU_Malloc(sizeof(pg_node));
		if (pg_iter == NULL) {
		    MPIU_ERR_POP(mpi_errno);
		}
		pg_iter->pg_id = MPIU_Strdup(comm_ptr->vcr[i]->pg->id);
		pg_iter->index = cur_index++;
		pg_iter->next = NULL;
		mpi_errno = MPIDI_PG_To_string(comm_ptr->vcr[i]->pg, &pg_iter->str);
		if (mpi_errno != MPI_SUCCESS) {
		    MPIU_ERR_POP(mpi_errno);
		}

		local_translation[i].pg_index = pg_iter->index;
		local_translation[i].pg_rank = comm_ptr->vcr[i]->pg_rank;
		pg_trailer->next = pg_iter;
	    }
	}
	n_local_pgs = cur_index;

#ifdef MPICH_DBG_OUTPUT
	pg_iter = pg_list;
	while (pg_iter != NULL)
	{
	    MPIU_DBG_PRINTF(("[%d]accept:PG: '%s'\n<%s>\n", rank, pg_iter->pg_id, pg_iter->str));
	    pg_iter = pg_iter->next;
	}
#endif

        /* Send the remote root: n_local_pgs, local_comm_size, context_id for 
	   newcomm.
           Recv from the remote root: n_remote_pgs, remote_comm_size */

        send_ints[0] = n_local_pgs;
        send_ints[1] = local_comm_size;
        send_ints[2] = (*newcomm)->context_id;

	/*printf("accept:sending 3 ints, %d, %d, %d, and receiving 2 ints\n", send_ints[0], send_ints[1], send_ints[2]);fflush(stdout);*/
        mpi_errno = MPIC_Sendrecv(send_ints, 3, MPI_INT, 0,
                                  sendtag++, recv_ints, 2, MPI_INT,
                                  0, recvtag++, tmp_comm->handle,
                                  MPI_STATUS_IGNORE);
        if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_POP(mpi_errno);
	}
    }

    /* broadcast the received info to local processes */
    /*printf("accept:broadcasting 2 ints - %d and %d\n", recv_ints[0], recv_ints[1]);fflush(stdout);*/
    mpi_errno = MPIR_Bcast(recv_ints, 2, MPI_INT, root, comm_ptr);
    if (mpi_errno) {
	MPIU_ERR_POP(mpi_errno);
    }

    n_remote_pgs = recv_ints[0];
    remote_comm_size = recv_ints[1];
    MPIU_CHKLMEM_MALLOC(remote_pg,MPIDI_PG_t**,
			n_remote_pgs * sizeof(MPIDI_PG_t*),
			mpi_errno,"remote_pg");
    MPIU_CHKLMEM_MALLOC(remote_translation,pg_translation*,
			remote_comm_size * sizeof(pg_translation),
			mpi_errno, "remote_translation");
    MPIU_DBG_PRINTF(("[%d]accept:remote process groups: %d\nremote comm size: %d\n", rank, n_remote_pgs, remote_comm_size));

    /* Exchange the process groups and their corresponding KVSes */
    if (rank == root)
    {
	char *pg_str;

	for (i=0; i<n_remote_pgs; i++)
	{
	    /* Receive the size and then the data */
	    /*printf("accept:receiving 1 int\n");fflush(stdout);*/
	    mpi_errno = MPIC_Recv(&j, 1, MPI_INT, 0, recvtag++, tmp_comm->handle, MPI_STATUS_IGNORE);
	    if (mpi_errno != MPI_SUCCESS) {
		MPIU_ERR_POP(mpi_errno);
	    }
	    pg_str = (char*)MPIU_Malloc(j);
	    if (pg_str == NULL)
	    {
		/* --BEGIN ERROR HANDLING-- */
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
		goto fn_exit;
		/* --END ERROR HANDLING-- */
	    }
	    /*printf("accept:receiving string of length %d\n", j);*/
	    mpi_errno = MPIC_Recv(pg_str, j, MPI_CHAR, 0, recvtag++, tmp_comm->handle, MPI_STATUS_IGNORE);
	    if (mpi_errno != MPI_SUCCESS) {
		MPIU_ERR_POP(mpi_errno);
	    }
	    /* Then broadcast the size and data to the local communicator */
	    /*printf("accept:broadcasting 1 int: %d\n", j);fflush(stdout);*/
	    mpi_errno = MPIR_Bcast(&j, 1, MPI_INT, root, comm_ptr);
	    if (mpi_errno != MPI_SUCCESS) {
		MPIU_ERR_POP(mpi_errno);
	    }
	    /*printf("accept:broadcasting string of length %d\n", j);fflush(stdout);*/
	    mpi_errno = MPIR_Bcast(pg_str, j, MPI_CHAR, root, comm_ptr);
	    if (mpi_errno != MPI_SUCCESS) {
		MPIU_ERR_POP(mpi_errno);
	    }
	    /* Then reconstruct the received process group */
	    mpi_errno = MPIDI_PG_Create_from_string(pg_str, &remote_pg[i], &flag);
	    if (mpi_errno != MPI_SUCCESS) {
		MPIU_ERR_POP(mpi_errno);
	    }

	    MPIU_Free(pg_str);
	    if (flag)
	    {
#ifdef MPIDI_CH3_USES_SSHM
		/* extra pg ref needed for shared memory modules because the shm_XXXXing_list's
		 * need to be walked though in the later stages of finalize to free queue_info's.
		 */
		MPIDI_PG_Add_ref(remote_pg[i]);
#endif
		for (p=0; p<remote_pg[i]->size; p++)
		{
		    MPIDI_PG_Get_vcr(remote_pg[i], p, &vc);
		    MPIDI_CH3_VC_Init( vc );
		}
	    }
	}

	while (pg_list != NULL)
	{
	    pg_iter = pg_list;
	    i = (int)(strlen(pg_iter->str) + 1);
	    /*printf("accept:sending 1 int: %d\n", i);fflush(stdout);*/
	    mpi_errno = MPIC_Send(&i, 1, MPI_INT, 0, sendtag++, tmp_comm->handle);
	    if (mpi_errno != MPI_SUCCESS) {
		MPIU_ERR_POP(mpi_errno);
	    }
	    /*printf("accept:sending string of length %d\n", i);fflush(stdout);*/
	    mpi_errno = MPIC_Send(pg_iter->str, i, MPI_CHAR, 0, sendtag++, tmp_comm->handle);
	    if (mpi_errno != MPI_SUCCESS) {
		MPIU_ERR_POP(mpi_errno);
	    }
	    pg_list = pg_list->next;
	    MPIU_Free(pg_iter->str);
	    MPIU_Free(pg_iter->pg_id);
	    MPIU_Free(pg_iter);
	}

	/* Receive the translations from remote process rank to process group index */
	/*printf("accept:sending %d ints and receiving %d ints\n", local_comm_size * 2, remote_comm_size * 2);fflush(stdout);*/
	mpi_errno = MPIC_Sendrecv(local_translation, local_comm_size * 2, MPI_INT, 0, sendtag++,
				  remote_translation, remote_comm_size * 2, MPI_INT, 0, recvtag++, tmp_comm->handle, MPI_STATUS_IGNORE);
#ifdef MPICH_DBG_OUTPUT
	MPIU_DBG_PRINTF(("[%d]accept:Received remote_translation:\n", rank));
	for (i=0; i<remote_comm_size; i++)
	{
	    MPIU_DBG_PRINTF((" remote_translation[%d].pg_index = %d\n remote_translation[%d].pg_rank = %d\n",
		i, remote_translation[i].pg_index, i, remote_translation[i].pg_rank));
	}
#endif
    }
    else
    {
	char *pg_str;
	for (i=0; i<n_remote_pgs; i++)
	{
	    /* Broadcast the size and data to the local communicator */
	    /*printf("accept:broadcasting 1 int\n");fflush(stdout);*/
	    mpi_errno = MPIR_Bcast(&j, 1, MPI_INT, root, comm_ptr);
	    if (mpi_errno != MPI_SUCCESS) {
		MPIU_ERR_POP(mpi_errno);
	    }
	    pg_str = (char*)MPIU_Malloc(j);
	    if (pg_str == NULL)
	    {
		/* --BEGIN ERROR HANDLING-- */
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
		goto fn_exit;
		/* --END ERROR HANDLING-- */
	    }
	    /*printf("accept:broadcasting string of length %d\n", j);fflush(stdout);*/
	    mpi_errno = MPIR_Bcast(pg_str, j, MPI_CHAR, root, comm_ptr);
	    if (mpi_errno != MPI_SUCCESS) {
		MPIU_ERR_POP(mpi_errno);
	    }
	    /* Then reconstruct the received process group */
	    mpi_errno = MPIDI_PG_Create_from_string(pg_str, &remote_pg[i], &flag);
	    if (mpi_errno != MPI_SUCCESS) {
		MPIU_ERR_POP(mpi_errno);
	    }

	    MPIU_Free(pg_str);
	    if (flag)
	    {
#ifdef MPIDI_CH3_USES_SSHM
		/* extra pg ref needed for shared memory modules because the shm_XXXXing_list's
		 * need to be walked though in the later stages of finalize to free queue_info's.
		 */
		MPIDI_PG_Add_ref(remote_pg[i]);
#endif
		for (p=0; p<remote_pg[i]->size; p++)
		{
		    MPIDI_PG_Get_vcr(remote_pg[i], p, &vc);
		    MPIDI_CH3_VC_Init( vc );
		}
	    }
	}
    }

    /* Broadcast out the remote rank translation array */
    /*printf("accept:broadcasting %d ints\n", remote_comm_size * 2);fflush(stdout);*/
    mpi_errno = MPIR_Bcast(remote_translation, remote_comm_size * 2, MPI_INT, root, comm_ptr);
#ifdef MPICH_DBG_OUTPUT
    MPIU_DBG_PRINTF(("[%d]accept:Received remote_translation after broadcast:\n", rank));
    for (i=0; i<remote_comm_size; i++)
    {
	MPIU_DBG_PRINTF((" remote_translation[%d].pg_index = %d\n remote_translation[%d].pg_rank = %d\n",
	    i, remote_translation[i].pg_index, i, remote_translation[i].pg_rank));
    }
#endif


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
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**init_vcrt");
    }

    mpi_errno = MPID_VCRT_Get_ptr(intercomm->vcrt, &intercomm->vcr);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**init_getptr");
    }

    for (i=0; i < intercomm->remote_size; i++)
    {
	MPIDI_PG_Get_vcr(remote_pg[remote_translation[i].pg_index], 
			 remote_translation[i].pg_rank, &vc);
        MPID_VCR_Dup(vc, &intercomm->vcr[i]);

    }

    /*printf("accept:barrier\n");fflush(stdout);*/
    mpi_errno = MPIR_Barrier(comm_ptr);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }

    /* synchronize with remote root */
    if (rank == root)
    {
	/*printf("accept:sending and receiving 0 ints (barrier?)\n");fflush(stdout);*/
        mpi_errno = MPIC_Sendrecv(&i, 0, MPI_INT, 0,
                                  sendtag++, &j, 0, MPI_INT,
                                  0, recvtag++, tmp_comm->handle,
                                  MPI_STATUS_IGNORE);
        if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_POP(mpi_errno);
        }

        /* All communication with remote root done. Release the communicator. */
        MPIR_Comm_release(tmp_comm);
    }

    /*printf("accept:barrier\n");fflush(stdout);*/
    mpi_errno = MPIR_Barrier(comm_ptr);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }

    /* Free new_vc. It was explicitly allocated in ch3_progress.c and returned by 
       MPIDI_CH3I_Acceptq_dequeue. */
    if (rank == root)
    {
        MPID_Progress_state progress_state;

        if (new_vc->state != MPIDI_VC_STATE_INACTIVE)
	{
            MPID_Progress_start(&progress_state);
            while (new_vc->state != MPIDI_VC_STATE_INACTIVE)
	    {
                mpi_errno = MPID_Progress_wait(&progress_state);
                /* --BEGIN ERROR HANDLING-- */
                if (mpi_errno != MPI_SUCCESS)
                {
                    MPID_Progress_end(&progress_state);
		    MPIU_ERR_POP(mpi_errno);
                }
                /* --END ERROR HANDLING-- */
            }
            MPID_Progress_end(&progress_state);
        }

#ifdef MPIDI_CH3_HAS_CONN_ACCEPT_HOOK
	mpi_errno = MPIDI_CH3_Cleanup_after_connection( new_vc );
#endif
        MPIU_Free(new_vc);
    }

fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_COMM_ACCEPT);
    return mpi_errno;

fn_fail:
    goto fn_exit;
}

#endif
