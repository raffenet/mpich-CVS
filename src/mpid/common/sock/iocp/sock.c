/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpidu_sock.h"

#define MPIDI_QUOTE(A) MPIDI_QUOTE2(A)
#define MPIDI_QUOTE2(A) #A

#define SOCKI_TCP_BUFFER_SIZE       32*1024
#define SOCKI_STREAM_PACKET_LENGTH  8*1024
#define SOCKI_DESCRIPTION_LENGTH    256

typedef enum SOCKI_TYPE { SOCKI_INVALID, SOCKI_LISTENER, SOCKI_SOCKET } SOCKI_TYPE;

typedef int SOCKI_STATE;
#define SOCKI_ACCEPTING  0x0001
#define SOCKI_ACCEPTED   0x0002
#define SOCKI_CONNECTING 0x0004
#define SOCKI_READING    0x0008
#define SOCKI_WRITING    0x0010

typedef struct sock_buffer
{
    int use_iov;
    DWORD num_bytes;
    OVERLAPPED ovl;
    void *buffer;
    MPIU_Size_t bufflen;
    MPIU_Size_t maxbufflen;
#ifdef USE_SOCK_IOV_COPY
    MPID_IOV iov[MPID_IOV_MAXLEN];
#else
    MPID_IOV *iov;
#endif
    int iovlen;
    int index;
    MPIU_Size_t total;
    MPIDU_Sock_progress_update_func_t progress_update;
} sock_buffer;

typedef struct sock_state_t
{
    SOCKI_TYPE type;
    SOCKI_STATE state;
    SOCKET sock;
    MPIDU_Sock_set_t set;
    int closing;
    int pending_operations;
    /* listener/accept structures */
    SOCKET listen_sock;
    char accept_buffer[sizeof(struct sockaddr_in)*2+32];
    /* read and write structures */
    sock_buffer read;
    sock_buffer write;
    /* connect structures */
    char *cur_host;
    char host_description[SOCKI_DESCRIPTION_LENGTH];
    /* user pointer */
    void *user_ptr;
} sock_state_t;

static int g_num_cp_threads = 2;
static int g_socket_buffer_size = SOCKI_TCP_BUFFER_SIZE;
static int g_stream_packet_length = SOCKI_STREAM_PACKET_LENGTH;
static int g_init_called = 0;

/* utility allocator functions */

typedef struct BlockAllocator_struct * BlockAllocator;
#ifdef WITH_ALLOCATOR_LOCKING
typedef volatile long SOCKI_Lock_t;
#endif

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
    SOCKI_Lock_t lock;
#endif
};

#ifdef WITH_ALLOCATOR_LOCKING

static int g_nLockSpinCount = 100;

static inline void SOCKI_Init_lock( SOCKI_Lock_t *lock )
{
    *(lock) = 0;
}

static inline void SOCKI_Lock( SOCKI_Lock_t *lock )
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
                    return;
                }
#elif defined(HAVE_COMPARE_AND_SWAP)
                if (compare_and_swap(lock, 0, 1) == 1)
                {
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

static inline void SOCKI_Unlock( SOCKI_Lock_t *lock )
{
    *(lock) = 0;
}

#endif

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
    SOCKI_Init_lock(&p->lock);
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
    SOCKI_Lock(&p->lock);
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
    SOCKI_Unlock(&p->lock);
#endif

    return pVoid;
}

static int BlockFree(BlockAllocator p, void *pBlock)
{
#ifdef WITH_ALLOCATOR_LOCKING
    SOCKI_Lock(&p->lock);
#endif

    ((void**)pBlock)--;
    *((void**)pBlock) = p->pNextFree;
    p->pNextFree = pBlock;

#ifdef WITH_ALLOCATOR_LOCKING
    SOCKI_Unlock(&p->lock);
#endif

    return 0;
}

#undef FUNCNAME
#define FUNCNAME easy_create
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int easy_create(SOCKET *sock, int port, unsigned long addr)
{
    int mpi_errno;
    /*struct linger linger;*/
    int optval, len;
    SOCKET temp_sock;
    SOCKADDR_IN sockAddr;

    /* create the non-blocking socket */
    temp_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (temp_sock == INVALID_SOCKET)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**socket", "**socket %d", WSAGetLastError());
	return mpi_errno;
    }
    
    memset(&sockAddr,0,sizeof(sockAddr));
    
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = addr;
    sockAddr.sin_port = htons((unsigned short)port);

    if (bind(temp_sock, (SOCKADDR*)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**socket", "**socket %d", WSAGetLastError());
	return mpi_errno;
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
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**duphandle", "**duphandle %d", GetLastError());
	return mpi_errno;
    }

    /* Set the no-delay option */
    setsockopt(*sock, IPPROTO_TCP, TCP_NODELAY, (char *)&optval, sizeof(optval));

    return MPI_SUCCESS;
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

#undef FUNCNAME
#define FUNCNAME post_next_accept
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int post_next_accept(sock_state_t * listen_state)
{
    int mpi_errno;
    listen_state->state = SOCKI_ACCEPTING;
    listen_state->sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (listen_state->sock == INVALID_SOCKET)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %d", WSAGetLastError());
	return mpi_errno;
    }
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
	mpi_errno = WSAGetLastError();
	if (mpi_errno == ERROR_IO_PENDING)
	    return MPI_SUCCESS;
	/*MPIU_Error_printf("AcceptEx failed with error %d\n", error);fflush(stdout);*/
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %d", mpi_errno);
	return mpi_errno;
    }
    return MPI_SUCCESS;
}

/* sock functions */

static BlockAllocator g_StateAllocator;

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDU_Sock_init()
{
    int mpi_errno;
    char *szNum;
    WSADATA wsaData;
    int err;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_INIT);

    if (g_init_called)
    {
	g_init_called++;
	/*
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**sock_init", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_INIT);
	return mpi_errno;
	*/
	return MPI_SUCCESS;
    }

    /* Start the Winsock dll */
    if ((err = WSAStartup(MAKEWORD(2, 0), &wsaData)) != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**wsasock", "**wsasock %d", err);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_INIT);
	return mpi_errno;
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

    g_init_called = 1;

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_INIT);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDU_Sock_finalize()
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_FINALIZE);
    if (!g_init_called)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**sock_init", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_FINALIZE);
	return mpi_errno;
    }
    g_init_called--;
    if (g_init_called == 0)
    {
	WSACleanup();
	BlockAllocFinalize(&g_StateAllocator);
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_FINALIZE);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_hostname_to_host_description
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDU_Sock_hostname_to_host_description(char *hostname, char * host_description, int len)
{
    int mpi_errno;
#if 0
    char **hlist;
    struct hostent *h = NULL;
    int n = 0;
#endif
    ADDRINFO *res, *iter, hint;
    char host[256];
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_HOSTNAME_TO_HOST_DESCRIPTION);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_HOSTNAME_TO_HOST_DESCRIPTION);
    if (!g_init_called)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**sock_init", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_HOSTNAME_TO_HOST_DESCRIPTION);
	return mpi_errno;
    }

    hint.ai_flags = AI_PASSIVE | AI_CANONNAME;
    hint.ai_family = PF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_protocol = 0;
    hint.ai_addrlen = 0;
    hint.ai_canonname = NULL;
    hint.ai_addr = 0;
    hint.ai_next = NULL;
    if (getaddrinfo(hostname, NULL, NULL, &res))
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**getinfo", "**getinfo %d", WSAGetLastError());
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_HOSTNAME_TO_HOST_DESCRIPTION);
	return mpi_errno;
    }

    /* add the host names */
    iter = res;
    while (iter)
    {
	if (iter->ai_canonname)
	{
	    /*printf("adding canonname: %s\n", iter->ai_canonname);*/
	    mpi_errno = MPIU_Str_add_string(&host_description, &len, iter->ai_canonname);
	    if (mpi_errno)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM, "**desc_len", 0);
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_HOSTNAME_TO_HOST_DESCRIPTION);
		return mpi_errno;
	    }
	}
	else
	{
	    switch (iter->ai_family)
	    {
	    case PF_INET:
	    case PF_INET6:
		if (getnameinfo(iter->ai_addr, (socklen_t)iter->ai_addrlen, host, 256, NULL, 0, 0) == 0)
		{
		    /*printf("adding nameinfo: %s\n", host);*/
		    mpi_errno = MPIU_Str_add_string(&host_description, &len, host);
		    if (mpi_errno)
		    {
			mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM, "**desc_len", 0);
			MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_HOSTNAME_TO_HOST_DESCRIPTION);
			return mpi_errno;
		    }
		}
		break;
	    }
	}
	iter = iter->ai_next;
    }
    /* add the names again, this time as ip addresses */
    iter = res;
    while (iter)
    {
	switch (iter->ai_family)
	{
	case PF_INET:
	case PF_INET6:
	    if (getnameinfo(iter->ai_addr, (socklen_t)iter->ai_addrlen, host, 256, NULL, 0, NI_NUMERICHOST) == 0)
	    {
		/*printf("adding nameinfo: %s\n", host);*/
		mpi_errno = MPIU_Str_add_string(&host_description, &len, host);
		if (mpi_errno)
		{
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM, "**desc_len", 0);
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_HOSTNAME_TO_HOST_DESCRIPTION);
		    return mpi_errno;
		}
	    }
	    break;
	}
	iter = iter->ai_next;
    }
    if (res)
	freeaddrinfo(res);

#if 0
    h = gethostbyname(hostname);
    if (h == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**sock_byname", "**sock_byname %d", WSAGetLastError());
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_HOSTNAME_TO_HOST_DESCRIPTION);
	return mpi_errno;
    }
    
    hlist = h->h_addr_list;
    while (*hlist != NULL && n<max)
    {
	pIP[n] = *(int32_t*)(*hlist);

	/*{	
	unsigned int a, b, c, d;
	a = ((unsigned char *)(&pIP[n]))[0];
	b = ((unsigned char *)(&pIP[n]))[1];
	c = ((unsigned char *)(&pIP[n]))[2];
	d = ((unsigned char *)(&pIP[n]))[3];
	printf("ip: %u.%u.%u.%u\n", a, b, c, d);fflush(stdout);
	}*/

	hlist++;
	n++;
    }
#endif
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_HOSTNAME_TO_HOST_DESCRIPTION);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_get_host_description
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDU_Sock_get_host_description(char * host_description, int len)
{
    int mpi_errno;
    char hostname[100];
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_GET_HOST_DESCRIPTION);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_GET_HOST_DESCRIPTION);
    if (!g_init_called)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**sock_init", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_GET_HOST_DESCRIPTION);
	return mpi_errno;
    }

    if (gethostname(hostname, 100) == SOCKET_ERROR)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**sock_gethost", "**sock_gethost %d", WSAGetLastError());
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_GET_HOST_DESCRIPTION);
	return mpi_errno;
    }

    mpi_errno = MPIDU_Sock_hostname_to_host_description(hostname, host_description, len);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_GET_HOST_DESCRIPTION);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_create_set
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDU_Sock_create_set(MPIDU_Sock_set_t * set)
{
    int mpi_errno;
    HANDLE port;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_CREATE_SET);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_CREATE_SET);
    if (!g_init_called)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**sock_init", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_CREATE_SET);
	return mpi_errno;
    }
    port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, g_num_cp_threads);
    if (port != NULL)
    {
	*set = port;
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_CREATE_SET);
	return MPI_SUCCESS;
    }
    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**iocp", "**iocp %d", GetLastError());
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_CREATE_SET);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_destroy_set
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDU_Sock_destroy_set(MPIDU_Sock_set_t set)
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_DESTROY_SET);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_DESTROY_SET);
    if (!g_init_called)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**sock_init", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_DESTROY_SET);
	return mpi_errno;
    }
    if (!CloseHandle(set))
    {
	mpi_errno = GetLastError();
	if (mpi_errno == ERROR_INVALID_HANDLE)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SET, "**bad_set", 0);
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_DESTROY_SET);
	    return mpi_errno;
	}
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %d", errno);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_DESTROY_SET);
	return mpi_errno;
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_DESTROY_SET);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_native_to_sock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDU_Sock_native_to_sock(MPIDU_Sock_set_t set, MPIDU_SOCK_NATIVE_FD fd, void *user_ptr, MPIDU_Sock_t *sock_ptr)
{
    int mpi_errno;
    /*int ret_val;*/
    sock_state_t *sock_state;
    /*u_long optval;*/
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_NATIVE_TO_SOCK);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_NATIVE_TO_SOCK);
    if (!g_init_called)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**sock_init", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_NATIVE_TO_SOCK);
	return mpi_errno;
    }

    /* setup the structures */
    sock_state = (sock_state_t*)BlockAlloc(g_StateAllocator);
    if (sock_state == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM, "**nomem", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_NATIVE_TO_SOCK);
	return mpi_errno;
    }
    init_state_struct(sock_state);
    sock_state->sock = (SOCKET)fd;

    /* set the socket to non-blocking */
    /* leave the native handle in the state passed in?
    optval = TRUE;
    ioctlsocket(sock_state->sock, FIONBIO, &optval);
    */

    sock_state->user_ptr = user_ptr;
    sock_state->type = SOCKI_SOCKET;
    sock_state->state = 0;
    sock_state->set = set;

    /* associate the socket with the completion port */
    /*printf("CreateIOCompletionPort(%d, %p, %p, %d)\n", sock_state->sock, set, sock_state, g_num_cp_threads);fflush(stdout);*/
    if (CreateIoCompletionPort((HANDLE)sock_state->sock, set, (ULONG_PTR)sock_state, g_num_cp_threads) == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM, "**nomem", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_NATIVE_TO_SOCK);
	return mpi_errno;
    }

    *sock_ptr = sock_state;

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_NATIVE_TO_SOCK);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_listen
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDU_Sock_listen(MPIDU_Sock_set_t set, void * user_ptr, int * port, MPIDU_Sock_t * sock)
{
    int mpi_errno;
    char host[100];
    DWORD num_read = 0;
    sock_state_t * listen_state;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_LISTEN);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_LISTEN);
    if (!g_init_called)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**sock_init", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_LISTEN);
	return mpi_errno;
    }

    listen_state = (sock_state_t*)BlockAlloc(g_StateAllocator);
    init_state_struct(listen_state);
    mpi_errno = easy_create(&listen_state->listen_sock, *port, INADDR_ANY);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**sock_create", "**sock_create %d", mpi_errno);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_LISTEN);
	return mpi_errno;
    }
    if (listen(listen_state->listen_sock, SOMAXCONN) == SOCKET_ERROR)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**listen", "**listen %d", WSAGetLastError());
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_LISTEN);
	return mpi_errno;
    }
    if (CreateIoCompletionPort((HANDLE)listen_state->listen_sock, set, (ULONG_PTR)listen_state, g_num_cp_threads) == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**iocp", "**iocp %d", GetLastError());
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_LISTEN);
	return mpi_errno;
    }
    easy_get_sock_info(listen_state->listen_sock, host, port);
    listen_state->user_ptr = user_ptr;
    listen_state->type = SOCKI_LISTENER;
    /* do this last to make sure the listener state structure is completely initialized before
       a completion thread has the chance to satisfy the AcceptEx call */
    mpi_errno = post_next_accept(listen_state);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**post_accept", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_LISTEN);
	return mpi_errno;
    }
    *sock = listen_state;
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_LISTEN);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_accept
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDU_Sock_accept(MPIDU_Sock_t listener_sock, MPIDU_Sock_set_t set, void * user_ptr, MPIDU_Sock_t * sock)
{
    int mpi_errno;
    BOOL b;
    struct linger linger;
    int optval, len;
    sock_state_t *accept_state;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_ACCEPT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_ACCEPT);
    if (!g_init_called)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**sock_init", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_ACCEPT);
	return mpi_errno;
    }

    if (!(listener_sock->state & SOCKI_ACCEPTED))
    {
	*sock = NULL;
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NO_NEW_SOCK, "**sock_nop_accept", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_ACCEPT);
	return mpi_errno;
    }

    accept_state = BlockAlloc(g_StateAllocator);
    if (accept_state == NULL)
    {
	*sock = NULL;
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM, "**nomem", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_ACCEPT);
	return mpi_errno;
    }
    init_state_struct(accept_state);

    accept_state->type = SOCKI_SOCKET;

    accept_state->sock = listener_sock->sock;
    mpi_errno = post_next_accept(listener_sock);
    if (mpi_errno != MPI_SUCCESS)
    {
	*sock = NULL;
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**post_accept", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_ACCEPT);
	return mpi_errno;
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
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**iocp", "**iocp %d", GetLastError());
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_ACCEPT);
	return mpi_errno;
    }

    accept_state->user_ptr = user_ptr;
    accept_state->set = set;
    *sock = accept_state;

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_ACCEPT);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_post_connect
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDU_Sock_post_connect(MPIDU_Sock_set_t set, void * user_ptr, char * host_description, int port, MPIDU_Sock_t * sock)
{
    int mpi_errno;
    struct hostent *lphost;
    struct sockaddr_in sockAddr;
    sock_state_t *connect_state;
    u_long optval;
    char host[100];
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_POST_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_POST_CONNECT);
    if (!g_init_called)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**sock_init", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CONNECT);
	return mpi_errno;
    }
    if (strlen(host_description) > SOCKI_DESCRIPTION_LENGTH)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM, "**nomem", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CONNECT);
	return mpi_errno;
    }

    memset(&sockAddr,0,sizeof(sockAddr));

    /* setup the structures */
    connect_state = (sock_state_t*)BlockAlloc(g_StateAllocator);
    init_state_struct(connect_state);
    connect_state->cur_host = connect_state->host_description;
    MPIU_Strncpy(connect_state->host_description, host_description, SOCKI_DESCRIPTION_LENGTH);

    mpi_errno = MPIU_Str_get_string(&connect_state->cur_host, host, 100);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	if (mpi_errno == MPIU_STR_NOMEM)
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_NOMEM, "**nomem", 0);
	else
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %d", mpi_errno);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CONNECT);
	return mpi_errno;
    }

    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = inet_addr(host);
    
    if (sockAddr.sin_addr.s_addr == INADDR_NONE || sockAddr.sin_addr.s_addr == 0)
    {
	lphost = gethostbyname(host);
	if (lphost != NULL)
	    sockAddr.sin_addr.s_addr = ((struct in_addr *)lphost->h_addr)->s_addr;
	else
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**gethostbyname", "**gethostbyname %d", WSAGetLastError());
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CONNECT);
	    return mpi_errno;
	}
    }
    sockAddr.sin_port = htons((u_short)port);

    /* create a socket */
    mpi_errno = easy_create(&connect_state->sock, ADDR_ANY, INADDR_ANY);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**sock_create", "**sock_create %d", WSAGetLastError());
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CONNECT);
	return mpi_errno;
    }

    /* connect */
    if (connect(connect_state->sock, (SOCKADDR*)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**sock_connect", "**sock_connect %d", WSAGetLastError());
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CONNECT);
	return mpi_errno;
    }

    /* set the socket to non-blocking */
    optval = TRUE;
    ioctlsocket(connect_state->sock, FIONBIO, &optval);

    connect_state->user_ptr = user_ptr;
    connect_state->type = SOCKI_SOCKET;
    connect_state->state = SOCKI_CONNECTING;
    connect_state->set = set;

    /* associate the socket with the completion port */
    if (CreateIoCompletionPort((HANDLE)connect_state->sock, set, (ULONG_PTR)connect_state, g_num_cp_threads) == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**iocp", "**iocp %d", GetLastError());
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CONNECT);
	return mpi_errno;
    }

    connect_state->pending_operations++;

    /* post a completion event so the sock_post_connect can be notified through sock_wait */
    PostQueuedCompletionStatus(set, 0, (ULONG_PTR)connect_state, &connect_state->write.ovl);

    *sock = connect_state;

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_CONNECT);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_set_user_ptr
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDU_Sock_set_user_ptr(MPIDU_Sock_t sock, void * user_ptr)
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_SET_USER_PTR);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_SET_USER_PTR);
    if (!g_init_called)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**sock_init", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_SET_USER_PTR);
	return mpi_errno;
    }
    if (sock == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK, "**bad_sock", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_SET_USER_PTR);
	return mpi_errno;
    }
    sock->user_ptr = user_ptr;
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_SET_USER_PTR);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_post_close
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDU_Sock_post_close(MPIDU_Sock_t sock)
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_SOCK_POST_CLOSE);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCK_POST_CLOSE);
    if (!g_init_called)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**sock_init", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_CLOSE);
	return mpi_errno;
    }

    if (sock == NULL || sock->sock == INVALID_SOCKET)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_BAD_SOCK, "**bad_sock", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_CLOSE);
	return mpi_errno;
    }

    if (sock->closing)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**pctwice", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCK_POST_CLOSE);
	return mpi_errno;
    }

    if (sock->pending_operations != 0)
    {
#ifdef MPICH_DBG_OUTPUT
	if (sock->state & SOCKI_CONNECTING)
	    MPIU_DBG_PRINTF(("sock_post_close(%d) called while sock is connecting.\n", MPIDU_Sock_get_sock_id(sock)));
	if (sock->state & SOCKI_READING)
	{
	    if (sock->read.use_iov)
	    {
		int i, n = 0;
		for (i=0; i<sock->read.iovlen; i++)
		    n += sock->read.iov[i].MPID_IOV_LEN;
		MPIU_DBG_PRINTF(("sock_post_close(%d) called while sock is reading: %d bytes out of %d, index %d, iovlen %d.\n",
		    MPIDU_Sock_get_sock_id(sock), sock->read.total, n, sock->read.index, sock->read.iovlen));
	    }
	    else
	    {
		MPIU_DBG_PRINTF(("sock_post_close(%d) called while sock is reading: %d bytes out of %d.\n",
		    MPIDU_Sock_get_sock_id(sock), sock->read.total, sock->read.bufflen));
	    }
	}
	if (sock->state & SOCKI_WRITING)
	{
	    if (sock->write.use_iov)
	    {
		int i, n = 0;
		for (i=0; i<sock->write.iovlen; i++)
		    n += sock->write.iov[i].MPID_IOV_LEN;
		MPIU_DBG_PRINTF(("sock_post_close(%d) called while sock is writing: %d bytes out of %d, index %d, iovlen %d.\n",
		    MPIDU_Sock_get_sock_id(sock), sock->write.total, n, sock->write.index, sock->write.iovlen));
	    }
	    else
	    {
		MPIU_DBG_PRINTF(("sock_post_close(%d) called while sock is writing: %d bytes out of %d.\n",
		    MPIDU_Sock_get_sock_id(sock), sock->write.total, sock->write.bufflen));
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
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_post_read
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDU_Sock_post_read(MPIDU_Sock_t sock, void * buf, MPIU_Size_t minbr, MPIU_Size_t maxbr,
                         MPIDU_Sock_progress_update_func_t fn)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_POST_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_POST_READ);
    if (!g_init_called)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**sock_init", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_READ);
	return mpi_errno;
    }
    sock->read.total = 0;
    sock->read.buffer = buf;
    sock->read.bufflen = minbr;
    sock->read.maxbufflen = maxbr;
    sock->read.use_iov = FALSE;
    sock->read.progress_update = fn;
    sock->state |= SOCKI_READING;
    sock->pending_operations++;
    MPIU_DBG_PRINTF(("sock_post_read - %d,%d bytes\n", minbr, maxbr));
    if (!ReadFile((HANDLE)(sock->sock), buf, (DWORD)minbr, &sock->read.num_bytes, &sock->read.ovl))
    {
	mpi_errno = GetLastError();
	switch (mpi_errno)
	{
        case ERROR_IO_PENDING:
	    mpi_errno = MPI_SUCCESS;
	    break;
	case ERROR_HANDLE_EOF:
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_CONN_CLOSED, "**sock_closed", 0);
	    break;
	default:
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %d", mpi_errno);
	    break;
	}
    }
    if (mpi_errno != MPI_SUCCESS)
	sock->state ^= SOCKI_READING;
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_READ);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_post_readv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDU_Sock_post_readv(MPIDU_Sock_t sock, MPID_IOV * iov, int iov_n, MPIDU_Sock_progress_update_func_t fn)
{
    int iter;
    int mpi_errno = MPI_SUCCESS;
#ifdef MPICH_DBG_OUTPUT
    int i;
#endif
    DWORD flags = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_POST_READV);
#ifdef USE_SOCK_IOV_COPY
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);
#endif

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_POST_READV);
    if (!g_init_called)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**sock_init", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_READV);
	return mpi_errno;
    }
    sock->read.total = 0;
#ifdef USE_SOCK_IOV_COPY
    MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
    memcpy(sock->read.iov, iov, sizeof(MPID_IOV) * n);
    MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
#else
    sock->read.iov = iov;
#endif
    sock->read.iovlen = iov_n;
    sock->read.index = 0;
    sock->read.use_iov = TRUE;
    sock->read.progress_update = fn;
    sock->state |= SOCKI_READING;
    sock->pending_operations++;
#ifdef MPICH_DBG_OUTPUT
    for (i=0; i<iov_n; i++)
    {
	MPIU_DBG_PRINTF(("sock_post_readv - iov[%d].len = %d\n", i, iov[i].MPID_IOV_LEN));
    }
#endif
    for (iter=0; iter<10; iter++)
    {
	if (WSARecv(sock->sock, sock->read.iov, iov_n, &sock->read.num_bytes, &flags, &sock->read.ovl, NULL) != SOCKET_ERROR)
	    break;

	mpi_errno = WSAGetLastError();
	if (mpi_errno == WSA_IO_PENDING)
	{
	    mpi_errno = MPI_SUCCESS;
	    break;
	}
	if (mpi_errno != WSAEWOULDBLOCK)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %d", mpi_errno);
	    break;
	}
	Sleep(200);
    }
    if (mpi_errno != MPI_SUCCESS)
	sock->state ^= SOCKI_READING;
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_READV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_post_write
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDU_Sock_post_write(MPIDU_Sock_t sock, void * buf, MPIU_Size_t min, MPIU_Size_t max, MPIDU_Sock_progress_update_func_t fn)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_POST_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_POST_WRITE);
    if (!g_init_called)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_INIT, "**sock_init", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_WRITE);
	return mpi_errno;
    }
    sock->write.total = 0;
    sock->write.buffer = buf;
    sock->write.bufflen = min;
    sock->write.maxbufflen = max;
    sock->write.use_iov = FALSE;
    sock->write.progress_update = fn;
    sock->state |= SOCKI_WRITING;
    sock->pending_operations++;
    MPIU_DBG_PRINTF(("sock_post_write - %d,%d bytes\n", min, max));
    if (!WriteFile((HANDLE)(sock->sock), buf, (DWORD)min, &sock->write.num_bytes, &sock->write.ovl))
    {
	mpi_errno = GetLastError();
	switch (mpi_errno)
	{
        case ERROR_IO_PENDING:
	    mpi_errno = MPI_SUCCESS;
	    break;
	case ERROR_HANDLE_EOF:
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_CONN_CLOSED, "**sock_closed", 0);
	    break;
	default:
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %d", mpi_errno);
	    break;
	}
    }
    if (mpi_errno != MPI_SUCCESS)
	sock->state ^= SOCKI_WRITING;
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_WRITE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_post_writev
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDU_Sock_post_writev(MPIDU_Sock_t sock, MPID_IOV * iov, int iov_n, MPIDU_Sock_progress_update_func_t fn)
{
    int mpi_errno = MPI_SUCCESS;
    int iter;
#ifdef MPICH_DBG_OUTPUT
    int i;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_POST_WRITEV);
#ifdef USE_SOCK_IOV_COPY
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);
#endif

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_POST_WRITEV);
    sock->write.total = 0;
#ifdef USE_SOCK_IOV_COPY
    MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
    memcpy(sock->write.iov, iov, sizeof(MPID_IOV) * iov_n);
    MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
#else
    sock->write.iov = iov;
#endif
    sock->write.iovlen = iov_n;
    sock->write.index = 0;
    sock->write.use_iov = TRUE;
    sock->write.progress_update = fn;
    sock->state |= SOCKI_WRITING;
    sock->pending_operations++;
#ifdef MPICH_DBG_OUTPUT
    for (i=0; i<iov_n; i++)
    {
	MPIU_DBG_PRINTF(("sock_post_writev - iov[%d].len = %d\n", i, iov[i].MPID_IOV_LEN));
    }
#endif
    for (iter=0; iter<10; iter++)
    {
	if (WSASend(sock->sock, sock->write.iov, iov_n, &sock->write.num_bytes, 0, &sock->write.ovl, NULL) != SOCKET_ERROR)
	    break;

	mpi_errno = WSAGetLastError();
	if (mpi_errno == WSA_IO_PENDING)
	{
	    mpi_errno = MPI_SUCCESS;
	    break;
	}
	if (mpi_errno != WSAEWOULDBLOCK)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %d", mpi_errno);
	    break;
	}
	Sleep(200);
    }
    if (mpi_errno != MPI_SUCCESS)
	sock->state ^= SOCKI_WRITING;
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_POST_WRITEV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_wait
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDU_Sock_wait(MPIDU_Sock_set_t set, int timeout, MPIDU_Sock_event_t * out)
{
    int mpi_errno;
    DWORD num_bytes;
    sock_state_t *sock;
    OVERLAPPED *ovl;
    DWORD dwFlags = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_WAIT);
    MPIDI_STATE_DECL(MPID_STATE_GETQUEUEDCOMPLETIONSTATUS);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_WAIT);

    for (;;) 
    {
	MPIDI_FUNC_ENTER(MPID_STATE_GETQUEUEDCOMPLETIONSTATUS);
	/* initialize to NULL so we can compare the output of GetQueuedCompletionStatus */
	sock = NULL;
	ovl = NULL;
	num_bytes = 0;
	if (GetQueuedCompletionStatus(set, &num_bytes, (DWORD*)&sock, &ovl, timeout))
	{
	    MPIDI_FUNC_EXIT(MPID_STATE_GETQUEUEDCOMPLETIONSTATUS);
	    if (sock->type == SOCKI_SOCKET)
	    {
		if (sock->closing && sock->pending_operations == 0)
		{
		    out->op_type = MPIDU_SOCK_OP_CLOSE;
		    out->num_bytes = 0;
		    out->error = MPI_SUCCESS;
		    out->user_ptr = sock->user_ptr;
		    CloseHandle(sock->read.ovl.hEvent);
		    CloseHandle(sock->write.ovl.hEvent);
		    sock->read.ovl.hEvent = NULL;
		    sock->write.ovl.hEvent = NULL;
#if 0
		    BlockFree(g_StateAllocator, sock); /* will this cause future io completion port errors since sock is the iocp user pointer? */
#endif
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
		    return MPI_SUCCESS;
		}
		if (ovl == &sock->read.ovl)
		{
		    if (num_bytes == 0)
		    {
			/* socket closed */
			MPIU_DBG_PRINTF(("sock_wait readv returning %d bytes and EOF\n", sock->read.total));
			out->op_type = MPIDU_SOCK_OP_READ;
			out->num_bytes = sock->read.total;
			out->error = MPIDU_SOCK_ERR_CONN_CLOSED;
			out->user_ptr = sock->user_ptr;
			sock->pending_operations--;
			sock->state ^= SOCKI_READING; /* remove the SOCKI_READING bit */
			if (sock->closing && sock->pending_operations == 0)
			{
			    MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after iov read completed.\n", MPIDU_Sock_get_sock_id(sock)));
			    FlushFileBuffers((HANDLE)sock->sock);
			    shutdown(sock->sock, SD_BOTH);
			    closesocket(sock->sock);
			    sock->sock = INVALID_SOCKET;
			}
			MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
			return MPI_SUCCESS;
		    }
		    MPIU_DBG_PRINTF(("sock_wait read%s update: %d bytes\n", sock->read.use_iov ? "v" : "", num_bytes));
		    sock->read.total += num_bytes;
		    if (sock->read.use_iov)
		    {
			while (num_bytes)
			{
			    if (sock->read.iov[sock->read.index].MPID_IOV_LEN <= num_bytes)
			    {
				num_bytes -= sock->read.iov[sock->read.index].MPID_IOV_LEN;
				sock->read.index++;
				sock->read.iovlen--;
			    }
			    else
			    {
				sock->read.iov[sock->read.index].MPID_IOV_LEN -= num_bytes;
				sock->read.iov[sock->read.index].MPID_IOV_BUF = 
				    (char*)(sock->read.iov[sock->read.index].MPID_IOV_BUF) + num_bytes;
				num_bytes = 0;
			    }
			}
			if (sock->read.iovlen == 0)
			{
			    MPIU_DBG_PRINTF(("sock_wait readv %d bytes\n", sock->read.total));
			    out->op_type = MPIDU_SOCK_OP_READ;
			    out->num_bytes = sock->read.total;
			    out->error = MPI_SUCCESS;
			    out->user_ptr = sock->user_ptr;
			    sock->pending_operations--;
			    sock->state ^= SOCKI_READING; /* remove the SOCKI_READING bit */
			    if (sock->closing && sock->pending_operations == 0)
			    {
				MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after iov read completed.\n", MPIDU_Sock_get_sock_id(sock)));
				FlushFileBuffers((HANDLE)sock->sock);
				shutdown(sock->sock, SD_BOTH);
				closesocket(sock->sock);
				sock->sock = INVALID_SOCKET;
			    }
			    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
			    return MPI_SUCCESS;
			}
			/* make the user upcall */
			if (sock->read.progress_update != NULL)
			    sock->read.progress_update(num_bytes, sock->user_ptr);
			/* post a read of the remaining data */
			/*WSARecv(sock->sock, sock->read.iov, sock->read.iovlen, &sock->read.num_bytes, &dwFlags, &sock->read.ovl, NULL);*/
			if (WSARecv(sock->sock, &sock->read.iov[sock->read.index], sock->read.iovlen, &sock->read.num_bytes, &dwFlags, &sock->read.ovl, NULL) == SOCKET_ERROR)
			{
			    mpi_errno = WSAGetLastError();
			    if (mpi_errno == 0)
			    {
				out->op_type = MPIDU_SOCK_OP_READ;
				out->num_bytes = sock->read.total;
				out->error = MPIDU_SOCK_ERR_CONN_CLOSED;
				out->user_ptr = sock->user_ptr;
				sock->pending_operations--;
				sock->state ^= SOCKI_READING;
				if (sock->closing && sock->pending_operations == 0)
				{
				    MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after iov read completed.\n", MPIDU_Sock_get_sock_id(sock)));
				    FlushFileBuffers((HANDLE)sock->sock);
				    shutdown(sock->sock, SD_BOTH);
				    closesocket(sock->sock);
				    sock->sock = INVALID_SOCKET;
				}
				MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
				return MPI_SUCCESS;
			    }
			    if (mpi_errno != WSA_IO_PENDING)
			    {
				out->op_type = MPIDU_SOCK_OP_READ;
				out->num_bytes = sock->read.total;
				out->error = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %d", mpi_errno);
				out->user_ptr = sock->user_ptr;
				sock->pending_operations--;
				sock->state ^= SOCKI_READING;
				if (sock->closing && sock->pending_operations == 0)
				{
				    MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after iov read completed.\n", MPIDU_Sock_get_sock_id(sock)));
				    FlushFileBuffers((HANDLE)sock->sock);
				    shutdown(sock->sock, SD_BOTH);
				    closesocket(sock->sock);
				    sock->sock = INVALID_SOCKET;
				}
				MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
				return MPI_SUCCESS;
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
			    out->op_type = MPIDU_SOCK_OP_READ;
			    out->num_bytes = sock->read.total;
			    out->error = MPI_SUCCESS;
			    out->user_ptr = sock->user_ptr;
			    sock->pending_operations--;
			    sock->state ^= SOCKI_READING; /* remove the SOCKI_READING bit */
			    if (sock->closing && sock->pending_operations == 0)
			    {
				MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after simple read completed.\n", MPIDU_Sock_get_sock_id(sock)));
				FlushFileBuffers((HANDLE)sock->sock);
				shutdown(sock->sock, SD_BOTH);
				closesocket(sock->sock);
				sock->sock = INVALID_SOCKET;
			    }
			    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
			    return MPI_SUCCESS;
			}
			/* make the user upcall */
			if (sock->read.progress_update != NULL)
			    sock->read.progress_update(num_bytes, sock->user_ptr);
			/* post a read of the remaining data */
			if (!ReadFile((HANDLE)(sock->sock), sock->read.buffer, (DWORD)sock->read.bufflen, &sock->read.num_bytes, &sock->read.ovl))
			{
			    mpi_errno = GetLastError();
			    MPIU_DBG_PRINTF(("GetLastError: %d\n", mpi_errno));
			    if (mpi_errno == ERROR_HANDLE_EOF || mpi_errno == 0)
			    {
				out->op_type = MPIDU_SOCK_OP_READ;
				out->num_bytes = sock->read.total;
				out->error = MPIDU_SOCK_ERR_CONN_CLOSED;
				out->user_ptr = sock->user_ptr;
				sock->pending_operations--;
				sock->state ^= SOCKI_READING;
				if (sock->closing && sock->pending_operations == 0)
				{
				    MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after iov read completed.\n", MPIDU_Sock_get_sock_id(sock)));
				    FlushFileBuffers((HANDLE)sock->sock);
				    shutdown(sock->sock, SD_BOTH);
				    closesocket(sock->sock);
				    sock->sock = INVALID_SOCKET;
				}
				MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
				return MPI_SUCCESS;
			    }
			    if (mpi_errno != ERROR_IO_PENDING)
			    {
				out->op_type = MPIDU_SOCK_OP_READ;
				out->num_bytes = sock->read.total;
				out->error = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %d", mpi_errno);
				out->user_ptr = sock->user_ptr;
				sock->pending_operations--;
				sock->state ^= SOCKI_READING;
				if (sock->closing && sock->pending_operations == 0)
				{
				    MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after iov read completed.\n", MPIDU_Sock_get_sock_id(sock)));
				    FlushFileBuffers((HANDLE)sock->sock);
				    shutdown(sock->sock, SD_BOTH);
				    closesocket(sock->sock);
				    sock->sock = INVALID_SOCKET;
				}
				MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
				return MPI_SUCCESS;
			    }
			}
		    }
		}
		else if (ovl == &sock->write.ovl)
		{
		    if (sock->state & SOCKI_CONNECTING)
		    {
			/* insert code here to determine that the connect succeeded */
			/* ... */
			out->op_type = MPIDU_SOCK_OP_CONNECT;
			out->num_bytes = 0;
			out->error = MPI_SUCCESS;
			out->user_ptr = sock->user_ptr;
			sock->pending_operations--;
			sock->state ^= SOCKI_CONNECTING; /* remove the SOCKI_CONNECTING bit */
			if (sock->closing && sock->pending_operations == 0)
			{
			    MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after connect completed.\n", MPIDU_Sock_get_sock_id(sock)));
			    FlushFileBuffers((HANDLE)sock->sock);
			    shutdown(sock->sock, SD_BOTH);
			    closesocket(sock->sock);
			    sock->sock = INVALID_SOCKET;
			}
			MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
			return MPI_SUCCESS;
		    }
		    else
		    {
			if (num_bytes == 0)
			{
			    /* socket closed */
			    MPIU_DBG_PRINTF(("sock_wait wreadv returning %d bytes and EOF\n", sock->write.total));
			    out->op_type = MPIDU_SOCK_OP_WRITE;
			    out->num_bytes = sock->write.total;
			    out->error = MPIDU_SOCK_ERR_CONN_CLOSED;
			    out->user_ptr = sock->user_ptr;
			    sock->pending_operations--;
			    sock->state ^= SOCKI_WRITING; /* remove the SOCKI_WRITING bit */
			    if (sock->closing && sock->pending_operations == 0)
			    {
				MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after iov write completed.\n", MPIDU_Sock_get_sock_id(sock)));
				FlushFileBuffers((HANDLE)sock->sock);
				shutdown(sock->sock, SD_BOTH);
				closesocket(sock->sock);
				sock->sock = INVALID_SOCKET;
			    }
			    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
			    return MPI_SUCCESS;
			}
			MPIU_DBG_PRINTF(("sock_wait: write update, total = %d + %d = %d\n", sock->write.total, num_bytes, sock->write.total + num_bytes));
			sock->write.total += num_bytes;
			if (sock->write.use_iov)
			{
			    while (num_bytes)
			    {
				if (sock->write.iov[sock->write.index].MPID_IOV_LEN <= num_bytes)
				{
				    /*MPIU_DBG_PRINTF(("sock_wait: write.index %d, len %d\n", sock->write.index, 
					sock->write.iov[sock->write.index].MPID_IOV_LEN));*/
				    num_bytes -= sock->write.iov[sock->write.index].MPID_IOV_LEN;
				    sock->write.index++;
				    sock->write.iovlen--;
				}
				else
				{
				    /*MPIU_DBG_PRINTF(("sock_wait: partial data written [%d].len = %d, num_bytes = %d\n", sock->write.index,
					sock->write.iov[sock->write.index].MPID_IOV_LEN, num_bytes));*/
				    sock->write.iov[sock->write.index].MPID_IOV_LEN -= num_bytes;
				    sock->write.iov[sock->write.index].MPID_IOV_BUF =
					(char*)(sock->write.iov[sock->write.index].MPID_IOV_BUF) + num_bytes;
				    num_bytes = 0;
				}
			    }
			    if (sock->write.iovlen == 0)
			    {
				if (sock->write.total > 0)
				{
				    MPIU_DBG_PRINTF(("sock_wait wrotev %d bytes\n", sock->write.total));
				}
				out->op_type = MPIDU_SOCK_OP_WRITE;
				out->num_bytes = sock->write.total;
				out->error = MPI_SUCCESS;
				out->user_ptr = sock->user_ptr;
				sock->pending_operations--;
				sock->state ^= SOCKI_WRITING; /* remove the SOCKI_WRITING bit */
				if (sock->closing && sock->pending_operations == 0)
				{
				    MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after iov write completed.\n", MPIDU_Sock_get_sock_id(sock)));
				    FlushFileBuffers((HANDLE)sock->sock);
				    shutdown(sock->sock, SD_BOTH);
				    closesocket(sock->sock);
				    sock->sock = INVALID_SOCKET;
				}
				MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
				return MPI_SUCCESS;
			    }
			    /* make the user upcall */
			    if (sock->write.progress_update != NULL)
				sock->write.progress_update(num_bytes, sock->user_ptr);
			    /* post a write of the remaining data */
			    MPIU_DBG_PRINTF(("sock_wait: posting write of the remaining data, vec size %d\n", sock->write.iovlen));
			    if (WSASend(sock->sock, sock->write.iov, sock->write.iovlen, &sock->write.num_bytes, 0, &sock->write.ovl, NULL) == SOCKET_ERROR)
			    {
				mpi_errno = WSAGetLastError();
				if (mpi_errno == 0)
				{
				    out->op_type = MPIDU_SOCK_OP_WRITE;
				    out->num_bytes = sock->write.total;
				    out->error = MPIDU_SOCK_ERR_CONN_CLOSED;
				    out->user_ptr = sock->user_ptr;
				    sock->pending_operations--;
				    sock->state ^= SOCKI_WRITING;
				    if (sock->closing && sock->pending_operations == 0)
				    {
					MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after iov read completed.\n", MPIDU_Sock_get_sock_id(sock)));
					FlushFileBuffers((HANDLE)sock->sock);
					shutdown(sock->sock, SD_BOTH);
					closesocket(sock->sock);
					sock->sock = INVALID_SOCKET;
				    }
				    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
				    return MPI_SUCCESS;
				}
				if (mpi_errno != WSA_IO_PENDING)
				{
				    out->op_type = MPIDU_SOCK_OP_WRITE;
				    out->num_bytes = sock->write.total;
				    out->error = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %d", mpi_errno);
				    out->user_ptr = sock->user_ptr;
				    sock->pending_operations--;
				    sock->state ^= SOCKI_WRITING;
				    if (sock->closing && sock->pending_operations == 0)
				    {
					MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after iov read completed.\n", MPIDU_Sock_get_sock_id(sock)));
					FlushFileBuffers((HANDLE)sock->sock);
					shutdown(sock->sock, SD_BOTH);
					closesocket(sock->sock);
					sock->sock = INVALID_SOCKET;
				    }
				    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
				    return MPI_SUCCESS;
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
				out->op_type = MPIDU_SOCK_OP_WRITE;
				out->num_bytes = sock->write.total;
				out->error = MPI_SUCCESS;
				out->user_ptr = sock->user_ptr;
				sock->pending_operations--;
				sock->state ^= SOCKI_WRITING; /* remove the SOCKI_WRITING bit */
				if (sock->closing && sock->pending_operations == 0)
				{
				    MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after simple write completed.\n", MPIDU_Sock_get_sock_id(sock)));
				    FlushFileBuffers((HANDLE)sock->sock);
				    shutdown(sock->sock, SD_BOTH);
				    closesocket(sock->sock);
				    sock->sock = INVALID_SOCKET;
				}
				MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
				return MPI_SUCCESS;
			    }
			    /* make the user upcall */
			    if (sock->write.progress_update != NULL)
				sock->write.progress_update(num_bytes, sock->user_ptr);
			    /* post a write of the remaining data */
			    if (!WriteFile((HANDLE)(sock->sock), sock->write.buffer, (DWORD)sock->write.bufflen, &sock->write.num_bytes, &sock->write.ovl))
			    {
				mpi_errno = GetLastError();
				MPIU_DBG_PRINTF(("GetLastError: %d\n", mpi_errno));
				if (mpi_errno == ERROR_HANDLE_EOF || mpi_errno == 0)
				{
				    out->op_type = MPIDU_SOCK_OP_WRITE;
				    out->num_bytes = sock->write.total;
				    out->error = MPIDU_SOCK_ERR_CONN_CLOSED;
				    out->user_ptr = sock->user_ptr;
				    sock->pending_operations--;
				    sock->state ^= SOCKI_WRITING;
				    if (sock->closing && sock->pending_operations == 0)
				    {
					MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after iov read completed.\n", MPIDU_Sock_get_sock_id(sock)));
					FlushFileBuffers((HANDLE)sock->sock);
					shutdown(sock->sock, SD_BOTH);
					closesocket(sock->sock);
					sock->sock = INVALID_SOCKET;
				    }
				    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
				    return MPI_SUCCESS;
				}
				if (mpi_errno != ERROR_IO_PENDING)
				{
				    out->op_type = MPIDU_SOCK_OP_WRITE;
				    out->num_bytes = sock->write.total;
				    out->error = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %d", mpi_errno);
				    out->user_ptr = sock->user_ptr;
				    sock->pending_operations--;
				    sock->state ^= SOCKI_WRITING;
				    if (sock->closing && sock->pending_operations == 0)
				    {
					MPIU_DBG_PRINTF(("sock_wait: closing socket(%d) after iov read completed.\n", MPIDU_Sock_get_sock_id(sock)));
					FlushFileBuffers((HANDLE)sock->sock);
					shutdown(sock->sock, SD_BOTH);
					closesocket(sock->sock);
					sock->sock = INVALID_SOCKET;
				    }
				    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
				    return MPI_SUCCESS;
				}
			    }
			}
		    }
		}
		else
		{
		    if (num_bytes == 0)
		    {
			if ((sock->state & SOCKI_READING))/* && sock-sock != INVALID_SOCKET) */
			{
			    MPIU_DBG_PRINTF(("EOF with posted read on sock %d\n", sock->sock));
			    out->op_type = MPIDU_SOCK_OP_READ;
			    out->num_bytes = sock->read.total;
			    out->error = MPIDU_SOCK_ERR_CONN_CLOSED;
			    out->user_ptr = sock->user_ptr;
			    sock->state ^= SOCKI_READING;
			    sock->pending_operations--;
			    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
			    return MPI_SUCCESS;
			}
			if ((sock->state & SOCKI_WRITING))/* && sock->sock != INVALID_SOCKET) */
			{
			    MPIU_DBG_PRINTF(("EOF with posted write on sock %d\n", sock->sock));
			    out->op_type = MPIDU_SOCK_OP_WRITE;
			    out->num_bytes = sock->write.total;
			    out->error = MPIDU_SOCK_ERR_CONN_CLOSED;
			    out->user_ptr = sock->user_ptr;
			    sock->state ^= SOCKI_WRITING;
			    sock->pending_operations--;
			    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
			    return MPI_SUCCESS;
			}
			/*
			if (sock->state & SOCKI_CLOSING)
			{
			    out->op_type = MPIDU_SOCK_OP_CLOSE;
			    out->num_bytes = 0;
			    out->error = MPI_SUCCESS;
			    out->user_ptr = sock->user_ptr;
			    sock->state ^= SOCKI_CLOSING;
			    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
			    return MPI_SUCCESS;
			}
			*/
			MPIU_DBG_PRINTF(("ignoring EOF notification on unknown sock %d.\n", MPIDU_Sock_get_sock_id(sock)));
		    }

		    if (sock->sock != INVALID_SOCKET)
		    {
			MPIU_DBG_PRINTF(("unmatched ovl: pending: %d, state = %d\n", sock->pending_operations, sock->state));
			MPIU_Error_printf("In sock_wait(), returned overlapped structure does not match the current read or write ovl: 0x%x\n", ovl);
			MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
			return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", 0);
		    }
		    else
		    {
			MPIU_DBG_PRINTF(("ignoring notification on invalid sock.\n"));
		    }
		}
	    }
	    else if (sock->type == SOCKI_LISTENER)
	    {
		if (sock->closing && sock->pending_operations == 0)
		{
		    out->op_type = MPIDU_SOCK_OP_CLOSE;
		    out->num_bytes = 0;
		    out->error = MPI_SUCCESS;
		    out->user_ptr = sock->user_ptr;
		    CloseHandle(sock->read.ovl.hEvent);
		    CloseHandle(sock->write.ovl.hEvent);
		    sock->read.ovl.hEvent = NULL;
		    sock->write.ovl.hEvent = NULL;
#if 0
		    BlockFree(g_StateAllocator, sock); /* will this cause future io completion port errors since sock is the iocp user pointer? */
#endif
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
		    return MPI_SUCCESS;
		}
		sock->state |= SOCKI_ACCEPTED;
		out->op_type = MPIDU_SOCK_OP_ACCEPT;
		out->num_bytes = num_bytes;
		out->error = MPI_SUCCESS;
		out->user_ptr = sock->user_ptr;
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
		return MPI_SUCCESS;
	    }
	    else
	    {
		MPIU_Error_printf("sock type is not a SOCKET or a LISTENER, it's %d\n", sock->type);
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
		return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", 0);
	    }
	}
	else
	{
	    /*MPIDI_FUNC_EXIT(MPID_STATE_GETQUEUEDCOMPLETIONSTATUS);*/ /* Maybe the logging will reset the last error? */
	    mpi_errno = GetLastError();
	    MPIU_DBG_PRINTF(("GetQueuedCompletionStatus failed, GetLastError: %d\n", mpi_errno));
	    MPIDI_FUNC_EXIT(MPID_STATE_GETQUEUEDCOMPLETIONSTATUS);
	    /* interpret error, return appropriate SOCK_ERR_... macro */
	    if (mpi_errno == WAIT_TIMEOUT)
	    {
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
		return MPIDU_SOCK_ERR_TIMEOUT;
	    }
	    if (sock != NULL)
	    {
		if (sock->type == SOCKI_SOCKET)
		{
		    if (ovl == &sock->read.ovl)
			out->op_type = MPIDU_SOCK_OP_READ;
		    else if (ovl == &sock->write.ovl)
			out->op_type = MPIDU_SOCK_OP_WRITE;
		    else
			out->op_type = -1;
		    out->num_bytes = 0;
		    out->error = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %d", mpi_errno);
		    out->user_ptr = sock->user_ptr;
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
		    return MPI_SUCCESS;
		}
		if (sock->type == SOCKI_LISTENER)
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
			out->op_type = MPIDU_SOCK_OP_CLOSE;
			out->num_bytes = 0;
			if (mpi_errno == ERROR_OPERATION_ABORTED)
			    out->error = MPI_SUCCESS;
			else
			    out->error = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %d", mpi_errno);
			out->user_ptr = sock->user_ptr;
			MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
			return MPI_SUCCESS;
#endif
		    }
		    else
		    {
			/* Should we return a SOCK_OP_ACCEPT with an error if there is a failure on the listener? */
			out->op_type = MPIDU_SOCK_OP_ACCEPT;
			out->num_bytes = 0;
			out->error = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %d", mpi_errno);
			out->user_ptr = sock->user_ptr;
			MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
			return MPI_SUCCESS;
		    }
		}
	    }

	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
	    return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %d", mpi_errno);
	}
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WAIT);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_wakeup
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDU_Sock_wakeup(MPIDU_Sock_set_t set)
{
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_read
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDU_Sock_read(MPIDU_Sock_t sock, void * buf, MPIU_Size_t len, MPIU_Size_t * num_read)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_READ);
    mpi_errno = recv(sock->sock, buf, (int)len, 0);
    if (mpi_errno == SOCKET_ERROR)
    {
	mpi_errno = WSAGetLastError();
	if (mpi_errno == WSAEWOULDBLOCK)
	{
	    *num_read = 0;
	    mpi_errno = MPI_SUCCESS;
	}
	else
	{
	    MPIU_DBG_PRINTF(("sock_read error %d\n", mpi_errno));
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %d", mpi_errno);
	}
    }
    else
    {
	*num_read = mpi_errno;
	if (mpi_errno == 0)
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_CONN_CLOSED, "**sock_closed", 0);
	else
	    mpi_errno = MPI_SUCCESS;
    }
#ifdef MPICH_DBG_OUTPUT
    if (mpi_errno == MPI_SUCCESS)
    {
	MPIU_DBG_PRINTF(("sock_read %d of %d bytes\n", *num_read, len));
    }
#endif
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_READ);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_readv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDU_Sock_readv(MPIDU_Sock_t sock, MPID_IOV * iov, int iov_n, MPIU_Size_t * num_read)
{
    int mpi_errno = MPI_SUCCESS;
    DWORD nFlags = 0;
    DWORD num_read_local;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_READV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_READV);

    if (WSARecv(sock->sock, iov, iov_n, &num_read_local, &nFlags, NULL/*overlapped*/, NULL/*completion routine*/) == SOCKET_ERROR)
    {
	mpi_errno = WSAGetLastError();
	*num_read = 0;
	if (mpi_errno == WSAEWOULDBLOCK)
	    mpi_errno = MPI_SUCCESS;
	else
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %d", mpi_errno);
    }
    else
    {
	*num_read = num_read_local;
    }
    MPIU_DBG_PRINTF(("sock_readv %d bytes\n", *num_read));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_READV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_write
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDU_Sock_write(MPIDU_Sock_t sock, void * buf, MPIU_Size_t len, MPIU_Size_t * num_written)
{
    int mpi_errno;
    int length, num_sent, total = 0;
#ifdef MPICH_DBG_OUTPUT
    MPIU_Size_t len_orig = len;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_WRITE);
    if ((int)len < g_stream_packet_length)
    {
	total = send(sock->sock, buf, (int)len, 0);
	if (total == SOCKET_ERROR)
	{
	    mpi_errno = WSAGetLastError();
	    if (mpi_errno == WSAEWOULDBLOCK)
		total = 0;
	    else
	    {
		MPIU_DBG_PRINTF(("single send failed: error %d\n", mpi_errno));
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %d", mpi_errno);
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WRITE);
		return mpi_errno;
	    }
	}
	*num_written = total;
	MPIU_DBG_PRINTF(("sock_write %d of %d bytes\n", total, len_orig));
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WRITE);
	return MPI_SUCCESS;
    }
    *num_written = 0;
    while (len)
    {
	length = min((int)len, g_stream_packet_length);
	num_sent = send(sock->sock, buf, length, 0);
	if (num_sent == SOCKET_ERROR)
	{
	    mpi_errno = WSAGetLastError();
	    if (mpi_errno == WSAEWOULDBLOCK)
		num_sent = 0;
	    else
	    {
		MPIU_DBG_PRINTF(("stream send failed: %d\n", mpi_errno));
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %d", mpi_errno);
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WRITE);
		return mpi_errno;
	    }
	}
	total += num_sent;
	len -= num_sent;
	buf = (char*)buf + num_sent;
	if (num_sent < length)
	{
	    *num_written = total;
	    MPIU_DBG_PRINTF(("sock_write %d of %d bytes\n", total, len_orig));
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WRITE);
	    return MPI_SUCCESS;
	}
    }
    *num_written = total;
    MPIU_DBG_PRINTF(("sock_write %d of %d bytes\n", total, len_orig));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WRITE);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_writev
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDU_Sock_writev(MPIDU_Sock_t sock, MPID_IOV * iov, int iov_n, MPIU_Size_t * num_written)
{
    int mpi_errno;
    int i;
    MPIU_Size_t total;
    MPIU_Size_t num_sent;
    DWORD num_written_local;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_WRITEV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_WRITEV);
    if (iov_n>1 && (int)iov[1].MPID_IOV_LEN > g_stream_packet_length)
    {
	total = 0;
	for (i=0; i<iov_n; i++)
	{
	    mpi_errno = MPIDU_Sock_write(sock, iov[i].MPID_IOV_BUF, iov[i].MPID_IOV_LEN, &num_sent);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		MPIU_DBG_PRINTF(("sock_write failed.\n"));
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %d", mpi_errno);
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WRITEV);
		return mpi_errno;
	    }
	    total += num_sent;
	    if (num_sent != iov[i].MPID_IOV_LEN)
	    {
		*num_written = total;
		MPIU_DBG_PRINTF(("sock_writev %d bytes\n", total));
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WRITEV);
		return MPI_SUCCESS;
	    }
	}
	*num_written = total;
    }
    else
    {
	if (WSASend(sock->sock, iov, iov_n, &num_written_local, 0, NULL/*overlapped*/, NULL/*completion routine*/) == SOCKET_ERROR)
	{
	    mpi_errno = WSAGetLastError();
	    if (mpi_errno != WSAEWOULDBLOCK)
	    {
		MPIU_DBG_PRINTF(("WSASend failed: error %d\n", mpi_errno));
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPIDU_SOCK_ERR_FAIL, "**fail", "**fail %d", mpi_errno);
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WRITEV);
		return mpi_errno;
	    }
	    *num_written = 0;
	}
	else
	{
	    *num_written = num_written_local;
	}
	MPIU_DBG_PRINTF(("sock_writev %d bytes\n", *num_written));
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_WRITEV);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_get_sock_id
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDU_Sock_get_sock_id(MPIDU_Sock_t sock)
{
    int ret_val;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_GET_SOCK_ID);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_GET_SOCK_ID);
    if (sock == MPIDU_SOCK_INVALID_SOCK)
	ret_val = -1;
    else
	ret_val = (int)sock->sock;
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_GET_SOCK_ID);
    return ret_val;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_get_sock_set_id
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDU_Sock_get_sock_set_id(MPIDU_Sock_set_t set)
{
    int ret_val;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_GET_SOCK_SET_ID);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_GET_SOCK_SET_ID);
    ret_val = (int)set;
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_GET_SOCK_SET_ID);
    return ret_val;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_get_error_class_string
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDU_Sock_get_error_class_string(int error, char *error_string, int length)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_GET_ERROR_CLASS_STRING);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_GET_ERROR_CLASS_STRING);
    switch (MPIR_ERR_GET_CLASS(error))
    {
    case MPIDU_SOCK_ERR_FAIL:
	MPIU_Strncpy(error_string, "generic socket failure", length);
	break;
    case MPIDU_SOCK_ERR_INIT:
	MPIU_Strncpy(error_string, "socket module not initialized", length);
	break;
    case MPIDU_SOCK_ERR_NOMEM:
	MPIU_Strncpy(error_string, "not enough memory to complete the socket operation", length);
	break;
    case MPIDU_SOCK_ERR_BAD_SET:
	MPIU_Strncpy(error_string, "invalid socket set", length);
	break;
    case MPIDU_SOCK_ERR_BAD_SOCK:
	MPIU_Strncpy(error_string, "invalid socket", length);
	break;
    case MPIDU_SOCK_ERR_BAD_HOST:
	MPIU_Strncpy(error_string, "host description buffer not large enough", length);
	break;
    case MPIDU_SOCK_ERR_BAD_HOSTNAME:
	MPIU_Strncpy(error_string, "invalid host name", length);
	break;
    case MPIDU_SOCK_ERR_BAD_PORT:
	MPIU_Strncpy(error_string, "invalid port", length);
	break;
    case MPIDU_SOCK_ERR_BAD_BUF:
	MPIU_Strncpy(error_string, "invalid buffer", length);
	break;
    case MPIDU_SOCK_ERR_BAD_LEN:
	MPIU_Strncpy(error_string, "invalid length", length);
	break;
    case MPIDU_SOCK_ERR_SOCK_CLOSED:
	MPIU_Strncpy(error_string, "socket closed", length);
	break;
    case MPIDU_SOCK_ERR_CONN_CLOSED:
	MPIU_Strncpy(error_string, "socket connection closed", length);
	break;
    case MPIDU_SOCK_ERR_CONN_FAILED:
	MPIU_Strncpy(error_string, "socket connection failed", length);
	break;
    case MPIDU_SOCK_ERR_INPROGRESS:
	MPIU_Strncpy(error_string, "socket operation in progress", length);
	break;
    case MPIDU_SOCK_ERR_TIMEOUT:
	MPIU_Strncpy(error_string, "socket operation timed out", length);
	break;
    case MPIDU_SOCK_ERR_INTR:
	MPIU_Strncpy(error_string, "socket operation interrupted", length);
	break;
    case MPIDU_SOCK_ERR_NO_NEW_SOCK:
	MPIU_Strncpy(error_string, "no new connection available", length);
	break;
    default:
	MPIU_Snprintf(error_string, length, "unknown socket error %d", error);
	break;
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_GET_ERROR_CLASS_STRING);
    return MPI_SUCCESS;
}

/*
int test_string_functions()
{
    char val[1024], val2[1024];
    char buffer[1024];
    char recursed[1024];
    int bin_buf[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    int bin_dest[10];
    int length, age, i;
    char *iter;

    iter = buffer;
    length = 1024;
    MPIU_Str_add_string_arg(&iter, &length, "name", "David Ashton");
    MPIU_Str_add_int_arg(&iter, &length, "age", 31);
    MPIU_Str_add_binary_arg(&iter, &length, "foo", bin_buf, 40);

    printf("encoded string: <%s>\n", buffer);

    MPIU_Str_get_string_arg(buffer, "name", val, 1024);
    printf("got name = '%s'\n", val);
    MPIU_Str_get_int_arg(buffer, "age", &age);
    printf("got age = %d\n", age);
    MPIU_Str_get_binary_arg(buffer, "foo", bin_dest, 40);
    printf("got binary data 'foo'\n");
    for (i=0; i<10; i++)
	printf(" bin[0] = %d\n", bin_dest[i]);

    iter = recursed;
    length = 1024;
    MPIU_Str_add_string_arg(&iter, &length, "name", "foobar");
    MPIU_Str_add_string_arg(&iter, &length, "buscard", buffer);
    MPIU_Str_add_string_arg(&iter, &length, "reason", "Because I say so");

    printf("recursive encoded string: <%s>\n", recursed);

    MPIU_Str_get_string_arg(recursed, "name", val, 1024);
    printf("got name = '%s'\n", val);
    MPIU_Str_get_string_arg(recursed, "buscard", val, 1024);
    printf("got buscard = '%s'\n", val);
    MPIU_Str_get_string_arg(val, "name", val2, 1024);
    printf("got name from buscard = '%s'\n", val2);
    MPIU_Str_get_string_arg(recursed, "reason", val, 1024);
    printf("got reason = '%s'\n", val);

    MPIU_Str_hide_string_arg(recursed, "reason");
    printf("hidden recursed string: <%s>\n", recursed);

    iter = buffer;
    iter += MPIU_Str_add_string(iter, 1024, "foo=\"");
    iter += MPIU_Str_add_string(iter, 1024, "=");
    iter += MPIU_Str_add_string(iter, 1024, "Wally World");
    printf("buffer = <%s>\n", buffer);
    iter = MPIU_Str_get_string(buffer, val, 1024, &i);
    printf("one = '%s'\n", val);
    iter = MPIU_Str_get_string(iter, val, 1024, &i);
    printf("two = '%s'\n", val);
    iter = MPIU_Str_get_string(iter, val, 1024, &i);
    printf("three = '%s'\n",val);

    return 0;
}
*/
