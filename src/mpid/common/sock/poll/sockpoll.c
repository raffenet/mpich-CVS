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

struct pollinfo
{
    struct sock * sock;
    void * user_ptr;
    SOCK_IOV * read_iov;
    int read_iov_n;
    SOCK_IOV read_iov_1;
    sock_progress_update_func_t read_progress_update_fn;
    SOCK_IOV * write_iov;
    int write_iov_n;
    SOCK_IOV write_iov_1;
    sock_progress_update_func_t write_progress_update_fn;
};

struct sock_set
{
    int sz;
    int n;
    struct pollfd * pollfds;
    struct pollinfo * pollinfos;
    struct sock_eventq_elem * eventq_head;
    struct sock_eventq_elem * eventq_tail;
};

struct sock
{
    struct sock_set * sock_set;
    struct pollfd * pollfd;
    struct pollinfo * pollinfo;
    int fd;
};

struct sock_eventq_elem
{
    struct sock * sock;
    struct sock_event event;
    struct sock_eventq_elem * next;
};


static int sock_alloc(struct sock_set * sock_set, int fd, struct sock ** sockp);
static void sock_free(struct sock * sock);
static int sock_set_enqueue_event(struct sock * sock, sock_op_t op, sock_size_t num_bytes, void * user_ptr, int error);
static int sock_set_dequeue_event(struct sock_set * sock_set, struct sock ** sockp, sock_event_t * eventp);


int sock_init(void)
{
    return SOCK_SUCCESS;
}

int sock_finalize(void)
{
    return SOCK_SUCCESS;
}

int sock_create_set(sock_set_t * sock_set)
{
    struct sock_set * set;
    
    set = MPIU_Malloc(sizeof(sock_set));
    if (sock_set == NULL) return SOCK_FAIL;
    set->sz = 0;
    set->n = -1;
    set->pollfds = NULL;
    set->pollinfos = NULL;
    *sock_set = set;
    return SOCK_SUCCESS;
}

int sock_destroy_set(sock_set_t sock_set)
{
    /* XXX: close unclosed socks in set?? */
    MPIU_Free(sock_set->pollinfos);
    MPIU_Free(sock_set->pollfds);
    MPIU_Free(sock_set);
    return SOCK_SUCCESS;
}

int sock_set_user_ptr(sock_t sock, void * user_ptr)
{
    sock->pollinfo->user_ptr = user_ptr;
    return SOCK_SUCCESS;
}


int sock_listen(sock_set_t sock_set, void * user_ptr, int * port, sock_t * sockp)
{
    int fd;
    long flags;
    struct sockaddr_in addr;
    socklen_t addr_len;
    int rc;
    struct sock * sock;
    
     /* establish non-blocking listener */
    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
	rc = SOCK_FAIL;
	goto fail;
    }

    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
	rc = SOCK_FAIL;
	goto fail_close;
    }
    
    rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if (rc == -1)
    {
	rc = SOCK_FAIL;
	goto fail_close;
    }
                
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(0);
    rc = bind(fd, (struct sockaddr *) &addr, sizeof(addr));
    if (rc == -1)
    {
	rc = SOCK_FAIL;
	goto fail_close;
    }
    
    rc = listen(fd, SOMAXCONN);
    if (rc == -1)
    {
	rc = SOCK_FAIL;
	goto fail_close;
    }

    /* get listener port */
    addr_len = sizeof(addr);
    rc = getsockname(fd, (struct sockaddr *) &addr, &addr_len);
    if (rc == -1)
    {
	rc = SOCK_FAIL;
	goto fail_close;
    }
    *port = ntohs(addr.sin_port);

    /* allocate and initialize sock and poll structures */
    rc = sock_alloc(sock_set, fd, &sock);
    if (rc != SOCK_SUCCESS)
    {
	goto fail_close;
    }

    sock->pollinfo->user_ptr = user_ptr;
    sock->pollfd->events = POLLIN;

    *sockp = sock;
    return SOCK_SUCCESS;

  fail_close:
    close(fd);
  fail:
    return rc;
}

int sock_post_connect(sock_set_t sock_set, void * user_ptr, char * host, int port, sock_t * sockp)
{
    struct hostent * hostent;
    struct sockaddr_in addr;
    long flags;
    int nodelay;
    int fd;
    struct sock * sock;
    int rc;

    /* create nonblocking socket */
    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
	rc = SOCK_FAIL;
	goto fail;
    }

    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
	rc = SOCK_FAIL;
	goto fail_close;
    }
    rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if (rc == -1)
    {
	rc = SOCK_FAIL;
	goto fail_close;
    }

    nodelay = 1;
    rc = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
    if (rc != 0)
    {
	rc = SOCK_FAIL;
	goto fail_close;
    }

    /* allocate and initialize sock and poll structures */
    rc = sock_alloc(sock_set, fd, &sock);
    if (rc != SOCK_SUCCESS)
    {
	goto fail_close;
    }
    sock->pollinfo->user_ptr = user_ptr;

    /* convert hostname to IP address */
    hostent = gethostbyname(host);
    if (hostent == NULL || hostent->h_addrtype != AF_INET)
    {
	rc = sock_set_enqueue_event(sock, SOCK_OP_CONNECT, 0, user_ptr, SOCK_ERR_HOST_LOOKUP);
	if (rc != SOCK_SUCCESS)
	{
	    goto fail_sock_free;
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
	/* TODO: queue sock in completion queue? */
	sock->pollfd->events |= POLLOUT;
    }
    else if (errno == EINPROGRESS)
    {
	/* connection pending */
	sock->pollfd->events |= POLLOUT;
    }
    else
    {
	if (errno == ECONNREFUSED)
	{ 
	    rc = sock_set_enqueue_event(sock, SOCK_OP_CONNECT, 0, user_ptr, SOCK_ERR_CONN_REFUSED);
	    if (rc != SOCK_SUCCESS)
	    {
		goto fail_sock_free;
	    }
	}
	else
	{
	    rc = sock_set_enqueue_event(sock, SOCK_OP_CONNECT, 0, user_ptr, SOCK_FAIL);
	    if (rc != SOCK_SUCCESS)
	    {
		goto fail_sock_free;
	    }
	}
    }

  fn_exit:
    *sockp = sock;
    return SOCK_SUCCESS;

  fail_sock_free:
    sock_free(sock);
  fail_close:
    close(fd);
  fail:
    return rc;
}

int sock_post_close(sock_t sock)
{
    /* XXX: what about active postings? */
    close(sock->fd);
    sock_free(sock);
    return SOCK_SUCCESS;
}

int sock_post_read(sock_t sock, void * buf, int len, sock_progress_update_func_t fn)
{
    struct pollinfo * pollinfo = sock->pollinfo;

    assert ((sock->pollfd->events & POLLIN) == 0);
    
    pollinfo->read_iov = &pollinfo->read_iov_1;
    pollinfo->read_iov_n = 1;
    pollinfo->read_iov->SOCK_IOV_BUF = buf;
    pollinfo->read_iov->SOCK_IOV_LEN = len;
    pollinfo->read_progress_update_fn = fn;
    sock->pollfd->events |= POLLIN;
    
    return SOCK_SUCCESS;
}

int sock_post_readv(sock_t sock, SOCK_IOV * iov, int n, sock_progress_update_func_t fn)
{
    struct pollinfo * pollinfo = sock->pollinfo;

    assert ((sock->pollfd->events & POLLIN) == 0);
    
    pollinfo->read_iov = iov;
    pollinfo->read_iov_n = n;
    pollinfo->read_progress_update_fn = fn;
    sock->pollfd->events |= POLLIN;
    
    return SOCK_SUCCESS;
}

int sock_post_write(sock_t sock, void * buf, int len, sock_progress_update_func_t fn)
{
    struct pollinfo * pollinfo = sock->pollinfo;
    
    assert ((sock->pollfd->events & POLLOUT) == 0);
    
    pollinfo->write_iov = &pollinfo->write_iov_1;
    pollinfo->write_iov_n = 1;
    pollinfo->write_iov->SOCK_IOV_BUF = buf;
    pollinfo->write_iov->SOCK_IOV_LEN = len;
    pollinfo->write_progress_update_fn = fn;
    sock->pollfd->events |= POLLOUT;
    
    return SOCK_SUCCESS;
}

int sock_post_writev(sock_t sock, SOCK_IOV * iov, int n, sock_progress_update_func_t fn)
{
    struct pollinfo * pollinfo = sock->pollinfo;
    
    assert ((sock->pollfd->events & POLLOUT) == 0);
    
    pollinfo->write_iov = iov;
    pollinfo->write_iov_n = n;
    pollinfo->write_progress_update_fn = fn;
    sock->pollfd->events |= POLLOUT;
    
    return SOCK_SUCCESS;
}

int sock_wait(sock_set_t sock_set, int millisecond_timeout, sock_event_t * eventp)
{
    struct sock * sock;
    int rc = SOCK_SUCCESS;
    
    if (sock_set_dequeue_event(sock_set, &sock, eventp) == SOCK_SUCCESS)
    {
	/* TODO: change sock state */
	goto fn_exit;
    }

  fn_exit:
    return rc;
}

int sock_accept(sock_set_t set, void * user_ptr, sock_t listener, sock_t * sockp)
{
    return SOCK_SUCCESS;
}

int sock_read(sock_t sock, void * buf, int len, int * num_read)
{
    int nb;
    int rc = SOCK_SUCCESS;
    
    assert ((sock->pollfd->events & POLLIN) == 0);
    
    do
    {
	nb = read(sock->fd, buf, len);
    }
    while (nb == -1 && errno == EINTR);

    if (nb > 0)
    {
	*num_read = nb;
    }
    else if (nb == 0)
    {
	*num_read = 0;
	rc = SOCK_EOF;
    }
    else
    {
	if (errno == EAGAIN || errno == EWOULDBLOCK)
	{
	    *num_read = 0;
	}
	else if (errno == EBADF)
	{
	    rc = SOCK_ERR_BAD_SOCK;
	}
	else if (errno == EFAULT)
	{
	    rc = SOCK_ERR_BAD_BUFFER;
	}
	else if (errno == ECONNRESET || errno == EPIPE)
	{
	    rc = SOCK_ERR_CONN_FAILED;
	}
	else if (errno == ENOMEM)
	{
	    rc = SOCK_ERR_NOMEM;
	}
	else
	{
	    rc = SOCK_FAIL;
	}
    }
    
    return rc;
}

int sock_readv(sock_t sock, SOCK_IOV * iov, int n, int * num_read)
{
    int nb;
    int rc = SOCK_SUCCESS;
    
    assert ((sock->pollfd->events & POLLIN) == 0);
    
    do
    {
	nb = readv(sock->fd, iov, n);
    }
    while (nb == -1 && errno == EINTR);

    if (nb > 0)
    {
	*num_read = nb;
    }
    else if (nb == 0)
    {
	*num_read = 0;
	rc = SOCK_EOF;
    }
    else
    {
	if (errno == EAGAIN || errno == EWOULDBLOCK)
	{
	    *num_read = 0;
	}
	else if (errno == EBADF)
	{
	    rc = SOCK_ERR_BAD_SOCK;
	}
	else if (errno == EFAULT)
	{
	    rc = SOCK_ERR_BAD_BUFFER;
	}
	else if (errno == ECONNRESET || errno == EPIPE)
	{
	    rc = SOCK_ERR_CONN_FAILED;
	}
	else if (errno == ENOMEM)
	{
	    rc = SOCK_ERR_NOMEM;
	}
	else
	{
	    rc = SOCK_FAIL;
	}
    }
    
    return rc;
}

int sock_write(sock_t sock, void * buf, int len, int * num_written)
{
    int nb;
    int rc = SOCK_SUCCESS;
    
    assert ((sock->pollfd->events & POLLOUT) == 0);
    
    do
    {
	nb = write(sock->fd, buf, len);
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
	else if (errno == EBADF)
	{
	    rc = SOCK_ERR_BAD_SOCK;
	}
	else if (errno == EFAULT)
	{
	    rc = SOCK_ERR_BAD_BUFFER;
	}
	else if (errno == ECONNRESET || errno == EPIPE)
	{
	    rc = SOCK_ERR_CONN_FAILED;
	}
	else if (errno == ENOMEM)
	{
	    rc = SOCK_ERR_NOMEM;
	}
	else
	{
	    rc = SOCK_FAIL;
	}
    }

    return rc;
}

int sock_writev(sock_t sock, SOCK_IOV * iov, int n, int * num_written)
{
    int nb;
    int rc = SOCK_SUCCESS;
    
    assert ((sock->pollfd->events & POLLOUT) == 0);
    
    do
    {
	nb = writev(sock->fd, iov, n);
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
	else if (errno == EBADF)
	{
	    rc = SOCK_ERR_BAD_SOCK;
	}
	else if (errno == EFAULT)
	{
	    rc = SOCK_ERR_BAD_BUFFER;
	}
	else if (errno == ECONNRESET || errno == EPIPE)
	{
	    rc = SOCK_ERR_CONN_FAILED;
	}
	else if (errno == ENOMEM)
	{
	    rc = SOCK_ERR_NOMEM;
	}
	else
	{
	    rc = SOCK_FAIL;
	}
    }

    return rc;
}

int sock_getid(sock_t sock)
{
    return sock->fd;
}


static int sock_alloc(struct sock_set * sock_set, int fd, struct sock ** sockp)
{
    struct sock * sock;
    int elem;
    struct pollfd * fds;
    struct pollinfo * infos;
    int rc;
    
    sock = MPIU_Malloc(sizeof(struct sock));
    if (sock == NULL)
    {
	rc = SOCK_ERR_NOMEM;
	goto fail;
    }
    
    for (elem = 0; elem < sock_set->sz; elem++)
    {
        if (sock_set->pollfds[elem].fd < 0)
        {
            if (elem >= sock_set->n)
            {
                sock_set->n = elem + 1;
            }
            sock_set->pollfds[elem].fd = fd;
            sock_set->pollfds[elem].events = 0;
            sock_set->pollfds[elem].revents = 0;
	    sock_set->pollinfos[elem].sock = sock;
            sock->sock_set = sock_set;
	    sock->fd = fd;
	    sock->pollfd = &sock_set->pollfds[elem];

	    *sockp = sock;
	    return SOCK_SUCCESS;
        }
    }

    /* No more pollfd and pollinfo elements.  Resize... */
    fds = MPIU_Malloc((sock_set->sz + SOCK_SET_DEFAULT_SIZE) * sizeof(struct pollfd));
    if (fds == NULL)
    {
	rc = SOCK_ERR_NOMEM;
	goto fail_free_sock;
    }
    infos = MPIU_Malloc((sock_set->sz + SOCK_SET_DEFAULT_SIZE) * sizeof(struct pollinfo));
    if (infos == NULL)
    {
	rc = SOCK_ERR_NOMEM;
	goto fail_free_fds;
    }
    
    if (sock_set->sz > 0)
    {
	/* Copy information from old arrays */
        memcpy(fds, sock_set->pollfds, sock_set->sz * sizeof(struct pollfd));
        memcpy(infos, sock_set->pollinfos, sock_set->sz * sizeof(struct pollinfo));

	/* Fix up pollfd pointer in sock structure */
	for (elem = 0; elem < sock_set->sz; elem++)
	{
	    sock_set->pollinfos[elem].sock->pollfd = &sock_set->pollfds[elem];
	}

	/* Free old arrays... */
	MPIU_Free(sock_set->pollfds);
	MPIU_Free(sock_set->pollinfos);
    }
    
    sock_set->n = elem + 1;
    sock_set->sz += SOCK_SET_DEFAULT_SIZE;
    sock_set->pollfds = fds;
    sock_set->pollinfos = infos;
	

    /* Register new file descriptor */
    sock_set->pollfds[elem].fd = fd;
    sock_set->pollfds[elem].events = 0;
    sock_set->pollfds[elem].revents = 0;
    sock_set->pollinfos[elem].sock = sock;
    sock->sock_set = sock_set;
    sock->fd = fd;
    sock->pollfd = &sock_set->pollfds[elem];
    sock->pollinfo = &sock_set->pollinfos[elem];
    
    /* Initialize new unallocated elements */
    for (elem = elem + 1; elem < sock_set->sz; elem++)
    {
        fds[elem].fd = -1;
#	if !defined(NDEBUG)
	{
	    fds[elem].events = 0;
	    fds[elem].revents = 0;
	    infos[elem].sock = NULL;
	    infos[elem].user_ptr = NULL;
	}
#	endif
    }

    *sockp = sock;
    return SOCK_SUCCESS;

  fail_free_fds:
    MPIU_Free(fds);
  fail_free_sock:
    MPIU_Free(sock);
  fail:
    return rc;
}

static void sock_free(struct sock * sock)
{
    sock->pollfd->fd = -1;
    /* TODO: compress poll array */
#   if !defined(NDEBUG)
    {
	sock->pollfd->events = 0;
	sock->pollfd->revents = 0;
	sock->pollfd = NULL;
	sock->pollinfo->sock = NULL;
	sock->pollinfo->user_ptr = NULL;
	sock->pollinfo = NULL;
	sock->sock_set = NULL;
	sock->fd = -1;
    }
#   endif
    MPIU_Free(sock);
}

static int sock_set_enqueue_event(struct sock * sock, sock_op_t op, sock_size_t num_bytes, void * user_ptr, int error)
{
    struct sock_set * sock_set = sock->sock_set;
    struct sock_eventq_elem * eventq_elem;
    int rc = SOCK_SUCCESS;

    eventq_elem = MPIU_Malloc(sizeof(struct sock_eventq_elem));
    if (eventq_elem == NULL)
    {
	rc = SOCK_ERR_NOMEM;
	goto fn_exit;
    }
    eventq_elem->event.op_type = op;
    eventq_elem->event.num_bytes = num_bytes;
    eventq_elem->event.user_ptr = user_ptr;
    eventq_elem->event.error = error;
    eventq_elem->sock = sock;
    eventq_elem->next = 0;

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
    return rc;
}

static int sock_set_dequeue_event(struct sock_set * sock_set, struct sock ** sockp, sock_event_t * eventp)
{
    struct sock_eventq_elem * eventq_elem;
    int rc = SOCK_SUCCESS;

    eventq_elem = sock_set->eventq_head;
    if (eventq_elem != NULL)
    {
	sock_set->eventq_head = eventq_elem->next;
	if (eventq_elem->next == NULL)
	{
	    sock_set->eventq_tail = NULL;
	}
	*sockp = eventq_elem->sock;
	*eventp = eventq_elem->event;
	MPIU_Free(eventq_elem);
	
    }
    else
    {
	rc = SOCK_FAIL;
    }

    return rc;
}
