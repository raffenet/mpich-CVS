/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "sockpollconf.h"

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#define SOCK_IOV             struct iovec
#define SOCK_IOV_LEN         iov_len
#define SOCK_IOV_BUF         iov_base
#define SOCK_IOV_MAXLEN      16
#define SOCK_INFINITE        -1
#define SOCK_INVALID_SOCKET  -1
#define SOCK_SIZE_MAX	     SSIZE_MAX

typedef struct sock_set_s * sock_set_t;
typedef struct sock_s * sock_t;
typedef size_t sock_size_t;
