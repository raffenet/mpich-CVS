/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */


static int MPIDU_Socki_read(struct pollfd * const pollfd, struct pollinfo * const pollinfo);
static int MPIDU_Socki_write(struct pollfd * const pollfd, struct pollinfo * const pollinfo);

int MPIDU_Socki_finalize_outstanding_ops(struct pollfd * pollfd, struct pollinfo * pollinfo, int mpi_errno_in);
int MPIDU_Socki_handle_immediate_os_errors(struct pollfd * pollfd, struct pollinfo * pollinfo, int unix_errno,
					   char * fcname, int line);
static int MPIDU_Socki_adjust_iov(ssize_t nb, MPID_IOV * const iov, const int count, int * const offsetp);
static int MPIDU_Socki_sock_alloc(struct MPIDU_Sock_set * sock_set, struct MPIDU_Sock ** sockp);
static void MPIDU_Socki_sock_free(struct MPIDU_Sock * sock);
static int MPIDU_Socki_event_enqueue(struct MPIDU_Sock_set * sock_set, enum MPIDU_Sock_op op, MPIU_Size_t num_bytes,
				     void * user_ptr, int error);
static int inline MPIDU_Socki_event_dequeue(struct MPIDU_Sock_set * sock_set, struct MPIDU_Sock_event * eventp);


#define MPIDU_Socki_get_pollfd_ptr(sock_)   (&(sock_)->sock_set->pollfds[(sock_)->elem])
#define MPIDU_Socki_get_pollinfo_ptr(sock_) (&(sock_)->sock_set->pollinfos[(sock_)->elem])


#define MPIDU_SOCKI_EVENT_ENQUEUE(sock_set_, op_, nb_, user_ptr_, mpi_errno_, label_)				\
{														\
    mpi_errno = MPIDU_Socki_event_enqueue((sock_set_), (op_), (nb_), (user_ptr_), (mpi_errno_));		\
    if (mpi_errno != MPI_SUCCESS)										\
    { 														\
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,	\
					 "**sock|poll|eqfail", "**sock|poll|eqfail %d %d %d",			\
					 pollinfo->sock_set->id, pollinfo->sock_id, (op_));			\
	goto label_;												\
    }														\
}


#define MPIDU_SOCKI_HANDLE_RW_ERR(pollfd_, pollinfo_, mpi_errno_, label_)				\
{													\
    if ((pollfd_)->events & POLLIN)									\
    {													\
	pollfd->events &= ~POLLIN;									\
	MPIDU_SOCKI_EVENT_ENQUEUE((pollinfo_)->sock_set, MPIDU_SOCK_OP_READ, (pollinfo_)->read_nb,	\
				  (pollinfo_)->user_ptr, (mpi_errno_), label_);				\
    }													\
													\
    if (pollfd->events & POLLOUT)									\
    {													\
	pollfd->events &= ~POLLOUT;									\
	MPIDU_SOCKI_EVENT_ENQUEUE((pollinfo_)->sock_set, MPIDU_SOCK_OP_WRITE, (pollinfo_)->write_nb,	\
				  (pollinfo_)->user_ptr, (mpi_errno_), label_);				\
    }													\
}


#define MPIDU_SOCKI_HANDLE_CONNECT_ERR(pollfd_, pollinfo_, mpi_errno_, label_)				\
{													\
    MPIDU_SOCKI_EVENT_ENQUEUE((pollinfo_)->sock_set, MPIDU_SOCK_OP_CONNECT, 0, (pollinfo_)->user_ptr,	\
			      (mpi_errno_), label_);							\
}


/*
 * Validation tests
 */
#define MPIDU_SOCKI_VERIFY_INIT(mpi_errno_)										\
{															\
    if (MPIDU_Socki_initialized <= 0)											\
    {															\
	(mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT,	\
					 "**sock|uninit", NULL);							\
	goto fn_exit;													\
    }															\
}


#define MPIDU_SOCKI_VALIDATE_SOCK(sock_, mpi_errno_)										\
{																\
    if ((sock_) == NULL || (sock_)->sock_set == NULL || (sock_)->elem < 0 || (sock_)->elem >= (sock_)->sock_set->poll_n_elem)	\
    {																\
	(mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK,	\
					    "**sock|badsock", NULL);								\
	goto fn_exit;														\
    }																\
}


#define MPIDU_SOCKI_VERIFY_CONNECTED(pollinfo_, mpi_errno_)									\
{																\
    if ((pollinfo_)->state == MPIDU_SOCKI_STATE_CONNECTING)									\
    {																\
	(mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK,	\
					    "**sock|notconnected", "**sock|notconnected %d %d",					\
					    (pollinfo_)->sock_set->id, (pollinfo_)->sock_id);					\
	goto fn_exit;														\
    }																\
    else if ((pollinfo_)->state == MPIDU_SOCKI_STATE_CONN_CLOSED)								\
    {																\
	(mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_CONN_CLOSED,	\
					    "**sock|connclosed", "**sock|connclosed %d %d",					\
					    (pollinfo_)->sock_set->id, (pollinfo_)->sock_id);					\
	goto fn_exit;														\
    }																\
    else if ((pollinfo_)->state == MPIDU_SOCKI_STATE_CONN_FAILED)								\
    {																\
	(mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_CONN_FAILED,	\
					    "**sock|connfailed", "**sock|connfailed %d %d",					\
					    (pollinfo_)->sock_set->id, (pollinfo_)->sock_id);					\
	goto fn_exit;														\
    }																\
    else if ((pollinfo_)->state == MPIDU_SOCKI_STATE_CLOSING)									\
    {																\
	(mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INPROGRESS,	\
					    "**sock|closing", "**sock|closing %d %d",						\
					    (pollinfo_)->sock_set->id, (pollinfo_)->sock_id);					\
																\
	goto fn_exit;														\
    }																\
 																\
    else if ((pollinfo_)->state != MPIDU_SOCKI_STATE_CONNECTED)									\
    {																\
	(mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK,	\
					    "**sock|badsock", NULL);								\
	goto fn_exit;														\
    }																\
}


#define MPIDU_SOCKI_VALIDATE_FD(pollfd_, pollinfo_, mpi_errno_)									\
{																\
    if ((pollfd_)->fd < 0)													\
    {																\
	(mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK,	\
					    "**sock|badhandle", "**sock|poll|badhandle %d %d %d",				\
					    (pollinfo_)->sock_set->id, (pollinfo_)->sock_id, (pollfd_)->fd);			\
	goto fn_exit;														\
    }																\
}


#define MPIDU_SOCKI_VERIFY_SOCK_READABLE(pollfd_, pollinfo_, mpi_errno_)							\
{																\
    if (((pollfd_)->events & POLLIN) != 0)											\
    {																\
	(mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INPROGRESS,	\
					    "**sock|reads", "**sock|reads %d %d",						\
					    (pollinfo_)->sock_set->id, (pollinfo_)->sock_id);					\
	goto fn_exit;														\
    }																\
}


#define MPIDU_SOCKI_VERIFY_SOCK_WRITABLE(pollfd_, pollinfo_, mpi_errno_)							\
{																\
    if (((pollfd_)->events & POLLOUT) != 0)											\
    {																\
	(mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INPROGRESS,	\
					    "**sock|writes", "**sock|writes %d %d",						\
					    (pollinfo_)->sock_set->id, (pollinfo_)->sock_id);					\
	goto fn_exit;														\
    }																\
}



#undef FUNCNAME
#define FUNCNAME MPIDU_Socki_read
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int MPIDU_Socki_read(struct pollfd * const pollfd, struct pollinfo * const pollinfo)
{
    int nb;
    int mpi_errno2;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_READ);
    MPIDI_STATE_DECL(MPID_STATE_READV);
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCKI_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCKI_READ);

    do
    {
	if (pollinfo->read_iov_flag)
	{ 
	    MPIDI_FUNC_ENTER(MPID_STATE_READV);
	    nb = readv(pollfd->fd, pollinfo->read.iov.ptr + pollinfo->read.iov.offset,
		       pollinfo->read.iov.count - pollinfo->read.iov.offset);
	    MPIDI_FUNC_EXIT(MPID_STATE_READV);
	}
	else
	{
	    MPIDI_FUNC_ENTER(MPID_STATE_READ);
	    nb = read(pollfd->fd, pollinfo->read.buf.ptr + pollinfo->read_nb,
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
	    pollfd->events &= ~POLLIN;
	    MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo->sock_set, MPIDU_SOCK_OP_READ, pollinfo->read_nb, pollinfo->user_ptr,
				      MPI_SUCCESS, fn_exit);
	}
    }
    else if (nb == 0)
    {
	close(pollfd->fd);
	pollfd->fd = -1;
	pollinfo->fd = -1;
	
	pollinfo->state = MPIDU_SOCKI_STATE_CONN_CLOSED;
	mpi_errno2 = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
					  MPIDU_SOCK_ERR_CONN_CLOSED, "**sock|connclosed", "**sock|connclosed %d %d",
					  pollinfo->sock_set->id, pollinfo->sock_id);
	MPIDU_SOCKI_HANDLE_RW_ERR(pollfd, pollinfo, mpi_errno2, fn_exit);
    }
    else
    {
	if (errno != EAGAIN && errno != EWOULDBLOCK)
	{
#warning handle read errors
	    pollfd->events &= ~POLLIN;
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_CONN_FAILED, "**sock|connfailed", "**sock|poll|connfailed %d", errno);
#if 0
	    sock_errno = socki_errno_to_sock_errno(errno);
	    socki_event_enqueue(pollinfo->sock->sock_set, SOCK_OP_READ, pollinfo->read_nb, pollinfo->user_ptr, sock_errno);
#endif	    
	}
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCKI_READ);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDU_Socki_write
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int MPIDU_Socki_write(struct pollfd * const pollfd, struct pollinfo * const pollinfo)
{
    int nb;
    int mpi_errno2;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_WRITE);
    MPIDI_STATE_DECL(MPID_STATE_WRITEV);
    MPIDI_STATE_DECL(MPID_STATE_SOCKI_HANDLE_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKI_HANDLE_WRITE);

    do
    {
	if (pollinfo->write_iov_flag)
	{ 
	    MPIDI_FUNC_ENTER(MPID_STATE_WRITEV);
	    nb = writev(pollfd->fd, pollinfo->write.iov.ptr + pollinfo->write.iov.offset,
			pollinfo->write.iov.count - pollinfo->write.iov.offset);
	    MPIDI_FUNC_EXIT(MPID_STATE_WRITEV);
	}
	else
	{
	    MPIDI_FUNC_ENTER(MPID_STATE_WRITE);
	    nb = write(pollfd->fd, pollinfo->write.buf.ptr + pollinfo->write_nb,
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
	    pollfd->events &= ~POLLOUT;
	    MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo->sock_set, MPIDU_SOCK_OP_WRITE, pollinfo->write_nb, pollinfo->user_ptr,
				      MPI_SUCCESS, fn_exit);
	}
    }
    else
    {
	if (errno != EAGAIN && errno != EWOULDBLOCK)
	{
#warning handle write errors
	    pollfd->events &= ~POLLOUT;
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_CONN_FAILED, "**sock|conn_failed", "**sock|conn_failed %d", errno);
#if 0	    
	    sock_errno = socki_errno_to_sock_errno(errno);
	    socki_event_enqueue(pollinfo->sock->sock_set, SOCK_OP_WRITE, pollinfo->write_nb, pollinfo->user_ptr, sock_errno);
#endif	    
	}
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_SOCKI_HANDLE_WRITE);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDU_Socki_finalize_outstanding_ops
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Socki_finalize_outstanding_ops(struct pollfd * pollfd, struct pollinfo * pollinfo, int mpi_errno_in)
{
    int mpi_errno =  MPI_SUCCESS;
    
#warning fix this error handling
    if (pollfd->events & POLLIN)
    {
	/* FIXME: this should read any data remaining in the receive buffer */
	
	mpi_errno = MPIDU_Socki_event_enqueue(pollinfo->sock_set, MPIDU_SOCK_OP_READ, pollinfo->read_nb, pollinfo->user_ptr,
					      mpi_errno_in);
	if (mpi_errno != MPI_SUCCESS)
	{ 
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					     "**sock|poll|eqfail", "**sock|poll|eqfail %d", MPIDU_SOCK_OP_READ);
	}
	
	pollfd->events &= ~POLLIN;
    }
	
    if (pollfd->events & POLLOUT)
    {
	mpi_errno = MPIDU_Socki_event_enqueue(pollinfo->sock_set, MPIDU_SOCK_OP_WRITE, pollinfo->write_nb, pollinfo->user_ptr,
					      mpi_errno_in);
	if (mpi_errno != MPI_SUCCESS)
	{ 
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					     "**sock|poll|eqfail", "**sock|poll|eqfail %d", MPIDU_SOCK_OP_WRITE);
	}
	
	pollfd->events &= ~POLLOUT;
    }

    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDU_Socki_handle_immediate_os_errors
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Socki_handle_immediate_os_errors(struct pollfd * pollfd, struct pollinfo * pollinfo, int unix_errno,
					   char * fcname, int line)
{
    int mpi_errno2;
    int mpi_errno = MPI_SUCCESS;

    if (unix_errno == EBADF)
    {
	if (pollinfo->state > 0 && pollinfo->state < MPIDU_SOCKI_STATE_LAST)
	{ 
	    if (pollfd->fd == -1)
	    {
		if (pollinfo->state == MPIDU_SOCKI_STATE_CONN_CLOSED)
		{ 
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, fcname, line, MPIDU_SOCK_ERR_CONN_CLOSED,
						     "**sock|connclosed", "**sock|connclosed %d %d",
						     pollinfo->sock_set->id, pollinfo->sock_id);
		}
		else if (pollinfo->state == MPIDU_SOCKI_STATE_CONN_FAILED)
		{
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, fcname, line, MPIDU_SOCK_ERR_CONN_FAILED,
						     "**sock|connfailed", "**sock|connfailed %d %d",
						     pollinfo->sock_set->id, pollinfo->sock_id);
		}
		else if (pollinfo->state == MPIDU_SOCKI_STATE_CLOSING)
		{
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, fcname, line, MPIDU_SOCK_ERR_INPROGRESS,
						     "**sock|closing", "**sock|closing %d %d",
						     pollinfo->sock_set->id, pollinfo->sock_id);
		}
		else if (pollinfo->state == MPIDU_SOCKI_STATE_UNCONNECTED)
		{
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, fcname, line, MPIDU_SOCK_ERR_SOCK_CLOSED,
						     "**sock|closed", NULL);
		}
		else
		{
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, fcname, line, MPIDU_SOCK_ERR_BAD_SOCK,
						     "**sock|badsock", NULL);
		}
	    }
	    else
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, fcname, line, MPIDU_SOCK_ERR_BAD_SOCK,
						 "**sock|badhandle", "**sock|poll|badhandle %d %d %d",
						 pollinfo->sock_set->id, pollinfo->sock_id, pollfd->fd);
	    }
	}
	else
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, fcname, line, MPIDU_SOCK_ERR_BAD_SOCK,
					     "**sock|badsock", NULL);
	}
    }
    else if (unix_errno == EFAULT)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, fcname, line, MPIDU_SOCK_ERR_BAD_BUF,
					 "**sock|badbuf", "**sock|badbuf %d %d", pollinfo->sock_set->id, pollinfo->sock_id);
    }
    else if (unix_errno == ECONNRESET || unix_errno == EPIPE)
    {
	/* MT: this probably needs to be atomic with respect to other operations on the same fd */
#warning fix this error handling
	pollfd->fd = -1;
	pollinfo->state = MPIDU_SOCKI_STATE_CONN_FAILED;
	close(pollinfo->fd);
	pollinfo->fd = -1;

	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, fcname, line, MPIDU_SOCK_ERR_CONN_FAILED,
					 "**sock|connfailed", "**sock|poll|connfailed %d %d %d",
					 pollinfo->sock_set->id, pollinfo->sock_id, unix_errno);

	mpi_errno2 = MPIDU_Socki_finalize_outstanding_ops(pollfd, pollinfo, mpi_errno);
	if (mpi_errno2 != MPI_SUCCESS)
	{
	    mpi_errno = mpi_errno2;
	}
    }
    else if (unix_errno == ENOMEM)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, fcname, line, MPIDU_SOCK_ERR_NOMEM,
					 "**sock|osnomem", "**sock|osnomem %d %d",
					 pollinfo->sock_set->id, pollinfo->sock_id);
    }
    else
    {
	/* MT: this probably needs to be atomic with respect to other operations on the same fd */
#warning fix this error handling
	pollfd->fd = -1;
	pollinfo->state = MPIDU_SOCKI_STATE_CONN_FAILED;
	close(pollinfo->fd);
	pollinfo->fd = -1;
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, fcname, line, MPIDU_SOCK_ERR_CONN_FAILED,
					 "**sock|oserror", "**sock|poll|oserror %d %d %d",
					 pollinfo->sock_set->id, pollinfo->sock_id, unix_errno);
	
	mpi_errno2 = MPIDU_Socki_finalize_outstanding_ops(pollfd, pollinfo, mpi_errno);
	if (mpi_errno2 != MPI_SUCCESS)
	{
	    mpi_errno = mpi_errno2;
	}
    }

    return mpi_errno;
}


/*
 * MPIDU_Socki_adjust_iov()
 *
 * Use the specified number of bytes (nb) to adjust the iovec and associated values.  If the iovec has been consumed, return
 * true; otherwise return false.
 */
#undef FUNCNAME
#define FUNCNAME MPIDU_Socki_adjust_iov
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int MPIDU_Socki_adjust_iov(ssize_t nb, MPID_IOV * const iov, const int count, int * const offsetp)
{
    int offset = *offsetp;
    
    while (offset < count)
    {
	if (iov[offset].MPID_IOV_LEN <= nb)
	{
	    nb -= iov[offset].MPID_IOV_LEN;
	    offset++;
	}
	else
	{
	    iov[offset].MPID_IOV_BUF = (char *) iov[offset].MPID_IOV_BUF + nb;
	    iov[offset].MPID_IOV_LEN -= nb;
	    *offsetp = offset;
	    return FALSE;
	}
    }
    
    *offsetp = offset;
    return TRUE;
}


#undef FUNCNAME
#define FUNCNAME MPIDU_Socki_sock_alloc
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int MPIDU_Socki_sock_alloc(struct MPIDU_Sock_set * sock_set, struct MPIDU_Sock ** sockp)
{
    struct MPIDU_Sock * sock = NULL;
    int elem;
    struct pollfd * fds = NULL;
    struct pollinfo * infos = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCKI_SOCK_ALLOC);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCKI_SOCK_ALLOC);
    
    sock = MPIU_Malloc(sizeof(struct MPIDU_Sock));
    if (sock == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM, "**nomem", 0);
	goto fn_exit;
    }

    /* MT: need to wake up all any call to poll() and then lock the sock set.  is this sufficient? */
    {
	for (elem = 0; elem < sock_set->poll_arr_sz; elem++)
	{
	    if (sock_set->pollinfos[elem].sock_id == -1)
	    {
		if (elem >= sock_set->poll_n_elem)
		{
		    sock_set->poll_n_elem = elem + 1;
		}

		break;
	    }
	}

	/* No free pollfd and pollinfo elements.  Resize... */
	if (elem == sock_set->poll_arr_sz)
	{
	    /* MT: we must guarantee that the code is not in poll when we do this! */
	    int elem2;
	    
	    fds = MPIU_Malloc((sock_set->poll_arr_sz + MPIDU_SOCK_SET_DEFAULT_SIZE) * sizeof(struct pollfd));
	    if (fds == NULL)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM,
						 "**nomem", 0);
		goto lock_exit;
	    }
	    infos = MPIU_Malloc((sock_set->poll_arr_sz + MPIDU_SOCK_SET_DEFAULT_SIZE) * sizeof(struct pollinfo));
	    if (infos == NULL)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM,
						 "**nomem", 0);
		goto lock_exit;
	    }

	    if (sock_set->poll_arr_sz > 0)
	    {
		/* Copy information from old arrays */
		memcpy(fds, sock_set->pollfds, sock_set->poll_arr_sz * sizeof(struct pollfd));
		memcpy(infos, sock_set->pollinfos, sock_set->poll_arr_sz * sizeof(struct pollinfo));

		/* Free old arrays... */
		MPIU_Free(sock_set->pollfds);
		MPIU_Free(sock_set->pollinfos);
	    }
    
	    sock_set->poll_n_elem = elem + 1;
	    sock_set->poll_arr_sz += MPIDU_SOCK_SET_DEFAULT_SIZE;
	    sock_set->pollfds = fds;
	    sock_set->pollinfos = infos;
	    
	    /* Initialize new elements */
	    for (elem2 = elem; elem2 < sock_set->poll_arr_sz; elem2++)
	    {
		fds[elem2].fd = -1;
		fds[elem2].events = 0;
		fds[elem2].revents = 0;
		infos[elem2].fd = -1;
		infos[elem2].sock_set = sock_set;
		infos[elem2].elem = elem2;
		infos[elem2].sock_id = -1;
	    }
	}

	/* Verify that memory hasn't been messed up */
	assert(sock_set->pollinfos[elem].sock_set == sock_set);
	assert(sock_set->pollinfos[elem].elem == elem);
	assert(sock_set->pollinfos[elem].fd == -1);
	assert(sock_set->pollfds[elem].fd == -1);
	assert(sock_set->pollfds[elem].events == 0);
	assert(sock_set->pollfds[elem].revents == 0);

	/* Initialize newly allocated sock structure and associated poll structures */
	sock_set->pollinfos[elem].sock_id = (sock_set->id << 24) | elem;
	sock->sock_set = sock_set;
	sock->elem = elem;

      lock_exit:
    }
    /* MT: release sock_set lock */

    if (mpi_errno == MPI_SUCCESS)
    {
	*sockp = sock;
    }
    else
    {
	if (infos != NULL)
	{
	    MPIU_Free(infos);
	}

	if (fds != NULL)
	{
	    MPIU_Free(fds);
	}
	
	if (sock != NULL)
	{
	    MPIU_Free(sock);
	}
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCKI_SOCK_ALLOC);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDU_Socki_sock_free
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static void MPIDU_Socki_sock_free(struct MPIDU_Sock * sock)
{
    struct pollfd * pollfd = MPIDU_Socki_get_pollfd_ptr(sock);
    struct pollinfo * pollinfo = MPIDU_Socki_get_pollinfo_ptr(sock);
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCKI_SOCK_FREE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCKI_SOCK_FREE);

    /* MT: need to lock the set */
    
    /* compress poll array */
    if (sock->elem + 1 == sock->sock_set->poll_n_elem)
    { 
	sock->sock_set->poll_n_elem -= 1;
    }

    /* remove entry from the poll list and mark the entry as free */
    pollfd->fd = -1;
    pollfd->events = 0;
    pollfd->revents = 0;
    pollinfo->sock_id = -1;
    pollinfo->fd = -1;

    /* mark the sock as invalid so that any future use might be caught */
    sock->sock_set = NULL;
    sock->elem = -1;
    
    MPIU_Free(sock);
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCKI_SOCK_FREE);
}


#undef FUNCNAME
#define FUNCNAME MPIDU_Socki_event_enqueue
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int MPIDU_Socki_event_enqueue(struct MPIDU_Sock_set * sock_set, MPIDU_Sock_op_t op, MPIU_Size_t num_bytes,
				     void * user_ptr, int error)
{
    struct MPIDU_Socki_eventq_elem * eventq_elem;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_SOCKI_EVENT_ENQUEUE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKI_EVENT_ENQUEUE);

    if (MPIDU_Socki_eventq_pool != NULL)  /* MT: eventq_pool needs to be locked */
    {
	eventq_elem = MPIDU_Socki_eventq_pool;
	MPIDU_Socki_eventq_pool = MPIDU_Socki_eventq_pool->next;
    }
    else
    {
	int i;
	
	eventq_elem = MPIU_Malloc(sizeof(struct MPIDU_Socki_eventq_elem) * MPIDU_SOCK_EVENTQ_POOL_SIZE);
	if (eventq_elem == NULL)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
					     "**sock|poll|eqmalloc", 0);
	    goto fn_exit;
	}

	if (MPIDU_SOCK_EVENTQ_POOL_SIZE > 1)
	{ 
	    MPIDU_Socki_eventq_pool = &eventq_elem[1];
	    for (i = 0; i < MPIDU_SOCK_EVENTQ_POOL_SIZE - 2; i++)
	    {
		MPIDU_Socki_eventq_pool[i].next = &MPIDU_Socki_eventq_pool[i+1];
	    }
	    MPIDU_Socki_eventq_pool[MPIDU_SOCK_EVENTQ_POOL_SIZE - 2].next = NULL;
	}
    }
    
    eventq_elem->event.op_type = op;
    eventq_elem->event.num_bytes = num_bytes;
    eventq_elem->event.user_ptr = user_ptr;
    eventq_elem->event.error = error;
    eventq_elem->next = NULL;

    /* MT: eventq is not thread safe */
    if (sock_set->eventq_head == NULL)
    { 
	sock_set->eventq_head = eventq_elem;
    }
    else
    {
	sock_set->eventq_tail->next = eventq_elem;
    }
    sock_set->eventq_tail = eventq_elem;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_SOCKI_EVENT_ENQUEUE);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDU_Socki_event_dequeue
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static inline int MPIDU_Socki_event_dequeue(struct MPIDU_Sock_set * sock_set, struct MPIDU_Sock_event * eventp)
{
    struct MPIDU_Socki_eventq_elem * eventq_elem;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_SOCKI_EVENT_DEQUEUE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKI_EVENT_DEQUEUE);

    /* MT: eventq is not thread safe */
    if (sock_set->eventq_head != NULL)
    {
	eventq_elem = sock_set->eventq_head;
	
	sock_set->eventq_head = eventq_elem->next;
	if (eventq_elem->next == NULL)
	{
	    sock_set->eventq_tail = NULL;
	}
	
	*eventp = eventq_elem->event;
	
	eventq_elem->next = MPIDU_Socki_eventq_pool;  /* MT: eventq_pool needs to be locked */
	MPIDU_Socki_eventq_pool = eventq_elem;
    }
    else
    {
	mpi_errno = MPIDU_SOCK_ERR_FAIL;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKI_EVENT_DEQUEUE);
    return mpi_errno;
}


