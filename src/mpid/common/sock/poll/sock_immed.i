/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */


#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_accept
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_accept(struct MPIDU_Sock * listener, struct MPIDU_Sock_set * sock_set, void * user_ptr, struct MPIDU_Sock ** sockp)
{
    struct MPIDU_Sock * sock;
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int fd = -1;
    struct sockaddr_in addr;
    socklen_t addr_len;
    long flags;
    int nodelay;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_ACCEPT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_ACCEPT);

    MPIDU_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    MPIDU_SOCKI_VALIDATE_SOCK(listener, mpi_errno, fn_exit);

    pollfd = MPIDU_Socki_sock_get_pollfd(listener);
    pollinfo = MPIDU_Socki_sock_get_pollinfo(listener);

    if (pollinfo->type != MPIDU_SOCKI_TYPE_LISTENER)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK,
					 "**sock|listener_bad_sock", "**sock|listener_bad_sock %d %d",
					 pollinfo->sock_set->id, pollinfo->sock_id);
	goto fn_exit;
    }
    
    if (pollinfo->state != MPIDU_SOCKI_STATE_CONNECTED_RO && pollinfo->state != MPIDU_SOCKI_STATE_CLOSING)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK,
					 "**sock|listener_bad_state", "**sock|listener_bad_state %d %d %d",
					 pollinfo->sock_set->id, pollinfo->sock_id, pollinfo->state);
	goto fn_exit;
    }

    /*
     * Get a socket for the new connection from the operating system.  Make the socket nonblocking, and disable Nagle's
     * alogorithm (to minimize latency of small messages).
     */
    addr_len = sizeof(struct sockaddr_in);
    fd = accept(pollinfo->fd, (struct sockaddr *) &addr, &addr_len);

    if (pollinfo->state != MPIDU_SOCKI_STATE_CLOSING)
    {
	/*
	 * Unless the listener sock is being closed, add it back into the poll list so that new connections will be detected.
	 */
	MPIDU_SOCKI_POLLFD_OP_SET(pollfd, pollinfo, POLLIN);
	/* MT: MPIDU_SOCKI_WAKEUP(sock_set, FALSE); */
    }
    
    if (fd == -1)
    {
	if (errno == EAGAIN || errno == EWOULDBLOCK)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NO_NEW_SOCK,
					     "**sock|nosock", NULL);
	}
	else if (errno == ENOBUFS || errno == ENOMEM)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM,
					     "**sock|osnomem", NULL);
	}
	else if (errno == EBADF || errno == ENOTSOCK || errno == EOPNOTSUPP)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					     "**sock|badhandle", "**sock|poll|badhandle %d %d %d",
					     pollinfo->sock_set->id, pollinfo->sock_id, pollinfo->fd);
	}
	else
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NO_NEW_SOCK,
					     "**sock|poll|accept", "**sock|poll|accept %d %s", errno, MPIU_Strerror(errno));
	}
	
	goto fn_fail;
    }

    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					 "**sock|poll|nonblock", "**sock|poll|nonblock %d %s", errno, MPIU_Strerror(errno));
	goto fn_fail;
    }
    rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if (rc == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					 "**sock|poll|nonblock", "**sock|poll|nonblock %d %s", errno, MPIU_Strerror(errno));
	goto fn_fail;
    }

    nodelay = 1;
    rc = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
    if (rc != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					 "**sock|poll|nodelay", "**sock|poll|nodelay %d %s", errno, MPIU_Strerror(errno));
	goto fn_fail;
    }

    /*
     * Allocate and initialize sock and poll structures.
     *
     * NOTE: pollfd->fd is initialized to -1.  It is only set to the true fd value when an operation is posted on the sock.  This
     * (hopefully) eliminates a little overhead in the OS and avoids repetitive POLLHUP events when the connection is closed by
     * the remote process.
     */
    mpi_errno = MPIDU_Socki_sock_alloc(sock_set, &sock);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM,
					 "**sock|sockalloc", NULL);
	goto fn_fail;
    }
    
    pollfd = MPIDU_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDU_Socki_sock_get_pollinfo(sock);

    pollinfo->fd = fd;
    pollinfo->user_ptr = user_ptr;
    pollinfo->type = MPIDU_SOCKI_TYPE_COMMUNICATION;
    pollinfo->state = MPIDU_SOCKI_STATE_CONNECTED_RW;
    pollinfo->os_errno = 0;
    
    pollfd->fd = -1;
    pollfd->events = 0;
    pollfd->revents = 0;
    *sockp = sock;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_ACCEPT);
    return mpi_errno;

  fn_fail:
    if (fd != -1)
    {
	close(fd);
    }

    goto fn_exit;
}
/* end MPIDU_Sock_accept() */


#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_read
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_read(MPIDU_Sock_t sock, void * buf, MPIU_Size_t len, MPIU_Size_t * num_read)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    size_t nb;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_READ);
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_READ);

    MPIDU_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    MPIDU_SOCKI_VALIDATE_SOCK(sock, mpi_errno, fn_exit);

    pollfd = MPIDU_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDU_Socki_sock_get_pollinfo(sock);

    MPIDU_SOCKI_VALIDATE_FD(pollinfo, mpi_errno, fn_exit);
    MPIDU_SOCKI_VERIFY_CONNECTED_READABLE(pollinfo, mpi_errno, fn_exit);
    MPIDU_SOCKI_VERIFY_NO_POSTED_READ(pollfd, pollinfo, mpi_errno, fn_exit);
    
    /* FIXME: multiple passes should be made if len > SSIZE_MAX and nb == SSIZE_MAX */
    if (len > SSIZE_MAX)
    {
	len = SSIZE_MAX;
    }
    
    do
    {
	MPIDI_FUNC_ENTER(MPID_STATE_READ);
	nb = read(pollinfo->fd, buf, len);
	MPIDI_FUNC_EXIT(MPID_STATE_READ);
    }
    while (nb == -1 && errno == EINTR);

    if (nb > 0)
    {
	*num_read = (MPIU_Size_t) nb;
    }
    else if (nb == 0)
    {
	*num_read = 0;
	
	mpi_errno = MPIR_Err_create_code(
	    MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_CONN_CLOSED,
	    "**sock|connclosed", "**sock|connclosed %d %d", pollinfo->sock_set->id, pollinfo->sock_id);
	
	if (MPIDU_SOCKI_POLLFD_OP_ISSET(pollfd, pollinfo, POLLOUT))
	{ 
	    /* A write is posted on this connection.  Enqueue an event for the write indicating the connection is closed. */
	    MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDU_SOCK_OP_WRITE, pollinfo->write_nb, pollinfo->user_ptr,
				      mpi_errno, mpi_errno, fn_exit);
	    MPIDU_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLOUT);
	}
	
	pollinfo->state = MPIDU_SOCKI_STATE_DISCONNECTED;
    }
    else if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
	*num_read = 0;
    }
    else
    {
	int disconnected;
	
	*num_read = 0;
	
	mpi_errno = MPIDU_Socki_os_to_mpi_errno(pollinfo, errno, FCNAME, __LINE__, &disconnected);
	if (MPIR_Err_is_fatal(mpi_errno))
	{
	    /*
	     * A serious error occurred.  There is no guarantee that the data structures are still intact.  Therefore, we avoid
	     * modifying them.
	     */
	    goto fn_exit;
	}

	if (disconnected)
	{
	    if (MPIDU_SOCKI_POLLFD_OP_ISSET(pollfd, pollinfo, POLLOUT))
	    { 
		/* A write is posted on this connection.  Enqueue an event for the write indicating the connection is closed. */
		MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDU_SOCK_OP_WRITE, pollinfo->write_nb, pollinfo->user_ptr,
					  mpi_errno, mpi_errno, fn_exit);
		MPIDU_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLOUT);
	    }
	    
	    pollinfo->state = MPIDU_SOCKI_STATE_DISCONNECTED;
	}
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_READ);
    return mpi_errno;
}
/* end MPIDU_Sock_read() */


#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_readv
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_readv(MPIDU_Sock_t sock, MPID_IOV * iov, int iov_n, MPIU_Size_t * num_read)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    ssize_t nb;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_READV);
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_READV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_READV);
    
    MPIDU_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    MPIDU_SOCKI_VALIDATE_SOCK(sock, mpi_errno, fn_exit);

    pollfd = MPIDU_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDU_Socki_sock_get_pollinfo(sock);

    MPIDU_SOCKI_VALIDATE_FD(pollinfo, mpi_errno, fn_exit);
    MPIDU_SOCKI_VERIFY_CONNECTED_READABLE(pollinfo, mpi_errno, fn_exit);
    MPIDU_SOCKI_VERIFY_NO_POSTED_READ(pollfd, pollinfo, mpi_errno, fn_exit);

    /*
     * FIXME: The IEEE 1003.1 standard says that if the sum of the iov_len fields exceeds SSIZE_MAX, an errno of EINVAL will be
     * returned.  How do we handle this?  Can we place an equivalent limitation in the Sock interface?
     */
    do
    {
	MPIDI_FUNC_ENTER(MPID_STATE_READV);
	nb = readv(pollinfo->fd, iov, iov_n);
	MPIDI_FUNC_EXIT(MPID_STATE_READV);
    }
    while (nb == -1 && errno == EINTR);

    if (nb > 0)
    {
	*num_read = (MPIU_Size_t) nb;
    }
    else if (nb == 0)
    {
	*num_read = 0;
	
	mpi_errno = MPIR_Err_create_code(
	    MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_CONN_CLOSED,
	    "**sock|connclosed", "**sock|connclosed %d %d", pollinfo->sock_set->id, pollinfo->sock_id);
	
	if (MPIDU_SOCKI_POLLFD_OP_ISSET(pollfd, pollinfo, POLLOUT))
	{ 
	    
	    /* A write is posted on this connection.  Enqueue an event for the write indicating the connection is closed. */
	    MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDU_SOCK_OP_WRITE, pollinfo->write_nb, pollinfo->user_ptr,
				      mpi_errno, mpi_errno, fn_exit);
	    MPIDU_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLOUT);
	}
	
	pollinfo->state = MPIDU_SOCKI_STATE_DISCONNECTED;
    }
    else if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
	*num_read = 0;
    }
    else
    {
	int disconnected;
	
	*num_read = 0;
	
	mpi_errno = MPIDU_Socki_os_to_mpi_errno(pollinfo, errno, FCNAME, __LINE__, &disconnected);
	if (MPIR_Err_is_fatal(mpi_errno))
	{
	    /*
	     * A serious error occurred.  There is no guarantee that the data structures are still intact.  Therefore, we avoid
	     * modifying them.
	     */
	    goto fn_exit;
	}

	if (disconnected)
	{
	    if (MPIDU_SOCKI_POLLFD_OP_ISSET(pollfd, pollinfo, POLLOUT))
	    { 
		/* A write is posted on this connection.  Enqueue an event for the write indicating the connection is closed. */
		MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo, MPIDU_SOCK_OP_WRITE, pollinfo->write_nb, pollinfo->user_ptr,
					  mpi_errno, mpi_errno, fn_exit);
		MPIDU_SOCKI_POLLFD_OP_CLEAR(pollfd, pollinfo, POLLOUT);
	    }
	    
	    pollinfo->state = MPIDU_SOCKI_STATE_DISCONNECTED;
	}
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_READV);
    return mpi_errno;
}
/* end MPIDU_Sock_readv() */


#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_write
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_write(MPIDU_Sock_t sock, void * buf, MPIU_Size_t len, MPIU_Size_t * num_written)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    ssize_t nb;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_WRITE);
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_WRITE);
    
    MPIDU_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    MPIDU_SOCKI_VALIDATE_SOCK(sock, mpi_errno, fn_exit);

    pollfd = MPIDU_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDU_Socki_sock_get_pollinfo(sock);

    MPIDU_SOCKI_VERIFY_CONNECTED_WRITABLE(pollinfo, mpi_errno, fn_exit);
    MPIDU_SOCKI_VALIDATE_FD(pollinfo, mpi_errno, fn_exit);
    MPIDU_SOCKI_VERIFY_NO_POSTED_WRITE(pollfd, pollinfo, mpi_errno, fn_exit);
    
    /* FIXME: multiple passes should be made if len > SSIZE_MAX and nb == SSIZE_MAX */
    if (len > SSIZE_MAX)
    {
	len = SSIZE_MAX;
    }
    
    do
    {
	MPIDI_FUNC_ENTER(MPID_STATE_WRITE);
	nb = write(pollinfo->fd, buf, len);
	MPIDI_FUNC_EXIT(MPID_STATE_WRITE);
    }
    while (nb == -1 && errno == EINTR);

    if (nb >= 0)
    {
	*num_written = nb;
    }
    else if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
	*num_written = 0;
    }
    else
    {
	int disconnected;
	
	*num_written = 0;
	
	mpi_errno = MPIDU_Socki_os_to_mpi_errno(pollinfo, errno, FCNAME, __LINE__, &disconnected);
	if (MPIR_Err_is_fatal(mpi_errno))
	{
	    /*
	     * A serious error occurred.  There is no guarantee that the data structures are still intact.  Therefore, we avoid
	     * modifying them.
	     */
	    goto fn_exit;
	}

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
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WRITE);
    return mpi_errno;
}
/* end MPIDU_Sock_write() */


#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_writev
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_writev(MPIDU_Sock_t sock, MPID_IOV * iov, int iov_n, MPIU_Size_t * num_written)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    ssize_t nb;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_WRITEV);
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_WRITEV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_WRITEV);
    
    MPIDU_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    MPIDU_SOCKI_VALIDATE_SOCK(sock, mpi_errno, fn_exit);

    pollfd = MPIDU_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDU_Socki_sock_get_pollinfo(sock);

    MPIDU_SOCKI_VALIDATE_FD(pollinfo, mpi_errno, fn_exit);
    MPIDU_SOCKI_VERIFY_CONNECTED_WRITABLE(pollinfo, mpi_errno, fn_exit);
    MPIDU_SOCKI_VERIFY_NO_POSTED_WRITE(pollfd, pollinfo, mpi_errno, fn_exit);
    
    /*
     * FIXME: The IEEE 1003.1 standard says that if the sum of the iov_len fields exceeds SSIZE_MAX, an errno of EINVAL will be
     * returned.  How do we handle this?  Can we place an equivalent limitation in the Sock interface?
     */
    do
    {
	MPIDI_FUNC_ENTER(MPID_STATE_WRITEV);
	nb = writev(pollinfo->fd, iov, iov_n);
	MPIDI_FUNC_EXIT(MPID_STATE_WRITEV);
    }
    while (nb == -1 && errno == EINTR);

    if (nb >= 0)
    {
	*num_written = (MPIU_Size_t) nb;
    }
    else if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
	*num_written = 0;
    }
    else
    {
	int disconnected;
	
	*num_written = 0;
	
	mpi_errno = MPIDU_Socki_os_to_mpi_errno(pollinfo, errno, FCNAME, __LINE__, &disconnected);
	if (MPIR_Err_is_fatal(mpi_errno))
	{
	    /*
	     * A serious error occurred.  There is no guarantee that the data structures are still intact.  Therefore, we avoid
	     * modifying them.
	     */
	    goto fn_exit;
	}

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
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WRITEV);
    return mpi_errno;
}
/* end MPIDU_Sock_writev() */
