/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_get_host_description
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_get_host_description(char * host_description, int len)
{
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_GET_HOST_DESCRIPTION);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_GET_HOST_DESCRIPTION);
    
    MPIDU_SOCKI_VERIFY_INIT(mpi_errno);
    if (len < 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_LEN,
					 "**sock|badhdmax", NULL);
	goto fn_exit;
    }

    rc = gethostname(host_description, len);
    if (rc == -1)
    {
	if (errno == EINVAL)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_HOST,
					     "**sock|badhdlen", NULL);
	}
	else if (errno == EFAULT)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_HOST,
					     "**sock|badhdbuf", NULL);
	}
	else
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL,
					     "**sock|oserror", "**sock|poll|oserror %d", errno);
	}
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_GET_HOST_DESCRIPTION);
    return mpi_errno;
    
}


#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_native_to_sock
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_native_to_sock(struct MPIDU_Sock_set * sock_set, MPIDU_SOCK_NATIVE_FD fd, void *user_ptr,
			      struct MPIDU_Sock ** sockp)
{
    struct MPIDU_Sock * sock = NULL;
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int rc;
    long flags;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_NATIVE_TO_SOCK);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_NATIVE_TO_SOCK);

    /* allocate sock and poll structures */
    mpi_errno = MPIDU_Socki_sock_alloc(sock_set, &sock);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM,
					 "**sock|sockalloc", NULL);
	goto fn_fail;
    }
    
    pollfd = MPIDU_Socki_get_pollfd_ptr(sock);
    pollinfo = MPIDU_Socki_get_pollinfo_ptr(sock);
    
    /* set file descriptor to non-blocking */
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

    /* initialize sock and poll structures */
    pollfd->fd = fd;
    pollfd->events = 0;
    pollfd->revents = 0;
    pollinfo->fd = fd;
    pollinfo->user_ptr = user_ptr;
    pollinfo->state = MPIDU_SOCKI_STATE_CONNECTED;

    *sockp = sock;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_NATIVE_TO_SOCK);
    return mpi_errno;

  fn_fail:
    if (sock != NULL)
    {
	MPIDU_Socki_sock_free(sock);
    }

    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_set_user_ptr
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_set_user_ptr(struct MPIDU_Sock * sock, void * user_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_SET_USER_PTR);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_SET_USER_PTR);
    
    MPIDU_SOCKI_VERIFY_INIT(mpi_errno);
    
    MPIDU_Socki_get_pollinfo_ptr(sock)->user_ptr = user_ptr;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_SET_USER_PTR);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_get_sock_id
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_get_sock_id(struct MPIDU_Sock * sock)
{
    int id;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_GET_SOCK_ID);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_GET_SOCK_ID);
    
    id = MPIDU_Socki_get_pollinfo_ptr(sock)->sock_id;

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_GET_SOCK_ID);
    return id;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_get_sock_set_id
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_get_sock_set_id(struct MPIDU_Sock_set * sock_set)
{
    int id;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_GET_SOCK_SET_ID);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_GET_SOCK_SET_ID);
    
    id = sock_set->id;

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_GET_SOCK_SET_ID);
    return id;
}
