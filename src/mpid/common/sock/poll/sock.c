/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidu_sock.h"
#include "mpiimpl.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/uio.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <fcntl.h>
#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif
#include <netdb.h>
#include <errno.h>

/*
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
*/

#ifdef USE_SELECT_FOR_POLL
#include "mpidu_sock_poll.h"
#endif

#if !defined(MPIDU_SOCK_SET_DEFAULT_SIZE)
#define MPIDU_SOCK_SET_DEFAULT_SIZE 1
#endif

#if !defined(MPIDU_SOCK_EVENTQ_POOL_SIZE)
#define MPIDU_SOCK_EVENTQ_POOL_SIZE 1
#endif

enum MPIDU_Socki_state
{
    MPIDU_SOCKI_STATE_UNCONNECTED = 1,
    MPIDU_SOCKI_STATE_INTERRUPTER,
    MPIDU_SOCKI_STATE_LISTENER,
    MPIDU_SOCKI_STATE_CONNECTING,
    MPIDU_SOCKI_STATE_CONNECTED,
    MPIDU_SOCKI_STATE_CONN_CLOSED,
    MPIDU_SOCKI_STATE_CONN_FAILED,
    MPIDU_SOCKI_STATE_CLOSING,
    MPIDU_SOCKI_STATE_LAST
};

struct pollinfo
{
    int sock_id;
    struct MPIDU_Sock_set * sock_set;
    int elem;
    int fd;
    void * user_ptr;
    enum MPIDU_Socki_state state;
    union
    {
	struct
	{
	    MPID_IOV * ptr;
	    int count;
	    int offset;
	} iov;
	struct
	{
	    char * ptr;
	    MPIU_Size_t min;
	    MPIU_Size_t max;
	} buf;
    } read;
    int read_iov_flag;
    MPIU_Size_t read_nb;
    MPIDU_Sock_progress_update_func_t read_progress_update_fn;
    union
    {
	struct
	{
	    MPID_IOV * ptr;
	    int count;
	    int offset;
	} iov;
	struct
	{
	    char * ptr;
	    MPIU_Size_t min;
	    MPIU_Size_t max;
	} buf;
    } write;
    int write_iov_flag;
    MPIU_Size_t write_nb;
    MPIDU_Sock_progress_update_func_t write_progress_update_fn;
};

struct MPIDU_Socki_eventq_elem
{
    struct MPIDU_Sock_event event;
    struct MPIDU_Socki_eventq_elem * next;
};

struct MPIDU_Sock_set
{
    /* MT: in poll lock? */
    int id;
    /* MT: poll entry allocation/deallocation lock? */
    int poll_arr_sz;
    int poll_n_elem;
    int starting_elem;
    struct pollfd * pollfds;
    struct pollinfo * pollinfos;
    int intr_fds[2];
    struct MPIDU_Sock * intr_sock;
    /* MT: lock for the enqueuing and dequeing events? */
    struct MPIDU_Socki_eventq_elem * eventq_head;
    struct MPIDU_Socki_eventq_elem * eventq_tail;
};

struct MPIDU_Sock
{
    struct MPIDU_Sock_set * sock_set;
    int elem;
};


int MPIDU_Socki_initialized = 0;

/* MT: lock for the pool of available event queue entries? */
static struct MPIDU_Socki_eventq_elem * MPIDU_Socki_eventq_pool = NULL;

/* MT: need to be atomically incremented */
static int MPIDU_Socki_set_next_id = 0;


#include "socki_util.i"

#include "sock_init.i"
#include "sock_set.i"
#include "sock_post.i"
#include "sock_immed.i"
#include "sock_misc.i"
#include "sock_wait.i"
