/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */


#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_wait
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_wait(struct MPIDU_Sock_set * sock_set, int millisecond_timeout, struct MPIDU_Sock_event * eventp)
{
    int elem;
    int nfds;
    int found_active_elem = FALSE;
    int mpi_errno2;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_WAIT);
    MPIDI_STATE_DECL(MPID_STATE_POLL);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_WAIT);

    for (;;)
    { 
	if (MPIDU_Socki_event_dequeue(sock_set, eventp) == MPI_SUCCESS)
	{
	    break;
	}

	do
	{
	    MPIDI_FUNC_ENTER(MPID_STATE_POLL);
	    nfds = poll(sock_set->pollfds, sock_set->poll_n_elem, millisecond_timeout);
	    MPIDI_FUNC_EXIT(MPID_STATE_POLL);
	}
	while (nfds < 0 && errno == EINTR);

	if (nfds == 0)
	{
	    mpi_errno = MPIDU_SOCK_ERR_TIMEOUT;
	    break;
	}

	if (nfds == -1) 
	{
	    if (errno == ENOMEM)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM,
						 "**sock|osnomem", NULL);
	    }
	    else if (errno == EBADF)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
						 "**sock|badhandle", NULL);
	    }
	    else
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
						 "**sock|oserror", "**sock|poll|oserror %d", errno);
	    }

	    goto fn_exit;
	}
	    
	elem = sock_set->starting_elem;
	while (nfds > 0)
	{
	    struct pollfd * const pollfd = &sock_set->pollfds[elem];
	    struct pollinfo * const pollinfo = &sock_set->pollinfos[elem];
	
	    if (pollfd->revents == 0)
	    {
		/* This optimization assumes that most FDs will not have a pending event. */
		elem = (elem + 1 < sock_set->poll_n_elem) ? elem + 1 : 0;
		continue;
	    }

	    if (found_active_elem == FALSE)
	    {
		found_active_elem = TRUE;
		sock_set->starting_elem = (elem + 1 < sock_set->poll_n_elem) ? elem + 1 : 0;
	    }

	    if (pollfd->revents & POLLNVAL)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
						 "**sock|badhandle", "**sock|poll|badhandle %d %d %d",
						 pollinfo->sock_set->id, pollinfo->sock_id, pollfd->fd);
		pollinfo->state = MPIDU_SOCKI_STATE_CONN_FAILED;
		pollfd->fd = -1;
		pollinfo->fd = -1;
		goto fn_exit;
	    }
	
	    if (pollfd->revents & POLLHUP)
	    {
		close(pollfd->fd);
		pollfd->fd = -1;
		pollinfo->fd = -1;

		if (pollinfo->state == MPIDU_SOCKI_STATE_CONNECTED)
		{ 
		    /* FIXME: might there still be data in the socket buffer?  for now we will assume not. */
		    pollinfo->state = MPIDU_SOCKI_STATE_CONN_CLOSED;
		    mpi_errno2 = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
						      MPIDU_SOCK_ERR_CONN_CLOSED, "**sock|connclosed", "**sock|connclosed %d %d",
						      pollinfo->sock_set->id, pollinfo->sock_id);
		    MPIDU_SOCKI_HANDLE_RW_ERR(pollfd, pollinfo, mpi_errno2, fn_exit);
		}
		else if (pollinfo->state == MPIDU_SOCKI_STATE_CONNECTING)
		{
		    /* I don't even know if this case is possible... */
		    pollinfo->state = MPIDU_SOCKI_STATE_CONN_FAILED;
		    mpi_errno2 = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
						      MPIDU_SOCK_ERR_CONN_FAILED, "**sock|connfailed", "**sock|connfailed %d %d",
						      pollinfo->sock_set->id, pollinfo->sock_id);
		    MPIDU_SOCKI_HANDLE_CONNECT_ERR(pollfd, pollinfo, mpi_errno2, fn_exit);
		}
		else
		{
		    pollinfo->state = MPIDU_SOCKI_STATE_CONN_FAILED;
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
						     "**sock|poll|unhandledstate", "**sock|poll|unhandledstate %d %s",
						     pollinfo->state, "POLLHUP");
		    goto fn_exit;
		}

		goto nfds_loop_continue;
	    }

	    if (pollfd->revents & POLLERR)
	    {
		close(pollfd->fd);
		pollfd->fd = -1;
		pollinfo->fd = -1;
		pollinfo->state = MPIDU_SOCKI_STATE_CONN_FAILED;

		if (pollinfo->state == MPIDU_SOCKI_STATE_CONNECTED)
		{ 
		    /* FIXME: right now we assume an error is a connection failure.  we should probably do a read() to get the
		       errno and do a better job of reporting. */
		    mpi_errno2 = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
						      MPIDU_SOCK_ERR_CONN_FAILED, "**sock|connfailed", "**sock|connfailed %d %d",
						      pollinfo->sock_set->id, pollinfo->sock_id);
		    MPIDU_SOCKI_HANDLE_RW_ERR(pollfd, pollinfo, mpi_errno2, fn_exit);
		}
		else if (pollinfo->state == MPIDU_SOCKI_STATE_CONNECTING)
		{
		    mpi_errno2 = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
						      MPIDU_SOCK_ERR_CONN_FAILED, "**sock|connfailed", "**sock|connfailed %d %d",
						      pollinfo->sock_set->id, pollinfo->sock_id);
		    MPIDU_SOCKI_HANDLE_CONNECT_ERR(pollfd, pollinfo, mpi_errno2, fn_exit);
		}
		else
		{
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
						     "**sock|poll|unhandledstate", "**sock|poll|unhandledstate %d %s",
						     pollinfo->state, "POLLERR");
		    goto fn_exit;
		}

		goto nfds_loop_continue;
	    }
	    
	    /* According to Stevens, some errors are reported as normal data and some are reported with POLLERR. */
	    if (pollfd->revents & POLLIN)
	    {
		if (pollinfo->state == MPIDU_SOCKI_STATE_CONNECTED)
		{
		    mpi_errno = MPIDU_Socki_read(pollfd, pollinfo);
		    if (mpi_errno != MPI_SUCCESS)
		    {
			goto fn_exit;
		    }
		}
		else if (pollinfo->state == MPIDU_SOCKI_STATE_LISTENER)
		{
		    MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo->sock_set, MPIDU_SOCK_OP_ACCEPT, 0, pollinfo->user_ptr,
					      MPI_SUCCESS, fn_exit);
		}
		else if (pollinfo->state == MPIDU_SOCKI_STATE_INTERRUPTER)
		{
		    char c;
		    int nb;

		    do
		    {
			nb = read(pollfd->fd, &c, 1);
		    }
		    while (nb > 0 || errno == EINTR);
		}
		else
		{
		    /* MT: we may arrive here because of an error that occurred during the use of the immediate functions */
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
						     "**sock|poll|unhandledstate", "**sock|poll|unhandledstate %d %s",
						     pollinfo->state, "POLLIN");
		    goto fn_exit;
		}
	    }

	    if (pollfd->revents & POLLOUT)
	    {
		if (pollinfo->state == MPIDU_SOCKI_STATE_CONNECTED)
		{
		    mpi_errno = MPIDU_Socki_write(pollfd, pollinfo);
		    if (mpi_errno != MPI_SUCCESS)
		    {
			goto fn_exit;
		    }
		}
		else if (pollinfo->state == MPIDU_SOCKI_STATE_CONNECTING)
		{
		    struct sockaddr_in addr;
		    socklen_t addr_len;
		    int rc;
		    
		    pollfd->events &= ~POLLOUT;
    
		    addr_len = sizeof(struct sockaddr_in);
		    rc = getpeername(pollfd->fd, (struct sockaddr *) &addr, &addr_len);
		    if (rc == 0)
		    {
			pollinfo->state = MPIDU_SOCKI_STATE_CONNECTED;
			MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo->sock_set, MPIDU_SOCK_OP_CONNECT, 0, pollinfo->user_ptr,
						  MPI_SUCCESS, fn_exit);
		    }
		    else
		    {
			/* FIXME: if getpeername() returns ENOTCONN, then we can now use getsockopt() to get the errno associated
			   with the failed connect(). */
			pollinfo->state = MPIDU_SOCKI_STATE_CONN_FAILED;
			mpi_errno2 = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
							  MPIDU_SOCK_ERR_CONN_FAILED, "**sock|connfailed",
							  "**sock|connfailed %d %d", pollinfo->sock_set->id, pollinfo->sock_id);
			MPIDU_SOCKI_HANDLE_CONNECT_ERR(pollfd, pollinfo, mpi_errno2, fn_exit);
		    }
		}
		else
		{
		    /* MT: we may arrive here because of an error that occurred during the use of the immediate functions */
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
						     "**sock|poll|unhandledstate", "**sock|poll|unhandledstate %d %s",
						     pollinfo->state, "POLLOUT");
		    goto fn_exit;
		}
	    }

	  nfds_loop_continue:
	    nfds--;
	    elem = (elem + 1 < sock_set->poll_n_elem) ? elem + 1 : 0;
	}
    }
    
  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
    return mpi_errno;
}
