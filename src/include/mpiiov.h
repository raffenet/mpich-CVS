/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIIOV_H
#define MPIIOV_H

/* IOVs */
/* The basic channel interface uses IOVs */
#define MPID_IOV_BUF_CAST void *
#ifdef HAVE_WINDOWS_H
#include <winsock2.h>
#define MPID_IOV         WSABUF
#define MPID_IOV_LEN     len
#define MPID_IOV_BUF     buf
#else
#ifdef HAVE_STDIO_H
#include <stdio.h> /* macs need stdio.h before uio.h can be included */
#endif
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#define MPID_IOV         struct iovec
#define MPID_IOV_LEN     iov_len
#define MPID_IOV_BUF     iov_base
#endif
/* FIXME: How is IOV_LIMIT chosen? */
#define MPID_IOV_LIMIT   16

#endif
