/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */


static int MPIDU_Socki_os_to_mpi_errno(struct pollinfo * pollinfo, int os_errno, char * fcname, int line, int * conn_failed);

static int MPIDU_Socki_adjust_iov(ssize_t nb, MPID_IOV * const iov, const int count, int * const offsetp);

static int MPIDU_Socki_sock_alloc(struct MPIDU_Sock_set * sock_set, struct MPIDU_Sock ** sockp);
static void MPIDU_Socki_sock_free(struct MPIDU_Sock * sock);

static int MPIDU_Socki_event_enqueue(struct pollinfo * pollinfo, enum MPIDU_Sock_op op, MPIU_Size_t num_bytes,
				     void * user_ptr, int error);
static int inline MPIDU_Socki_event_dequeue(struct MPIDU_Sock_set * sock_set,
					    struct pollinfo ** pollinfop, struct MPIDU_Sock_event * eventp);



#define MPIDU_Socki_sock_get_pollfd(sock_)          (&(sock_)->sock_set->pollfds[(sock_)->elem])
#define MPIDU_Socki_sock_get_pollinfo(sock_)        (&(sock_)->sock_set->pollinfos[(sock_)->elem])
#define MPIDU_Socki_pollinfo_get_pollfd(pollinfo_) (&(pollinfo_)->sock_set->pollfds[(pollinfo_)->elem])


/* Enqueue a new event.  If the enqueue fails, generate an error and jump to the fail_label_ */
#define MPIDU_SOCKI_EVENT_ENQUEUE(pollinfo_, op_, nb_, user_ptr_, event_mpi_errno_, mpi_errno_, fail_label_)	\
{														\
    mpi_errno_ = MPIDU_Socki_event_enqueue((pollinfo_), (op_), (nb_), (user_ptr_), (event_mpi_errno_));		\
    if (mpi_errno_ != MPI_SUCCESS)										\
    {														\
	mpi_errno_ = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,	\
					  "**sock|poll|eqfail", "**sock|poll|eqfail %d %d %d",			\
					  pollinfo->sock_set->id, pollinfo->sock_id, (op_));			\
	goto fail_label_;											\
    }														\
}


#define MPIDU_SOCKI_POLLFD_OP_SET(pollfd_, pollinfo_, op_)	\
{								\
    (pollfd_)->events |= (op_);					\
    (pollfd_)->fd = (pollinfo_)->fd;				\
}

#define MPIDU_SOCKI_POLLFD_OP_ISSET(pollfd_, pollinfo_, op_) ((pollfd_)->events & (op_))

#define MPIDU_SOCKI_POLLFD_OP_CLEAR(pollfd_, pollinfo_, op_)	\
{								\
    (pollfd_)->events &= ~(op_);				\
    (pollfd_)->revents &= ~(op_);				\
    if (((pollfd_)->events & (POLLIN | POLLOUT)) == 0)		\
    {								\
	(pollfd_)->fd = -1;					\
    }								\
								\
}


#define MPIDU_SOCKI_GET_SOCKET_ERROR(pollinfo_, os_errno_, mpi_errno_, fail_label_)				\
{														\
    int rc__;													\
    int sz__;													\
														\
    sz__ = sizeof(os_errno_);											\
    rc__ = getsockopt((pollinfo_)->fd, SOL_SOCKET, SO_ERROR, &(os_errno_), &sz__);				\
    if (rc__ != 0)												\
    {														\
	if (errno == ENOMEM || errno == ENOBUFS)								\
	{													\
	    mpi_errno_ = MPIR_Err_create_code(									\
		MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM, "**sock|osnomem",	\
		"**sock|osnomem %s %d %d", "getsockopt", pollinfo->sock_set->id, pollinfo->sock_id);		\
	}													\
	else													\
	{													\
	    mpi_errno = MPIR_Err_create_code(									\
		MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**sock|oserror",		\
		"**sock|poll|oserror %s %d %d %d %s", "getsockopt", pollinfo->sock_set->id, pollinfo->sock_id,	\
		 (os_errno_), MPIU_Strerror(os_errno_));							\
	}													\
														\
        goto fail_label_;											\
    }														\
}


/*
 * Validation tests
 */
#define MPIDU_SOCKI_VERIFY_INIT(mpi_errno_, fail_label_)								\
{															\
    if (MPIDU_Socki_initialized <= 0)											\
    {															\
	(mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT,	\
					 "**sock|uninit", NULL);							\
	goto fail_label_;												\
    }															\
}


#define MPIDU_SOCKI_VALIDATE_SOCK_SET(sock_, mpi_errno_, fail_label_)


#define MPIDU_SOCKI_VALIDATE_SOCK(sock_, mpi_errno_, fail_label_)								\
{																\
    struct pollinfo * pollinfo__;												\
																\
    if ((sock_) == NULL || (sock_)->sock_set == NULL || (sock_)->elem < 0 || (sock_)->elem >= (sock_)->sock_set->poll_n_elem)	\
    {																\
	(mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK,	\
					    "**sock|badsock", NULL);								\
	goto fail_label_;													\
    }																\
																\
    pollinfo__ = MPIDU_Socki_sock_get_pollinfo(sock_);										\
																\
    if (pollinfo__->type <= MPIDU_SOCKI_TYPE_FIRST || pollinfo__->type >= MPIDU_SOCKI_TYPE_INTERRUPTER ||			\
	pollinfo__->state <= MPIDU_SOCKI_STATE_FIRST || pollinfo__->state >= MPIDU_SOCKI_STATE_LAST)				\
    {																\
	(mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK,	\
					    "**sock|badsock", NULL);								\
	goto fail_label_;													\
    }																\
}


#define MPIDU_SOCKI_VERIFY_CONNECTED_READABLE(pollinfo_, mpi_errno_, fail_label_)						\
{																\
    if ((pollinfo_)->type == MPIDU_SOCKI_TYPE_COMMUNICATION)									\
    {																\
	if ((pollinfo_)->state == MPIDU_SOCKI_STATE_CONNECTING)									\
	{															\
	    (mpi_errno_) = MPIR_Err_create_code(										\
		(mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK, "**sock|notconnected",		\
		"**sock|notconnected %d %d", (pollinfo_)->sock_set->id, (pollinfo_)->sock_id);					\
	    goto fail_label_;													\
	}															\
	else if ((pollinfo_)->state == MPIDU_SOCKI_STATE_DISCONNECTED)								\
	{															\
	    if ((pollinfo_)->os_errno == 0)											\
	    {															\
		(mpi_errno_) = MPIR_Err_create_code(										\
		    (mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_CONN_CLOSED, "**sock|connclosed",	\
		    "**sock|connclosed %d %d", (pollinfo_)->sock_set->id, (pollinfo_)->sock_id);				\
	    }															\
	    else														\
	    {															\
		(mpi_errno_) = MPIR_Err_create_code(										\
		    (mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_CONN_FAILED, "**sock|connfailed",	\
		    "**sock|poll|connfailed %d %d %d %s", (pollinfo_)->sock_set->id, (pollinfo_)->sock_id,			\
		    (pollinfo_)->os_errno, MPIU_Strerror((pollinfo_)->os_errno));						\
	    }															\
	    goto fail_label_;													\
	}															\
	else if ((pollinfo_)->state == MPIDU_SOCKI_STATE_CLOSING)								\
	{															\
	    (mpi_errno_) = MPIR_Err_create_code(										\
		(mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INPROGRESS, "**sock|closing",		\
		"**sock|closing %d %d", (pollinfo_)->sock_set->id, (pollinfo_)->sock_id);					\
																\
	    goto fail_label_;													\
	}															\
	else if ((pollinfo_)->state != MPIDU_SOCKI_STATE_CONNECTED_RW && (pollinfo_)->state != MPIDU_SOCKI_STATE_CONNECTED_RO)	\
	{															\
	    (mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK,	\
						"**sock|badsock", NULL);							\
	    goto fail_label_;													\
	}															\
    }																\
    else if ((pollinfo_)->type == MPIDU_SOCKI_TYPE_LISTENER)									\
    {																\
	(mpi_errno_) = MPIR_Err_create_code(											\
	    (mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK, "**sock|listener_read",		\
	    "**sock|listener_read %d %d", (pollinfo_)->sock_set->id, (pollinfo_)->sock_id);					\
																\
	goto fail_label_;													\
    }																\
}


#define MPIDU_SOCKI_VERIFY_CONNECTED_WRITABLE(pollinfo_, mpi_errno_, fail_label_)						 \
{																 \
    if ((pollinfo_)->type == MPIDU_SOCKI_TYPE_COMMUNICATION)									 \
    {																 \
	if ((pollinfo_)->state == MPIDU_SOCKI_STATE_CONNECTING)									 \
	{															 \
	    (mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK,	 \
						"**sock|notconnected", "**sock|notconnected %d %d",				 \
						(pollinfo_)->sock_set->id, (pollinfo_)->sock_id);				 \
	    goto fail_label_;													 \
	}															 \
	else if ((pollinfo_)->state == MPIDU_SOCKI_STATE_DISCONNECTED || (pollinfo_)->state == MPIDU_SOCKI_STATE_CONNECTED_RO)	 \
	{															 \
	    if ((pollinfo_)->os_errno == 0)											 \
	    {															 \
		(mpi_errno_) = MPIR_Err_create_code(										 \
		    (mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_CONN_CLOSED, "**sock|connclosed",	 \
		    "**sock|connclosed %d %d", (pollinfo_)->sock_set->id, (pollinfo_)->sock_id);				 \
	    }															 \
	    else														 \
	    {															 \
		(mpi_errno_) = MPIR_Err_create_code(										 \
		    (mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_CONN_FAILED, "**sock|connfailed",	 \
		    "**sock|poll|connfailed %d %d %d %s", (pollinfo_)->sock_set->id, (pollinfo_)->sock_id,			 \
		    (pollinfo_)->os_errno, MPIU_Strerror((pollinfo_)->os_errno));						 \
	    }															 \
	    goto fail_label_;													 \
	}															 \
	else if ((pollinfo_)->state == MPIDU_SOCKI_STATE_CLOSING)								 \
	{															 \
	    (mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INPROGRESS, \
						"**sock|closing", "**sock|closing %d %d",					 \
						(pollinfo_)->sock_set->id, (pollinfo_)->sock_id);				 \
																 \
	    goto fail_label_;													 \
	}															 \
	else if ((pollinfo_)->state != MPIDU_SOCKI_STATE_CONNECTED_RW)								 \
	{															 \
	    (mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK,	 \
						"**sock|badsock", NULL);							 \
	    goto fail_label_;													 \
	}															 \
    }																 \
    else if ((pollinfo_)->type == MPIDU_SOCKI_TYPE_LISTENER)									 \
    {																 \
	(mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK,	 \
					    "**sock|listener_write", "**sock|listener_write %d %d",				 \
					    (pollinfo_)->sock_set->id, (pollinfo_)->sock_id);					 \
																 \
	goto fail_label_;													 \
    }																 \
}


#define MPIDU_SOCKI_VALIDATE_FD(pollinfo_, mpi_errno_, fail_label_)								\
{																\
    if ((pollinfo_)->fd < 0)													\
    {																\
	(mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK,	\
					    "**sock|badhandle", "**sock|poll|badhandle %d %d %d",				\
					    (pollinfo_)->sock_set->id, (pollinfo_)->sock_id, (pollinfo_)->fd);			\
	goto fail_label_;													\
    }																\
}


#define MPIDU_SOCKI_VERIFY_NO_POSTED_READ(pollfd_, pollinfo_, mpi_errno_, fail_label_)						\
{																\
    if (MPIDU_SOCKI_POLLFD_OP_ISSET((pollfd_), (pollinfo_), POLLIN))								\
    {																\
	(mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INPROGRESS,	\
					    "**sock|reads", "**sock|reads %d %d",						\
					    (pollinfo_)->sock_set->id, (pollinfo_)->sock_id);					\
	goto fail_label_;													\
    }																\
}


#define MPIDU_SOCKI_VERIFY_NO_POSTED_WRITE(pollfd_, pollinfo_, mpi_errno_, fail_label_)						\
{																\
    if (MPIDU_SOCKI_POLLFD_OP_ISSET((pollfd_), (pollinfo_), POLLOUT))							\
    {																\
	(mpi_errno_) = MPIR_Err_create_code((mpi_errno_), MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INPROGRESS,	\
					    "**sock|writes", "**sock|writes %d %d",						\
					    (pollinfo_)->sock_set->id, (pollinfo_)->sock_id);					\
	goto fail_label_;													\
    }																\
}


/*
 * MPIDU_Socki_os_to_mpi_errno()
 *
 * This routine assumes that no thread can change the state between state check before the nonblocking OS operation and the call
 * to this routine.
 */
#undef FUNCNAME
#define FUNCNAME MPIDU_Socki_os_to_mpi_errno
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Socki_os_to_mpi_errno(struct pollinfo * pollinfo, int os_errno, char * fcname, int line, int * disconnected)
{
    int mpi_errno;

    if (os_errno == ENOMEM || os_errno == ENOBUFS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, fcname, line, MPIDU_SOCK_ERR_NOMEM,
					 "**sock|osnomem", "**sock|poll|osnomem %d %d %d %s",
					 pollinfo->sock_set->id, pollinfo->sock_id, os_errno, MPIU_Strerror(os_errno));
	*disconnected = FALSE;
    }
    else if (os_errno == EFAULT || os_errno == EINVAL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, fcname, line, MPIDU_SOCK_ERR_BAD_BUF,
					 "**sock|badbuf", "**sock|poll|badbuf %d %d %d %s",
					 pollinfo->sock_set->id, pollinfo->sock_id, os_errno, MPIU_Strerror(os_errno));
	*disconnected = FALSE;
    }
    else if (os_errno == EPIPE)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, fcname, line, MPIDU_SOCK_ERR_CONN_CLOSED,
					 "**sock|connclosed", "**sock|poll|connclosed %d %d %d %s",
					 pollinfo->sock_set->id, pollinfo->sock_id, os_errno, MPIU_Strerror(os_errno));
	*disconnected = TRUE;
    }
    else if (os_errno == ECONNRESET || os_errno == ENOTCONN || os_errno == ETIMEDOUT)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, fcname, line, MPIDU_SOCK_ERR_CONN_FAILED,
					 "**sock|connfailed", "**sock|poll|connfailed %d %d %d %s",
					 pollinfo->sock_set->id, pollinfo->sock_id, os_errno, MPIU_Strerror(os_errno));
	pollinfo->os_errno = os_errno;
	*disconnected = TRUE;
    }
    else if (os_errno == EBADF)
    {
	/*
	 * If we have a bad file descriptor, then either the sock was bad to start with and we didn't catch it in the preliminary
	 * checks, or a sock closure was finalized after the preliminary checks were performed.  The latter should not happen if
	 * the thread safety code is correctly implemented.  In any case, the data structures associated with the sock are no
	 * longer valid and should not be modified.  We indicate this by returning a fatal error.
	 */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, fcname, line, MPIDU_SOCK_ERR_BAD_SOCK,
					 "**sock|badsock", NULL);
	*disconnected = FALSE;
    }
    else
    {
	/*
	 * Unexpected OS error.
	 *
	 * FIXME: technically we should never reach this section of code.  What's the right way to handle this situation?  Should
	 * we print an immediate message asking the user to report the errno so that we can plug the hole?
	 */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, fcname, line, MPIDU_SOCK_ERR_CONN_FAILED,
					 "**sock|oserror", "**sock|poll|oserror %d %d %d %s",
					 pollinfo->sock_set->id, pollinfo->sock_id, os_errno, MPIU_Strerror(os_errno));
	pollinfo->os_errno = os_errno;
	*disconnected = TRUE;
    }

    return mpi_errno;
}
/* end MPIDU_Socki_os_to_mpi_errno() */


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
/* end MPIDU_Socki_adjust_iov() */


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
	goto fn_fail;
    }

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
	int elem2;
	    
	fds = MPIU_Malloc((sock_set->poll_arr_sz + MPIDU_SOCK_SET_DEFAULT_SIZE) * sizeof(struct pollfd));
	if (fds == NULL)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM,
					     "**nomem", 0);
	    goto fn_fail;
	}
	infos = MPIU_Malloc((sock_set->poll_arr_sz + MPIDU_SOCK_SET_DEFAULT_SIZE) * sizeof(struct pollinfo));
	if (infos == NULL)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM,
					     "**nomem", 0);
	    goto fn_fail;
	}

	if (sock_set->poll_arr_sz > 0)
	{
	    /* Copy information from old arrays */
	    memcpy(fds, sock_set->pollfds, sock_set->poll_arr_sz * sizeof(struct pollfd));
	    memcpy(infos, sock_set->pollinfos, sock_set->poll_arr_sz * sizeof(struct pollinfo));

	    /* FIXME: MT: XXX: we must guarantee that the code is not in poll when we do this!!!  Perhaps we can leave them
               allocated, set a flag in the sock set, and have MPIDU_Sock_wait() deallocate them when it comes out of poll... */
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
	    infos[elem2].sock = NULL;
	    infos[elem2].sock_id = -1;
	    infos[elem2].type = 0;
	    infos[elem2].state = 0;
		
	}
    }

    /* Verify that memory hasn't been messed up */
    MPIU_Assert(sock_set->pollfds[elem].fd == -1);
    MPIU_Assert(sock_set->pollfds[elem].events == 0);
    MPIU_Assert(sock_set->pollfds[elem].revents == 0);
    MPIU_Assert(sock_set->pollinfos[elem].sock_set == sock_set);
    MPIU_Assert(sock_set->pollinfos[elem].elem == elem);
    MPIU_Assert(sock_set->pollinfos[elem].fd == -1);
    MPIU_Assert(sock_set->pollinfos[elem].sock == NULL);
    MPIU_Assert(sock_set->pollinfos[elem].sock_id == -1);
    MPIU_Assert(sock_set->pollinfos[elem].type == 0);
    MPIU_Assert(sock_set->pollinfos[elem].state == 0);

    /* Initialize newly allocated sock structure and associated poll structures */
    sock_set->pollinfos[elem].sock_id = (sock_set->id << 24) | elem;
    sock_set->pollinfos[elem].sock = sock;
    sock->sock_set = sock_set;
    sock->elem = elem;


    *sockp = sock;
    
  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCKI_SOCK_ALLOC);
    return mpi_errno;

  fn_fail:
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
    
    goto fn_exit;
}
/* end MPIDU_Socki_sock_alloc() */


#undef FUNCNAME
#define FUNCNAME MPIDU_Socki_sock_free
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static void MPIDU_Socki_sock_free(struct MPIDU_Sock * sock)
{
    struct pollfd * pollfd = MPIDU_Socki_sock_get_pollfd(sock);
    struct pollinfo * pollinfo = MPIDU_Socki_sock_get_pollinfo(sock);
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCKI_SOCK_FREE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCKI_SOCK_FREE);

    /* compress poll array */
    if (sock->elem + 1 == sock->sock_set->poll_n_elem)
    { 
	sock->sock_set->poll_n_elem -= 1;
    }

    /* remove entry from the poll list and mark the entry as free */
    pollfd->fd = -1;
    pollfd->events = 0;
    pollfd->revents = 0;
    pollinfo->fd = -1;
    pollinfo->sock = NULL;
    pollinfo->sock_id = -1;
    pollinfo->type = 0;
    pollinfo->state = 0;
		

    /* mark the sock as invalid so that any future use might be caught */
    sock->sock_set = NULL;
    sock->elem = -1;
    
    MPIU_Free(sock);
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCKI_SOCK_FREE);
}
/* end MPIDU_Socki_sock_free() */


#undef FUNCNAME
#define FUNCNAME MPIDU_Socki_event_enqueue
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int MPIDU_Socki_event_enqueue(struct pollinfo * pollinfo, MPIDU_Sock_op_t op, MPIU_Size_t num_bytes,
				     void * user_ptr, int error)
{
    struct MPIDU_Sock_set * sock_set = pollinfo->sock_set;
    struct MPIDU_Socki_eventq_elem * eventq_elem;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_SOCKI_EVENT_ENQUEUE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKI_EVENT_ENQUEUE);

    if (MPIDU_Socki_eventq_pool != NULL)
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
	    mpi_errno = MPIR_Err_create_code(errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
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
    eventq_elem->pollinfo = pollinfo;
    eventq_elem->next = NULL;

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
/* end MPIDU_Socki_event_enqueue() */


#undef FUNCNAME
#define FUNCNAME MPIDU_Socki_event_dequeue
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static inline int MPIDU_Socki_event_dequeue(struct MPIDU_Sock_set * sock_set,
					    struct pollinfo ** pollinfop, struct MPIDU_Sock_event * eventp)
{
    struct MPIDU_Socki_eventq_elem * eventq_elem;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_SOCKI_EVENT_DEQUEUE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKI_EVENT_DEQUEUE);

    if (sock_set->eventq_head != NULL)
    {
	eventq_elem = sock_set->eventq_head;
	
	sock_set->eventq_head = eventq_elem->next;
	if (eventq_elem->next == NULL)
	{
	    sock_set->eventq_tail = NULL;
	}
	
	*eventp = eventq_elem->event;
	*pollinfop = eventq_elem->pollinfo;
	
	eventq_elem->next = MPIDU_Socki_eventq_pool;
	MPIDU_Socki_eventq_pool = eventq_elem;
    }
    else
    {
	mpi_errno = MPIDU_SOCK_ERR_FAIL;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKI_EVENT_DEQUEUE);
    return mpi_errno;
}
/* end MPIDU_Socki_event_dequeue() */
