/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "sock.h"
#include "mpiimpl.h"

#if (WITH_SOCK_TYPE == SOCK_IOCP)

#include <mswsock.h>
#include <stdio.h>

/*#define USE_SOCK_IOV_COPY*/

#define SOCKI_TCP_BUFFER_SIZE       32*1024
#define SOCKI_STREAM_PACKET_LENGTH  8*1024

typedef enum SOCK_TYPE { SOCK_INVALID, SOCK_LISTENER, SOCK_SOCKET, SOCK_NATIVE } SOCK_TYPE;

typedef int SOCK_STATE;
#define SOCK_ACCEPTING  0x0001
#define SOCK_ACCEPTED   0x0002
#define SOCK_CONNECTING 0x0004
#define SOCK_READING    0x0008
#define SOCK_WRITING    0x0010

typedef struct sock_buffer
{
    int use_iov;
    DWORD num_bytes;
    OVERLAPPED ovl;
    void *buffer;
    int bufflen;
#ifdef USE_SOCK_IOV_COPY
    SOCK_IOV iov[SOCK_IOV_MAXLEN];
#else
    SOCK_IOV *iov;
#endif
    int iovlen;
    int index;
    int total;
    int (*progress_update)(int,void*);
} sock_buffer;

typedef struct sock_state_t
{
    SOCK_TYPE type;
    SOCK_STATE state;
    SOCKET sock;
    sock_set_t set;
    int closing;
    int pending_operations;
    /* listener/accept structures */
    SOCKET listen_sock;
    char accept_buffer[sizeof(struct sockaddr_in)*2+32];
    /* read and write structures */
    sock_buffer read;
    sock_buffer write;
    /* user pointer */
    void *user_ptr;
} sock_state_t;

#define DEFAULT_NUM_RETRIES 10

static int g_connection_attempts = DEFAULT_NUM_RETRIES;
static int g_num_cp_threads = 2;
static int g_socket_buffer_size = SOCKI_TCP_BUFFER_SIZE;
static int g_stream_packet_length = SOCKI_STREAM_PACKET_LENGTH;


/* utility allocator functions */

typedef struct BlockAllocator_struct * BlockAllocator;

BlockAllocator BlockAllocInit(unsigned int blocksize, int count, int incrementsize, void *(* alloc_fn)(unsigned int size), void (* free_fn)(void *p));
int BlockAllocFinalize(BlockAllocator *p);
void * BlockAlloc(BlockAllocator p);
int BlockFree(BlockAllocator p, void *pBlock);

struct BlockAllocator_struct
{
    void **pNextFree;
    void *(* alloc_fn)(size_t size);
    void (* free_fn)(void *p);
    struct BlockAllocator_struct *pNextAllocation;
    unsigned int nBlockSize;
    int nCount, nIncrementSize;
#ifdef WITH_ALLOCATOR_LOCKING
    MPIDU_Lock_t lock;
#endif
};

static int g_nLockSpinCount = 100;

#ifdef WITH_ALLOCATOR_LOCKING

typedef volatile long MPIDU_Lock_t;

#include <errno.h>
#ifdef HAVE_WINDOWS_H
#include <winsock2.h>
#include <windows.h>
#endif

static inline void MPIDU_Init_lock( MPIDU_Lock_t *lock )
{
    *(lock) = 0;
}

static inline void MPIDU_Lock( MPIDU_Lock_t *lock )
{
    int i;
    for (;;)
    {
        for (i=0; i<g_nLockSpinCount; i++)
        {
            if (*lock == 0)
            {
#ifdef HAVE_INTERLOCKEDEXCHANGE
                if (InterlockedExchange((LPLONG)lock, 1) == 0)
                {
                    /*printf("lock %x\n", lock);fflush(stdout);*/
                    MPID_PROFILE_OUT(MPIDU_BUSY_LOCK);
                    return;
                }
#elif defined(HAVE_COMPARE_AND_SWAP)
                if (compare_and_swap(lock, 0, 1) == 1)
                {
                    MPID_PROFILE_OUT(MPIDU_BUSY_LOCK);
                    return;
                }
#else
#error Atomic memory operation needed to implement busy locks
#endif
            }
        }
        MPIDU_Yield();
    }
}

static inline void MPIDU_Unlock( MPIDU_Lock_t *lock )
{
    *(lock) = 0;
}

static inline void MPIDU_Busy_wait( MPIDU_Lock_t *lock )
{
    int i;
    for (;;)
    {
        for (i=0; i<g_nLockSpinCount; i++)
            if (!*lock)
            {
                return;
            }
        MPIDU_Yield();
    }
}

static inline void MPIDU_Free_lock( MPIDU_Lock_t *lock )
{
}

/*@
   MPIDU_Compare_swap - 

   Parameters:
+  void **dest
.  void *new_val
.  void *compare_val
.  MPIDU_Lock_t *lock
-  void **original_val

   Notes:
@*/
static inline int MPIDU_Compare_swap( void **dest, void *new_val, void *compare_val,            
                        MPIDU_Lock_t *lock, void **original_val )
{
    /* dest = pointer to value to be checked (address size)
       new_val = value to set dest to if *dest == compare_val
       original_val = value of dest prior to this operation */

#ifdef HAVE_NT_LOCKS
    /* *original_val = (void*)InterlockedCompareExchange(dest, new_val, compare_val); */
    *original_val = InterlockedCompareExchangePointer(dest, new_val, compare_val);
#elif defined(HAVE_COMPARE_AND_SWAP)
    if (compare_and_swap((volatile long *)dest, (long)compare_val, (long)new_val))
        *original_val = new_val;
#else
#error Locking functions not defined
#endif

    return 0;
}
#endif /* WITH_ALLOCATOR_LOCKING */

static BlockAllocator BlockAllocInit(unsigned int blocksize, int count, int incrementsize, void *(* alloc_fn)(unsigned int size), void (* free_fn)(void *p))
{
    BlockAllocator p;
    void **ppVoid;
    int i;

    p = alloc_fn( sizeof(struct BlockAllocator_struct) + ((blocksize + sizeof(void**)) * count) );

    p->alloc_fn = alloc_fn;
    p->free_fn = free_fn;
    p->nIncrementSize = incrementsize;
    p->pNextAllocation = NULL;
    p->nCount = count;
    p->nBlockSize = blocksize;
    p->pNextFree = (void**)(p + 1);
#ifdef WITH_ALLOCATOR_LOCKING
    MPIDU_Init_lock(&p->lock);
#endif

    ppVoid = (void**)(p + 1);
    for (i=0; i<count-1; i++)
    {
	*ppVoid = (void*)((char*)ppVoid + sizeof(void**) + blocksize);
	ppVoid = *ppVoid;
    }
    *ppVoid = NULL;

    return p;
}

static int BlockAllocFinalize(BlockAllocator *p)
{
    if (*p == NULL)
	return 0;
    BlockAllocFinalize(&(*p)->pNextAllocation);
    if ((*p)->free_fn != NULL)
	(*p)->free_fn(*p);
    *p = NULL;
    return 0;
}

static void * BlockAlloc(BlockAllocator p)
{
    void *pVoid;
    
#ifdef WITH_ALLOCATOR_LOCKING
    MPIDU_Lock(&p->lock);
#endif

    pVoid = p->pNextFree + 1;
    
    if (*(p->pNextFree) == NULL)
    {
	BlockAllocator pIter = p;
	while (pIter->pNextAllocation != NULL)
	    pIter = pIter->pNextAllocation;
	pIter->pNextAllocation = BlockAllocInit(p->nBlockSize, p->nIncrementSize, p->nIncrementSize, p->alloc_fn, p->free_fn);
	p->pNextFree = pIter->pNextFree;
    }
    else
	p->pNextFree = *(p->pNextFree);

#ifdef WITH_ALLOCATOR_LOCKING
    MPIDU_Unlock(&p->lock);
#endif

    return pVoid;
}

static int BlockFree(BlockAllocator p, void *pBlock)
{
#ifdef WITH_ALLOCATOR_LOCKING
    MPIDU_Lock(&p->lock);
#endif

    ((void**)pBlock)--;
    *((void**)pBlock) = p->pNextFree;
    p->pNextFree = pBlock;

#ifdef WITH_ALLOCATOR_LOCKING
    MPIDU_Unlock(&p->lock);
#endif

    return 0;
}




/* utility socket functions */

static int g_last_os_error = SOCK_SUCCESS;

int WinToSockError(int error)
{
    switch (error)
    {
    case WSAEINTR:
	break;
    case WSAEACCES:
	break;
    case WSAEFAULT:
	break;
    case WSAEINVAL:
	break;
    case WSAEMFILE:
	break;
    case WSAEWOULDBLOCK:
	break;
    case WSAEINPROGRESS:
	break;
    case WSAEALREADY:
	break;
    case WSAENOTSOCK:
	return SOCK_ERR_BAD_SOCK;
	break;
    case WSAEDESTADDRREQ:
	break;
    case WSAEMSGSIZE:
	break;
    case WSAEPROTOTYPE:
	break;
    case WSAENOPROTOOPT:
	break;
    case WSAEPROTONOSUPPORT:
	break;
    case WSAESOCKTNOSUPPORT:
	break;
    case WSAEOPNOTSUPP:
	break;
    case WSAEPFNOSUPPORT:
	break;
    case WSAEAFNOSUPPORT:
	break;
    case WSAEADDRINUSE:
	return SOCK_ERR_ADDR_INUSE;
	break;
    case WSAEADDRNOTAVAIL:
	break;
    case WSAENETDOWN:
	break;
    case WSAENETUNREACH:
	break;
    case WSAENETRESET:
	break;
    case WSAECONNABORTED:
	return SOCK_ERR_CONN_FAILED;
	break;
    case WSAECONNRESET:
	return SOCK_ERR_CONN_FAILED;
	/*return SOCK_EOF;*/
	break;
    case WSAENOBUFS:
	return SOCK_ERR_BAD_BUFFER;
	break;
    case WSAEISCONN:
	break;
    case WSAENOTCONN:
	break;
    case WSAESHUTDOWN:
	break;
    case WSAETIMEDOUT:
	return SOCK_ERR_TIMEOUT;
	break;
    case WSAECONNREFUSED:
	return SOCK_ERR_CONN_REFUSED;
	break;
    case WSAEHOSTDOWN:
	break;
    case WSAEHOSTUNREACH:
	break;
    case WSAEPROCLIM:
	break;
    case WSASYSNOTREADY:
	break;
    case WSAVERNOTSUPPORTED:
	break;
    case WSANOTINITIALISED:
	break;
    case WSAEDISCON:
	break;
    case WSATYPE_NOT_FOUND:
	break;
    case WSAHOST_NOT_FOUND:
	return SOCK_ERR_HOST_LOOKUP;
	break;
    case WSATRY_AGAIN:
	break;
    case WSANO_RECOVERY:
	break;
    case WSANO_DATA:
	break;
    case WSA_INVALID_HANDLE:
	return SOCK_ERR_BAD_SET;
	break;
    case WSA_INVALID_PARAMETER:
	break;
    case WSA_IO_INCOMPLETE:
	break;
    case WSA_IO_PENDING:
	break;
    case WSA_NOT_ENOUGH_MEMORY:
	return SOCK_ERR_NOMEM;
	break;
    case WSA_OPERATION_ABORTED:
	return SOCK_ERR_OP_ABORTED;
	break;
    case WSASYSCALLFAILURE:
	break;
    }
    /* save error */
    g_last_os_error = error;
    MPIU_DBG_PRINTF(("***** WinToSockError returning os specific error %d *****\n", error));
    return SOCK_ERR_OS_SPECIFIC;
}

int sock_get_last_os_error(void)
{
    return g_last_os_error;
}

static int easy_create(SOCKET *sock, int port, unsigned long addr)
{
    /*struct linger linger;*/
    int optval, len;
    SOCKET temp_sock;
    SOCKADDR_IN sockAddr;

    /* create the non-blocking socket */
    temp_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (temp_sock == INVALID_SOCKET)
    {
	return WinToSockError(WSAGetLastError());
    }
    
    memset(&sockAddr,0,sizeof(sockAddr));
    
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = addr;
    sockAddr.sin_port = htons((unsigned short)port);

    if (bind(temp_sock, (SOCKADDR*)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
    {
	return WinToSockError(WSAGetLastError());
    }
    
    /* Set the linger on close option */
    /*
    linger.l_onoff = 1 ;
    linger.l_linger = 60;
    setsockopt(temp_sock, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));
    */

    /* set the socket to non-blocking */
    /*
    optval = TRUE;
    ioctlsocket(temp_sock, FIONBIO, &optval);
    */

    /* set the socket buffers */
    len = sizeof(int);
    if (!getsockopt(temp_sock, SOL_SOCKET, SO_RCVBUF, (char*)&optval, &len))
    {
	optval = g_socket_buffer_size;
	setsockopt(temp_sock, SOL_SOCKET, SO_RCVBUF, (char*)&optval, sizeof(int));
    }
    len = sizeof(int);
    if (!getsockopt(temp_sock, SOL_SOCKET, SO_SNDBUF, (char*)&optval, &len))
    {
	optval = g_socket_buffer_size;
	setsockopt(temp_sock, SOL_SOCKET, SO_SNDBUF, (char*)&optval, sizeof(int));
    }

    /* prevent the socket from being inherited by child processes */
    if (!DuplicateHandle(
	GetCurrentProcess(), (HANDLE)temp_sock,
	GetCurrentProcess(), (HANDLE*)sock,
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	return WinToSockError(GetLastError());
    }

    /* Set the no-delay option */
    setsockopt(*sock, IPPROTO_TCP, TCP_NODELAY, (char *)&optval, sizeof(optval));

    return SOCK_SUCCESS;
}

static inline int easy_get_sock_info(SOCKET sock, char *name, int *port)
{
    struct sockaddr_in addr;
    int name_len = sizeof(addr);

    getsockname(sock, (struct sockaddr*)&addr, &name_len);
    *port = ntohs(addr.sin_port);
    gethostname(name, 100);

    return 0;
}

static inline void init_state_struct(sock_state_t *p)
{
    p->listen_sock = INVALID_SOCKET;
    p->sock = INVALID_SOCKET;
    p->set = INVALID_HANDLE_VALUE;
    p->user_ptr = NULL;
    p->type = 0;
    p->state = 0;
    p->closing = FALSE;
    p->pending_operations = 0;
    p->read.total = 0;
    p->read.num_bytes = 0;
    p->read.buffer = NULL;
#ifndef USE_SOCK_IOV_COPY
    p->read.iov = NULL;
#endif
    p->read.iovlen = 0;
    p->read.ovl.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    p->read.ovl.Offset = 0;
    p->read.ovl.OffsetHigh = 0;
    p->read.progress_update = NULL;
    p->write.total = 0;
    p->write.num_bytes = 0;
    p->write.buffer = NULL;
#ifndef USE_SOCK_IOV_COPY
    p->write.iov = NULL;
#endif
    p->write.iovlen = 0;
    p->write.ovl.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    p->write.ovl.Offset = 0;
    p->write.ovl.OffsetHigh = 0;
    p->write.progress_update = NULL;
}

static inline int post_next_accept(sock_state_t * listen_state)
{
    int error;
    listen_state->state = SOCK_ACCEPTING;
    listen_state->sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (listen_state->sock == INVALID_SOCKET)
	return SOCK_FAIL;
    if (!AcceptEx(
	listen_state->listen_sock, 
	listen_state->sock, 
	listen_state->accept_buffer, 
	0, 
	sizeof(struct sockaddr_in)+16, 
	sizeof(struct sockaddr_in)+16, 
	&listen_state->read.num_bytes,
	&listen_state->read.ovl))
    {
	error = WSAGetLastError();
	if (error == ERROR_IO_PENDING)
	    return TRUE;
	MPIU_Error_printf("AcceptEx failed with error %d\n", error);fflush(stdout);
	return FALSE;
    }
    return TRUE;
}

/* sock functions */

static BlockAllocator g_StateAllocator;

/*
SOCKET socki_get_handle(sock_t sock)
{
    if (sock != NULL)
	return sock->sock;
    return INVALID_SOCKET;
}
*/

int sock_native_to_sock(sock_set_t set, SOCK_NATIVE_FD fd, void *user_ptr, sock_t *sock_ptr)
{
    /*int ret_val;*/
    sock_state_t *sock_state;
    /*u_long optval;*/
    MPIDI_STATE_DECL(MPID_STATE_SOCK_NATIVE_TO_SOCK);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_NATIVE_TO_SOCK);

    /* setup the structures */
    sock_state = (sock_state_t*)BlockAlloc(g_StateAllocator);
    if (sock_state == NULL)
    {
	return SOCK_ERR_NOMEM;
    }
    init_state_struct(sock_state);
    sock_state->sock = (SOCKET)fd;

    /* set the socket to non-blocking */
    /* leave the native handle in the state passed in?
    optval = TRUE;
    ioctlsocket(sock_state->sock, FIONBIO, &optval);
    */

    sock_state->user_ptr = user_ptr;
    sock_state->type = SOCK_SOCKET;
    sock_state->state = 0;
    sock_state->set = set;

    /* associate the socket with the completion port */
    /*printf("CreateIOCompletionPort(%d, %p, %p, %d)\n", sock_state->sock, set, sock_state, g_num_cp_threads);fflush(stdout);*/
    if (CreateIoCompletionPort((HANDLE)sock_state->sock, set, (ULONG_PTR)sock_state, g_num_cp_threads) == NULL)
    {
	MPIU_Error_printf("Unable to associate native handle with the completion port, setting NATIVE mode.\n");fflush(stdout);
	/* If it cannot be associated with the completion port, use the callback interface */
	sock_state->type = SOCK_NATIVE;
	/* the hEvent fields are not used by the OS so use them to save a pointer to the sock structure */
	CloseHandle(sock_state->read.ovl.hEvent);
	CloseHandle(sock_state->write.ovl.hEvent);
	sock_state->read.ovl.hEvent = sock_state;
	sock_state->write.ovl.hEvent = sock_state;
	/*
	ret_val = WinToSockError(GetLastError());
	MPIU_Error_printf("CreateIOCompletionPort failed, error %d\n", GetLastError());
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_NATIVE_TO_SOCK);
	return ret_val;
	*/
    }

    *sock_ptr = sock_state;

    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_NATIVE_TO_SOCK);
    return SOCK_SUCCESS;
}

static int g_init_called = 0;
int sock_init()
{
    char *szNum;
    WSADATA wsaData;
    int err;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_INIT);

    if (g_init_called)
    {
	g_init_called++;
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_INIT);
	return SOCK_SUCCESS;
    }
    g_init_called = 1;

    /* Start the Winsock dll */
    if ((err = WSAStartup(MAKEWORD(2, 0), &wsaData)) != 0)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_INIT);
	return err;
    }

    /* get the connection retry value */
    szNum = getenv("SOCK_CONNECT_TRIES");
    if (szNum != NULL)
    {
	g_connection_attempts = atoi(szNum);
	if (g_connection_attempts < 1)
	    g_connection_attempts = DEFAULT_NUM_RETRIES;
    }

    /* get the socket buffer size */
    szNum = getenv("SOCK_TCP_BUFFER_SIZE");
    if (szNum != NULL)
    {
	g_socket_buffer_size = atoi(szNum);
	if (g_socket_buffer_size < 1)
	    g_socket_buffer_size = SOCKI_TCP_BUFFER_SIZE;
    }

    /* get the stream packet size */
    /* messages larger than this size will be broken into pieces of this size when sending */
    szNum = getenv("SOCK_STREAM_PACKET_SIZE");
    if (szNum != NULL)
    {
	g_stream_packet_length = atoi(szNum);
	if (g_stream_packet_length < 1)
	    g_stream_packet_length = SOCKI_STREAM_PACKET_LENGTH;
    }

    g_StateAllocator = BlockAllocInit(sizeof(sock_state_t), 1000, 500, malloc, free);

    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_INIT);
    return SOCK_SUCCESS;
}

int sock_finalize()
{
    MPIDI_STATE_DECL(MPID_STATE_SOCK_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_FINALIZE);
    g_init_called--;
    if (g_init_called == 0)
    {
	WSACleanup();
	BlockAllocFinalize(&g_StateAllocator);
    }
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_FINALIZE);
    return SOCK_SUCCESS;
}

int sock_create_set(sock_set_t *set)
{
    HANDLE port;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_CREATE_SET);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_CREATE_SET);
    port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, g_num_cp_threads);
    if (port != NULL)
    {
	*set = port;
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_CREATE_SET);
	return SOCK_SUCCESS;
    }
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_CREATE_SET);
    return SOCK_FAIL;
}

int sock_destroy_set(sock_set_t set)
{
    BOOL b;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_DESTROY_SET);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_DESTROY_SET);
    b = CloseHandle(set);
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_DESTROY_SET);
    return (b == TRUE) ? SOCK_SUCCESS : SOCK_FAIL;
}

static int listening = 0;
int sock_listen(sock_set_t set, void * user_ptr, int *port, sock_t *listener)
{
    int error;
    char host[100];
    DWORD num_read = 0;
    sock_state_t * listen_state;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_LISTEN);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_LISTEN);

    if (listening)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_LISTEN);
	return SOCK_FAIL;
    }
    listening = 1;

    listen_state = (sock_state_t*)BlockAlloc(g_StateAllocator);
    init_state_struct(listen_state);
    error = easy_create(&listen_state->listen_sock, *port, INADDR_ANY);
    if (error != SOCK_SUCCESS)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_LISTEN);
	return error;
    }
    if (listen(listen_state->listen_sock, SOMAXCONN) == SOCKET_ERROR)
    {
	error = WinToSockError(WSAGetLastError());
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_LISTEN);
	return error;
    }
    if (CreateIoCompletionPort((HANDLE)listen_state->listen_sock, set, (ULONG_PTR)listen_state, g_num_cp_threads) == NULL)
    {
	error = WinToSockError(GetLastError());
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_LISTEN);
	return error;
    }
    easy_get_sock_info(listen_state->listen_sock, host, port);
    listen_state->user_ptr = user_ptr;
    listen_state->type = SOCK_LISTENER;
    /* do this last to make sure the listener state structure is completely initialized before
       a completion thread has the chance to satisfy the AcceptEx call */
    if (!post_next_accept(listen_state))
    {
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_LISTEN);
	return SOCK_FAIL;
    }
    *listener = listen_state;
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_LISTEN);
    return SOCK_SUCCESS;
}

int sock_post_connect(sock_set_t set, void * user_ptr, char *host, int port, sock_t *connected)
{
    int ret_val;
    struct hostent *lphost;
    struct sockaddr_in sockAddr;
    sock_state_t *connect_state;
    u_long optval;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_POST_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_POST_CONNECT);

    memset(&sockAddr,0,sizeof(sockAddr));

    /* setup the structures */
    connect_state = (sock_state_t*)BlockAlloc(g_StateAllocator);
    init_state_struct(connect_state);

    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = inet_addr(host);
    
    if (sockAddr.sin_addr.s_addr == INADDR_NONE || sockAddr.sin_addr.s_addr == 0)
    {
	lphost = gethostbyname(host);
	if (lphost != NULL)
	    sockAddr.sin_addr.s_addr = ((struct in_addr *)lphost->h_addr)->s_addr;
	else
	{
	    ret_val = WinToSockError(WSAGetLastError());
	    MPIU_Error_printf("In sock_post_connect(), gethostbyname failed, error %d\n", WSAGetLastError());
	    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_CONNECT);
	    return ret_val;
	}
    }
    
    sockAddr.sin_port = htons((u_short)port);

    /* create a socket */
    if (easy_create(&connect_state->sock, ADDR_ANY, INADDR_ANY) == SOCKET_ERROR)
    {
	ret_val = WinToSockError(WSAGetLastError());
	MPIU_Error_printf("In sock_post_connect(), easy_create failed, error %d\n", WSAGetLastError());
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_CONNECT);
	return ret_val;
    }

    /* connect */
    if (connect(connect_state->sock, (SOCKADDR*)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
    {
	ret_val = WinToSockError(WSAGetLastError());
	MPIU_Error_printf("In sock_post_connect(), connect failed, error %d\n", WSAGetLastError());
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_CONNECT);
	return ret_val;
    }

    /* set the socket to non-blocking */
    optval = TRUE;
    ioctlsocket(connect_state->sock, FIONBIO, &optval);

    connect_state->user_ptr = user_ptr;
    connect_state->type = SOCK_SOCKET;
    connect_state->state = SOCK_CONNECTING;
    connect_state->set = set;

    /* associate the socket with the completion port */
    if (CreateIoCompletionPort((HANDLE)connect_state->sock, set, (ULONG_PTR)connect_state, g_num_cp_threads) == NULL)
    {
	ret_val = WinToSockError(GetLastError());
	MPIU_Error_printf("In sock_post_connect(), CreateIOCompletionPort failed, error %d\n", GetLastError());
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_CONNECT);
	return ret_val;
    }

    connect_state->pending_operations++;

    /* post a completion event so the sock_post_connect can be notified through sock_wait */
    PostQueuedCompletionStatus(set, 0, (ULONG_PTR)connect_state, &connect_state->write.ovl);

    *connected = connect_state;

    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_CONNECT);
    return SOCK_SUCCESS;
}

int sock_accept(sock_t listener, sock_set_t set, void * user_ptr, sock_t * accepted)
{
    BOOL b;
    struct linger linger;
    int optval, len;
    sock_state_t *accept_state;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_ACCEPT);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_ACCEPT);

    if (!(listener->state & SOCK_ACCEPTED))
    {
	*accepted = NULL;
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_ACCEPT);
	return SOCK_FAIL;
    }

    accept_state = BlockAlloc(g_StateAllocator);
    if (accept_state == NULL)
    {
	*accepted = NULL;
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_ACCEPT);
	return SOCK_FAIL;
    }
    init_state_struct(accept_state);

    accept_state->type = SOCK_SOCKET;

    accept_state->sock = listener->sock;
    if (!post_next_accept(listener))
    {
	*accepted = NULL;
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_ACCEPT);
	return SOCK_FAIL;
    }

    /* set the socket to non-blocking */
    optval = TRUE;
    ioctlsocket(accept_state->sock, FIONBIO, &optval);

    /* set the linger option */
    linger.l_onoff = 1;
    linger.l_linger = 60;
    setsockopt(accept_state->sock, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));

    /* set the socket buffers */
    len = sizeof(int);
    if (!getsockopt(accept_state->sock, SOL_SOCKET, SO_RCVBUF, (char*)&optval, &len))
    {
	optval = g_socket_buffer_size;
	setsockopt(accept_state->sock, SOL_SOCKET, SO_RCVBUF, (char*)&optval, sizeof(int));
    }
    len = sizeof(int);
    if (!getsockopt(accept_state->sock, SOL_SOCKET, SO_SNDBUF, (char*)&optval, &len))
    {
	optval = g_socket_buffer_size;
	setsockopt(accept_state->sock, SOL_SOCKET, SO_SNDBUF, (char*)&optval, sizeof(int));
    }

    /* set the no-delay option */
    b = TRUE;
    setsockopt(accept_state->sock, IPPROTO_TCP, TCP_NODELAY, (char*)&b, sizeof(BOOL));

    /* prevent the socket from being inherited by child processes */
    DuplicateHandle(
	GetCurrentProcess(), (HANDLE)accept_state->sock,
	GetCurrentProcess(), (HANDLE*)&accept_state->sock,
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS);

    /* associate the socket with the completion port */
    if (CreateIoCompletionPort((HANDLE)accept_state->sock, set, (ULONG_PTR)accept_state, g_num_cp_threads) == NULL)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_ACCEPT);
	return SOCK_FAIL;
    }

    accept_state->user_ptr = user_ptr;
    accept_state->set = set;
    *accepted = accept_state;

    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_ACCEPT);
    return SOCK_SUCCESS;
}

int sock_post_close(sock_t sock)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCK_POST_CLOSE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_POST_CLOSE);

    if (sock == SOCK_INVALID_SOCK)
    {
	MPIU_DBG_PRINTF(("attempting to post a close but sock == SOCK_INVALID_SOCK.\n"));
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_CLOSE);
	return SOCK_FAIL;
    }
    if (sock->sock == INVALID_SOCKET)
    {
	MPIU_DBG_PRINTF(("attempting to post a close but sock.sock == INVALID_SOCKET.\n"));
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_CLOSE);
	return SOCK_FAIL;
    }

    if (sock->closing)
    {
	MPIU_DBG_PRINTF(("sock_post_close(%d) called more than once.\n", sock_getid(sock)));
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_CLOSE);
	return SOCK_FAIL;
    }

    if (sock->pending_operations != 0)
    {
#ifdef MPICH_DBG_OUTPUT
	if (sock->state & SOCK_CONNECTING)
	    MPIU_DBG_PRINTF(("sock_post_close(%d) called while sock is connecting.\n", sock_getid(sock)));
	if (sock->state & SOCK_READING)
	{
	    if (sock->read.use_iov)
	    {
		int i, n = 0;
		for (i=0; i<sock->read.iovlen; i++)
		    n += sock->read.iov[i].SOCK_IOV_LEN;
		MPIU_DBG_PRINTF(("sock_post_close(%d) called while sock is reading: %d bytes out of %d, index %d, iovlen %d.\n",
		    sock_getid(sock), sock->read.total, n, sock->read.index, sock->read.iovlen));
	    }
	    else
	    {
		MPIU_DBG_PRINTF(("sock_post_close(%d) called while sock is reading: %d bytes out of %d.\n",
		    sock_getid(sock), sock->read.total, sock->read.bufflen));
	    }
	}
	if (sock->state & SOCK_WRITING)
	{
	    if (sock->write.use_iov)
	    {
		int i, n = 0;
		for (i=0; i<sock->write.iovlen; i++)
		    n += sock->write.iov[i].SOCK_IOV_LEN;
		MPIU_DBG_PRINTF(("sock_post_close(%d) called while sock is writing: %d bytes out of %d, index %d, iovlen %d.\n",
		    sock_getid(sock), sock->write.total, n, sock->write.index, sock->write.iovlen));
	    }
	    else
	    {
		MPIU_DBG_PRINTF(("sock_post_close(%d) called while sock is writing: %d bytes out of %d.\n",
		    sock_getid(sock), sock->write.total, sock->write.bufflen));
	    }
	}
	fflush(stdout);
#endif
	/*
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_CLOSE);
	return SOCK_ERR_OP_IN_PROGRESS;
	*/
	/* posting a close cancels all outstanding operations */
    }

    sock->closing = TRUE;

    /*
    if (sock->pending_operations == 0)
    {
    */
	sock->pending_operations = 0;
	/*printf("flushing socket buffer before closing\n");fflush(stdout);*/
	FlushFileBuffers((HANDLE)sock->sock);
	shutdown(sock->sock, SD_BOTH);
	closesocket(sock->sock);
	sock->sock = INVALID_SOCKET;
	PostQueuedCompletionStatus(sock->set, 0, (ULONG_PTR)sock, NULL);
    /*
    }
    */
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_CLOSE);
    return SOCK_SUCCESS;
}

static volatile LONG g_nPostedIO = 0;

VOID CALLBACK socki_read(DWORD error, DWORD num_bytes, OVERLAPPED *ovl)
{
    sock_t sock;
    sock = (sock_t)ovl->hEvent;
    InterlockedDecrement(&g_nPostedIO);
    PostQueuedCompletionStatus(sock->set, num_bytes, (ULONG_PTR)sock, &sock->read.ovl);
}

void CALLBACK socki_wsaread(IN DWORD error, IN DWORD num_bytes, IN LPWSAOVERLAPPED ovl, IN DWORD flags)
{
    sock_t sock;
    sock = (sock_t)ovl->hEvent;
    InterlockedDecrement(&g_nPostedIO);
    PostQueuedCompletionStatus(sock->set, num_bytes, (ULONG_PTR)sock, &sock->read.ovl);
}

VOID CALLBACK socki_written(DWORD error, DWORD num_bytes, OVERLAPPED *ovl)
{
    sock_t sock;
    sock = (sock_t)ovl->hEvent;
    InterlockedDecrement(&g_nPostedIO);
    PostQueuedCompletionStatus(sock->set, num_bytes, (ULONG_PTR)sock, &sock->write.ovl);
}

void CALLBACK socki_wsawritten(IN DWORD error, IN DWORD num_bytes, IN LPWSAOVERLAPPED ovl, IN DWORD flags)
{
    sock_t sock;
    sock = (sock_t)ovl->hEvent;
    InterlockedDecrement(&g_nPostedIO);
    PostQueuedCompletionStatus(sock->set, num_bytes, (ULONG_PTR)sock, &sock->write.ovl);
}

int sock_wait(sock_set_t set, int millisecond_timeout, sock_event_t *out)
{
    int error;
    DWORD num_bytes;
    sock_state_t *sock;
    OVERLAPPED *ovl;
    DWORD dwFlags = 0;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_WAIT);
    MPIDI_STATE_DECL(MPID_STATE_GETQUEUEDCOMPLETIONSTATUS);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_WAIT);

    for (;;) 
    {
	if (g_nPostedIO > 0)
	    SleepEx(0, TRUE); /* allow the completion routines finish */
	MPIDI_FUNC_ENTER(MPID_STATE_GETQUEUEDCOMPLETIONSTATUS);
	/* initialize to NULL so we can compare the output of GetQueuedCompletionStatus */
	sock = NULL;
	ovl = NULL;
	num_bytes = 0;
	if (GetQueuedCompletionStatus(set, &num_bytes, (DWORD*)&sock, &ovl, millisecond_timeout))
	{
	    MPIDI_FUNC_EXIT(MPID_STATE_GETQUEUEDCOMPLETIONSTATUS);
	    if (sock->type == SOCK_SOCKET || sock->type == SOCK_NATIVE)
	    {
		if (sock->closing && sock->pending_operations == 0)
		{
		    out->op_type = SOCK_OP_CLOSE;
		    out->num_bytes = 0;
		    out->error = SOCK_SUCCESS;
		    out->user_ptr = sock->user_ptr;
		    CloseHandle(sock->read.ovl.hEvent);
		    CloseHandle(sock->write.ovl.hEvent);
		    sock->read.ovl.hEvent = NULL;
		    sock->write.ovl.hEvent = NULL;
#if 0
		    BlockFree(g_StateAllocator, sock); /* will this cause future io completion port errors since sock is the iocp user pointer? */
#endif
		    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
		    return SOCK_SUCCESS;
		}
		if (ovl == &sock->read.ovl)
		{
		    if (num_bytes == 0)
		    {
			/* socket closed */
			MPIU_DBG_PRINTF(("sock_wait readv returning %d bytes and EOF\n", sock->read.total));
			out->op_type = SOCK_OP_READ;
			out->num_bytes = sock->read.total;
			out->error = SOCK_EOF;
			out->user_ptr = sock->user_ptr;
			sock->pending_operations--;
			sock->state ^= SOCK_READING; /* remove the SOCK_READING bit */
			if (sock->closing && sock->pending_operations == 0)
			{
			    MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after iov read completed.\n", sock_getid(sock)));
			    FlushFileBuffers((HANDLE)sock->sock);
			    shutdown(sock->sock, SD_BOTH);
			    closesocket(sock->sock);
			    sock->sock = INVALID_SOCKET;
			}
			MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
			return SOCK_SUCCESS;
		    }
		    MPIU_DBG_PRINTF(("sock_wait read%s update: %d bytes\n", sock->read.use_iov ? "v" : "", num_bytes));
		    sock->read.total += num_bytes;
		    if (sock->read.use_iov)
		    {
			while (num_bytes)
			{
			    if (sock->read.iov[sock->read.index].SOCK_IOV_LEN <= num_bytes)
			    {
				num_bytes -= sock->read.iov[sock->read.index].SOCK_IOV_LEN;
				sock->read.index++;
				sock->read.iovlen--;
			    }
			    else
			    {
				sock->read.iov[sock->read.index].SOCK_IOV_LEN -= num_bytes;
				sock->read.iov[sock->read.index].SOCK_IOV_BUF = 
				    (char*)(sock->read.iov[sock->read.index].SOCK_IOV_BUF) + num_bytes;
				num_bytes = 0;
			    }
			}
			if (sock->read.iovlen == 0)
			{
			    MPIU_DBG_PRINTF(("sock_wait readv %d bytes\n", sock->read.total));
			    out->op_type = SOCK_OP_READ;
			    out->num_bytes = sock->read.total;
			    out->error = SOCK_SUCCESS;
			    out->user_ptr = sock->user_ptr;
			    sock->pending_operations--;
			    sock->state ^= SOCK_READING; /* remove the SOCK_READING bit */
			    if (sock->closing && sock->pending_operations == 0)
			    {
				MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after iov read completed.\n", sock_getid(sock)));
				FlushFileBuffers((HANDLE)sock->sock);
				shutdown(sock->sock, SD_BOTH);
				closesocket(sock->sock);
				sock->sock = INVALID_SOCKET;
			    }
			    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
			    return SOCK_SUCCESS;
			}
			/* make the user upcall */
			if (sock->read.progress_update != NULL)
			    sock->read.progress_update(num_bytes, sock->user_ptr);
			/* post a read of the remaining data */
			/*WSARecv(sock->sock, sock->read.iov, sock->read.iovlen, &sock->read.num_bytes, &dwFlags, &sock->read.ovl, NULL);*/
			if (WSARecv(sock->sock, &sock->read.iov[sock->read.index], sock->read.iovlen, &sock->read.num_bytes, &dwFlags, &sock->read.ovl, NULL) == SOCKET_ERROR)
			{
			    error = WSAGetLastError();
			    if (error == 0)
			    {
				out->op_type = SOCK_OP_READ;
				out->num_bytes = sock->read.total;
				out->error = SOCK_EOF;
				out->user_ptr = sock->user_ptr;
				sock->pending_operations--;
				sock->state ^= SOCK_READING;
				if (sock->closing && sock->pending_operations == 0)
				{
				    MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after iov read completed.\n", sock_getid(sock)));
				    FlushFileBuffers((HANDLE)sock->sock);
				    shutdown(sock->sock, SD_BOTH);
				    closesocket(sock->sock);
				    sock->sock = INVALID_SOCKET;
				}
				MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
				return SOCK_SUCCESS;
			    }
			    if (error != WSA_IO_PENDING)
			    {
				out->op_type = SOCK_OP_READ;
				out->num_bytes = sock->read.total;
				out->error = WinToSockError(error);
				out->user_ptr = sock->user_ptr;
				sock->pending_operations--;
				sock->state ^= SOCK_READING;
				if (sock->closing && sock->pending_operations == 0)
				{
				    MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after iov read completed.\n", sock_getid(sock)));
				    FlushFileBuffers((HANDLE)sock->sock);
				    shutdown(sock->sock, SD_BOTH);
				    closesocket(sock->sock);
				    sock->sock = INVALID_SOCKET;
				}
				MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
				return SOCK_SUCCESS;
			    }
			}
		    }
		    else
		    {
			sock->read.buffer = (char*)(sock->read.buffer) + num_bytes;
			sock->read.bufflen -= num_bytes;
			if (sock->read.bufflen == 0)
			{
			    MPIU_DBG_PRINTF(("sock_wait read %d bytes\n", sock->read.total));
			    out->op_type = SOCK_OP_READ;
			    out->num_bytes = sock->read.total;
			    out->error = SOCK_SUCCESS;
			    out->user_ptr = sock->user_ptr;
			    sock->pending_operations--;
			    sock->state ^= SOCK_READING; /* remove the SOCK_READING bit */
			    if (sock->closing && sock->pending_operations == 0)
			    {
				MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after simple read completed.\n", sock_getid(sock)));
				FlushFileBuffers((HANDLE)sock->sock);
				shutdown(sock->sock, SD_BOTH);
				closesocket(sock->sock);
				sock->sock = INVALID_SOCKET;
			    }
			    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
			    return SOCK_SUCCESS;
			}
			/* make the user upcall */
			if (sock->read.progress_update != NULL)
			    sock->read.progress_update(num_bytes, sock->user_ptr);
			/* post a read of the remaining data */
			if (!ReadFile((HANDLE)(sock->sock), sock->read.buffer, sock->read.bufflen, &sock->read.num_bytes, &sock->read.ovl))
			{
			    error = GetLastError();
			    MPIU_DBG_PRINTF(("GetLastError: %d\n", error));
			    if (error == ERROR_HANDLE_EOF || error == 0)
			    {
				out->op_type = SOCK_OP_READ;
				out->num_bytes = sock->read.total;
				out->error = SOCK_EOF;
				out->user_ptr = sock->user_ptr;
				sock->pending_operations--;
				sock->state ^= SOCK_READING;
				if (sock->closing && sock->pending_operations == 0)
				{
				    MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after iov read completed.\n", sock_getid(sock)));
				    FlushFileBuffers((HANDLE)sock->sock);
				    shutdown(sock->sock, SD_BOTH);
				    closesocket(sock->sock);
				    sock->sock = INVALID_SOCKET;
				}
				MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
				return SOCK_SUCCESS;
			    }
			    if (error != ERROR_IO_PENDING)
			    {
				out->op_type = SOCK_OP_READ;
				out->num_bytes = sock->read.total;
				out->error = WinToSockError(error);
				out->user_ptr = sock->user_ptr;
				sock->pending_operations--;
				sock->state ^= SOCK_READING;
				if (sock->closing && sock->pending_operations == 0)
				{
				    MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after iov read completed.\n", sock_getid(sock)));
				    FlushFileBuffers((HANDLE)sock->sock);
				    shutdown(sock->sock, SD_BOTH);
				    closesocket(sock->sock);
				    sock->sock = INVALID_SOCKET;
				}
				MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
				return SOCK_SUCCESS;
			    }
			}
		    }
		}
		else if (ovl == &sock->write.ovl)
		{
		    if (sock->state & SOCK_CONNECTING)
		    {
			/* insert code here to determine that the connect succeeded */
			/* ... */
			out->op_type = SOCK_OP_CONNECT;
			out->num_bytes = 0;
			out->error = SOCK_SUCCESS;
			out->user_ptr = sock->user_ptr;
			sock->pending_operations--;
			sock->state ^= SOCK_CONNECTING; /* remove the SOCK_CONNECTING bit */
			if (sock->closing && sock->pending_operations == 0)
			{
			    MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after connect completed.\n", sock_getid(sock)));
			    FlushFileBuffers((HANDLE)sock->sock);
			    shutdown(sock->sock, SD_BOTH);
			    closesocket(sock->sock);
			    sock->sock = INVALID_SOCKET;
			}
			MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
			return SOCK_SUCCESS;
		    }
		    else
		    {
			if (num_bytes == 0)
			{
			    /* socket closed */
			    MPIU_DBG_PRINTF(("sock_wait wreadv returning %d bytes and EOF\n", sock->write.total));
			    out->op_type = SOCK_OP_WRITE;
			    out->num_bytes = sock->write.total;
			    out->error = SOCK_EOF;
			    out->user_ptr = sock->user_ptr;
			    sock->pending_operations--;
			    sock->state ^= SOCK_WRITING; /* remove the SOCK_WRITING bit */
			    if (sock->closing && sock->pending_operations == 0)
			    {
				MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after iov write completed.\n", sock_getid(sock)));
				FlushFileBuffers((HANDLE)sock->sock);
				shutdown(sock->sock, SD_BOTH);
				closesocket(sock->sock);
				sock->sock = INVALID_SOCKET;
			    }
			    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
			    return SOCK_SUCCESS;
			}
			MPIU_DBG_PRINTF(("sock_wait: write update, total = %d + %d = %d\n", sock->write.total, num_bytes, sock->write.total + num_bytes));
			sock->write.total += num_bytes;
			if (sock->write.use_iov)
			{
			    while (num_bytes)
			    {
				if (sock->write.iov[sock->write.index].SOCK_IOV_LEN <= num_bytes)
				{
				    /*MPIU_DBG_PRINTF(("sock_wait: write.index %d, len %d\n", sock->write.index, 
					sock->write.iov[sock->write.index].SOCK_IOV_LEN));*/
				    num_bytes -= sock->write.iov[sock->write.index].SOCK_IOV_LEN;
				    sock->write.index++;
				    sock->write.iovlen--;
				}
				else
				{
				    /*MPIU_DBG_PRINTF(("sock_wait: partial data written [%d].len = %d, num_bytes = %d\n", sock->write.index,
					sock->write.iov[sock->write.index].SOCK_IOV_LEN, num_bytes));*/
				    sock->write.iov[sock->write.index].SOCK_IOV_LEN -= num_bytes;
				    sock->write.iov[sock->write.index].SOCK_IOV_BUF =
					(char*)(sock->write.iov[sock->write.index].SOCK_IOV_BUF) + num_bytes;
				    num_bytes = 0;
				}
			    }
			    if (sock->write.iovlen == 0)
			    {
				if (sock->write.total > 0)
				{
				    MPIU_DBG_PRINTF(("sock_wait wrotev %d bytes\n", sock->write.total));
				}
				out->op_type = SOCK_OP_WRITE;
				out->num_bytes = sock->write.total;
				out->error = SOCK_SUCCESS;
				out->user_ptr = sock->user_ptr;
				sock->pending_operations--;
				sock->state ^= SOCK_WRITING; /* remove the SOCK_WRITING bit */
				if (sock->closing && sock->pending_operations == 0)
				{
				    MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after iov write completed.\n", sock_getid(sock)));
				    FlushFileBuffers((HANDLE)sock->sock);
				    shutdown(sock->sock, SD_BOTH);
				    closesocket(sock->sock);
				    sock->sock = INVALID_SOCKET;
				}
				MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
				return SOCK_SUCCESS;
			    }
			    /* make the user upcall */
			    if (sock->write.progress_update != NULL)
				sock->write.progress_update(num_bytes, sock->user_ptr);
			    /* post a write of the remaining data */
			    MPIU_DBG_PRINTF(("sock_wait: posting write of the remaining data, vec size %d\n", sock->write.iovlen));
			    if (WSASend(sock->sock, sock->write.iov, sock->write.iovlen, &sock->write.num_bytes, 0, &sock->write.ovl, NULL) == SOCKET_ERROR)
			    {
				error = WSAGetLastError();
				if (error == 0)
				{
				    out->op_type = SOCK_OP_WRITE;
				    out->num_bytes = sock->write.total;
				    out->error = SOCK_EOF;
				    out->user_ptr = sock->user_ptr;
				    sock->pending_operations--;
				    sock->state ^= SOCK_WRITING;
				    if (sock->closing && sock->pending_operations == 0)
				    {
					MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after iov read completed.\n", sock_getid(sock)));
					FlushFileBuffers((HANDLE)sock->sock);
					shutdown(sock->sock, SD_BOTH);
					closesocket(sock->sock);
					sock->sock = INVALID_SOCKET;
				    }
				    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
				    return SOCK_SUCCESS;
				}
				if (error != WSA_IO_PENDING)
				{
				    out->op_type = SOCK_OP_WRITE;
				    out->num_bytes = sock->write.total;
				    out->error = WinToSockError(error);
				    out->user_ptr = sock->user_ptr;
				    sock->pending_operations--;
				    sock->state ^= SOCK_WRITING;
				    if (sock->closing && sock->pending_operations == 0)
				    {
					MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after iov read completed.\n", sock_getid(sock)));
					FlushFileBuffers((HANDLE)sock->sock);
					shutdown(sock->sock, SD_BOTH);
					closesocket(sock->sock);
					sock->sock = INVALID_SOCKET;
				    }
				    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
				    return SOCK_SUCCESS;
				}
			    }
			}
			else
			{
			    sock->write.buffer = (char*)(sock->write.buffer) + num_bytes;
			    sock->write.bufflen -= num_bytes;
			    if (sock->write.bufflen == 0)
			    {
#ifdef MPICH_DBG_OUTPUT
				if (sock->write.total > 0)
				{
				    MPIU_DBG_PRINTF(("sock_wait wrote %d bytes\n", sock->write.total));
				}
#endif
				out->op_type = SOCK_OP_WRITE;
				out->num_bytes = sock->write.total;
				out->error = SOCK_SUCCESS;
				out->user_ptr = sock->user_ptr;
				sock->pending_operations--;
				sock->state ^= SOCK_WRITING; /* remove the SOCK_WRITING bit */
				if (sock->closing && sock->pending_operations == 0)
				{
				    MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after simple write completed.\n", sock_getid(sock)));
				    FlushFileBuffers((HANDLE)sock->sock);
				    shutdown(sock->sock, SD_BOTH);
				    closesocket(sock->sock);
				    sock->sock = INVALID_SOCKET;
				}
				MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
				return SOCK_SUCCESS;
			    }
			    /* make the user upcall */
			    if (sock->write.progress_update != NULL)
				sock->write.progress_update(num_bytes, sock->user_ptr);
			    /* post a write of the remaining data */
			    if (!WriteFile((HANDLE)(sock->sock), sock->write.buffer, sock->write.bufflen, &sock->write.num_bytes, &sock->write.ovl))
			    {
				error = GetLastError();
				MPIU_DBG_PRINTF(("GetLastError: %d\n", error));
				if (error == ERROR_HANDLE_EOF || error == 0)
				{
				    out->op_type = SOCK_OP_WRITE;
				    out->num_bytes = sock->write.total;
				    out->error = SOCK_EOF;
				    out->user_ptr = sock->user_ptr;
				    sock->pending_operations--;
				    sock->state ^= SOCK_WRITING;
				    if (sock->closing && sock->pending_operations == 0)
				    {
					MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after iov read completed.\n", sock_getid(sock)));
					FlushFileBuffers((HANDLE)sock->sock);
					shutdown(sock->sock, SD_BOTH);
					closesocket(sock->sock);
					sock->sock = INVALID_SOCKET;
				    }
				    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
				    return SOCK_SUCCESS;
				}
				if (error != ERROR_IO_PENDING)
				{
				    out->op_type = SOCK_OP_WRITE;
				    out->num_bytes = sock->write.total;
				    out->error = WinToSockError(error);
				    out->user_ptr = sock->user_ptr;
				    sock->pending_operations--;
				    sock->state ^= SOCK_WRITING;
				    if (sock->closing && sock->pending_operations == 0)
				    {
					MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after iov read completed.\n", sock_getid(sock)));
					FlushFileBuffers((HANDLE)sock->sock);
					shutdown(sock->sock, SD_BOTH);
					closesocket(sock->sock);
					sock->sock = INVALID_SOCKET;
				    }
				    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
				    return SOCK_SUCCESS;
				}
			    }
			}
		    }
		}
		else
		{
		    if (num_bytes == 0)
		    {
			if ((sock->state & SOCK_READING))/* && sock-sock != INVALID_SOCKET) */
			{
			    MPIU_DBG_PRINTF(("EOF with posted read on sock %d\n", sock->sock));
			    out->op_type = SOCK_OP_READ;
			    out->num_bytes = sock->read.total;
			    out->error = SOCK_EOF;
			    out->user_ptr = sock->user_ptr;
			    sock->state ^= SOCK_READING;
			    sock->pending_operations--;
			    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
			    return SOCK_SUCCESS;
			}
			if ((sock->state & SOCK_WRITING))/* && sock->sock != INVALID_SOCKET) */
			{
			    MPIU_DBG_PRINTF(("EOF with posted write on sock %d\n", sock->sock));
			    out->op_type = SOCK_OP_WRITE;
			    out->num_bytes = sock->write.total;
			    out->error = SOCK_EOF;
			    out->user_ptr = sock->user_ptr;
			    sock->state ^= SOCK_WRITING;
			    sock->pending_operations--;
			    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
			    return SOCK_SUCCESS;
			}
			/*
			if (sock->state & SOCK_CLOSING)
			{
			    out->op_type = SOCK_OP_CLOSE;
			    out->num_bytes = 0;
			    out->error = SOCK_SUCCESS;
			    out->user_ptr = sock->user_ptr;
			    sock->state ^= SOCK_CLOSING;
			    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
			    return SOCK_SUCCESS;
			}
			*/
			MPIU_DBG_PRINTF(("ignoring EOF notification on unknown sock %d.\n", sock_getid(sock)));
		    }

		    if (sock->sock != INVALID_SOCKET)
		    {
			MPIU_DBG_PRINTF(("unmatched ovl: pending: %d, state = %d\n", sock->pending_operations, sock->state));
			MPIU_Error_printf("In sock_wait(), returned overlapped structure does not match the current read or write ovl: 0x%x\n", ovl);
			MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
			return SOCK_FAIL;
		    }
		    else
		    {
			MPIU_DBG_PRINTF(("ignoring notification on invalid sock.\n"));
		    }
		}
	    }
	    else if (sock->type == SOCK_LISTENER)
	    {
		if (sock->closing && sock->pending_operations == 0)
		{
		    out->op_type = SOCK_OP_CLOSE;
		    out->num_bytes = 0;
		    out->error = SOCK_SUCCESS;
		    out->user_ptr = sock->user_ptr;
		    CloseHandle(sock->read.ovl.hEvent);
		    CloseHandle(sock->write.ovl.hEvent);
		    sock->read.ovl.hEvent = NULL;
		    sock->write.ovl.hEvent = NULL;
#if 0
		    BlockFree(g_StateAllocator, sock); /* will this cause future io completion port errors since sock is the iocp user pointer? */
#endif
		    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
		    return SOCK_SUCCESS;
		}
		sock->state |= SOCK_ACCEPTED;
		out->op_type = SOCK_OP_ACCEPT;
		out->num_bytes = num_bytes;
		out->error = SOCK_SUCCESS;
		out->user_ptr = sock->user_ptr;
		MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
		return SOCK_SUCCESS;
	    }
	    else
	    {
		MPIU_Error_printf("sock type is not a SOCKET or a LISTENER, it's %d\n", sock->type);
		MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
		return SOCK_FAIL;
	    }
	}
	else
	{
	    /*MPIDI_FUNC_EXIT(MPID_STATE_GETQUEUEDCOMPLETIONSTATUS);*/ /* Maybe the logging will reset the last error? */
	    error = GetLastError();
	    MPIU_DBG_PRINTF(("GetQueuedCompletionStatus failed, GetLastError: %d\n", error));
	    MPIDI_FUNC_EXIT(MPID_STATE_GETQUEUEDCOMPLETIONSTATUS);
	    /* interpret error, return appropriate SOCK_ERR_... macro */
	    if (error == WAIT_TIMEOUT)
	    {
		MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
		return SOCK_ERR_TIMEOUT;
	    }
	    if (sock != NULL)
	    {
		if (sock->type == SOCK_SOCKET || sock->type == SOCK_NATIVE)
		{
		    if (ovl == &sock->read.ovl)
			out->op_type = SOCK_OP_READ;
		    else if (ovl == &sock->write.ovl)
			out->op_type = SOCK_OP_WRITE;
		    else
			out->op_type = -1;
		    out->num_bytes = 0;
		    out->error = WinToSockError(error);
		    out->user_ptr = sock->user_ptr;
		    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
		    return SOCK_SUCCESS;
		}
		if (sock->type == SOCK_LISTENER)
		{
		    /* this only works if the aborted operation is reported before the close is reported
		       because the close will free the sock structure causing these dereferences bogus.
		       I need to reference count the sock */
		    if (sock->closing)
		    {
#if 0
			/* This doesn't work because no more completion packets are received after an error? */
			if (error == ERROR_OPERATION_ABORTED)
			{
			    /* If the listener is being closed, ignore the aborted accept operation */
			    continue;
			}
#else
			out->op_type = SOCK_OP_CLOSE;
			out->num_bytes = 0;
			if (error == ERROR_OPERATION_ABORTED)
			    out->error = SOCK_SUCCESS;
			else
			    out->error = WinToSockError(error);
			out->user_ptr = sock->user_ptr;
			MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
			return SOCK_SUCCESS;
#endif
		    }
		    else
		    {
			/* Should we return a SOCK_OP_ACCEPT with an error if there is a failure on the listener? */
			out->op_type = SOCK_OP_ACCEPT;
			out->num_bytes = 0;
			out->error = WinToSockError(error);
			out->user_ptr = sock->user_ptr;
			MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
			return SOCK_SUCCESS;
		    }
		}
	    }

	    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
	    return WinToSockError(error);
	}
    }
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WAIT);
}

int sock_set_user_ptr(sock_t sock, void *user_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCK_SET_USER_PTR);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_SET_USER_PTR);
    if (sock == NULL)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_SET_USER_PTR);
	return SOCK_FAIL;
    }
    sock->user_ptr = user_ptr;
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_SET_USER_PTR);
    return SOCK_SUCCESS;
}

/* immediate functions */

int sock_read(sock_t sock, void *buf, sock_size_t len, sock_size_t *num_read)
{
    int e = SOCK_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_READ);
    e = recv(sock->sock, buf, len, 0);
    if (e == SOCKET_ERROR)
    {
	e = WSAGetLastError();
	if (e == WSAEWOULDBLOCK)
	{
	    *num_read = 0;
	    e = SOCK_SUCCESS;
	}
	else
	{
	    MPIU_DBG_PRINTF(("sock_read error %d\n", e));
	    e = WinToSockError(e);
	}
    }
    else
    {
	*num_read = e;
	e = (e == 0) ? SOCK_EOF : SOCK_SUCCESS;
    }
#ifdef MPICH_DBG_OUTPUT
    if (e == SOCK_SUCCESS)
    {
	MPIU_DBG_PRINTF(("sock_read %d of %d bytes\n", *num_read, len));
    }
#endif
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_READ);
    return e;
}

int sock_readv(sock_t sock, SOCK_IOV *iov, int n, sock_size_t *num_read)
{
    int e = SOCK_SUCCESS;
    DWORD nFlags = 0;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_READV);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_READV);

    if (WSARecv(sock->sock, iov, n, num_read, &nFlags, NULL/*overlapped*/, NULL/*completion routine*/) == SOCKET_ERROR)
    {
	e = WSAGetLastError();
	*num_read = 0;
	e = (e == WSAEWOULDBLOCK) ? SOCK_SUCCESS : WinToSockError(e);
    }
    MPIU_DBG_PRINTF(("sock_readv %d bytes\n", *num_read));
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_READV);
    return e;
}

int sock_write(sock_t sock, void *buf, sock_size_t len, sock_size_t *num_written)
{
    int length, num_sent, total = 0;
#ifdef MPICH_DBG_OUTPUT
    sock_size_t len_orig = len;
#endif
    MPIDI_STATE_DECL(MPID_STATE_SOCK_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_WRITE);
    if (len < g_stream_packet_length)
    {
	total = send(sock->sock, buf, len, 0);
	if (total == SOCKET_ERROR)
	{
	    if (WSAGetLastError() == WSAEWOULDBLOCK)
		total = 0;
	    else
	    {
		MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WRITE);
		return WinToSockError(WSAGetLastError());
	    }
	}
	*num_written = total;
	MPIU_DBG_PRINTF(("sock_write %d of %d bytes\n", total, len_orig));
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WRITE);
	return SOCK_SUCCESS;
    }
    *num_written = 0;
    while (len)
    {
	length = min(len, g_stream_packet_length);
	num_sent = send(sock->sock, buf, length, 0);
	if (num_sent == SOCKET_ERROR)
	{
	    if (WSAGetLastError() == WSAEWOULDBLOCK)
		num_sent = 0;
	    else
	    {
		MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WRITE);
		return WinToSockError(WSAGetLastError());
	    }
	}
	total += num_sent;
	len -= num_sent;
	buf = (char*)buf + num_sent;
	if (num_sent < length)
	{
	    *num_written = total;
	    MPIU_DBG_PRINTF(("sock_write %d of %d bytes\n", total, len_orig));
	    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WRITE);
	    return SOCK_SUCCESS;
	}
    }
    *num_written = total;
    MPIU_DBG_PRINTF(("sock_write %d of %d bytes\n", total, len_orig));
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WRITE);
    return SOCK_SUCCESS;
}

int sock_writev(sock_t sock, SOCK_IOV *iov, int n, sock_size_t *num_written)
{
    int error;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_WRITEV);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_WRITEV);
    assert(n > 0);
    /*
    if (n == 0)
    {
	MPIU_DBG_PRINTF(("empty vector passed into sock_writev\n"));
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WRITEV);
	return 0;
    }
    */
    if (n>1 && (int)iov[1].MPID_IOV_LEN > g_stream_packet_length)
    {
	int total = 0;
	int i;
	int num_sent;
	for (i=0; i<n; i++)
	{
	    error = sock_write(sock, iov[i].MPID_IOV_BUF, iov[i].MPID_IOV_LEN, &num_sent);
	    if (error != SOCK_SUCCESS)
	    {
		MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WRITEV);
		return error;
	    }
	    total += num_sent;
	    if (num_sent != iov[i].MPID_IOV_LEN)
	    {
		*num_written = total;
		MPIU_DBG_PRINTF(("sock_writev %d bytes\n", total));
		MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WRITEV);
		return SOCK_SUCCESS;
	    }
	}
	*num_written = total;
    }
    else
    {
	if (WSASend(sock->sock, iov, n, num_written, 0, NULL/*overlapped*/, NULL/*completion routine*/) == SOCKET_ERROR)
	{
	    if (WSAGetLastError() != WSAEWOULDBLOCK)
	    {
		MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WRITEV);
		return WinToSockError(WSAGetLastError());
	    }
	    *num_written = 0;
	}
	MPIU_DBG_PRINTF(("sock_writev %d bytes\n", *num_written));
    }
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_WRITEV);
    return SOCK_SUCCESS;
}

/* non-blocking functions */

int sock_post_read(sock_t sock, void *buf, sock_size_t len, int (*rfn)(sock_size_t, void*))
{
    int e = SOCK_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_POST_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_POST_READ);
    sock->read.total = 0;
    sock->read.buffer = buf;
    sock->read.bufflen = len;
    sock->read.use_iov = FALSE;
    sock->read.progress_update = rfn;
    sock->state |= SOCK_READING;
    sock->pending_operations++;
    if (sock->type == SOCK_NATIVE)
    {
	InterlockedIncrement(&g_nPostedIO);
	if (!ReadFileEx((HANDLE)(sock->sock), buf, len, &sock->read.ovl, (LPOVERLAPPED_COMPLETION_ROUTINE)socki_read))
	{
	    switch (e = GetLastError())
	    {
	    case ERROR_HANDLE_EOF:
		MPIU_DBG_PRINTF(("GetLastError: %d\n", e));
		e = SOCK_EOF;
		InterlockedDecrement(&g_nPostedIO);
		break;
	    case ERROR_IO_PENDING:
		e = SOCK_SUCCESS;
		break;
	    default:
		MPIU_DBG_PRINTF(("GetLastError: %d\n", e));
		e = WinToSockError(e);
		InterlockedDecrement(&g_nPostedIO);
		break;
	    }
	}
    }
    else
    {
    MPIU_DBG_PRINTF(("sock_post_read - %d bytes\n", len));
    if (!ReadFile((HANDLE)(sock->sock), buf, len, &sock->read.num_bytes, &sock->read.ovl))
    {
	switch (e = GetLastError())
	{
	case ERROR_HANDLE_EOF:
	    MPIU_DBG_PRINTF(("GetLastError: %d\n", e));
	    MPIU_Error_printf("sock_post_read: ReadFile failed, error %d\n", GetLastError());fflush(stdout);
	    e = SOCK_EOF;
	    break;
        case ERROR_IO_PENDING:
	    e = SOCK_SUCCESS;
	    break;
	default:
	    MPIU_DBG_PRINTF(("GetLastError: %d\n", e));
	    MPIU_Error_printf("sock_post_read: ReadFile failed, error %d\n", GetLastError());fflush(stdout);
	    e = WinToSockError(e);
	    break;
	}
    }
    }
    if (e != SOCK_SUCCESS)
	sock->state ^= SOCK_READING;
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_READ);
    return e;
}

int sock_post_readv(sock_t sock, SOCK_IOV *iov, int n, int (*rfn)(sock_size_t, void*))
{
    int iter;
    int e = SOCK_SUCCESS;
#ifdef MPICH_DBG_OUTPUT
    int i;
#endif
    DWORD flags = 0;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_POST_READV);
#ifdef USE_SOCK_IOV_COPY
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);
#endif

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_POST_READV);
    sock->read.total = 0;
#ifdef USE_SOCK_IOV_COPY
    MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
    memcpy(sock->read.iov, iov, sizeof(SOCK_IOV) * n);
    MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
#else
    sock->read.iov = iov;
#endif
    sock->read.iovlen = n;
    sock->read.index = 0;
    sock->read.use_iov = TRUE;
    sock->read.progress_update = rfn;
    sock->state |= SOCK_READING;
    sock->pending_operations++;
#ifdef MPICH_DBG_OUTPUT
    for (i=0; i<n; i++)
    {
	MPIU_DBG_PRINTF(("sock_post_readv - iov[%d].len = %d\n", i, iov[i].MPID_IOV_LEN));
    }
#endif
    if (sock->type == SOCK_NATIVE)
        InterlockedIncrement(&g_nPostedIO);
    for (iter=0; iter<10; iter++)
    {
	if (WSARecv(sock->sock, sock->read.iov, n, &sock->read.num_bytes, &flags, &sock->read.ovl, 
	    (sock->type == SOCK_NATIVE) ? socki_wsaread : NULL) != SOCKET_ERROR)
	    break;

	e = WSAGetLastError();
	if (e == WSA_IO_PENDING)
	{
	    e = SOCK_SUCCESS;
	    break;
	}
	if (e != WSAEWOULDBLOCK)
	{
	    if (sock->type == SOCK_NATIVE)
		InterlockedDecrement(&g_nPostedIO);
	    e = WinToSockError(e);
	    break;
	}
	Sleep(200);
    }
    if (e != SOCK_SUCCESS)
	sock->state ^= SOCK_READING;
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_READV);
    return e;
}

int sock_post_write(sock_t sock, void *buf, sock_size_t len, int (*wfn)(sock_size_t, void*))
{
    int e = SOCK_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_POST_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_POST_WRITE);
    if (len == 0)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_WRITE);
	return SOCK_FAIL;
    }
    sock->write.total = 0;
    sock->write.buffer = buf;
    sock->write.bufflen = len;
    sock->write.use_iov = FALSE;
    sock->write.progress_update = wfn;
    sock->state |= SOCK_WRITING;
    sock->pending_operations++;
    MPIU_DBG_PRINTF(("sock_post_write - %d bytes\n", len));
    if (sock->type == SOCK_NATIVE)
    {
        InterlockedIncrement(&g_nPostedIO);
	if (!WriteFileEx((HANDLE)(sock->sock), buf, len, &sock->write.ovl, (LPOVERLAPPED_COMPLETION_ROUTINE)socki_written))
	{
	    switch (e = GetLastError())
	    {
	    case ERROR_HANDLE_EOF:
		MPIU_DBG_PRINTF(("GetLastError: %d\n", e));
		e = SOCK_EOF;
		InterlockedDecrement(&g_nPostedIO);
		break;
	    case ERROR_IO_PENDING:
		MPIU_DBG_PRINTF(("GetLastError: %d\n", e));
		e = SOCK_SUCCESS;
		break;
	    default:
		MPIU_DBG_PRINTF(("GetLastError: %d\n", e));
		e = WinToSockError(e);
		InterlockedDecrement(&g_nPostedIO);
		break;
	    }
	}
    }
    else
    {
    if (!WriteFile((HANDLE)(sock->sock), buf, len, &sock->write.num_bytes, &sock->write.ovl))
    {
	switch (e = GetLastError())
	{
	case ERROR_HANDLE_EOF:
	    MPIU_DBG_PRINTF(("GetLastError: %d\n", e));
	    e = SOCK_EOF;
	    break;
        case ERROR_IO_PENDING:
	    e = SOCK_SUCCESS;
	    break;
	default:
	    MPIU_DBG_PRINTF(("GetLastError: %d\n", e));
	    e = WinToSockError(e);
	    break;
	}
    }
    }
    if (e != SOCK_SUCCESS)
	sock->state ^= SOCK_WRITING;
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_WRITE);
    return e;
}

int sock_post_writev(sock_t sock, SOCK_IOV *iov, int n, int (*wfn)(sock_size_t, void*))
{
    int iter, e = SOCK_SUCCESS;
#ifdef MPICH_DBG_OUTPUT
    int i;
#endif
    MPIDI_STATE_DECL(MPID_STATE_SOCK_POST_WRITEV);
#ifdef USE_SOCK_IOV_COPY
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);
#endif

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_POST_WRITEV);
    if (n == 0)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_WRITEV);
	return SOCK_FAIL;
    }
    sock->write.total = 0;
#ifdef USE_SOCK_IOV_COPY
    MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
    memcpy(sock->write.iov, iov, sizeof(SOCK_IOV) * n);
    MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
#else
    sock->write.iov = iov;
#endif
    sock->write.iovlen = n;
    sock->write.index = 0;
    sock->write.use_iov = TRUE;
    sock->write.progress_update = wfn;
    sock->state |= SOCK_WRITING;
    sock->pending_operations++;
    /*
    {
	char str[1024], *s = str;
	int i;
	s += sprintf(s, "sock_post_writev(");
	for (i=0; i<n; i++)
	    s += sprintf(s, "%d,", iov[i].SOCK_IOV_LEN);
	sprintf(s, ")\n");
	MPIU_DBG_PRINTF(("%s", str));
    }
    */
#ifdef MPICH_DBG_OUTPUT
    for (i=0; i<n; i++)
    {
	MPIU_DBG_PRINTF(("sock_post_writev - iov[%d].len = %d\n", i, iov[i].MPID_IOV_LEN));
	fflush(stdout);
    }
#endif
    if (sock->type == SOCK_NATIVE)
	InterlockedIncrement(&g_nPostedIO);
    for (iter=0; iter<10; iter++)
    {
	if (WSASend(sock->sock, sock->write.iov, n, &sock->write.num_bytes, 0, &sock->write.ovl, 
	    (sock->type == SOCK_NATIVE) ? socki_wsawritten : NULL) != SOCKET_ERROR)
	    break;

	e = WSAGetLastError();
	if (e == WSA_IO_PENDING)
	{
	    e = SOCK_SUCCESS;
	    break;
	}
	if (e != WSAEWOULDBLOCK)
	{
	    if (sock->type == SOCK_NATIVE)
		InterlockedDecrement(&g_nPostedIO);
	    e = WinToSockError(e);
	    break;
	}
	Sleep(200);
    }
    if (e != SOCK_SUCCESS)
	sock->state ^= SOCK_WRITING;
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_WRITEV);
    return e;
}

/* extended functions */

int sock_getid(sock_t sock)
{
    int ret_val;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_GETID);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_GETID);
    if (sock == SOCK_INVALID_SOCK)
	ret_val = -1;
    else
	ret_val = (int)sock->sock;
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_GETID);
    return ret_val;
}

int sock_getsetid(sock_set_t set)
{
    int ret_val;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_GETSETID);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_GETSETID);
    ret_val = (int)set;
    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_GETSETID);
    return ret_val;
}

int sock_easy_receive(sock_t sock, void *buf, sock_size_t len, sock_size_t *num_read)
{
    int error;
    int n;
    int total = 0;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_EASY_RECEIVE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_EASY_RECEIVE);

    while (len)
    {
	error = sock_read(sock, buf, len, &n);
	if (error != SOCK_SUCCESS)
	{
	    *num_read = total;
	    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_EASY_RECEIVE);
	    return error;
	}
	total += n;
	buf = (char*)buf + n;
	len -= n;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_EASY_RECEIVE);
    return SOCK_SUCCESS;
}

int sock_easy_send(sock_t sock, void *buf, sock_size_t len, sock_size_t *num_written)
{
    int error;
    int n;
    int total = 0;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_EASY_SEND);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_EASY_SEND);

    while (len)
    {
	error = sock_write(sock, buf, len, &n);
	if (error != SOCK_SUCCESS)
	{
	    *num_written = total;
	    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_EASY_SEND);
	    return error;
	}
	total += n;
	buf = (char*)buf + n;
	len -= n;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCK_EASY_SEND);
    return SOCK_SUCCESS;
}

#endif /* WITH_SOCK_TYPE == SOCK_IOCP  */
