/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */


static int MPIDU_Socki_handle_pollhup(struct pollfd * const pollfd, struct pollinfo * const pollinfo);
static int MPIDU_Socki_handle_pollerr(struct pollfd * const pollfd, struct pollinfo * const pollinfo);
static int MPIDU_Socki_handle_read(struct pollfd * const pollfd, struct pollinfo * const pollinfo);
static int MPIDU_Socki_handle_write(struct pollfd * const pollfd, struct pollinfo * const pollinfo);
static int MPIDU_Socki_handle_connect(struct pollfd * const pollfd, struct pollinfo * const pollinfo);

/*
 * MPIDU_Sock_wait()
 *
 * NOTES:
 *
 * For fatal errors, the state of the connection progresses directly to the failed state and the connection is marked inactive in
 * the poll array.  Under normal conditions, the fatal error should result in the termination of the process; but, if that
 * doesn't happen, we try to leave the implementation in a somewhat sane state.
 */
#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_wait
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_wait(struct MPIDU_Sock_set * sock_set, int millisecond_timeout, struct MPIDU_Sock_event * eventp)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_WAIT);
    MPIDI_STATE_DECL(MPID_STATE_POLL);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_WAIT);

    for (;;)
    { 
	int elem;
	int nfds;
	int found_active_elem = FALSE;
	
	if (MPIDU_Socki_event_dequeue(sock_set, &elem, eventp) == MPI_SUCCESS)
	{
	    struct pollinfo * pollinfo;
	    int flags;
	    
	    if (eventp->op_type != MPIDU_SOCK_OP_CLOSE)
	    {
		break;
	    }

	    pollinfo = &sock_set->pollinfos[elem];

	    /*
	     * Attempt to set socket back to blocking.  This *should* prevent any data in the socket send buffer from being
	     * discarded.  Instead close() will block until the buffer is flushed or the connection timeouts and is considered
	     * lost.  Theoretically, this could cause the MPIDU_Sock_wait() to hang indefinitely; however, the calling code
	     * should ensure this will not happen by going through a shutdown protocol before posting a close operation.
	     *
	     * FIXME: If the attempt to set the socket back to blocking fails, we presently ignore it.  Should we return an
	     * error?  We need to define acceptible data loss at close time.  MS Windows has worse problems with this, so it
	     * may not be possible to make any guarantees.
	     */
	    flags = fcntl(pollinfo->fd, F_GETFL, 0);
	    if (flags != -1)
	    { 
		fcntl(pollinfo->fd, F_SETFL, flags & ~O_NONBLOCK);
	    }

	    /* FIXME: return code?  If an error occurs do we return it instead of the error specified in the event? */
	    close(pollinfo->fd);

	    MPIDU_Socki_sock_free(pollinfo->sock);

	    break;
	}

	for(;;)
	{
	    MPIDI_FUNC_ENTER(MPID_STATE_POLL);
	    nfds = poll(sock_set->pollfds, sock_set->poll_n_elem, millisecond_timeout);
	    MPIDI_FUNC_EXIT(MPID_STATE_POLL);

	    if (nfds > 0)
	    {
		break;
	    }
	    else if (nfds == 0)
	    {
		mpi_errno = MPIDU_SOCK_ERR_TIMEOUT;
		goto fn_exit;
	    }
	    else if (errno == EINTR)
	    {
		if (millisecond_timeout != MPIDU_SOCK_INFINITE_TIME)
		{
		    mpi_errno = MPIDU_SOCK_ERR_TIMEOUT;
		    goto fn_exit;
		}
	    }
	    if (errno == ENOMEM || errno == EAGAIN)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM,
						 "**sock|osnomem", NULL);
		goto fn_exit;
	    }
	    else
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
						 "**sock|oserror", "**sock|poll|oserror %d %s", errno, MPIU_Strerror(errno));
		goto fn_exit;
	    }
	}

	elem = sock_set->starting_elem;
	while (nfds > 0)
	{
	    struct pollfd * const pollfd = &sock_set->pollfds[elem];
	    struct pollinfo * const pollinfo = &sock_set->pollinfos[elem];
	
	    MPIU_Assert((pollfd->events & (POLLIN | POLLOUT)) || pollfd->fd == -1);
	    MPIU_Assert(pollfd->fd >= 0 || pollfd->fd == -1);
			
	    if (pollfd->fd < 0 || pollfd->revents == 0)
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
		mpi_errno = MPIR_Err_create_code(
		    MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**sock|badhandle",
		    "**sock|poll|badhandle %d %d %d %d", pollinfo->sock_set->id, pollinfo->sock_id, pollfd->fd, pollinfo->fd);
		goto fn_exit;
	    }
	
	    if (pollfd->revents & POLLHUP)
	    {
		mpi_errno = MPIDU_Socki_handle_pollhup(pollfd, pollinfo);
		if (MPIR_Err_is_fatal(mpi_errno))
		{
		    goto fn_exit;
		}
	    }

	    /* According to Stevens, some errors are reported as normal data (POLLIN) and some are reported with POLLERR. */
	    if (pollfd->revents & POLLERR)
	    {
		mpi_errno = MPIDU_Socki_handle_pollerr(pollfd, pollinfo);
		if (MPIR_Err_is_fatal(mpi_errno))
		{
		    goto fn_exit;
		}
	    }
	    
	    if (pollfd->revents & POLLIN)
	    {
		if (pollinfo->type == MPIDU_SOCKI_TYPE_COMMUNICATION)
		{ 
		    if (pollinfo->state == MPIDU_SOCKI_STATE_CONNECTED_RW || pollinfo->state == MPIDU_SOCKI_STATE_CONNECTED_RO)
		    {
			mpi_errno = MPIDU_Socki_handle_read(pollfd, pollinfo);
			if (MPIR_Err_is_fatal(mpi_errno))
			{
			    goto fn_exit;
			}
		    }
		    else
		    {
			mpi_errno = MPIR_Err_create_code(
			    MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**sock|poll|unhandledstate",
			    "**sock|poll|unhandledstate %d", pollinfo->state);
			goto fn_exit;
		    }

		}
		else if (pollinfo->type == MPIDU_SOCKI_TYPE_LISTENER)
		{
		    pollfd->events &= ~POLLIN;
		    MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDU_SOCK_OP_ACCEPT, 0, pollinfo->user_ptr,
					      MPI_SUCCESS, mpi_errno, fn_exit);
		}
		else if (pollinfo->type == MPIDU_SOCKI_TYPE_INTERRUPTER)
		{
		    char c[16];
		    int nb;

		    do
		    {
			nb = read(pollfd->fd, c, 16);
		    }
		    while (nb > 0 || (nb < 0 && errno == EINTR));
		}
		else
		{
		    mpi_errno = MPIR_Err_create_code(
			MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**sock|poll|unhandledtype",
			"**sock|poll|unhandledtype %d", pollinfo->type);
		    goto fn_exit;
		}
	    }

	    if (pollfd->revents & POLLOUT)
	    {
		if (pollinfo->type == MPIDU_SOCKI_TYPE_COMMUNICATION)
		{
		    if (pollinfo->state == MPIDU_SOCKI_STATE_CONNECTED_RW)
		    {
			mpi_errno = MPIDU_Socki_handle_write(pollfd, pollinfo);
			if (MPIR_Err_is_fatal(mpi_errno))
			{
			    goto fn_exit;
			}
		    }
		    else if (pollinfo->state == MPIDU_SOCKI_STATE_CONNECTING)
		    {
			mpi_errno = MPIDU_Socki_handle_connect(pollfd, pollinfo);
			if (MPIR_Err_is_fatal(mpi_errno))
			{
			    goto fn_exit;
			}
		    }
		    else
		    {
			mpi_errno = MPIR_Err_create_code(
			    MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**sock|poll|unhandledstate",
			    "**sock|poll|unhandledstate %d", pollinfo->state);
			goto fn_exit;
		    }
		}
		else
		{
		    mpi_errno = MPIR_Err_create_code(
			MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**sock|poll|unhandledtype",
			"**sock|poll|unhandledtype %d", pollinfo->type);
		    goto fn_exit;
		}
	    }

	    nfds--;
	    elem = (elem + 1 < sock_set->poll_n_elem) ? elem + 1 : 0;
	}
    }
    
  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Socki_handle_pollhup
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int MPIDU_Socki_handle_pollhup(struct pollfd * const pollfd, struct pollinfo * const pollinfo)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCKI_HANDLE_POLLHUP);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCKI_HANDLE_POLLHUP);
    
    if (pollinfo->state == MPIDU_SOCKI_STATE_CONNECTED_RW)
    {
	/*
	 * If a write was posted then cancel it and generate an connection closed event.  If a read is posted, it will be handled
	 * by the POLLIN handler.
	 */
	if (pollfd->events & POLLOUT)
	{
	    int event_mpi_errno;
	    
	    event_mpi_errno = MPIR_Err_create_code(
		MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_CONN_CLOSED,
		"**sock|connclosed", "**sock|connclosed %d %d", pollinfo->sock_set->id, pollinfo->sock_id);
	    MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDU_SOCK_OP_WRITE, pollinfo->write_nb, pollinfo->user_ptr,
				      event_mpi_errno, mpi_errno, fn_exit);
	    MPIDU_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLOUT);
	    pollinfo->state = MPIDU_SOCKI_STATE_CONNECTED_RO;
	}
    }
    else if (pollinfo->state == MPIDU_SOCKI_STATE_CONNECTED_RO)
    {
	/*
	 * If we are in the read-only state, then we should only get an error if we are looking to read data.  If we are not
	 * reading data, then pollfd->fd should be set to -1 and we should not be getting a POLLHUP event.
	 *
	 * There may still be data in the socket buffer, so we will let the POLLIN handler deal with the error.  Once all of the
	 * data has been read, the POLLIN handler will change the connection state and remove the connection from the active poll
	 * list.
	 */
	MPIU_Assert(pollinfo->state == MPIDU_SOCKI_STATE_CONNECTED_RO && (pollfd->events & POLLIN) && (pollfd->revents & POLLIN));
    }
    else if (pollinfo->state == MPIDU_SOCKI_STATE_DISCONNECTED)
    {
	/*
	 * We should never reach this state because pollfd->fd should be set to -1 if we are in the disconnected state.
	 */
	MPIU_Assert(pollinfo->state == MPIDU_SOCKI_STATE_DISCONNECTED && pollfd->fd == -1);
    }
    else if (pollinfo->state == MPIDU_SOCKI_STATE_CONNECTING)
    {
	/*
	 * The process we were connecting to died.  Let the POLLOUT handler deal with the error.
	 */
	MPIU_Assert(pollinfo->state == MPIDU_SOCKI_STATE_CONNECTING && (pollfd->events & POLLOUT));
	pollfd->revents = POLLOUT;
    }
    else
    {
	mpi_errno = MPIR_Err_create_code(
	    MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**sock|poll|unhandledstate",
	    "**sock|poll|unhandledstate %d", pollinfo->state);
	goto fn_exit;
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCKI_HANDLE_POLLHUP);
    return mpi_errno;
}
/* end MPIDU_Socki_handle_pollhup() */


#undef FUNCNAME
#define FUNCNAME MPIDU_Socki_handle_pollerr
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int MPIDU_Socki_handle_pollerr(struct pollfd * const pollfd, struct pollinfo * const pollinfo)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCKI_HANDLE_POLLERR);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCKI_HANDLE_POLLERR);
    
    if (pollinfo->type != MPIDU_SOCKI_TYPE_COMMUNICATION)
    { 
	mpi_errno = MPIR_Err_create_code(
	    MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**sock|poll|unhandledtype",
	    "**sock|poll|unhandledtype %d", pollinfo->type);
	goto fn_exit;
    }
		
    if (pollinfo->state == MPIDU_SOCKI_STATE_CONNECTED_RW)
    { 
	/*
	 * Stevens suggests that some older version of UNIX did not properly reset so_error, which could allow POLLERR to be
	 * continuously triggered.  We remove the socket from the poll list (pollfd->fd = 1) in order to prevent this issue.
	 * Here, we simple check that things are as we expect them to be.
	 */
	MPIU_Assert((pollfd->events & (POLLIN | POLLOUT)) || pollfd->fd == -1);

	/* If a write was posted then cancel it and generate an write completion event */
	if (pollfd->events & POLLOUT)
	{
	    int disconnected;
	    int os_errno;
	    int event_mpi_errno;
	    
	    MPIDU_SOCKI_GET_SOCKET_ERROR(pollinfo, os_errno, mpi_errno, fn_exit);
		
	    event_mpi_errno = MPIDU_Socki_os_to_mpi_errno(pollinfo, os_errno, FCNAME, __LINE__, &disconnected);
	    if (MPIR_Err_is_fatal(event_mpi_errno))
	    {
		mpi_errno = event_mpi_errno;
		goto fn_exit;
	    }
			
	    MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDU_SOCK_OP_WRITE, pollinfo->write_nb, pollinfo->user_ptr,
				      event_mpi_errno, mpi_errno, fn_exit);
	    MPIDU_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLOUT);
	    pollinfo->state = MPIDU_SOCKI_STATE_CONNECTED_RO;
	}
    }
    else if (pollinfo->state == MPIDU_SOCKI_STATE_CONNECTED_RO)
    {
	/*
	 * If we are in the read-only state, then we should only get an error if we are looking to read data.  If we are not
	 * reading data, then pollfd->fd should be set to -1 and we should not be getting a POLLERR event.  
	 *
	 * There may still be data in the socket buffer, so we will let the POLLIN handler deal with the error.  Once all of the
	 * data has been read, the POLLIN handler will change the connection state and remove the connection from the active poll
	 * list.
	 */
	MPIU_Assert(pollinfo->state == MPIDU_SOCKI_STATE_CONNECTED_RO && (pollfd->events & POLLIN) && (pollfd->revents & POLLIN));
    }
    else if (pollinfo->state == MPIDU_SOCKI_STATE_CONNECTING)
    {
	/*
	 * The process we were connecting to died.  Let the POLLOUT handler deal with the error.
	 */
	MPIU_Assert(pollinfo->state == MPIDU_SOCKI_STATE_CONNECTING && (pollfd->events & POLLOUT));
	pollfd->revents = POLLOUT;
    }
    else if (pollinfo->state == MPIDU_SOCKI_STATE_DISCONNECTED)
    {
	/* We are already disconnected!  Why are we handling an error? */
	MPIU_Assert(pollfd->fd == -1);
    }
    else
    {
	mpi_errno = MPIR_Err_create_code(
	    MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**sock|poll|unhandledstate",
	    "**sock|poll|unhandledstate %d", pollinfo->state);
	goto fn_exit;
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCKI_HANDLE_POLLERR);
    return mpi_errno;
}
/* end MPIDU_Socki_handle_pollerr() */


#undef FUNCNAME
#define FUNCNAME MPIDU_Socki_handle_read
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int MPIDU_Socki_handle_read(struct pollfd * const pollfd, struct pollinfo * const pollinfo)
{
    int nb;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_READ);
    MPIDI_STATE_DECL(MPID_STATE_READV);
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCKI_HANDLE_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCKI_HANDLE_READ);

    do
    {
	if (pollinfo->read_iov_flag)
	{ 
	    MPIDI_FUNC_ENTER(MPID_STATE_READV);
	    nb = readv(pollinfo->fd, pollinfo->read.iov.ptr + pollinfo->read.iov.offset,
		       pollinfo->read.iov.count - pollinfo->read.iov.offset);
	    MPIDI_FUNC_EXIT(MPID_STATE_READV);
	}
	else
	{
	    MPIDI_FUNC_ENTER(MPID_STATE_READ);
	    nb = read(pollinfo->fd, pollinfo->read.buf.ptr + pollinfo->read_nb,
		      pollinfo->read.buf.max - pollinfo->read_nb);
	    MPIDI_FUNC_EXIT(MPID_STATE_READ);
	}
    }
    while (nb < 0 && errno == EINTR);
	
    if (nb > 0)
    {
	int done;
	    
	pollinfo->read_nb += nb;

	done = pollinfo->read_iov_flag ?
	    MPIDU_Socki_adjust_iov(nb, pollinfo->read.iov.ptr, pollinfo->read.iov.count, &pollinfo->read.iov.offset) :
	    (pollinfo->read_nb >= pollinfo->read.buf.min);

	if (done)
	{
	    MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDU_SOCK_OP_READ, pollinfo->read_nb, pollinfo->user_ptr,
				      MPI_SUCCESS, mpi_errno, fn_exit);
	    MPIDU_SOCKI_POLLFD_OP_CLEAR(pollfd, pollfd, POLLIN);
	}
    }
    else if (nb == 0)
    {
	int event_mpi_errno;
	
	event_mpi_errno = MPIR_Err_create_code(
	    MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_CONN_CLOSED, "**sock|connclosed",
	    "**sock|connclosed %d %d", pollinfo->sock_set->id, pollinfo->sock_id);
	if (MPIDU_SOCKI_POLLFD_OP_ISSET(pollfd, pollinfo, POLLOUT))
	{ 
	    MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDU_SOCK_OP_WRITE, pollinfo->write_nb, pollinfo->user_ptr,
				      event_mpi_errno, mpi_errno, fn_exit);
	}
	MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDU_SOCK_OP_READ, pollinfo->read_nb, pollinfo->user_ptr,
				  event_mpi_errno, mpi_errno, fn_exit);
	
	MPIDU_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLIN | POLLOUT);
	pollinfo->state = MPIDU_SOCKI_STATE_DISCONNECTED;

    }
    else if (errno == EAGAIN && errno == EWOULDBLOCK)
    {
	/* do nothing... */
	goto fn_exit;
    }
    else
    {
	int disconnected;
	int event_mpi_errno;
	
	event_mpi_errno = MPIDU_Socki_os_to_mpi_errno(pollinfo, errno, FCNAME, __LINE__, &disconnected);
	if (MPIR_Err_is_fatal(event_mpi_errno))
	{
	    /*
	     * A serious error occurred.  There is no guarantee that the data structures are still intact.  Therefore, we avoid
	     * modifying them.
	     */
	    mpi_errno = event_mpi_errno;
	    goto fn_exit;
	}

	if (disconnected)
	{
	    if (MPIDU_SOCKI_POLLFD_OP_ISSET(pollfd, pollinfo, POLLOUT))
	    { 
		MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDU_SOCK_OP_WRITE, pollinfo->write_nb, pollinfo->user_ptr,
					  event_mpi_errno, mpi_errno, fn_exit);
		MPIDU_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLOUT);
	    }
	    
	    pollinfo->state = MPIDU_SOCKI_STATE_DISCONNECTED;
	}
	
	MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDU_SOCK_OP_READ, pollinfo->read_nb, pollinfo->user_ptr,
				  event_mpi_errno, mpi_errno, fn_exit);
	MPIDU_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLIN);
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCKI_HANDLE_READ);
    return mpi_errno;
}
/* end MPIDU_Socki_handle_read() */


#undef FUNCNAME
#define FUNCNAME MPIDU_Socki_handle_write
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int MPIDU_Socki_handle_write(struct pollfd * const pollfd, struct pollinfo * const pollinfo)
{
    int nb;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_WRITE);
    MPIDI_STATE_DECL(MPID_STATE_WRITEV);
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCKI_HANDLE_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCKI_HANDLE_WRITE);

    do
    {
	if (pollinfo->write_iov_flag)
	{ 
	    MPIDI_FUNC_ENTER(MPID_STATE_WRITEV);
	    nb = writev(pollinfo->fd, pollinfo->write.iov.ptr + pollinfo->write.iov.offset,
			pollinfo->write.iov.count - pollinfo->write.iov.offset);
	    MPIDI_FUNC_EXIT(MPID_STATE_WRITEV);
	}
	else
	{
	    MPIDI_FUNC_ENTER(MPID_STATE_WRITE);
	    nb = write(pollinfo->fd, pollinfo->write.buf.ptr + pollinfo->write_nb,
		       pollinfo->write.buf.max - pollinfo->write_nb);
	    MPIDI_FUNC_EXIT(MPID_STATE_WRITE);
	}
    }
    while (nb < 0 && errno == EINTR);

    if (nb >= 0)
    {
	int done;
	    
	pollinfo->write_nb += nb;

	done = pollinfo->write_iov_flag ?
	    MPIDU_Socki_adjust_iov(nb, pollinfo->write.iov.ptr, pollinfo->write.iov.count, &pollinfo->write.iov.offset) :
	    (pollinfo->write_nb >= pollinfo->write.buf.min);

	if (done)
	{
	    MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDU_SOCK_OP_WRITE, pollinfo->write_nb, pollinfo->user_ptr,
				      MPI_SUCCESS, mpi_errno, fn_exit);
	    MPIDU_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLOUT);
	}
    }
    else if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
	/* do nothing... */
	goto fn_exit;
    }
    else
    {
	int disconnected;
	int event_mpi_errno;
	
	event_mpi_errno = MPIDU_Socki_os_to_mpi_errno(pollinfo, errno, FCNAME, __LINE__, &disconnected);
	if (MPIR_Err_is_fatal(event_mpi_errno))
	{
	    /*
	     * A serious error occurred.  There is no guarantee that the data structures are still intact.  Therefore, we avoid
	     * modifying them.
	     */
	    mpi_errno = event_mpi_errno;
	    goto fn_exit;
	}

	MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDU_SOCK_OP_WRITE, pollinfo->write_nb, pollinfo->user_ptr,
				  event_mpi_errno, mpi_errno, fn_exit);
	MPIDU_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLOUT);
	if (disconnected)
	{
	    /*
	     * The connection is dead but data may still be in the socket buffer; thus, we change the state and let
	     * MPIDU_Sock_wait() clean things up.
	     */
	    pollinfo->state = MPIDU_SOCKI_STATE_CONNECTED_RO;
	}
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCKI_HANDLE_WRITE);
    return mpi_errno;
}
/* end MPIDU_Socki_handle_write() */


#undef FUNCNAME
#define FUNCNAME MPIDU_Socki_handle_connect
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int MPIDU_Socki_handle_connect(struct pollfd * const pollfd, struct pollinfo * const pollinfo)
{
    struct sockaddr_in addr;
    socklen_t addr_len;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCKI_HANDLE_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCKI_HANDLE_CONNECT);

    addr_len = sizeof(struct sockaddr_in);
    rc = getpeername(pollfd->fd, (struct sockaddr *) &addr, &addr_len);
    if (rc == 0)
    {
	MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDU_SOCK_OP_CONNECT, 0, pollinfo->user_ptr, MPI_SUCCESS, mpi_errno, fn_exit);
	pollinfo->state = MPIDU_SOCKI_STATE_CONNECTED_RW;
    }
    else
    {
	int event_mpi_errno;
	
	MPIDU_SOCKI_GET_SOCKET_ERROR(pollinfo, pollinfo->os_errno, mpi_errno, fn_exit);
	event_mpi_errno = MPIR_Err_create_code(
	    MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_CONN_FAILED, "**sock|connfailed",
	    "**sock|poll|connfailed %d %d %d %s", pollinfo->sock_set->id, pollinfo->sock_id, pollinfo->os_errno,
	    MPIU_Strerror(pollinfo->os_errno));
	MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDU_SOCK_OP_CONNECT, 0, pollinfo->user_ptr, event_mpi_errno, mpi_errno, fn_exit);
	pollinfo->state = MPIDU_SOCKI_STATE_DISCONNECTED;
    }
    
    MPIDU_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLOUT);
    
  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCKI_HANDLE_CONNECT);
    return mpi_errno;
}
/* end MPIDU_Socki_handle_connect() */
