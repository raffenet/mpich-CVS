/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_create_set
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_create_set(struct MPIDU_Sock_set ** sock_setp)
{
    struct MPIDU_Sock_set * sock_set = NULL;
    struct MPIDU_Sock * sock = NULL;
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    long flags;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_CREATE_SET);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_CREATE_SET);
    
    MPIDU_SOCKI_VERIFY_INIT(mpi_errno);

    /* alloc and initialized a new sock set structure */
    sock_set = MPIU_Malloc(sizeof(struct MPIDU_Sock_set));
    if (sock_set == NULL)
    { 
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM,
					 "**sock|setalloc", 0);
	goto fn_fail;
    }
    
    sock_set->poll_arr_sz = 0;
    sock_set->poll_n_elem = -1;
    sock_set->starting_elem = 0;
    sock_set->pollfds = NULL;
    sock_set->pollinfos = NULL;
    sock_set->eventq_head = NULL;
    sock_set->eventq_tail = NULL;
    sock_set->intr_fds[0] = -1;
    sock_set->intr_fds[1] = -1;
    sock_set->intr_sock = NULL;

    /* acquire a pipe to wake up a blocking poll should it become necessary */
    rc = pipe(sock_set->intr_fds);
    if (rc != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					 "**sock|poll|pipe", "**sock|poll|pipe %d", errno);
	goto fn_fail;
    }

    flags = fcntl(sock_set->intr_fds[0], F_GETFL, 0);
    if (flags == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					 "**sock|poll|pipenonblock", "**sock|poll|pipenonblock %d", errno);
	goto fn_fail;
    }
    
    rc = fcntl(sock_set->intr_fds[0], F_SETFL, flags | O_NONBLOCK);
    if (rc == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					 "**sock|poll|pipenonblock", "**sock|poll|pipenonblock %d", errno);
	goto fn_fail;
    }
    
    flags = fcntl(sock_set->intr_fds[1], F_GETFL, 0);
    if (flags == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					 "**sock|poll|pipenonblock", "**sock|poll|pipenonblock %d", errno);
	goto fn_fail;
    }
    
    rc = fcntl(sock_set->intr_fds[1], F_SETFL, flags | O_NONBLOCK);
    if (rc == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					 "**sock|poll|pipenonblock", "**sock|poll|pipenonblock %d", errno);
	goto fn_fail;
    }

    /* allocate and initialize a sock structure for the pipe */
    mpi_errno = MPIDU_Socki_sock_alloc(sock_set, &sock);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM,
					 "**sock|sockalloc", NULL);
	goto fn_fail;
    }
    
    pollfd = MPIDU_Socki_get_pollfd_ptr(sock);
    pollinfo = MPIDU_Socki_get_pollinfo_ptr(sock);
    
    pollfd->fd = sock_set->intr_fds[0];
    pollfd->events = POLLIN;
    pollfd->revents = 0;
    pollinfo->user_ptr = NULL;
    pollinfo->state = MPIDU_SOCKI_STATE_INTERRUPTER;
    sock_set->intr_sock = sock;
    /* MT: must be atomically incremented */
    sock_set->id = MPIDU_Socki_set_next_id++;
    
    *sock_setp = sock_set;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_CREATE_SET);
    return mpi_errno;

  fn_fail:
    if (sock_set != NULL)
    {
	if (sock_set->intr_fds[0] != -1)
	{
	    close(sock_set->intr_fds[0]);
	}
	
	if (sock_set->intr_fds[1] != -1)
	{
	    close(sock_set->intr_fds[1]);
	}
	
	MPIU_Free(sock_set);
    }

    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_destroy_set
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_destroy_set(struct MPIDU_Sock_set * sock_set)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_DESTROY_SET);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_DESTROY_SET);

    MPIDU_SOCKI_VERIFY_INIT(mpi_errno);

    pollfd = MPIDU_Socki_get_pollfd_ptr(sock_set->intr_sock);
    pollinfo = MPIDU_Socki_get_pollinfo_ptr(sock_set->intr_sock);

    /* close pipe for interrupting a blocking poll() */
    pollinfo->state = MPIDU_SOCKI_STATE_UNCONNECTED;
    pollfd->fd = -1;
    pollfd->events = 0;
    close(sock_set->intr_fds[1]);
    close(sock_set->intr_fds[0]);
    MPIDU_Socki_sock_free(sock_set->intr_sock);

    MPIU_Free(sock_set->pollinfos);
    MPIU_Free(sock_set->pollfds);
    MPIU_Free(sock_set);

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_DESTROY_SET);
    return mpi_errno;
}
