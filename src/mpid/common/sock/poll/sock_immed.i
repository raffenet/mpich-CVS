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

    MPIDU_SOCKI_VERIFY_INIT(mpi_errno);
    MPIDU_SOCKI_VALIDATE_SOCK(listener, mpi_errno);

    pollfd = MPIDU_Socki_get_pollfd_ptr(listener);
    pollinfo = MPIDU_Socki_get_pollinfo_ptr(listener);
    
    addr_len = sizeof(struct sockaddr_in);
    fd = accept(pollfd->fd, (struct sockaddr *) &addr, &addr_len);
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
					     "**sock|noosmem", NULL);
	}
	else if (errno == EBADF || errno == ENOTSOCK || errno == EOPNOTSUPP)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					     "**sock|badhandle", "**sock|poll|badhandle %d %d %d",
					     pollinfo->sock_set->id, pollinfo->sock_id, pollfd->fd);
	}
	else
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NO_NEW_SOCK,
					     "**sock|poll|accept", "**sock|poll|accept %d", errno);
	}
	
	goto fn_fail;
    }
    
    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					 "**sock|poll|nonblock", "**sock|poll|nonblock %d", errno);
	goto fn_fail;
    }
    rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if (rc == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					 "**sock|poll|nonblock", "**sock|poll|nonblock %d", errno);
	goto fn_fail;
    }

    nodelay = 1;
    rc = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
    if (rc != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					 "**sock|poll|nodelay", "**sock|poll|nodelay %d", errno);
	goto fn_fail;
    }

    mpi_errno = MPIDU_Socki_sock_alloc(sock_set, &sock);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM,
					 "**sock|sockalloc", NULL);
	goto fn_fail;
    }
    
    pollfd = MPIDU_Socki_get_pollfd_ptr(sock);
    pollinfo = MPIDU_Socki_get_pollinfo_ptr(sock);

    pollfd->fd = fd;
    pollfd->events = 0;
    pollfd->revents = 0;
    pollinfo->fd = fd;
    pollinfo->user_ptr = user_ptr;
    pollinfo->state = MPIDU_SOCKI_STATE_CONNECTED;

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

    MPIDU_SOCKI_VERIFY_INIT(mpi_errno);
    MPIDU_SOCKI_VALIDATE_SOCK(sock, mpi_errno);

    pollfd = MPIDU_Socki_get_pollfd_ptr(sock);
    pollinfo = MPIDU_Socki_get_pollinfo_ptr(sock);

    MPIDU_SOCKI_VERIFY_CONNECTED(pollinfo, mpi_errno);
    MPIDU_SOCKI_VALIDATE_FD(pollfd, pollinfo, mpi_errno);
    MPIDU_SOCKI_VERIFY_SOCK_READABLE(pollfd, pollinfo, mpi_errno);
    
    /* FIXME: multiple passes should be made if len > SSIZE_MAX and nb == SSIZE_MAX */
    if (len > SSIZE_MAX)
    {
	len = SSIZE_MAX;
    }
    
    do
    {
	MPIDI_FUNC_ENTER(MPID_STATE_READ);
	nb = read(pollfd->fd, buf, len);
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
	
	/* MT: what about other threads that are or might soon be using the fd? */
	pollinfo->state = MPIDU_SOCKI_STATE_CONN_CLOSED;
	close(pollfd->fd);
	pollfd->fd = -1;
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_CONN_CLOSED,
					 "**sock|connclosed", "**sock|connclosed %d %d",
					 pollinfo->sock_set->id, pollinfo->sock_id);
    }
    else
    {
	if (errno == EAGAIN || errno == EWOULDBLOCK)
	{
	    *num_read = 0;
	}
	else
	{
	    mpi_errno = MPIDU_Socki_handle_immediate_os_errors(pollfd, pollinfo, errno, FCNAME, __LINE__);
	}
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_READ);
    return mpi_errno;
}

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
    
    MPIDU_SOCKI_VERIFY_INIT(mpi_errno);
    MPIDU_SOCKI_VALIDATE_SOCK(sock, mpi_errno);

    pollfd = MPIDU_Socki_get_pollfd_ptr(sock);
    pollinfo = MPIDU_Socki_get_pollinfo_ptr(sock);

    MPIDU_SOCKI_VERIFY_CONNECTED(pollinfo, mpi_errno);
    MPIDU_SOCKI_VALIDATE_FD(pollfd, pollinfo, mpi_errno);
    MPIDU_SOCKI_VERIFY_SOCK_READABLE(pollfd, pollinfo, mpi_errno);

    /* FIXME: what happens if more than SSIZE_MAX data is read?  do we need to copy the iovec and limit it to requesting
       SSIZE_MAX data at once?  Regardless, if SSIZE_MAX is returned, then we should (copy and) adjust the iovec and try
       to read more data. */
    do
    {
	MPIDI_FUNC_ENTER(MPID_STATE_READV);
	nb = readv(pollfd->fd, iov, iov_n);
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
	
	/* MT: what about other threads that are or might soon be using the fd? */
	pollinfo->state = MPIDU_SOCKI_STATE_CONN_CLOSED;
	close(pollfd->fd);
	pollfd->fd = -1;
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_CONN_CLOSED,
					 "**sock|connclosed", "**sock|connclosed %d %d",
					 pollinfo->sock_set->id, pollinfo->sock_id);
    }
    else
    {
	if (errno == EAGAIN || errno == EWOULDBLOCK)
	{
	    *num_read = 0;
	}
	else
	{
	    mpi_errno = MPIDU_Socki_handle_immediate_os_errors(pollfd, pollinfo, errno, FCNAME, __LINE__);
	}
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_READV);
    return mpi_errno;
}

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
    
    MPIDU_SOCKI_VERIFY_INIT(mpi_errno);
    MPIDU_SOCKI_VALIDATE_SOCK(sock, mpi_errno);

    pollfd = MPIDU_Socki_get_pollfd_ptr(sock);
    pollinfo = MPIDU_Socki_get_pollinfo_ptr(sock);

    MPIDU_SOCKI_VERIFY_CONNECTED(pollinfo, mpi_errno);
    MPIDU_SOCKI_VALIDATE_FD(pollfd, pollinfo, mpi_errno);
    MPIDU_SOCKI_VERIFY_SOCK_WRITABLE(pollfd, pollinfo, mpi_errno);
    
    /* FIXME: multiple passes should be made if len > SSIZE_MAX and nb == SSIZE_MAX */
    if (len > SSIZE_MAX)
    {
	len = SSIZE_MAX;
    }
    
    do
    {
	MPIDI_FUNC_ENTER(MPID_STATE_WRITE);
	nb = write(pollfd->fd, buf, len);
	MPIDI_FUNC_EXIT(MPID_STATE_WRITE);
    }
    while (nb == -1 && errno == EINTR);

    if (nb >= 0)
    {
	*num_written = nb;
    }
    else
    {
	if (errno == EAGAIN || errno == EWOULDBLOCK)
	{
	    *num_written = 0;
	}
	else
	{
	    mpi_errno = MPIDU_Socki_handle_immediate_os_errors(pollfd, pollinfo, errno, FCNAME, __LINE__);
	}
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WRITE);
    return mpi_errno;
}

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
    
    MPIDU_SOCKI_VERIFY_INIT(mpi_errno);
    MPIDU_SOCKI_VALIDATE_SOCK(sock, mpi_errno);

    pollfd = MPIDU_Socki_get_pollfd_ptr(sock);
    pollinfo = MPIDU_Socki_get_pollinfo_ptr(sock);

    MPIDU_SOCKI_VERIFY_CONNECTED(pollinfo, mpi_errno);
    MPIDU_SOCKI_VALIDATE_FD(pollfd, pollinfo, mpi_errno);
    MPIDU_SOCKI_VERIFY_SOCK_WRITABLE(pollfd, pollinfo, mpi_errno);
    
    /* FIXME: what happens if more than SSIZE_MAX data is read?  do we need to copy the iovec and limit it to requesting
       SSIZE_MAX data at once?  Regardless, if SSIZE_MAX is returned, then we should (copy and) adjust the iovec and try
       to read more data. */
    do
    {
	MPIDI_FUNC_ENTER(MPID_STATE_WRITEV);
	nb = writev(pollfd->fd, iov, iov_n);
	MPIDI_FUNC_EXIT(MPID_STATE_WRITEV);
    }
    while (nb == -1 && errno == EINTR);

    if (nb >= 0)
    {
	*num_written = (MPIU_Size_t) nb;
    }
    else
    {
	if (errno == EAGAIN || errno == EWOULDBLOCK)
	{
	    *num_written = 0;
	}
	else
	{
	    mpi_errno = MPIDU_Socki_handle_immediate_os_errors(pollfd, pollinfo, errno, FCNAME, __LINE__);
	}
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WRITEV);
    return mpi_errno;
}

