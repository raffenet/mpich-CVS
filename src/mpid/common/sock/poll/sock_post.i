/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_post_connect
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_post_connect(struct MPIDU_Sock_set * sock_set, void * user_ptr, char * host_description, int port,
			    struct MPIDU_Sock ** sockp)
{
    struct MPIDU_Sock * sock = NULL;
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    struct hostent * hostent;
    int fd = -1;
    struct sockaddr_in addr;
    long flags;
    int nodelay;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_POST_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_POST_CONNECT);

    MPIDU_SOCKI_VERIFY_INIT(mpi_errno);

    /* create nonblocking socket */
    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					 "**sock|poll|socket", "**sock|poll|socket %d", errno);
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

    /* allocate and initialize sock and poll structures */
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
    
    /* convert hostname to IP address */
    hostent = gethostbyname(host_description);
    if (hostent == NULL || hostent->h_addrtype != AF_INET)
    {
	/* FIXME: we should make multiple attempts and try different interfaces */
	pollfd->fd = -1;
	pollinfo->state = MPIDU_SOCKI_STATE_CONN_FAILED;
	close(pollinfo->fd);
	pollinfo->fd = -1;
	mpi_errno = MPIDU_Socki_event_enqueue(pollinfo->sock_set, MPIDU_SOCK_OP_CONNECT, 0, user_ptr, MPIR_Err_create_code(
	    MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_CONN_FAILED,
	    "**sock|hostres", "**sock|poll|hostres %d %d %s", pollinfo->sock_set->id, pollinfo->sock_id, host_description));
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM,
					     "**sock|poll|eqfail", "**sock|poll|eqfail %d", MPIDU_SOCK_OP_CONNECT);
	    goto fn_fail;
	}
	
	goto fn_exit;
    }
    assert(hostent->h_length == sizeof(addr.sin_addr.s_addr));
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    memcpy(&addr.sin_addr.s_addr, hostent->h_addr_list[0], sizeof(addr.sin_addr.s_addr));
    addr.sin_port = htons(port);

    /* attempt to establish connection */
    do
    {
        rc = connect(fd, (struct sockaddr *) &addr, sizeof(addr));
    }
    while (rc == -1 && errno == EINTR);
    
    if (rc == 0)
    {
	/* connection succeeded */
	pollinfo->state = MPIDU_SOCKI_STATE_CONNECTED;
	mpi_errno = MPIDU_Socki_event_enqueue(pollinfo->sock_set, MPIDU_SOCK_OP_CONNECT, 0, user_ptr, MPI_SUCCESS);
	if (mpi_errno != MPI_SUCCESS)
	{ 
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM,
					     "**sock|poll|eqfail", "**sock|poll|eqfail %d", MPIDU_SOCK_OP_CONNECT);
	    goto fn_fail;
	}
    }
    else if (errno == EINPROGRESS)
    {
	/* connection pending */
	pollinfo->state = MPIDU_SOCKI_STATE_CONNECTING;
	pollfd->events |= POLLOUT | POLLERR;
    }
    else
    {
	pollfd->fd = -1;
	pollinfo->state = MPIDU_SOCKI_STATE_CONN_FAILED;
	close(pollinfo->fd);
	pollinfo->fd = -1;

	if (errno == ECONNREFUSED)
	{
	    mpi_errno = MPIDU_Socki_event_enqueue(pollinfo->sock_set, MPIDU_SOCK_OP_CONNECT, 0, user_ptr, MPIR_Err_create_code(
		MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_CONN_FAILED,
		"**sock|connrefused", "**sock|poll|connrefused %d %d %s",
		pollinfo->sock_set->id, pollinfo->sock_id, host_description));
	    if (mpi_errno != MPI_SUCCESS)
	    { 
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM,
						 "**sock|poll|eqfail", "**sock|poll|eqfail %d", MPIDU_SOCK_OP_CONNECT);
		goto fn_fail;
	    }
	}
	else
	{
	    mpi_errno = MPIDU_Socki_event_enqueue(pollinfo->sock_set, MPIDU_SOCK_OP_CONNECT, 0, user_ptr, MPIR_Err_create_code(
		MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_CONN_FAILED,
		"**sock|oserror", "**sock|poll|oserror %d %d %d", pollinfo->sock_set->id, pollinfo->sock_id, errno));
	    if (mpi_errno != MPI_SUCCESS)
	    { 
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM,
						 "**sock|poll|eqfail", "**sock|poll|eqfail %d", MPIDU_SOCK_OP_CONNECT);
		goto fn_fail;
	    }
	}
    }

    *sockp = sock;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_CONNECT);
    return mpi_errno;

  fn_fail:
    if (fd != -1)
    { 
	close(fd);
    }

    if (sock != NULL)
    {
	MPIDU_Socki_sock_free(sock);
    }

    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_listen
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
#ifndef USHRT_MAX
#define USHRT_MAX 65535   /* 2^16-1 */
#endif
int MPIDU_Sock_listen(struct MPIDU_Sock_set * sock_set, void * user_ptr, int * port, struct MPIDU_Sock ** sockp)
{
    struct MPIDU_Sock * sock;
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int fd = -1;
    long flags;
    struct sockaddr_in addr;
    socklen_t addr_len;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_LISTEN);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_LISTEN);

    MPIDU_SOCKI_VERIFY_INIT(mpi_errno);
    if (*port < 0 || *port > USHRT_MAX)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_PORT,
					 "**sock|badport", "**sock|badport %d", *port);
	goto fn_exit;
    }

     /* establish non-blocking listener */
    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					 "**sock|poll|socket", "**sock|poll|socket %d", errno);
	goto fn_fail;
    }

    if (*port != 0)
    {
	flags = 1;
	rc = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(long));
	if (rc == -1)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					     "**sock|poll|reuseaddr", "**sock|poll|reuseaddr %d", errno);
	    goto fn_fail;
	}
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

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((short) *port);
    rc = bind(fd, (struct sockaddr *) &addr, sizeof(addr));
    if (rc == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					 "**sock|poll|bind", "**sock|poll|bind %d %d", port, errno);
	goto fn_fail;
    }
    
    rc = listen(fd, SOMAXCONN);
    if (rc == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					 "**sock|poll|listen", "**sock|poll|listen %d", errno);
	goto fn_fail;
    }

    /* get listener port */
    addr_len = sizeof(addr);
    rc = getsockname(fd, (struct sockaddr *) &addr, &addr_len);
    if (rc == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					 "**sock|getport", "**sock|poll|getport %d", errno);
	goto fn_fail;
    }
    *port = ntohs(addr.sin_port);

    /* allocate and initialize sock and poll structures */
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
    pollfd->events = POLLIN;
    pollfd->revents = 0;
    pollinfo->fd = fd;
    pollinfo->user_ptr = user_ptr;
    pollinfo->state = MPIDU_SOCKI_STATE_LISTENER;
    
    *sockp = sock;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_LISTEN);
    return mpi_errno;

  fn_fail:
    if (fd != -1)
    { 
	close(fd);
    }

    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_post_read
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_post_read(struct MPIDU_Sock * sock, void * buf, MPIU_Size_t minlen, MPIU_Size_t maxlen,
			 MPIDU_Sock_progress_update_func_t fn)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_POST_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_POST_READ);

    MPIDU_SOCKI_VERIFY_INIT(mpi_errno);
    MPIDU_SOCKI_VALIDATE_SOCK(sock, mpi_errno);

    pollfd = MPIDU_Socki_get_pollfd_ptr(sock);
    pollinfo = MPIDU_Socki_get_pollinfo_ptr(sock);

    MPIDU_SOCKI_VERIFY_CONNECTED(pollinfo, mpi_errno);
    MPIDU_SOCKI_VALIDATE_FD(pollfd, pollinfo, mpi_errno);
    MPIDU_SOCKI_VERIFY_SOCK_READABLE(pollfd, pollinfo, mpi_errno);

    if (minlen < 1 || minlen > maxlen)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_LEN,
					 "**sock|badlen", "**sock|badlen %d %d %d %d",
					 pollinfo->sock_set->id, pollinfo->sock_id, minlen, maxlen);
	goto fn_exit;
    }

    pollinfo->read.buf.ptr = buf;
    pollinfo->read.buf.min = minlen;
    pollinfo->read.buf.max = maxlen;
    pollinfo->read_iov_flag = FALSE;
    pollinfo->read_nb = 0;
    pollinfo->read_progress_update_fn = fn;
    pollfd->events |= POLLIN;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_READ);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_post_readv
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_post_readv(struct MPIDU_Sock * sock, MPID_IOV * iov, int iov_n, MPIDU_Sock_progress_update_func_t fn)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_POST_READV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_POST_READV);

    MPIDU_SOCKI_VERIFY_INIT(mpi_errno);
    MPIDU_SOCKI_VALIDATE_SOCK(sock, mpi_errno);

    pollfd = MPIDU_Socki_get_pollfd_ptr(sock);
    pollinfo = MPIDU_Socki_get_pollinfo_ptr(sock);

    MPIDU_SOCKI_VERIFY_CONNECTED(pollinfo, mpi_errno);
    MPIDU_SOCKI_VALIDATE_FD(pollfd, pollinfo, mpi_errno);
    MPIDU_SOCKI_VERIFY_SOCK_READABLE(pollfd, pollinfo, mpi_errno);

    if (iov_n < 1 || iov_n >= MPID_IOV_LIMIT)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_LEN,
					 "**sock|badiovn", "**sock|badiovn %d %d %d",
					 pollinfo->sock_set->id, pollinfo->sock_id, iov_n);
	goto fn_exit;
    }

    pollinfo->read.iov.ptr = iov;
    pollinfo->read.iov.count = iov_n;
    pollinfo->read.iov.offset = 0;
    pollinfo->read_iov_flag = TRUE;
    pollinfo->read_nb = 0;
    pollinfo->read_progress_update_fn = fn;
    pollfd->events |= POLLIN;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_READV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_post_write
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_post_write(struct MPIDU_Sock * sock, void * buf, MPIU_Size_t minlen, MPIU_Size_t maxlen,
			  MPIDU_Sock_progress_update_func_t fn)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_POST_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_POST_WRITE);
    
    MPIDU_SOCKI_VERIFY_INIT(mpi_errno);
    MPIDU_SOCKI_VALIDATE_SOCK(sock, mpi_errno);

    pollfd = MPIDU_Socki_get_pollfd_ptr(sock);
    pollinfo = MPIDU_Socki_get_pollinfo_ptr(sock);

    MPIDU_SOCKI_VERIFY_CONNECTED(pollinfo, mpi_errno);
    MPIDU_SOCKI_VALIDATE_FD(pollfd, pollinfo, mpi_errno);
    MPIDU_SOCKI_VERIFY_SOCK_WRITABLE(pollfd, pollinfo, mpi_errno);

    if (minlen < 1 || minlen > maxlen)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_LEN,
					 "**sock|badlen", "**sock|badlen %d %d %d %d",
					 pollinfo->sock_set->id, pollinfo->sock_id, minlen, maxlen);
	goto fn_exit;
    }

    pollinfo->write.buf.ptr = buf;
    pollinfo->write.buf.min = minlen;
    pollinfo->write.buf.max = maxlen;
    pollinfo->write_iov_flag = FALSE;
    pollinfo->write_nb = 0;
    pollinfo->write_progress_update_fn = fn;
    pollfd->events |= POLLOUT;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_WRITE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_post_writev
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_post_writev(struct MPIDU_Sock * sock, MPID_IOV * iov, int iov_n, MPIDU_Sock_progress_update_func_t fn)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_POST_WRITEV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_POST_WRITEV);
    
    MPIDU_SOCKI_VERIFY_INIT(mpi_errno);
    MPIDU_SOCKI_VALIDATE_SOCK(sock, mpi_errno);

    pollfd = MPIDU_Socki_get_pollfd_ptr(sock);
    pollinfo = MPIDU_Socki_get_pollinfo_ptr(sock);

    MPIDU_SOCKI_VERIFY_CONNECTED(pollinfo, mpi_errno);
    MPIDU_SOCKI_VALIDATE_FD(pollfd, pollinfo, mpi_errno);
    MPIDU_SOCKI_VERIFY_SOCK_WRITABLE(pollfd, pollinfo, mpi_errno);

    if (iov_n < 1 || iov_n >= MPID_IOV_LIMIT)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_LEN,
					 "**sock|badiovn", "**sock|badiovn %d %d %d",
					 pollinfo->sock_set->id, pollinfo->sock_id, iov_n);
	goto fn_exit;
    }

    pollinfo->write.iov.ptr = iov;
    pollinfo->write.iov.count = iov_n;
    pollinfo->write.iov.offset = 0;
    pollinfo->write_iov_flag = TRUE;
    pollinfo->write_nb = 0;
    pollinfo->write_progress_update_fn = fn;
    pollfd->events |= POLLOUT;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_WRITEV);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_post_close
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_post_close(struct MPIDU_Sock * sock)
{
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int rc;
    int flags;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_POST_CLOSE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_POST_CLOSE);

    MPIDU_SOCKI_VERIFY_INIT(mpi_errno);
    MPIDU_SOCKI_VALIDATE_SOCK(sock, mpi_errno);

    pollfd = MPIDU_Socki_get_pollfd_ptr(sock);
    pollinfo = MPIDU_Socki_get_pollinfo_ptr(sock);

    if (pollinfo->state <= 0 || pollinfo->state >= MPIDU_SOCKI_STATE_LAST || pollinfo->state == MPIDU_SOCKI_STATE_UNCONNECTED ||
	pollinfo->state == MPIDU_SOCKI_STATE_INTERRUPTER)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK,
					 "**sock|badsock", NULL);
	goto fn_exit;
    }

    if (pollinfo->state == MPIDU_SOCKI_STATE_CONNECTED || pollinfo->state == MPIDU_SOCKI_STATE_LISTENER)
    { 
	MPIDU_SOCKI_VALIDATE_FD(pollfd, pollinfo, mpi_errno);
    }

    pollfd->fd = -1;
    
    /* MT: need to wake up poll() if we are blocking in it */
    
    if (pollinfo->state == MPIDU_SOCKI_STATE_LISTENER)
    {
	pollfd->events &= ~POLLIN;
    }

/*#warning need to handle outstanding posted reads and writes.  should pull data out of read buffer.*/ /* Solaris complains about #warnings */
    
    mpi_errno = MPIDU_Socki_event_enqueue(sock->sock_set, MPIDU_SOCK_OP_CLOSE, 0, pollinfo->user_ptr, MPI_SUCCESS);
    if (mpi_errno != MPI_SUCCESS)
    { 
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM,
					 "**sock|poll|eqfail", "**sock|poll|eqfail %d", MPIDU_SOCK_OP_CLOSE);
	goto fn_exit;
    }

/*#warning where should the close really happen?  probably in sock_wait where it can block if need be.*/ /* Solaris complains about #warnings */
    if (pollinfo->fd != -1)
    { 
	flags = fcntl(pollinfo->fd, F_GETFL, 0);
	assert(flags != -1);
	rc = fcntl(pollinfo->fd, F_SETFL, flags & ~O_NONBLOCK);
	assert(rc != -1);
	close(pollinfo->fd);
    }
    MPIDU_Socki_sock_free(sock);

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CLOSE);
    return mpi_errno;
}
