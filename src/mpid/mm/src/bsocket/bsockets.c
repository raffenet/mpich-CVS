/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "bsocketimpl.h"
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#elif defined(HAVE_SYS_IOCTL_H)
#include <sys/ioctl.h>
#endif
#include <stdio.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h> 
#endif
#include <errno.h> 
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
/* FIONBIO (solaris sys/filio.h) */
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h> 
#endif
/* TCP_NODELAY */
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h> 
#endif
/* defs of gethostbyname */
#ifdef HAVE_NETDB_H
#include <netdb.h> 
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef HAVE_WINSOCK2_H
#include <time.h>
#endif

/*#define DEBUG_BSOCKET*/
#undef DEBUG_BSOCKET

#define BPRINTF printf

#ifdef DEBUG_BSOCKET
#define DBG_MSG(paramlist) BPRINTF( paramlist )
#else
#define DBG_MSG(paramlist) 
#endif

typedef enum { 
    BFD_FD_NOT_IN_USE, 
    BFD_ALLOCATING, 
    BFD_NEW_FD, 
    BFD_BOUND, 
    BFD_LISTENING, 
    BFD_ACCEPTED, 
    BFD_CONNECTED, 
    BFD_WRITING, 
    BFD_READING, 
    BFD_IDLE, 
    BFD_NOT_READY, 
    BFD_SOCKET_READY, 
    BFD_ERROR 
} BFD_State;

struct BFD_Buffer_struct {
    int        real_fd;        /* socket descriptor */
    int        read_flag;      /* set if reading */
    int        write_flag;     /* set if writing */
    int        curpos;         /* holds current position in bbuf */
    int        num_avail;      /* bytes in our buffered read buffer */
    BFD_State  state;          /* state of our socket */
    int        errval;         /* errno value */
    char       read_buf[1];    /* read buffer */
    struct BFD_Buffer_struct *next;
};

BlockAllocator Bsocket_mem;

#define BSOCKET_MIN(a, b) ((a) < (b) ? (a) : (b))
#define BSOCKET_MAX(a, b) ((a) > (b) ? (a) : (b))

static int g_bbuflen = 1024;
static int g_buf_pool_size = FD_SETSIZE;
static int g_beasy_connection_attempts = 5;

#ifdef HAVE_WINSOCK2_H
static void log_warning(char *str, ...)
{
    char    szMsg[256] = "bsocket error";
    HANDLE  hEventSource;
    char   *lpszStrings[2];
    char pszStr[4096];
    va_list list;

    // Write to a temporary string
    va_start(list, str);
    vsprintf(pszStr, str, list);
    va_end(list);
    
    hEventSource = RegisterEventSource(NULL, "bsocket");
    
    lpszStrings[0] = szMsg;
    lpszStrings[1] = pszStr;
    
    if (hEventSource != NULL) 
    {
	ReportEvent(hEventSource, /* handle of event source */
	    EVENTLOG_WARNING_TYPE,  /* event type */
	    0,                    /* event category */
	    0,                    /* event ID */
	    NULL,                 /* current user's SID */
	    2,                    /* strings in lpszStrings */
	    0,                    /* no bytes of raw data */
	    (LPCTSTR*)lpszStrings,/* array of error strings */
	    NULL);                /* no raw data */
	
	DeregisterEventSource(hEventSource);
    }
}
#else
#define log_warning()
#endif

/*@
   bget_fd - get fd

   Parameters:
+  int bfd - bfd

   Notes:
@*/
unsigned int bget_fd(int bfd)
{
    return (unsigned int)(((BFD_Buffer*)bfd)->real_fd);
}

/*@
   bset - bset

   Parameters:
+  int bfd - bfd
-  bfd_set *s - set

   Notes:
@*/
void bset(int bfd, bfd_set *s)
{
    int i;
    FD_SET( bget_fd(bfd), & (s) -> set );
    for (i=0; i<s->n; i++)
    {
	if (s->p[i] == (BFD_Buffer*)bfd)
	    return;
    }
    s->p[s->n] = (BFD_Buffer*)bfd;
    s->n++;
}

/*@
   bclr - blcr

   Parameters:
+  int bfd - bfd
-  bfd_set *s - set

   Notes:
@*/
void bclr(int bfd, bfd_set *s)
{
    int i;
    BFD_Buffer* p;

    FD_CLR( bget_fd(bfd), & (s) -> set );

    if (s->n == 0)
	return;

    p = (BFD_Buffer*)bfd;
    for (i=0; i<s->n; i++)
    {
	if (s->p[i] == p)
	{
	    s->p[i] = s->p[s->n-1];
	    s->n--;
	    return;
	}
    }
}

/*@
bsocket_init - init

  
    Notes:
@*/
static int g_bInitFinalize = 0;
int bsocket_init(void)
{
    char *pszEnvVar;
    char *szNum;
#ifdef HAVE_WINSOCK2_H
    WSADATA wsaData;
    int err;
    
    if (g_bInitFinalize == 1)
	return 0;

    /* Start the Winsock dll */
    if ((err = WSAStartup(MAKEWORD(2, 0), &wsaData)) != 0)
    {
	BPRINTF("Winsock2 dll not initialized, error %d\n", err);
	return err;
    }
#else
    if (g_bInitFinalize == 1)
	return 0;
#endif

    szNum = getenv("BSOCKET_CONN_TRIES");
    if (szNum != NULL)
	g_beasy_connection_attempts = atoi(szNum);

    pszEnvVar = getenv("BSOCKET_BBUFLEN");
    if (pszEnvVar != NULL)
	g_bbuflen = atoi(pszEnvVar);

    Bsocket_mem = BlockAllocInit(sizeof(BFD_Buffer) + g_bbuflen, 64, 64, malloc, free);

    g_bInitFinalize = 1;

    return 0;
}

/*@
bsocket_finalize - finalize

  
    Notes:
@*/
int bsocket_finalize(void)
{
    if (g_bInitFinalize == 0)
	return 0;

    /* Free up the memory used by Bsocket_mem */
    BlockAllocFinalize(&Bsocket_mem);
    
#ifdef HAVE_WINSOCK2_H
    WSACleanup();
#endif

    g_bInitFinalize = 0;
    return 0;
}

/*@
bsocket - socket

  Parameters:
+  int family - family
.  int type - type
-  int protocol - protocol
  
    Notes:
@*/
int bsocket(int family, int type, int protocol)
{
    BFD_Buffer *pbfd;
    
    DBG_MSG("Enter bsocket\n");
    
    pbfd = (BFD_Buffer *)BlockAlloc( Bsocket_mem );
    if (pbfd == 0) 
    {
	DBG_MSG(("ERROR in bsocket: BlockAlloc returned NULL"));
	return -1;
    }
    
    memset(pbfd, 0, sizeof(BFD_Buffer));
    pbfd->state = BFD_FD_NOT_IN_USE;
    pbfd->real_fd = socket(family, type, protocol);
    if (pbfd->real_fd == SOCKET_ERROR) 
    {
	DBG_MSG("ERROR in bsocket: socket returned SOCKET_ERROR\n");
	memset(pbfd, 0, sizeof(BFD_Buffer));
	BlockFree( Bsocket_mem, pbfd );
	return -1;
    }
    
    return (int)pbfd;
}

/*@
bbind - bind

Parameters:
+  int bfd - bsocket
.  const struct sockaddr *servaddr - address
-  socklen_t servaddr_len - address length
  
    Notes:
@*/
int bbind(int bfd, const struct sockaddr *servaddr,	      
	  socklen_t servaddr_len)
{
    DBG_MSG("Enter bbind\n");
    
    return bind(((BFD_Buffer*)bfd)->real_fd, servaddr, servaddr_len);
}

/*@
blisten - listen

Parameters:
+  int bfd - bsocket
-  int backlog - backlog
  
    Notes:
@*/
int blisten(int bfd, int backlog)
{
    return listen(((BFD_Buffer*)bfd)->real_fd, backlog);
}

/*@
bsetsockopt - setsockopt

  Parameters:
  +  int bfd - bsocket
  .  int level - level
  .  int optname - optname
  .  const void *optval - optval
  -  socklen_t optlen - optlen
  
    Notes:
@*/
int bsetsockopt(int bfd, int level, int optname, const void *optval,		    
		socklen_t optlen)
{
    return setsockopt(((BFD_Buffer*)bfd)->real_fd, level, optname, optval, optlen);
}

/*@
baccept - accept

  Parameters:
  +  int bfd - bsocket
  .  struct sockaddr *cliaddr - client address
  -  socklen_t *clilen - address length
  
    Notes:
@*/
int baccept(int bfd, struct sockaddr *cliaddr, socklen_t *clilen)
{
    int 	       conn_fd;
    BFD_Buffer 	       *new_bfd;
    
    DBG_MSG("Enter baccept\n");
    
    conn_fd = accept(((BFD_Buffer*)bfd)->real_fd, cliaddr, clilen);
    if (conn_fd == SOCKET_ERROR) 
    {
	DBG_MSG("ERROR in baccept: accept returned SOCKET_ERROR\n");
	return BFD_INVALID_SOCKET;
    }
    
    new_bfd = (BFD_Buffer *)BlockAlloc( Bsocket_mem );
    if (new_bfd == 0) 
    {
	DBG_MSG(("ERROR in baccept: BlockAlloc return NULL\n"));
	return BFD_INVALID_SOCKET;
    }

    memset(new_bfd, 0, sizeof(BFD_Buffer));
    new_bfd->real_fd = conn_fd;
    new_bfd->state = BFD_IDLE;

    return (int)new_bfd;
}

/*@
bconnect - connect

  Parameters:
  +  int bfd - bsocket
  .  const struct sockaddr *servaddr - address
  -  socklen_t servaddr_len - address length
  
    Notes:
@*/
int bconnect(int bfd, const struct sockaddr *servaddr,		    
	     socklen_t servaddr_len)
{
    return connect(((BFD_Buffer*)bfd)->real_fd, servaddr, servaddr_len);
}

/*@
bselect - select

  Parameters:
  +  int maxfds - max bfd - 1 You must use BFD_MAX to get this value
  .  bfd_set *readbfds - read set
  .  bfd_set *writebfds - write set
  .  bfd_set *execbfds - exec set
  -  struct timeval *tv - timeout
  
    Notes:
@*/
int bselect(int maxfds, bfd_set *readbfds, bfd_set *writebfds,		   
	    bfd_set *execbfds, struct timeval *tv)
{
    int 	   nbfds;
    bfd_set        rcopy;
    BFD_Buffer     *p;
    int            i;

    DBG_MSG("Enter bselect\n");
    
    if (readbfds)
	rcopy = *readbfds;

    maxfds = ((BFD_Buffer*)maxfds)->real_fd + 1;
    nbfds = select(maxfds, 
	readbfds ? &readbfds->set : NULL, 
	writebfds ? &writebfds->set : NULL, 
	execbfds ? &execbfds->set : NULL, tv);
    if (nbfds == SOCKET_ERROR) 
	return SOCKET_ERROR;
    
    if (readbfds)
    {
	for (i=0; i<readbfds->n; i++)
	{
	    p = readbfds->p[i];
	    if (p->num_avail > 0 && (FD_ISSET(p->real_fd, &rcopy.set)) && !(FD_ISSET(p->real_fd, &readbfds->set)))
	    {
		FD_SET((unsigned int)p->real_fd, &readbfds->set);
		nbfds++;
	    }
	}
    }
    
    return nbfds;
}

/*@
bwrite - write

  Parameters:
  +  int bfd - bsocket
  .  char *ubuf - buffer
  -  int len - length
  
    Notes:
@*/
int bwrite(int bfd, char *ubuf, int len)
{
    return write(((BFD_Buffer*)bfd)->real_fd, ubuf, len);
}

/*
#define DBG_BWRITEV
#define DBG_BWRITEV_PRINT(a) BPRINTF a
*/
#undef DBG_BWRITEV
#define DBG_BWRITEV_PRINT

/*@
   bwritev - writev

   Parameters:
+  int bfd - bsocket
.  B_VECTOR *pIOVec - iovec structure
-  int n - length of iovec

   Notes:
@*/
int bwritev(int bfd, B_VECTOR *pIOVec, int n)
{
#ifdef HAVE_WINSOCK2_H
#ifdef DBG_BWRITEV
    int i;
#endif
    DWORD dwNumSent = 0;
    if (n == 0)
	return 0;
#ifdef DBG_BWRITEV
    BPRINTF("(bwritev");
    for (i=0; i<n; i++)
	BPRINTF(":%d", pIOVec[i].B_VECTOR_LEN);
#endif
    if (WSASend(((BFD_Buffer*)bfd)->real_fd, pIOVec, n, &dwNumSent, 0, NULL/*overlapped*/, NULL/*completion routine*/) == SOCKET_ERROR)
    {
	if (WSAGetLastError() != WSAEWOULDBLOCK)
	{
	    return SOCKET_ERROR;
	}
    }
    DBG_BWRITEV_PRINT(("->%d)", dwNumSent));
    return dwNumSent;
#else
    int nWritten;
    nWritten = writev(((BFD_Buffer*)bfd)->real_fd, pIOVec, n);
    return nWritten;
#endif
}

/*@
bread - read

  Parameters:
  +  int bfd - bsocket
  .  char *ubuf - buffer
  -  int len - length
  
    Notes:
@*/
int bread(int bfd, char *ubuf, int len)
{
    int      fd;
    int      num_used;
    int      num_copied;
    int      n;
    char     *bbuf;
    BFD_Buffer *pbfd;
    
    DBG_MSG("Enter bread\n");
    
    pbfd = (BFD_Buffer*)bfd;

    if (pbfd->state == BFD_ERROR) 
	return pbfd->errval;
    
    pbfd->state = BFD_READING;
    fd = pbfd->real_fd;
    bbuf = pbfd->read_buf;
    
    if (len <= pbfd->num_avail) 
    {
	memcpy(ubuf, bbuf + pbfd->curpos, len);
	pbfd->curpos += len;
	pbfd->num_avail -= len;
	if (pbfd->num_avail == 0)
	    pbfd->curpos = 0;

	DBG_MSG(("bread: copied %d bytes into ubuf starting at bbuf[%d]\n", len, *(bbuf) + conn->curpos));
	
	return len;
    }
    
    if (pbfd->num_avail > 0) 
    {
	memcpy(ubuf, bbuf + pbfd->curpos, pbfd->num_avail);
	ubuf += pbfd->num_avail;
	len -= pbfd->num_avail;
	pbfd->curpos = 0;
    }
    
    if (len > g_bbuflen) 
    {
	n = read(fd, ubuf, len);
	if (n == 0) 
	{
	    pbfd->state = BFD_ERROR;
	    pbfd->errval = 0;
	}
	else if (n == -1) 
	{
	    if ((errno != EINTR) || (errno != EAGAIN)) 
	    {
		pbfd->state = BFD_ERROR;
		pbfd->errval = -1;
	    }
	    n = 0;
	}
	
	DBG_MSG(("bread: Read %d bytes directly into ubuf\n", n));
	n += pbfd->num_avail;
	pbfd->num_avail = 0;

	return n;
    }
    
    num_copied = pbfd->num_avail;
    n = read(fd, bbuf, g_bbuflen);
    pbfd->curpos = 0;
    if (n == 0) 
    {
	pbfd->state = BFD_ERROR;
	pbfd->errval = 0;
    }
    else if (n == -1) 
    {
	if ((errno != EINTR) || (errno != EAGAIN)) 
	{
	    pbfd->state = BFD_ERROR;
	    pbfd->errval = -1;
	}
	return -1;
    }
    
    pbfd->num_avail = n;
    num_used = (((len) < (pbfd->num_avail)) ? (len) : (pbfd->num_avail));
    memcpy(ubuf, bbuf, num_used);
    pbfd->curpos += num_used;
    pbfd->num_avail -= num_used;
    /*if (pbfd->num_avail > 0) BPRINTF("bread: %d extra bytes read into bbuf %d\n", pbfd->num_avail, pbfd->real_fd);*/
    pbfd->state = BFD_IDLE;

    DBG_MSG(("bread: Read %d bytes on socket %d into bbuf\n", n, fd));
    DBG_MSG(("bread: copied %d bytes into ubuf from bbuf\n", num_used));
    
    n = num_used + num_copied;
    return n;
}

/*
#define DBG_BREADV
#define DBG_BREADV_PRINT(a) BPRINTF a
*/
#undef DBG_BREADV
#define DBG_BREADV_PRINT(a) 

/*@
   breadv - readv

   Parameters:
+  int bfd - bsocket
.  B_VECTOR *uvec - iovec array
-  int len - length of array

   Notes:
   The vec parameter must have one more element than veclen.  This extra
   element is used by this function to read additional data into an internal
   buffer.
   The elements of the vec parameter may be changed by this function.
@*/
int breadv(int bfd, B_VECTOR *vec, int veclen)
{
    int k;
    int      fd;
    int      i;
    char     *bbuf;
    BFD_Buffer *pbfd;
    int      num_read = 0;
#ifdef HAVE_WINSOCK2_H
    DWORD    n = 0;
    DWORD    nFlags = 0;
#else
    int      n = 0;
#endif
    B_VECTOR pVector[B_VECTOR_LIMIT];
    int iVector;

    DBG_MSG("Enter bread\n");
    
    pbfd = (BFD_Buffer*)bfd;
    
    if (pbfd->state == BFD_ERROR) 
	return pbfd->errval;
    
    pbfd->state = BFD_READING;
    fd = pbfd->real_fd;
    bbuf = pbfd->read_buf;
    
#ifdef DBG_BREADV
    BPRINTF("(breadv");
    for (i=0; i<veclen; i++)
	BPRINTF(":%d", vec[i].B_VECTOR_LEN);
#endif
    num_read = 0;
    for (i=0; i<veclen; i++)
    {
	if (pbfd->num_avail)
	{
	    n = BSOCKET_MIN((unsigned int)pbfd->num_avail, vec[i].B_VECTOR_LEN);
	    DBG_BREADV_PRINT((",bcopy %d", n));
	    memcpy(vec[i].B_VECTOR_BUF, bbuf + pbfd->curpos, n);
	    if ((unsigned int)pbfd->num_avail <= vec[i].B_VECTOR_LEN)
	    {
		if ((unsigned int)pbfd->num_avail == vec[i].B_VECTOR_LEN)
		{
		    i++;
		    if (i==veclen)
		    {
			pbfd->num_avail = 0;
			pbfd->curpos = 0;
			DBG_BREADV_PRINT(("->%d,%da)", num_read+n, pbfd->num_avail));
			return num_read + n;
		    }
		}
		else
		{
		    /* Make a copy of the vector */
		    for (iVector = 0; iVector <= i; iVector++)
		    {
			pVector[iVector].B_VECTOR_BUF = vec[iVector].B_VECTOR_BUF;
			pVector[iVector].B_VECTOR_LEN = vec[iVector].B_VECTOR_LEN;
		    }
		    pVector[i].B_VECTOR_BUF += n;
		    pVector[i].B_VECTOR_LEN -= n;
		    vec = pVector;
		}
	    }
	    pbfd->num_avail -= n;
	    pbfd->curpos += n;
	    num_read += n;
	}
	
	if (pbfd->num_avail == 0)
	{
	    pbfd->curpos = 0;
	    break;
	}
	if (i == veclen - 1)
	{
	    DBG_BREADV_PRINT(("->%d,%db)", num_read, conn->num_avail));
	    return num_read;
	}
    }
    
    vec[veclen].B_VECTOR_BUF = bbuf;
    vec[veclen].B_VECTOR_LEN = g_bbuflen;

#ifdef DBG_BREADV
    BPRINTF(",breadv");
    for (k=0; k<veclen-i+1; k++)
	BPRINTF(":%d", vec[k+i].B_VECTOR_LEN);
#endif
#ifdef HAVE_WINSOCK2_H
    if (WSARecv(fd, &vec[i], veclen - i + 1, &n, &nFlags, NULL/*overlapped*/, NULL/*completion routine*/) == SOCKET_ERROR)
    {
	if (WSAGetLastError() != WSAEWOULDBLOCK)
	{
	    pbfd->state = BFD_ERROR;
	    pbfd->errval = WSAGetLastError();
	    BPRINTF("***WSARecv failed reading %d WSABUFs, error %d***\n", veclen - i + 1, pbfd->errval);
	    for (k=0; k<veclen-i+1; k++)
		BPRINTF("vec[%d] len: %d\nvec[%d] buf: 0x%x\n", k+i, vec[k+i].B_VECTOR_LEN, k+i, vec[k+i].B_VECTOR_BUF);
	    n = 0; /* Set this to zero so it can be added to num_read */
	}
    }
#else
    n = readv(fd, &vec[i], veclen - i + 1);
    if (n == -1) 
    {
	if ((errno != EINTR) || (errno != EAGAIN)) 
	{
	    pbfd->state = BFD_ERROR;
	    pbfd->errval = -1;
	}
	n = 0; /* Set this to zero so it can be added to num_read */
    }
#endif
    
    if (n)
    {
	for ( ; i <= veclen; i++)
	{
	    if (i == veclen)
	    {
		pbfd->num_avail = n;
	    }
	    else
	    {
		num_read += BSOCKET_MIN(vec[i].B_VECTOR_LEN, n);
		n = n - BSOCKET_MIN(vec[i].B_VECTOR_LEN, n);
		if (n == 0)
		{
		    DBG_BREADV_PRINT(("->%d,%dc)", num_read, pbfd->num_avail));
		    return num_read;
		}
	    }
	}
    }

    DBG_BREADV_PRINT(("->%d,%dd)", num_read, pbfd->num_avail));
    return num_read;
}

/*@
   bclose - close

   Parameters:
.  int bfd - bsocket

   Notes:
@*/
int bclose(int bfd)
{
    DBG_MSG("Enter bclose\n");

    close(((BFD_Buffer*)bfd)->real_fd);
    memset((void*)bfd, 0, sizeof(BFD_Buffer));
    BlockFree( Bsocket_mem, (BFD_Buffer*)bfd );

    return 0;
}

/*@
bgetsockname - 

  Parameters:
  +  int bfd
  .  struct sockaddr *name
  -  int *namelen
  
    Notes:
@*/
int bgetsockname(int bfd, struct sockaddr *name, int *namelen)
{
    return getsockname(((BFD_Buffer*)bfd)->real_fd, name, namelen);
}

/*@
make_nonblocking - make a bsocket non-blocking

  Parameters:
  . int bfd - bsocket
  
    Notes:
@*/
int bmake_nonblocking(int bfd)
{
    
    int      flag = 1;
    int      rc;
    
    DBG_MSG("Enter make_nonblocking\n");
    
#ifdef HAVE_WINDOWS_SOCKET
    rc = ioctlsocket(((BFD_Buffer*)bfd)->real_fd, FIONBIO, &flag);
#else
    rc = ioctl(((BFD_Buffer*)bfd)->real_fd, FIONBIO, &flag);
#endif
    
    return rc;
}

/*@
make_blocking - make a bsocket blocking

  Parameters:
  . int bfd - bsocket
  
    Notes:
@*/
int bmake_blocking(int bfd)
{
    int      flag = 0;
    int      rc;
    
    DBG_MSG("Enter make_blocking\n");
    
#ifdef HAVE_WINDOWS_SOCKET
    rc = ioctlsocket(((BFD_Buffer*)bfd)->real_fd, FIONBIO, &flag);
#else
    rc = ioctl(((BFD_Buffer*)bfd)->real_fd, FIONBIO, &flag);
#endif
    
    return rc;
}
/*@
   beasy_create - create a bsocket

   Parameters:
+  int *bfd - bsocket
.  int port - port
-  unsigned long addr - address

   Notes:
@*/
int beasy_create(int *bfd, int port, unsigned long addr)
{
    struct sockaddr_in sin;
    int optval = 1;
    struct linger linger;

    /* Create a new bsocket */
    *bfd = bsocket(AF_INET, SOCK_STREAM, 0);
    if (*bfd == BFD_INVALID_SOCKET)
    {
	return SOCKET_ERROR;
    }
    
    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = addr;
    sin.sin_port = htons((u_short)port);

    /* bind it to the port provided */
    if (bbind(*bfd, (const struct sockaddr *)&sin, sizeof(struct sockaddr)) == SOCKET_ERROR)
    {
	return SOCKET_ERROR;
    }

    /* Set the no-delay option */
    bsetsockopt(*bfd, IPPROTO_TCP, TCP_NODELAY, (char *)&optval, sizeof(optval));

    /* Set the linger on close option */
    linger.l_onoff = 1 ;
    linger.l_linger = 60;
    bsetsockopt(*bfd, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));

    return 0;
}

/*@
   beasy_connect - connect

   Parameters:
+  int bfd - bsocket
.  char *host - hostname
-  int port - port

   Notes:
@*/
int beasy_connect(int bfd, char *host, int port)
{
    int error;
    int reps = 0;
    struct hostent *lphost;
    struct sockaddr_in sockAddr;
    struct linger linger;
#ifdef HAVE_WINSOCK2_H
    /* use this array to make sure the warning only gets logged once */
    BOOL bWarningLogged[4] = { FALSE, FALSE, FALSE, FALSE };
#endif
    memset(&sockAddr,0,sizeof(sockAddr));
    
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = inet_addr(host);
    
    if (sockAddr.sin_addr.s_addr == INADDR_NONE || sockAddr.sin_addr.s_addr == 0)
    {
	lphost = gethostbyname(host);
	if (lphost != NULL)
	    sockAddr.sin_addr.s_addr = ((struct in_addr *)lphost->h_addr)->s_addr;
	else
	    return SOCKET_ERROR;
    }
    
    sockAddr.sin_port = htons((u_short)port);
    
    while (bconnect(bfd, (struct sockaddr*)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
    {
#ifdef HAVE_WINSOCK2_H
	error = WSAGetLastError();
	srand(clock());
	if( (error == WSAECONNREFUSED || error == WSAETIMEDOUT || error == WSAENETUNREACH || error == WSAEADDRINUSE)
	    && (reps < g_beasy_connection_attempts) )
	{
	    double d = (double)rand() / (double)RAND_MAX;
	    Sleep(200 + (int)(d*200));
	    reps++;
	    switch (error)
	    {
	    case WSAECONNREFUSED:
		if (!bWarningLogged[0])
		{
		    /*log_warning("WSAECONNREFUSED error, re-attempting bconnect(%s)", host);*/
		    bWarningLogged[0] = TRUE;
		}
		break;
	    case WSAETIMEDOUT:
		if (!bWarningLogged[1])
		{
		    log_warning("WSAETIMEDOUT error, re-attempting bconnect(%s)", host);
		    bWarningLogged[1] = TRUE;
		}
		break;
	    case WSAENETUNREACH:
		if (!bWarningLogged[2])
		{
		    log_warning("WSAENETUNREACH error, re-attempting bconnect(%s)", host);
		    bWarningLogged[2] = TRUE;
		}
		break;
	    case WSAEADDRINUSE:
		if (!bWarningLogged[3])
		{
		    log_warning("WSAEADDRINUSE error, re-attempting bconnect(%s)", host);
		    bWarningLogged[3] = TRUE;
		}
		break;
	    default:
		log_warning("%d error, re-attempting bconnect");
		break;
	    }
	}
	else
	{
	    return SOCKET_ERROR;
	}
#else
	if( (errno == ECONNREFUSED || errno == ETIMEDOUT || errno == ENETUNREACH)
	    && (reps < 10) )
	{
#ifdef HAVE_USLEEP
	    usleep(200);
#else
	    sleep(0);
#endif
	    reps++;
	}
	else
	{
	    return SOCKET_ERROR;
	}
#endif
    }

    /* Set the linger on close option */
    linger.l_onoff = 1 ;
    linger.l_linger = 60;
    bsetsockopt(bfd, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));

    return 0;
}

/*@
   beasy_accept - accept

   Parameters:
.  int bfd - listening bsocket

   Notes:
@*/
int beasy_accept(int bfd)
{
#ifdef HAVE_WINSOCK2_H
    BOOL b;
#endif
    struct linger linger;
    struct sockaddr addr;
    int len;
    int client;

    len = sizeof(addr);
    client = baccept(bfd, &addr, &len);

    if (client == BFD_INVALID_SOCKET)
    {
	return BFD_INVALID_SOCKET;
    }

    linger.l_onoff = 1;
    linger.l_linger = 60;
    bsetsockopt(client, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));

#ifdef HAVE_WINSOCK2_H
    b = TRUE;
    bsetsockopt(client, IPPROTO_TCP, TCP_NODELAY, (char*)&b, sizeof(BOOL));
#endif
    return client;
}

/*@
   beasy_closesocket - closesocket

   Parameters:
+  int bfd - bsocket

   Notes:
@*/
int beasy_closesocket(int bfd)
{
#ifdef HAVE_WINSOCK2_H
    WSAEVENT hEvent = WSACreateEvent();
    if (hEvent != WSA_INVALID_EVENT)
    {
	if (WSAEventSelect(bget_fd(bfd), hEvent, FD_CLOSE) == 0)
	{
	    shutdown(bget_fd(bfd), SD_BOTH);
	    WaitForSingleObject(hEvent, 200);
	    /*
	    if (WaitForSingleObject(hEvent, 100) == WAIT_TIMEOUT)
	    {
		BPRINTF("wait for close timed out\n");fflush(stdout);
	    }
	    else
	    {
		BPRINTF("wait for close succeeded\n");fflush(stdout);
	    }
	    */
	    WSACloseEvent(hEvent);
	}
	else
	    shutdown(bget_fd(bfd), SD_BOTH);
    }
    else
	shutdown(bget_fd(bfd), SD_BOTH);
#endif
    bclose(bfd);
    return 0;
}

/*@
   beasy_get_sock_info - get bsocket information

   Parameters:
+  int bfd - bsocket
.  char *name - hostname
-  int *port - port

   Notes:
@*/
int beasy_get_sock_info(int bfd, char *name, int *port)
{
    struct sockaddr_in addr;
    int name_len = sizeof(addr);

    getsockname(bget_fd(bfd), (struct sockaddr*)&addr, &name_len);
    *port = ntohs(addr.sin_port);
    gethostname(name, 100);
    return 0;
}

/*@
   beasy_get_ip_string - get ip string a.b.c.d

   Parameters:
.  char *ipstring - string

   Notes:
@*/
int beasy_get_ip_string(char *ipstring)
{
    char hostname[100];
    unsigned int a, b, c, d;
    struct hostent *pH;

    gethostname(hostname, 100);
    pH = gethostbyname(hostname);
    if (pH == NULL)
	return SOCKET_ERROR;
    a = (unsigned char)(pH->h_addr_list[0][0]);
    b = (unsigned char)(pH->h_addr_list[0][1]);
    c = (unsigned char)(pH->h_addr_list[0][2]);
    d = (unsigned char)(pH->h_addr_list[0][3]);
    snprintf(ipstring, 100, "%u.%u.%u.%u", a, b, c, d);
    return 0;
}

/*@
   beasy_get_ip - get ip address

   Parameters:
.  long *ip - ip address

   Notes:
@*/
int beasy_get_ip(unsigned long *ip)
{
    char hostname[100];
    struct hostent *pH;

    gethostname(hostname, 100);
    pH = gethostbyname(hostname);
    *ip = *((unsigned long *)(pH->h_addr_list));
    return 0;
}

/*@
   beasy_receive - receive

   Parameters:
+  int bfd - bsocket
.  char *buffer - buffer
-  int len - length

   Notes:
@*/
int beasy_receive(int bfd, char *buffer, int len)
{
    int ret_val;
    int num_received;
    bfd_set readfds;
    int total = len;
    
    num_received = bread(bfd, buffer, len);
    if (num_received == SOCKET_ERROR)
    {
	return SOCKET_ERROR;
    }
    else
    {
	len -= num_received;
	buffer += num_received;
    }
    
    while (len)
    {
	BFD_ZERO(&readfds); 
	BFD_SET(bfd, &readfds);
	
	ret_val = bselect(bfd+1, &readfds, NULL, NULL, NULL);
	if (ret_val == 1)
	{
	    num_received = bread(bfd, buffer, len);
	    if (num_received == SOCKET_ERROR)
	    {
		return SOCKET_ERROR;
	    }
	    else
	    {
		if (num_received == 0)
		{
		    /*BPRINTF("beasy_receive: socket closed\n");*/
		    /*bmake_blocking(bfd);*/
		    return 0;
		}
		len -= num_received;
		buffer += num_received;
	    }
	}
	else
	{
	    if (ret_val == SOCKET_ERROR)
		return SOCKET_ERROR;
	}
    }

    /*bmake_blocking(bfd);*/
    return total;
}

/*@
   beasy_receive_some - receive

   Parameters:
+  int bfd - bsocket
.  char *buffer - buffer
-  int len - length

   Notes:
@*/
int beasy_receive_some(int bfd, char *buffer, int len)
{
    int ret_val;
    int num_received;
    bfd_set readfds;
    
    num_received = bread(bfd, buffer, len);
    if (num_received == SOCKET_ERROR)
    {
	return SOCKET_ERROR;
    }
    else
    {
	if (num_received > 0)
	    return num_received;
    }
    
    BFD_ZERO(&readfds); 
    BFD_SET(bfd, &readfds);
    
    ret_val = bselect(bfd+1, &readfds, NULL, NULL, NULL);
    if (ret_val == 1)
    {
	num_received = bread(bfd, buffer, len);
	if (num_received == SOCKET_ERROR)
	{
	    return SOCKET_ERROR;
	}
	else
	{
	    if (num_received == 0)
	    {
		/*BPRINTF("beasy_receive_some: socket closed\n");*/
		/*bmake_blocking(bfd);*/
		return 0;
	    }
	    return num_received;
	}
    }

    return SOCKET_ERROR;
}

/*@
   beasy_receive_timeout - receive

   Parameters:
+  int bfd - bsocket
.  char *buffer - buffer
.  int len - length
-  int timeout - timeout

   Notes:
@*/
int beasy_receive_timeout(int bfd, char *buffer, int len, int timeout)
{
    int ret_val;
    int num_received;
    bfd_set readfds;
    struct timeval tv;
    int total = len;
    
    /*
    num_received = bread(bfd, buffer, len);
    if (num_received == SOCKET_ERROR)
    {
	return SOCKET_ERROR;
    }
    else
    {
	len -= num_received;
	buffer += num_received;
    }
    */
    
    while (len)
    {
	BFD_ZERO(&readfds); 
	BFD_SET(bfd, &readfds);
	tv.tv_sec = timeout;
	tv.tv_usec = 0;
	
	ret_val = bselect(bfd+1, &readfds, NULL, NULL, &tv);
	if (ret_val == 1)
	{
	    num_received = bread(bfd, buffer, len);
	    if (num_received == SOCKET_ERROR)
	    {
		return SOCKET_ERROR;
	    }
	    else
	    {
		if (num_received == 0)
		{
		    /*BPRINTF("beasy_receive_timeout: socket closed\n");*/
		    /*bmake_blocking(bfd);*/
		    return total - len;
		}
		len -= num_received;
		buffer += num_received;
	    }
	}
	else
	{
	    if (ret_val == SOCKET_ERROR)
		return SOCKET_ERROR;
	    else
	    {
		/*bmake_blocking(bfd);*/
		return total - len;
	    }
	}
    }
    /*bmake_blocking(bfd);*/
    return total;
}

/*@
   beasy_send - send

   Parameters:
+  int bfd - bsocket
.  char *buffer - buffer
-  int length - length

   Notes:
@*/
int beasy_send(int bfd, char *buffer, int length)
{
#ifdef HAVE_WINSOCK2_H
    int error;
    int num_sent;

    while ((num_sent = write(((BFD_Buffer*)bfd)->real_fd, buffer, length)) == SOCKET_ERROR)
    {
	error = WSAGetLastError();
	if (error == WSAEWOULDBLOCK)
	{
            /*Sleep(0);*/
	    continue;
	}
	if (error == WSAENOBUFS)
	{
	    /* If there is no buffer space available then split the buffer in half and send each piece separately.*/
	    if (beasy_send(bfd, buffer, length/2) == SOCKET_ERROR)
		return SOCKET_ERROR;
	    if (beasy_send(bfd, buffer+(length/2), length - (length/2)) == SOCKET_ERROR)
		return SOCKET_ERROR;
	    return length;
	}
	WSASetLastError(error);
	return SOCKET_ERROR;
    }
    
    return length;
#else
    int ret_val;
    int num_written;
    bfd_set writefds;
    int total = length;
    
    num_written = write(((BFD_Buffer*)bfd)->real_fd, buffer, length);
    if (num_written == SOCKET_ERROR)
    {
	return SOCKET_ERROR;
    }
    else
    {
	length -= num_written;
	buffer += num_written;
    }
    
    while (length)
    {
	BFD_ZERO(&writefds); 
	BFD_SET(bfd, &writefds);
	
	ret_val = bselect(1, NULL, &writefds, NULL, NULL);
	if (ret_val == 1)
	{
	    num_written = write(((BFD_Buffer*)bfd)->real_fd, buffer, length);
	    if (num_written == SOCKET_ERROR)
	    {
		return SOCKET_ERROR;
	    }
	    else
	    {
		if (num_written == 0)
		{
		    /*BPRINTF("beasy_send: socket closed\n");*/
		    return total - length;
		}
		length -= num_written;
		buffer += num_written;
	    }
	}
	else
	{
	    if (ret_val == SOCKET_ERROR)
		return SOCKET_ERROR;
	}
    }
    return total;
#endif
}

int beasy_getlasterror()
{
#ifdef HAVE_WINSOCK2_H
    return WSAGetLastError();
#else
    return errno;
#endif
}

int beasy_error_to_string(int error, char *str, int length)
{
#ifdef HAVE_WINSOCK2_H
    HLOCAL str_local;
    int num_bytes;
    num_bytes = FormatMessage(
	FORMAT_MESSAGE_FROM_SYSTEM |
	FORMAT_MESSAGE_ALLOCATE_BUFFER,
	0,
	error,
	MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
	(LPTSTR) &str_local,
	0,0);
    if (num_bytes < length)
	memcpy(str, str_local, num_bytes+1);
    else
    {
	LocalFree(str);
	return num_bytes+1;
    }
    LocalFree(str);
    strtok(str, "\r\n"); /* remove any CR/LF characters from the output */
#else
    snprintf(str, length, "error %d", error);
#endif
    return 0;
}
