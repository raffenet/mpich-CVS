/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_hostname_to_host_description
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_hostname_to_host_description(char *hostname, char *host_description, int len)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_HOSTNAME_TO_HOST_DESCRIPTION);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_HOSTNAME_TO_HOST_DESCRIPTION);
    
    MPIDU_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    
    if (MPIU_Strncpy(host_description, hostname, len))
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_LEN,
					 "**sock|badhdmax", 0);
    }
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_HOSTNAME_TO_HOST_DESCRIPTION);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_get_host_description
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_get_host_description(char * host_description, int len)
{
    char * env_hostname;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_GET_HOST_DESCRIPTION);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_GET_HOST_DESCRIPTION);
    
    MPIDU_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);
    if (len < 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_LEN,
					 "**sock|badhdmax", NULL);
	goto fn_exit;
    }

    /* Use hostname supplied in environment variable, if it exists */
    env_hostname = getenv("MPICH_INTERFACE_HOSTNAME");
    if (env_hostname != NULL)
    {
	rc = MPIU_Strncpy(host_description, env_hostname, len);
	if (rc != 0)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_HOST,
					     "**sock|badhdlen", NULL);
	}

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
					     "**sock|oserror", "**sock|poll|oserror %d %s", errno, MPIU_Strerror(errno));
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

    MPIDU_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);

    /* allocate sock and poll structures */
    mpi_errno = MPIDU_Socki_sock_alloc(sock_set, &sock);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM,
					 "**sock|sockalloc", NULL);
	goto fn_fail;
    }
    
    pollfd = MPIDU_Socki_sock_get_pollfd(sock);
    pollinfo = MPIDU_Socki_sock_get_pollinfo(sock);
    
    /* set file descriptor to non-blocking */
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

    /* initialize sock and poll structures */
    pollfd->fd = -1;
    pollfd->events = 0;
    pollfd->revents = 0;
    
    pollinfo->fd = fd;
    pollinfo->user_ptr = user_ptr;
    pollinfo->type = MPIDU_SOCKI_TYPE_COMMUNICATION;
    pollinfo->state = MPIDU_SOCKI_STATE_CONNECTED_RW;

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
    
    MPIDU_SOCKI_VERIFY_INIT(mpi_errno, fn_exit);

    if (sock != MPIDU_SOCK_INVALID_SOCK &&
	sock->sock_set != MPIDU_SOCK_INVALID_SET)
    {
	MPIDU_Socki_sock_get_pollinfo(sock)->user_ptr = user_ptr;
    }
    else
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK,
					 "**sock|badsock", NULL);
    }

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

    if (sock != MPIDU_SOCK_INVALID_SOCK)
    {
	if (sock->sock_set != MPIDU_SOCK_INVALID_SET)
	{
	    id = MPIDU_Socki_sock_get_pollinfo(sock)->sock_id;
	}
	else
	{
	    id = -1;
	}
    }
    else
    {
	id = -1;
    }

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

    if (sock_set != MPIDU_SOCK_INVALID_SET)
    {    
	id = sock_set->id;
    }
    else
    {
	id = -1;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_GET_SOCK_SET_ID);
    return id;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_get_error_class_string
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_get_error_class_string(int error, char *error_string, int length)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_GET_ERROR_CLASS_STRING);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_GET_ERROR_CLASS_STRING);
    switch (MPIR_ERR_GET_CLASS(error))
    {
    case MPIDU_SOCK_ERR_FAIL:
	MPIU_Strncpy(error_string, "generic socket failure", length);
	break;
    case MPIDU_SOCK_ERR_INIT:
	MPIU_Strncpy(error_string, "socket module not initialized", length);
	break;
    case MPIDU_SOCK_ERR_NOMEM:
	MPIU_Strncpy(error_string, "not enough memory to complete the socket operation", length);
	break;
    case MPIDU_SOCK_ERR_BAD_SET:
	MPIU_Strncpy(error_string, "invalid socket set", length);
	break;
    case MPIDU_SOCK_ERR_BAD_SOCK:
	MPIU_Strncpy(error_string, "invalid socket", length);
	break;
    case MPIDU_SOCK_ERR_BAD_HOST:
	MPIU_Strncpy(error_string, "host description buffer not large enough", length);
	break;
    case MPIDU_SOCK_ERR_BAD_HOSTNAME:
	MPIU_Strncpy(error_string, "invalid host name", length);
	break;
    case MPIDU_SOCK_ERR_BAD_PORT:
	MPIU_Strncpy(error_string, "invalid port", length);
	break;
    case MPIDU_SOCK_ERR_BAD_BUF:
	MPIU_Strncpy(error_string, "invalid buffer", length);
	break;
    case MPIDU_SOCK_ERR_BAD_LEN:
	MPIU_Strncpy(error_string, "invalid length", length);
	break;
    case MPIDU_SOCK_ERR_SOCK_CLOSED:
	MPIU_Strncpy(error_string, "socket closed", length);
	break;
    case MPIDU_SOCK_ERR_CONN_CLOSED:
	MPIU_Strncpy(error_string, "socket connection closed", length);
	break;
    case MPIDU_SOCK_ERR_CONN_FAILED:
	MPIU_Strncpy(error_string, "socket connection failed", length);
	break;
    case MPIDU_SOCK_ERR_INPROGRESS:
	MPIU_Strncpy(error_string, "socket operation in progress", length);
	break;
    case MPIDU_SOCK_ERR_TIMEOUT:
	MPIU_Strncpy(error_string, "socket operation timed out", length);
	break;
    case MPIDU_SOCK_ERR_INTR:
	MPIU_Strncpy(error_string, "socket operation interrupted", length);
	break;
    case MPIDU_SOCK_ERR_NO_NEW_SOCK:
	MPIU_Strncpy(error_string, "no new connection available", length);
	break;
    default:
	MPIU_Snprintf(error_string, length, "unknown socket error %d", error);
	break;
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_GET_ERROR_CLASS_STRING);
    return MPI_SUCCESS;
}
