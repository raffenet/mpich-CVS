/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "sock.h"
#include "mpiimpl.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/uio.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <netdb.h>
#include <errno.h>

#if !defined(SOCK_SET_DEFAULT_SIZE)
#define SOCK_SET_DEFAULT_SIZE 16
#endif

#if !defined(SOCK_EVENTQ_POOL_SIZE)
#define SOCK_EVENTQ_POOL_SIZE 16
#endif

#define SOCKI_QUOTE(A) SOCKI_QUOTE2(A)
#define SOCKI_QUOTE2(A) #A

enum sock_state
{
    SOCK_STATE_UNCONNECTED,
    SOCK_STATE_LISTENING,
    SOCK_STATE_CONNECTING,
    SOCK_STATE_CONNECTED,
    SOCK_STATE_CLOSED,
    SOCK_STATE_FAILED
};

struct pollinfo
{
    struct sock * sock;
    void * user_ptr;
    enum sock_state state;
    SOCK_IOV * read_iov;
    int read_iov_count;
    int read_iov_offset;
    sock_size_t read_nb;
    SOCK_IOV read_iov_1;
    sock_progress_update_func_t read_progress_update_fn;
    SOCK_IOV * write_iov;
    int write_iov_count;
    int write_iov_offset;
    sock_size_t write_nb;
    SOCK_IOV write_iov_1;
    sock_progress_update_func_t write_progress_update_fn;
};

struct sock_eventq_elem
{
    struct sock_event event;
    struct sock_eventq_elem * next;
};

struct sock_set
{
    int poll_arr_sz;
    int poll_n_elem;
    int starting_elem;
    struct pollfd * pollfds;
    struct pollinfo * pollinfos;
    struct sock_eventq_elem * eventq_head;
    struct sock_eventq_elem * eventq_tail;
};

struct sock
{
    int fd;
    struct sock_set * sock_set;
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
};

static struct sock_eventq_elem * eventq_pool = NULL;

static void inline socki_handle_accept(struct pollfd * const pollfd, struct pollinfo * const pollinfo);
static void inline socki_handle_connect(struct pollfd * const pollfd, struct pollinfo * const pollinfo);
static void inline socki_handle_read(struct pollfd * const pollfd, struct pollinfo * const pollinfo);
static void inline socki_handle_write(struct pollfd * const pollfd, struct pollinfo * const pollinfo);
static int socki_sock_alloc(struct sock_set * sock_set, struct sock ** sockp);
static void socki_sock_free(struct sock * sock);
static void socki_event_enqueue(struct sock_set * sock_set, sock_op_t op, sock_size_t num_bytes, void * user_ptr, int error);
static int inline socki_event_dequeue(struct sock_set * sock_set, sock_event_t * eventp);
static int socki_adjust_iov(MPIDI_msg_sz_t nb, SOCK_IOV * const iov, const int count, int * const offsetp);
static int socki_errno_to_sock_errno(int error);

#undef FUNCNAME
#define FUNCNAME sock_init
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_init(void)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCK_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_INIT);
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_INIT);
    return SOCK_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME sock_finalize
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_finalize(void)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCK_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_FINALIZE);
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_FINALIZE);
    return SOCK_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME sock_create_set
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_create_set(sock_set_t * sock_set)
{
    struct sock_set * set;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_CREATE_SET);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_CREATE_SET);
    
    set = MPIU_Malloc(sizeof(struct sock_set));
    if (sock_set == NULL) return SOCK_FAIL;
    set->poll_arr_sz = 0;
    set->poll_n_elem = -1;
    set->starting_elem = 0;
    set->pollfds = NULL;
    set->pollinfos = NULL;
    set->eventq_head = NULL;
    set->eventq_tail = NULL;
    *sock_set = set;
    
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_CREATE_SET);
    return SOCK_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME sock_destroy_set
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_destroy_set(sock_set_t sock_set)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCK_DESTROY_SET);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_DESTROY_SET);
    
    /* XXX: close unclosed socks in set?? */
    MPIU_Free(sock_set->pollinfos);
    MPIU_Free(sock_set->pollfds);
    MPIU_Free(sock_set);
    
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_DESTROY_SET);
    return SOCK_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME sock_set_user_ptr
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_set_user_ptr(sock_t sock, void * user_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCK_SET_USER_PTR);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_SET_USER_PTR);
    
    sock->pollinfo->user_ptr = user_ptr;

    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_SET_USER_PTR);
    return SOCK_SUCCESS;
}


#undef FUNCNAME
#define FUNCNAME sock_native_to_sock
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_native_to_sock(sock_set_t sock_set, SOCK_NATIVE_FD fd, void *user_ptr, sock_t *sockp)
{
    struct sock * sock;
    int rc;
    long flags;
    int sock_errno = SOCK_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_NATIVE_TO_SOCK);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_NATIVE_TO_SOCK);

    /* set file descriptor to non-blocking */
    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
	sock_errno = SOCK_FAIL;
	goto fn_exit;
    }
    
    rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if (rc == -1)
    {
	sock_errno = SOCK_FAIL;
	goto fn_exit;
    }
    
    /* allocate and initialize sock and poll structures */
    sock_errno = socki_sock_alloc(sock_set, &sock);
    if (sock_errno != SOCK_SUCCESS)
    {
	sock_errno = SOCK_ERR_NOMEM;
	goto fn_exit;
    }

    sock->fd = fd;
    sock->pollfd->fd = fd;
    sock->pollfd->events = 0;
    sock->pollfd->revents = 0;
    sock->pollinfo->user_ptr = user_ptr;
    sock->pollinfo->state = SOCK_STATE_CONNECTED;

    *sockp = sock;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_NATIVE_TO_SOCK);
    return sock_errno;
}


#undef FUNCNAME
#define FUNCNAME sock_listen
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_listen(sock_set_t sock_set, void * user_ptr, int * port, sock_t * sockp)
{
    int fd;
    long flags;
    struct sockaddr_in addr;
    socklen_t addr_len;
    struct sock * sock;
    int rc;
    int sock_errno;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_LISTEN);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_LISTEN);
    
     /* establish non-blocking listener */
    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
	sock_errno = SOCK_FAIL;
	goto fail;
    }

    if (*port != 0)
    {
	flags = 1;
	rc = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(long));
	if (rc == -1)
	{
	    sock_errno = SOCK_FAIL;
	    goto fail_close;
	}
    }

    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
	sock_errno = SOCK_FAIL;
	goto fail_close;
    }

    rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if (rc == -1)
    {
	sock_errno = SOCK_FAIL;
	goto fail_close;
    }
                
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((short) *port);
    rc = bind(fd, (struct sockaddr *) &addr, sizeof(addr));
    if (rc == -1)
    {
#ifdef EADDRINUSE
	if (errno == EADDRINUSE)
	{
	    sock_errno = SOCK_ERR_ADDR_INUSE;
	}
	else
	{
	    sock_errno = SOCK_FAIL;
	}
#else
	sock_errno = SOCK_FAIL;
#endif
	goto fail_close;
    }
    
    rc = listen(fd, SOMAXCONN);
    if (rc == -1)
    {
	sock_errno = SOCK_FAIL;
	goto fail_close;
    }

    /* get listener port */
    addr_len = sizeof(addr);
    rc = getsockname(fd, (struct sockaddr *) &addr, &addr_len);
    if (rc == -1)
    {
	sock_errno = SOCK_FAIL;
	goto fail_close;
    }
    *port = ntohs(addr.sin_port);

    /* allocate and initialize sock and poll structures */
    sock_errno = socki_sock_alloc(sock_set, &sock);
    if (sock_errno != SOCK_SUCCESS)
    {
	goto fail_close;
    }

    sock->fd = fd;
    sock->pollfd->fd = fd;
    sock->pollfd->events = POLLIN;
    sock->pollfd->revents = 0;
    sock->pollinfo->user_ptr = user_ptr;
    sock->pollinfo->state = SOCK_STATE_LISTENING;

    *sockp = sock;
    
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_LISTEN);
    return SOCK_SUCCESS;

  fail_close:
    close(fd);
  fail:
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_LISTEN);
    return sock_errno;
}

#undef FUNCNAME
#define FUNCNAME sock_post_connect
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_post_connect(sock_set_t sock_set, void * user_ptr, char * host, int port, sock_t * sockp)
{
    struct hostent * hostent;
    struct sockaddr_in addr;
    long flags;
    int nodelay;
    int fd;
    struct sock * sock;
    int rc;
    int sock_errno;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_POST_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_POST_CONNECT);

    /* create nonblocking socket */
    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
	sock_errno = SOCK_FAIL;
	goto fail;
    }

    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
	sock_errno = SOCK_FAIL;
	goto fail_close;
    }
    rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if (rc == -1)
    {
	sock_errno = SOCK_FAIL;
	goto fail_close;
    }

    nodelay = 1;
    rc = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
    if (rc != 0)
    {
	sock_errno = SOCK_FAIL;
	goto fail_close;
    }

    /* allocate and initialize sock and poll structures */
    sock_errno = socki_sock_alloc(sock_set, &sock);
    if (sock_errno != SOCK_SUCCESS)
    {
	goto fail_close;
    }
    sock->pollinfo->user_ptr = user_ptr;

    sock->fd = fd;
    sock->pollfd->fd = fd;
    sock->pollfd->events = 0;
    sock->pollfd->revents = 0;
    sock->pollinfo->user_ptr = user_ptr;
    sock->pollinfo->state = SOCK_STATE_CONNECTING;
    
    /* convert hostname to IP address */
    hostent = gethostbyname(host);
    if (hostent == NULL || hostent->h_addrtype != AF_INET)
    {
	socki_event_enqueue(sock->sock_set, SOCK_OP_CONNECT, 0, user_ptr, SOCK_ERR_HOST_LOOKUP);
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
	socki_event_enqueue(sock->sock_set, SOCK_OP_CONNECT, 0, user_ptr, SOCK_SUCCESS);
	sock->pollinfo->state = SOCK_STATE_CONNECTED;
    }
    else if (errno == EINPROGRESS)
    {
	/* connection pending */
	sock->pollfd->events |= POLLOUT | POLLERR;
    }
    else
    {
	if (errno == ECONNREFUSED)
	{
	    socki_event_enqueue(sock->sock_set, SOCK_OP_CONNECT, 0, user_ptr, SOCK_ERR_CONN_REFUSED);
	}
	else
	{
	    socki_event_enqueue(sock->sock_set, SOCK_OP_CONNECT, 0, user_ptr, SOCK_FAIL);
	}
    }

  fn_exit:
    *sockp = sock;
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_CONNECT);
    return SOCK_SUCCESS;

  fail_close:
    close(fd);
  fail:
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_CONNECT);
    return sock_errno;
}

#undef FUNCNAME
#define FUNCNAME sock_post_close
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_post_close(sock_t sock)
{
    int rc;
    int flags;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_POST_CLOSE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_POST_CLOSE);

    if (sock->pollinfo->state == SOCK_STATE_LISTENING)
    {
	sock->pollfd->events &= ~POLLIN;
    }

    if (sock->pollfd->events & (POLLIN | POLLOUT))
    {
	return SOCK_FAIL;
    }

    socki_event_enqueue(sock->sock_set, SOCK_OP_CLOSE, 0, sock->pollinfo->user_ptr, SOCK_SUCCESS);

    flags = fcntl(sock->fd, F_GETFL, 0);
    assert(flags != -1);
    rc = fcntl(sock->fd, F_SETFL, flags & ~O_NONBLOCK);
    assert(rc != -1);
    
    close(sock->fd);
    socki_sock_free(sock);

    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_CLOSE);
    return SOCK_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME sock_post_read
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_post_read(sock_t sock, void * buf, sock_size_t len, sock_progress_update_func_t fn)
{
    struct pollinfo * pollinfo = sock->pollinfo;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_POST_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_POST_READ);

    assert ((sock->pollfd->events & POLLIN) == 0);
    
    pollinfo->read_iov = &pollinfo->read_iov_1;
    pollinfo->read_iov_count = 1;
    pollinfo->read_iov_offset = 0;
    pollinfo->read_iov->SOCK_IOV_BUF = buf;
    pollinfo->read_iov->SOCK_IOV_LEN = len;
    pollinfo->read_nb = 0;
    pollinfo->read_progress_update_fn = fn;
    sock->pollfd->events |= POLLIN;
    
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_READ);
    return SOCK_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME sock_post_readv
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_post_readv(sock_t sock, SOCK_IOV * iov, int n, sock_progress_update_func_t fn)
{
    struct pollinfo * pollinfo = sock->pollinfo;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_POST_READV);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_POST_READV);

    assert ((sock->pollfd->events & POLLIN) == 0);
    
    pollinfo->read_iov = iov;
    pollinfo->read_iov_count = n;
    pollinfo->read_iov_offset = 0;
    pollinfo->read_nb = 0;
    pollinfo->read_progress_update_fn = fn;
    sock->pollfd->events |= POLLIN;
    
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_READV);
    return SOCK_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME sock_post_write
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_post_write(sock_t sock, void * buf, sock_size_t len, sock_progress_update_func_t fn)
{
    struct pollinfo * pollinfo = sock->pollinfo;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_POST_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_POST_WRITE);
    
    assert ((sock->pollfd->events & POLLOUT) == 0);
    
    pollinfo->write_iov = &pollinfo->write_iov_1;
    pollinfo->write_iov_count = 1;
    pollinfo->write_iov_offset = 0;
    pollinfo->write_iov->SOCK_IOV_BUF = buf;
    pollinfo->write_iov->SOCK_IOV_LEN = len;
    pollinfo->write_nb = 0;
    pollinfo->write_progress_update_fn = fn;
    sock->pollfd->events |= POLLOUT;
    
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_WRITE);
    return SOCK_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME sock_post_writev
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_post_writev(sock_t sock, SOCK_IOV * iov, int n, sock_progress_update_func_t fn)
{
    struct pollinfo * pollinfo = sock->pollinfo;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_POST_WRITEV);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_POST_WRITEV);
    
    assert ((sock->pollfd->events & POLLOUT) == 0);
    
    pollinfo->write_iov = iov;
    pollinfo->write_iov_count = n;
    pollinfo->write_iov_offset = 0;
    pollinfo->write_nb = 0;
    pollinfo->write_progress_update_fn = fn;
    sock->pollfd->events |= POLLOUT;
    
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_WRITEV);
    return SOCK_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME sock_wait
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_wait(sock_set_t sock_set, int millisecond_timeout, sock_event_t * eventp)
{
    int elem;
    int nfds;
    int found_active_elem = FALSE;
    int sock_errno = SOCK_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_WAIT);
    MPIDI_STATE_DECL(MPID_STATE_POLL);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_WAIT);

    for (;;)
    { 
	if (socki_event_dequeue(sock_set, eventp) == SOCK_SUCCESS)
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
	    sock_errno = SOCK_ERR_TIMEOUT;
	    break;
	}

	if (nfds == -1) 
	{
	    if (errno == ENOMEM)
	    {
		sock_errno = SOCK_ERR_NOMEM;
	    }
	    else
	    {
		assert(errno == ENOMEM);
		sock_errno = SOCK_FAIL;
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

	    assert((pollfd->revents & POLLNVAL) == 0);
	
	    if (found_active_elem == FALSE)
	    {
		found_active_elem = TRUE;
		sock_set->starting_elem = (elem + 1 < sock_set->poll_n_elem) ? elem + 1 : 0;
	    }

	    /* According to Stevens, some errors are reported as normal data and some are reported with POLLERR.  */
	    if (pollfd->revents & (POLLIN | POLLERR))
	    {
		if (pollinfo->state == SOCK_STATE_CONNECTED)
		{
		    socki_handle_read(pollfd, pollinfo);
		}
		else if (pollinfo->state == SOCK_STATE_LISTENING)
		{
		    socki_handle_accept(pollfd, pollinfo);
		}
		else
		{
		    /* The connection must have failed. */
		    if (pollfd->events & POLLIN)
		    { 
			socki_event_enqueue(pollinfo->sock->sock_set, SOCK_OP_READ, pollinfo->read_nb, pollinfo->user_ptr,
					    SOCK_ERR_CONN_FAILED);
			pollfd->events &= ~POLLIN;
		    }
		    if (pollfd->events & POLLOUT)
		    {
			if (pollinfo->state == SOCK_STATE_CONNECTING)
			{
			    socki_event_enqueue(pollinfo->sock->sock_set, SOCK_OP_CONNECT, 0, pollinfo->user_ptr, SOCK_ERR_CONN_FAILED);
			}
			else
			{
			    socki_event_enqueue(pollinfo->sock->sock_set, SOCK_OP_WRITE, pollinfo->write_nb, pollinfo->user_ptr, SOCK_ERR_CONN_FAILED);
			}
			pollfd->events &= ~POLLOUT;
		    }
		}
	    }

	    if (pollfd->revents & POLLOUT)
	    {
		if (pollinfo->state == SOCK_STATE_CONNECTED)
		{
		    socki_handle_write(pollfd, pollinfo);
		}
		else if (pollinfo->state == SOCK_STATE_CONNECTING)
		{
		    socki_handle_connect(pollfd, pollinfo);
		}
		else
		{
		    /* The connection must have failed. */
		    if (pollfd->events & POLLOUT)
		    { 
			socki_event_enqueue(pollinfo->sock->sock_set, SOCK_OP_WRITE, pollinfo->write_nb, pollinfo->user_ptr,
					    SOCK_ERR_CONN_FAILED);
			pollfd->events &= ~POLLOUT;
		    }
		}
	    }

	    nfds--;
	    elem = (elem + 1 < sock_set->poll_n_elem) ? elem + 1 : 0;
	}
    }
    
  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
    return sock_errno;
}

#undef FUNCNAME
#define FUNCNAME sock_accept
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_accept(sock_t listener, sock_set_t sock_set, void * user_ptr, sock_t * sockp)
{
    int fd;
    struct sockaddr_in addr;
    socklen_t addr_len;
    int sock_errno = SOCK_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_ACCEPT);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_ACCEPT);
    
    addr_len = sizeof(struct sockaddr_in);
    fd = accept(listener->fd, (struct sockaddr *) &addr, &addr_len);
    if (fd >= 0)
    {
	struct sock * sock;
	long flags;
	int nodelay;
	int rc;

	sock_errno = socki_sock_alloc(sock_set, &sock);
	if (sock_errno != SOCK_SUCCESS)
	{
	    goto fn_exit;
	}
	    
	flags = fcntl(fd, F_GETFL, 0);
	assert(flags != -1);
	rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	assert(rc != -1);
	    
	nodelay = 1;
	rc = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
	assert(rc == 0);

	sock->fd = fd;
	sock->pollfd->fd = fd;
	sock->pollfd->events = 0;
	sock->pollfd->revents = 0;
	sock->pollinfo->user_ptr = user_ptr;
	sock->pollinfo->state = SOCK_STATE_CONNECTED;

	*sockp = sock;
    }
    else
    {
	sock_errno = SOCK_FAIL;
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_ACCEPT);
    return sock_errno;
}

#undef FUNCNAME
#define FUNCNAME sock_read
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_read(sock_t sock, void * buf, sock_size_t len, sock_size_t * num_read)
{
    int nb;
    int sock_errno = SOCK_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_READ);
    MPIDI_STATE_DECL(MPID_STATE_SOCK_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_READ);
    
    assert ((sock->pollfd->events & POLLIN) == 0);
    
    do
    {
	MPIDI_FUNC_ENTER(MPID_STATE_READ);
	nb = read(sock->fd, buf, len);
	MPIDI_FUNC_EXIT(MPID_STATE_READ);
    }
    while (nb == -1 && errno == EINTR);

    if (nb > 0)
    {
	*num_read = nb;
    }
    else if (nb == 0)
    {
	*num_read = 0;
	sock_errno = SOCK_EOF;
    }
    else
    {
	if (errno == EAGAIN || errno == EWOULDBLOCK)
	{
	    *num_read = 0;
	}
	else
	{
	    sock_errno = socki_errno_to_sock_errno(errno);
	}
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_READ);
    return sock_errno;
}

#undef FUNCNAME
#define FUNCNAME sock_readv
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_readv(sock_t sock, SOCK_IOV * iov, int n, sock_size_t * num_read)
{
    int nb;
    int sock_errno = SOCK_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_READV);
    MPIDI_STATE_DECL(MPID_STATE_SOCK_READV);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_READV);
    
    assert ((sock->pollfd->events & POLLIN) == 0);
    
    do
    {
	MPIDI_FUNC_ENTER(MPID_STATE_READV);
	nb = readv(sock->fd, iov, n);
	MPIDI_FUNC_EXIT(MPID_STATE_READV);
    }
    while (nb == -1 && errno == EINTR);

    if (nb > 0)
    {
	*num_read = nb;
    }
    else if (nb == 0)
    {
	*num_read = 0;
	sock_errno = SOCK_EOF;
    }
    else
    {
	if (errno == EAGAIN || errno == EWOULDBLOCK)
	{
	    *num_read = 0;
	}
	else
	{
	    sock_errno = socki_errno_to_sock_errno(errno);
	}
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_READV);
    return sock_errno;
}

#undef FUNCNAME
#define FUNCNAME sock_write
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_write(sock_t sock, void * buf, sock_size_t len, sock_size_t * num_written)
{
    int nb;
    int sock_errno = SOCK_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_WRITE);
    MPIDI_STATE_DECL(MPID_STATE_SOCK_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_WRITE);
    
    assert ((sock->pollfd->events & POLLOUT) == 0);
    
    do
    {
	MPIDI_FUNC_ENTER(MPID_STATE_WRITE);
	nb = write(sock->fd, buf, len);
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
	    sock_errno = socki_errno_to_sock_errno(errno);
	}
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WRITE);
    return sock_errno;
}

#undef FUNCNAME
#define FUNCNAME sock_writev
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_writev(sock_t sock, SOCK_IOV * iov, int n, sock_size_t * num_written)
{
    int nb;
    int sock_errno = SOCK_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_WRITEV);
    MPIDI_STATE_DECL(MPID_STATE_SOCK_WRITEV);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_WRITEV);
    
    assert ((sock->pollfd->events & POLLOUT) == 0);
    
    do
    {
	MPIDI_FUNC_ENTER(MPID_STATE_WRITEV);
	nb = writev(sock->fd, iov, n);
	MPIDI_FUNC_EXIT(MPID_STATE_WRITEV);
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
	    sock_errno = socki_errno_to_sock_errno(errno);
	}
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WRITEV);
    return sock_errno;
}

#undef FUNCNAME
#define FUNCNAME sock_getid
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_getid(sock_t sock)
{
    int ret_val;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_GETID);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_GETID);
    ret_val = (int)sock->fd;
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_GETID);
    return ret_val;
}

#undef FUNCNAME
#define FUNCNAME sock_getsetid
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_getsetid(sock_set_t set)
{
    int ret_val;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_GETSETID);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_GETSETID);
    ret_val = (int)set;
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_GETSETID);
    return ret_val;
}

#undef FUNCNAME
#define FUNCNAME socki_handle_accept
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
static inline void socki_handle_accept(struct pollfd * const pollfd, struct pollinfo * const pollinfo)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCKI_HANDLE_ACCEPT);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKI_HANDLE_ACCEPT);
    
    socki_event_enqueue(pollinfo->sock->sock_set, SOCK_OP_ACCEPT, 0, pollinfo->user_ptr, SOCK_SUCCESS);
    
    MPIDI_FUNC_EXIT(MPID_STATE_SOCKI_HANDLE_ACCEPT);
}

#undef FUNCNAME
#define FUNCNAME socki_handle_connect
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
static inline void socki_handle_connect(struct pollfd * const pollfd, struct pollinfo * const pollinfo)
{
    struct sockaddr_in addr;
    socklen_t addr_len;
    int rc;
    MPIDI_STATE_DECL(MPID_STATE_SOCKI_HANDLE_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKI_HANDLE_CONNECT);

    pollfd->events &= ~POLLOUT;
    
    addr_len = sizeof(struct sockaddr_in);
    rc = getpeername(pollfd->fd, (struct sockaddr *) &addr, &addr_len);
    if (rc == 0)
    {
	socki_event_enqueue(pollinfo->sock->sock_set, SOCK_OP_CONNECT, 0, pollinfo->user_ptr, SOCK_SUCCESS);
	pollinfo->state = SOCK_STATE_CONNECTED;
    }
    else
    {
	/* FIXME: if getpeername() returns ENOTCONN, then we can now use getsockopt() to get the errno associated with the failed
	   connect(). */
	assert(errno != ENOTCONN);
	socki_event_enqueue(pollinfo->sock->sock_set, SOCK_OP_CONNECT, 0, pollinfo->user_ptr, SOCK_ERR_CONN_FAILED);
	pollinfo->state = SOCK_STATE_FAILED;
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_SOCKI_HANDLE_CONNECT);
}

#undef FUNCNAME
#define FUNCNAME socki_handle_read
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
static inline void socki_handle_read(struct pollfd * const pollfd, struct pollinfo * const pollinfo)
{
    int nb;
    MPIDI_STATE_DECL(MPID_STATE_READV);
    MPIDI_STATE_DECL(MPID_STATE_SOCKI_HANDLE_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKI_HANDLE_READ);

    assert(pollfd->events & POLLIN);
    
    do
    {
	MPIDI_FUNC_ENTER(MPID_STATE_READV);
	nb = readv(pollfd->fd, pollinfo->read_iov + pollinfo->read_iov_offset,
		   pollinfo->read_iov_count - pollinfo->read_iov_offset);
	MPIDI_FUNC_EXIT(MPID_STATE_READV);
    }
    while (nb < 0 && errno == EINTR);

    if (nb > 0)
    {
	pollinfo->read_nb += nb;
		
	if (socki_adjust_iov(nb, pollinfo->read_iov, pollinfo->read_iov_count, &pollinfo->read_iov_offset))
	{
	    pollfd->events &= ~POLLIN;
	    socki_event_enqueue(pollinfo->sock->sock_set, SOCK_OP_READ, pollinfo->read_nb, pollinfo->user_ptr, SOCK_SUCCESS);
	}
    }
    else if (nb == 0)
    {
	pollfd->events &= ~POLLIN;
	socki_event_enqueue(pollinfo->sock->sock_set, SOCK_OP_READ, pollinfo->read_nb, pollinfo->user_ptr, SOCK_EOF);
    }
    else
    {
	if (errno != EAGAIN && errno != EWOULDBLOCK)
	{
	    int sock_errno;

	    sock_errno = socki_errno_to_sock_errno(errno);
	    pollfd->events &= ~POLLIN;
	    socki_event_enqueue(pollinfo->sock->sock_set, SOCK_OP_READ, pollinfo->read_nb, pollinfo->user_ptr, sock_errno);
	}
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_SOCKI_HANDLE_READ);
}

#undef FUNCNAME
#define FUNCNAME socki_handle_write
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
static inline void socki_handle_write(struct pollfd * const pollfd, struct pollinfo * const pollinfo)
{
    int nb;
    MPIDI_STATE_DECL(MPID_STATE_WRITEV);
    MPIDI_STATE_DECL(MPID_STATE_SOCKI_HANDLE_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKI_HANDLE_WRITE);

    do
    {
	MPIDI_FUNC_ENTER(MPID_STATE_WRITEV);
	nb = writev(pollfd->fd, pollinfo->write_iov + pollinfo->write_iov_offset,
		    pollinfo->write_iov_count - pollinfo->write_iov_offset);
	MPIDI_FUNC_EXIT(MPID_STATE_WRITEV);
    }
    while (nb < 0 && errno == EINTR);

    if (nb > 0)
    {
	/* printf("[%d] %s(): wrote %d bytes\n", MPIR_Process.comm_world->rank, FCNAME, nb); */
	pollinfo->write_nb += nb;
	
	if (socki_adjust_iov(nb, pollinfo->write_iov, pollinfo->write_iov_count, &pollinfo->write_iov_offset))
	{
	    pollfd->events &= ~POLLOUT;
	    socki_event_enqueue(pollinfo->sock->sock_set, SOCK_OP_WRITE, pollinfo->write_nb, pollinfo->user_ptr, SOCK_SUCCESS);
	}
    }
    else
    {
	assert(nb < 0);
	
	if (errno != EAGAIN && errno != EWOULDBLOCK)
	{
	    int sock_errno;

	    sock_errno = socki_errno_to_sock_errno(errno);
	    
	    pollfd->events &= ~POLLOUT;
	    socki_event_enqueue(pollinfo->sock->sock_set, SOCK_OP_WRITE, pollinfo->write_nb, pollinfo->user_ptr, sock_errno);
	}
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKI_HANDLE_WRITE);
}

#undef FUNCNAME
#define FUNCNAME socki_sock_alloc
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
static int socki_sock_alloc(struct sock_set * sock_set, struct sock ** sockp)
{
    struct sock * sock;
    int elem;
    struct pollfd * fds;
    struct pollinfo * infos;
    int sock_errno;
    MPIDI_STATE_DECL(MPID_STATE_SOCKI_SOCK_ALLOC);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKI_SOCK_ALLOC);
    
    sock = MPIU_Malloc(sizeof(struct sock));
    if (sock == NULL)
    {
	sock_errno = SOCK_ERR_NOMEM;
	goto fail;
    }
    
    for (elem = 0; elem < sock_set->poll_arr_sz; elem++)
    {
        if (sock_set->pollinfos[elem].sock == NULL)
        {
            if (elem >= sock_set->poll_n_elem)
            {
                sock_set->poll_n_elem = elem + 1;
            }
	    
	    /* Initialize new sock structure and associated poll structures */
	    sock_set->pollfds[elem].fd = -1;
	    sock_set->pollfds[elem].events = 0;
	    sock_set->pollfds[elem].revents = 0;
	    sock_set->pollinfos[elem].sock = sock;
	    sock_set->pollinfos[elem].read_iov = NULL;
	    sock_set->pollinfos[elem].write_iov = NULL;
	    sock->sock_set = sock_set;
	    sock->fd = -1;
	    sock->pollfd = &sock_set->pollfds[elem];
	    sock->pollinfo = &sock_set->pollinfos[elem];

	    *sockp = sock;
	    return SOCK_SUCCESS;
        }
    }

    /* No more pollfd and pollinfo elements.  Resize... */
    fds = MPIU_Malloc((sock_set->poll_arr_sz + SOCK_SET_DEFAULT_SIZE) * sizeof(struct pollfd));
    if (fds == NULL)
    {
	sock_errno = SOCK_ERR_NOMEM;
	goto fail_free_sock;
    }
    infos = MPIU_Malloc((sock_set->poll_arr_sz + SOCK_SET_DEFAULT_SIZE) * sizeof(struct pollinfo));
    if (infos == NULL)
    {
	sock_errno = SOCK_ERR_NOMEM;
	goto fail_free_fds;
    }

    if (sock_set->poll_arr_sz > 0)
    {
	/* Copy information from old arrays */
        memcpy(fds, sock_set->pollfds, sock_set->poll_arr_sz * sizeof(struct pollfd));
        memcpy(infos, sock_set->pollinfos, sock_set->poll_arr_sz * sizeof(struct pollinfo));

	/* Fix up pollfd and pollinfo pointer in sock structure; also correct read_iov and write_iov in pollinfo structure */
	for (elem = 0; elem < sock_set->poll_arr_sz; elem++)
	{
	    struct pollinfo * const oldinfo = &sock_set->pollinfos[elem];
	    struct sock * const sock = oldinfo->sock;
	    
	    sock->pollfd = &fds[elem];
	    sock->pollinfo = &infos[elem];

	    if (oldinfo->read_iov == &oldinfo->read_iov_1)
	    {
		infos[elem].read_iov = &infos[elem].read_iov_1;
	    }
	    
	    if (oldinfo->write_iov == &oldinfo->write_iov_1)
	    {
		infos[elem].write_iov = &infos[elem].write_iov_1;
	    }
	}

	/* Free old arrays... */
	MPIU_Free(sock_set->pollfds);
	MPIU_Free(sock_set->pollinfos);
    }
    
    sock_set->poll_n_elem = elem + 1;
    sock_set->poll_arr_sz += SOCK_SET_DEFAULT_SIZE;
    sock_set->pollfds = fds;
    sock_set->pollinfos = infos;
	
    /* Initialize new sock structure and associated poll structures */
    sock_set->pollfds[elem].fd = -1;
    sock_set->pollfds[elem].events = 0;
    sock_set->pollfds[elem].revents = 0;
    sock_set->pollinfos[elem].sock = sock;
    sock_set->pollinfos[elem].read_iov = NULL;
    sock_set->pollinfos[elem].write_iov = NULL;
    sock->sock_set = sock_set;
    sock->fd = -1;
    sock->pollfd = &sock_set->pollfds[elem];
    sock->pollinfo = &sock_set->pollinfos[elem];
    
    /* Initialize new unallocated elements */
    for (elem = elem + 1; elem < sock_set->poll_arr_sz; elem++)
    {
        fds[elem].fd = -1;
	fds[elem].events = 0;
	fds[elem].revents = 0;
	infos[elem].sock = NULL;
    }

    *sockp = sock;
    
    MPIDI_FUNC_EXIT(MPID_STATE_SOCKI_SOCK_ALLOC);
    return SOCK_SUCCESS;

  fail_free_fds:
    MPIU_Free(fds);
  fail_free_sock:
    MPIU_Free(sock);
  fail:
    MPIDI_FUNC_EXIT(MPID_STATE_SOCKI_SOCK_ALLOC);
    return sock_errno;
}

#undef FUNCNAME
#define FUNCNAME socki_sock_free
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
static void socki_sock_free(struct sock * sock)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCKI_SOCK_FREE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKI_SOCK_FREE);
    
    /* TODO: compress poll array */
    
    sock->pollfd->fd = -1;
    sock->pollfd->events = 0;
    sock->pollfd->revents = 0;
    sock->pollinfo->sock = NULL;
    
    MPIU_Free(sock);

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKI_SOCK_FREE);
}

#undef FUNCNAME
#define FUNCNAME socki_event_enqueue
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
static void socki_event_enqueue(struct sock_set * sock_set, sock_op_t op, sock_size_t num_bytes, void * user_ptr, int error)
{
    struct sock_eventq_elem * eventq_elem;
    MPIDI_STATE_DECL(MPID_STATE_SOCKI_EVENT_ENQUEUE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKI_EVENT_ENQUEUE);

    if (eventq_pool != NULL)  /* MT: eventq_pool needs to be locked */
    {
	eventq_elem = eventq_pool;
	eventq_pool = eventq_pool->next;
    }
    else
    {
	int i;
	
	eventq_elem = MPIU_Malloc(sizeof(struct sock_eventq_elem) * SOCK_EVENTQ_POOL_SIZE);
	if (eventq_elem == NULL)
	{
	    int mpi_errno;
	    
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**sock|poll|eqmalloc", 0);
	    MPID_Abort(NULL, mpi_errno, 13);
	}

	eventq_pool = &eventq_elem[1];
	for (i = 0; i < SOCK_EVENTQ_POOL_SIZE - 2; i++)
	{
	    eventq_pool[i].next = &eventq_pool[i+1];
	}
	eventq_pool[SOCK_EVENTQ_POOL_SIZE-2].next = NULL;
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

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKI_EVENT_ENQUEUE);
}

#undef FUNCNAME
#define FUNCNAME socki_event_dequeue
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
static inline int socki_event_dequeue(struct sock_set * sock_set, sock_event_t * eventp)
{
    struct sock_eventq_elem * eventq_elem;
    int sock_errno = SOCK_SUCCESS;
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
	
	eventq_elem->next = eventq_pool;  /* MT: eventq_pool needs to be locked */
	eventq_pool = eventq_elem;
    }
    else
    {
	sock_errno = SOCK_FAIL;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKI_EVENT_DEQUEUE);
    return sock_errno;
}


/*
 * socki_adjust_iov()
 *
 * Use the specified number of bytes (nb) to adjust the iovec and associated values.  If the iovec has been consumed, return
 * true; otherwise return false.
 */
#undef FUNCNAME
#define FUNCNAME socki_adjust_iov
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
static int socki_adjust_iov(MPIDI_msg_sz_t nb, SOCK_IOV * const iov, const int count, int * const offsetp)
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

static int g_last_os_error = 0;

static int socki_errno_to_sock_errno(int unix_errno)
{
    int sock_errno;
    
    if (unix_errno == EBADF)
    {
	sock_errno = SOCK_ERR_BAD_SOCK;
    }
    else if (unix_errno == EFAULT)
    {
	sock_errno = SOCK_ERR_BAD_BUFFER;
    }
    else if (unix_errno == ECONNRESET || unix_errno == EPIPE)
    {
	sock_errno = SOCK_ERR_CONN_FAILED;
    }
    else if (unix_errno == ENOMEM)
    {
	sock_errno = SOCK_ERR_NOMEM;
    }
    else
    {
	g_last_os_error = unix_errno;
	sock_errno = SOCK_ERR_OS_SPECIFIC;
    }

    return sock_errno;
}

#undef FUNCNAME
#define FUNCNAME sock_get_last_os_error
#undef FCNAME
#define FCNAME SOCKI_QUOTE(FUNCNAME)
int sock_get_last_os_error(void)
{
    return g_last_os_error;
}
