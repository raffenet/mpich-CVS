/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "sock.h"
#include "mpiimpl.h"

#if !defined(SOCK_SET_DEFAULT_SIZE)
#define SOCK_SET_DEFAULT_SIZE 16
#endif

struct sock_set
{
    int n;
    struct pollfd * pollfds;
    sock_t * socks;
};

struct sock
{
    struct sock_set * sock_set;
    struct pollfd * pollfd;
    int fd;
    void * user_ptr;
};

int sock_init(void)
{
    return SOCK_SUCCESS;
}

int sock_finalize(void);
{
    return SOCK_SUCCESS;
}

int sock_create_set(sock_set_t * set)
{
    return SOCK_SUCCESS;
}

int sock_destroy_set(sock_set_t set)
{
    return SOCK_SUCCESS;
}

int sock_set_user_ptr(sock_t sock, void *user_ptr)
{
    sock->user_ptr = user_ptr;
    return SOCK_SUCCESS;
}


int sock_listen(sock_set_t set, void *user_ptr, int *port, sock_t *listener)
{
    return SOCK_SUCCESS;
}

int sock_post_connect(sock_set_t set, void *user_ptr, char *host, int port, sock_t *connected)
{
    return SOCK_SUCCESS;
}

int sock_post_close(sock_t sock)
{
    return SOCK_SUCCESS;
}

int sock_post_read(sock_t sock, void *buf, int len, int (*read_progress_update)(int, void*))
{
    return SOCK_SUCCESS;
}

int sock_post_readv(sock_t sock, SOCK_IOV *iov, int n, int (*read_progress_update)(int, void*))
{
    return SOCK_SUCCESS;
}

int sock_post_write(sock_t sock, void *buf, int len, int (*write_progress_update)(int, void*))
{
    return SOCK_SUCCESS;
}

int sock_post_writev(sock_t sock, SOCK_IOV *iov, int n, int (*write_progress_update)(int, void*))
{
    return SOCK_SUCCESS;
}

int sock_wait(sock_set_t set, int millisecond_timeout, sock_wait_t *out)
{
    return SOCK_SUCCESS;
}

int sock_accept(sock_set_t set, void *user_ptr, sock_t listener, sock_t *accepted)
{
    return SOCK_SUCCESS;
}

int sock_read(sock_t sock, void *buf, int len, int *num_read)
{
    return SOCK_SUCCESS;
}

int sock_readv(sock_t sock, SOCK_IOV *iov, int n, int *num_read)
{
    return SOCK_SUCCESS;
}

int sock_write(sock_t sock, void *buf, int len, int *num_written)
{
    return SOCK_SUCCESS;
}

int sock_writev(sock_t sock, SOCK_IOV *iov, int n, int *num_written)
{
    return SOCK_SUCCESS;
}

int sock_getid(sock_t sock)
{
    return sock->fd;
}
