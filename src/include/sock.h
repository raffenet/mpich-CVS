/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef SOCK_H
#define SOCK_H

#ifdef __cplusplus
extern "C" {
#endif

/* config header file */
#include "mpichconf.h"



/* definitions */

#define SOCK_POLL 1
#define SOCK_IOCP 2

typedef enum SOCK_OP
{
    SOCK_OP_TIMEOUT,
    SOCK_OP_READ,
    SOCK_OP_WRITE,
    SOCK_OP_ACCEPT,
    SOCK_OP_CONNECT,
    SOCK_OP_CLOSE
} SOCK_OP;
/*
#define SOCK_OP_READ    1
#define SOCK_OP_WRITE   2
#define SOCK_OP_ACCEPT  3
#define SOCK_OP_CONNECT 4
#define SOCK_OP_CLOSE   5
#define SOCK_OP_TIMEOUT 6
*/
#define SOCK_IOV_LIMIT   16

/* insert error codes here */
#define SOCK_SUCCESS           0
#define SOCK_FAIL              -1
#define SOCK_ERR_TIMEOUT       1001
#define SOCK_ERR_CONN_REFUSED  1002
#define SOCK_ERR_OS_SPECIFIC   1003



/* definitions/structures specific to IOCP */
#if (WITH_SOCK_TYPE == SOCK_IOCP)

#include <winsock2.h>
#define SOCK_IOV             WSABUF
#define SOCK_IOV_LEN         len
#define SOCK_IOV_BUF         buf
#define SOCK_INFINITE        INFINITE
#define SOCK_INVALID_SOCKET  NULL

typedef HANDLE sock_set_t;
typedef struct sock_state_t * sock_t;

/* definitions/structures specific to poll */
#elif (WITH_SOCK_TYPE == SOCK_POLL)

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#define SOCK_IOV             struct iovec
#define SOCK_IOV_LEN         iov_len
#define SOCK_IOV_BUF         iov_base
#define SOCK_INFINITE        -1
#define SOCK_INVALID_SOCKET  -1

typedef void * sock_set_t;
typedef int sock_t;

#else
#error Error: WITH_SOCK_TYPE not defined
#endif



/* common structures */

typedef struct sock_wait_t
{
    SOCK_OP op_type;
    int num_bytes;
    void *user_ptr;
    int error;
} sock_wait_t;


/* user callback function */

/* If a user function is passed to one of the sock_post_... functions the following applies:
   1) The sock progress engine will call this function when partial data has been read or
      written for the posted operation.
   2) All progress_update calls must complete before completion notification is signalled.
      In other words, sock_wait will not return until all progress_update calls have completed.
*/
int progress_update(int num_bytes, void *user_ptr);

/* function prototypes */
int sock_init();
int sock_finalize();

int sock_create_set(sock_set_t *set);
int sock_destroy_set(sock_set_t set);

int sock_set_user_ptr(sock_t sock, void *user_ptr);

int sock_listen(sock_set_t set, void *user_ptr, int *port, sock_t *listener);
int sock_post_connect(sock_set_t set, void *user_ptr, char *host, int port, sock_t *connected);
int sock_post_close(sock_t sock);
int sock_post_read(sock_t sock, void *buf, int len, int (*read_progress_update)(int, void*));
int sock_post_readv(sock_t sock, SOCK_IOV *iov, int n, int (*read_progress_update)(int, void*));
int sock_post_write(sock_t sock, void *buf, int len, int (*write_progress_update)(int, void*));
int sock_post_writev(sock_t sock, SOCK_IOV *iov, int n, int (*write_progress_update)(int, void*));

int sock_wait(sock_set_t set, int millisecond_timeout, sock_wait_t *out);

int sock_accept(sock_set_t set, void *user_ptr, sock_t listener, sock_t *accepted);
int sock_read(sock_t sock, void *buf, int len, int *num_read);
int sock_readv(sock_t sock, SOCK_IOV *iov, int n, int *num_read);
int sock_write(sock_t sock, void *buf, int len, int *num_written);
int sock_writev(sock_t sock, SOCK_IOV *iov, int n, int *num_written);

/* extended functions */
int sock_easy_receive(sock_t sock, void *buf, int len, int *num_read);
int sock_easy_send(sock_t sock, void *buf, int len, int *num_written);
int sock_getid(sock_t sock);

#ifdef __cplusplus
}
#endif

#endif
