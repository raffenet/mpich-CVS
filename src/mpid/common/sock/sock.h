/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(SOCK_H_INCLUDED)
#define SOCK_H_INCLUDED

#if defined(__cplusplus)
#if !defined(CPLUSPLUS_BEGIN)
#define CPLUSPLUS_BEGIN extern "C" {
#define CPLUSPLUS_END }
#endif
#else
#define CPLUSPLUS_BEGIN
#define CPLUSPLUS_END
#endif

CPLUSPLUS_BEGIN

/* config header file */
#ifdef USE_WINCONF_H
#include "winsockconf.h"
#else
#include "sockconf.h"
#endif

/* implemenatation specific header file */    
#include "socki.h"


/*
 * definitions
 */
typedef enum sock_op
{
    SOCK_OP_READ,
    SOCK_OP_WRITE,
    SOCK_OP_ACCEPT,
    SOCK_OP_CONNECT,
    SOCK_OP_CLOSE
} sock_op_t;

#define SOCK_IOV_LIMIT   16

/* insert error codes here */
#define SOCK_SUCCESS            0
#define SOCK_FAIL              -1
#define SOCK_EOF	        1 
#define SOCK_ERR_NOMEM          1000
#define SOCK_ERR_TIMEOUT        1001
#define SOCK_ERR_HOST_LOOKUP    1002
#define SOCK_ERR_CONN_REFUSED   1003
#define SOCK_ERR_CONN_FAILED    1004
#define SOCK_ERR_BAD_SOCK       1005
#define SOCK_ERR_BAD_BUFFER     1006
#define SOCK_ERR_OP_IN_PROGRESS 1007
#define SOCK_ERR_OS_SPECIFIC    1008

/*
 * structures
 */
typedef struct sock_event
{
    sock_op_t op_type;
    sock_size_t num_bytes;
    void *user_ptr;
    int error;
} sock_event_t;


/* Progress update callback functions
 *
 * If a function pointer is passed to one of the sock_post_... functions the following applies:
 *
 * 1) The sock progress engine will call this function when partial data has been read or written for the posted operation.
 *     
 * 2) All progress_update calls must complete before completion notification is signalled.  In other words, sock_wait will not
 * return until all progress_update calls have completed.
 */
typedef int (*sock_progress_update_func_t)(sock_size_t num_bytes, void *user_ptr);


/*
 * function prototypes
 */
int sock_init(void);
int sock_finalize(void);

int sock_create_set(sock_set_t * set);
int sock_destroy_set(sock_set_t set);

int sock_set_user_ptr(sock_t sock, void * user_ptr);

int sock_listen(sock_set_t set, void * user_ptr, int * port, sock_t * listener);
int sock_post_connect(sock_set_t set, void * user_ptr, char * host, int port, sock_t * connected);
int sock_post_close(sock_t sock);
int sock_post_read(sock_t sock, void * buf, sock_size_t len, sock_progress_update_func_t fn);
int sock_post_readv(sock_t sock, SOCK_IOV * iov, int n, sock_progress_update_func_t fn);
int sock_post_write(sock_t sock, void * buf, sock_size_t len, sock_progress_update_func_t fn);
int sock_post_writev(sock_t sock, SOCK_IOV * iov, int n, sock_progress_update_func_t fn);

int sock_wait(sock_set_t set, int millisecond_timeout, sock_event_t * out);

int sock_accept(sock_t listener, sock_set_t set, void * user_ptr, sock_t * accepted);
int sock_read(sock_t sock, void * buf, sock_size_t len, sock_size_t * num_read);
int sock_readv(sock_t sock, SOCK_IOV * iov, int n, sock_size_t * num_read);
int sock_write(sock_t sock, void * buf, sock_size_t len, sock_size_t * num_written);
int sock_writev(sock_t sock, SOCK_IOV * iov, int n, sock_size_t * num_written);

/* extended functions */
int sock_getid(sock_t sock);

CPLUSPLUS_END

#define SOCK_STATE_LIST \
MPID_STATE_SOCK_INIT, \
MPID_STATE_SOCK_FINALIZE, \
MPID_STATE_SOCK_CREATE_SET, \
MPID_STATE_SOCK_DESTROY_SET, \
MPID_STATE_SOCK_LISTEN, \
MPID_STATE_SOCK_POST_CONNECT, \
MPID_STATE_SOCK_ACCEPT, \
MPID_STATE_SOCK_POST_CLOSE, \
MPID_STATE_SOCK_WAIT, \
MPID_STATE_SOCK_SET_USER_PTR, \
MPID_STATE_SOCK_READ, \
MPID_STATE_SOCK_READV, \
MPID_STATE_SOCK_WRITE, \
MPID_STATE_SOCK_WRITEV, \
MPID_STATE_SOCK_POST_READ, \
MPID_STATE_SOCK_POST_READV, \
MPID_STATE_SOCK_POST_WRITE, \
MPID_STATE_SOCK_POST_WRITEV, \
SOCKI_STATE_LIST

#endif /* !defined(SOCK_H_INCLUDED) */
