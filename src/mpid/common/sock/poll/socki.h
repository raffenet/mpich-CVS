/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(SOCKI_H_INCLUDED)
#define SOCKI_H_INCLUDED

#include "sockpollconf.h"

#if defined(HAVE_SYS_UIO_H)
#include <sys/uio.h>
#endif
#if defined(HAVE_LIMITS_H)
#include <limits.h>
#endif
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#define SOCK_IOV             struct iovec
#define SOCK_IOV_LEN         iov_len
#define SOCK_IOV_BUF         iov_base
#define SOCK_IOV_MAXLEN      16
#define SOCK_INFINITE_TIME   -1
#define SOCK_INVALID_SOCK    NULL
#define SOCK_SIZE_MAX	     SSIZE_MAX

typedef struct sock_set * sock_set_t;
typedef struct sock * sock_t;
typedef size_t sock_size_t;

#define SOCKI_STATE_LIST \
SOCKI_SOCK_ALLOC, \
SOCKI_SOCK_FREE, \
SOCKI_EVENT_ENQUEUE, \
SOCKI_EVENT_DEQUEUE, \
SOCKI_ADJUST_IOV,

#endif /* !defined(SOCKI_H_INCLUDED) */
