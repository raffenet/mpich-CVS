/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2004 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * The routines in this file provide an event-driven I/O handler
 * 
 * Each active fd has a handler associated with it.
 */

#include "pmutilconf.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include "ioloop.h"

#define MAXFD 4096
static IOHandle handlesByFD[MAXFD+1];
static int maxFD = -1;

void TimeoutInit( int );
int  TimeoutGetTimeout( void );

/*@ 
  MPIE_IORegister - Register a handler for an FD

  Input Parameters:

  @*/
int MPIE_IORegister( int fd, int rdwr, 
		     int (*handler)(int,int,void*), void *extra_data )
{
    int i;

    printf( "Registering fd %d\n", fd );
    if (fd > MAXFD) {
	/* Error; fd is too large */
	return 1;
    }

    /* Remember the largest set FD, and clear any FDs between this
       fd and the last maximum */
    if (fd > maxFD) {
	for (i=maxFD+1; i<fd; i++) {
	    handlesByFD[i].fd      = -1;
	    handlesByFD[i].handler = 0;
	}
	maxFD = fd;
    }
    handlesByFD[fd].fd         = fd;
    handlesByFD[fd].rdwr       = rdwr;
    handlesByFD[fd].handler    = handler;
    handlesByFD[fd].extra_data = extra_data;
}

/*@
  MPIE_IOLoop - Handle all registered I/O

  Input Parameters:
.  timeoutSeconds - Seconds until this routine should return with a 
   timeout error.  If negative, no timeout.  If 0, return immediatedly
   after a nonblocking check for I/O.
@*/
int MPIE_IOLoop( int timeoutSeconds )
{
    int    i, maxfd, fd, nfds,rc;
    fd_set readfds, writefds;
    int (*handler)(int,int,void*);
    struct timeval tv;

    /* Determine the active FDs */
    FD_ZERO( &readfds );
    FD_ZERO( &writefds );
    maxfd = -1;
    for (i=0; i<=maxFD; i++) {
	if (handlesByFD[i].handler) {
	    fd = handlesByFD[i].fd;
	    if (handlesByFD[i].rdwr & IO_READ) {
		FD_SET( fd, &readfds );
	    }
	    if (handlesByFD[i].rdwr & IO_WRITE) {
		FD_SET( fd, &writefds );
	    }
	    maxfd = i;
	}
    }

    /* Loop on the fds, with the timeout */
    TimeoutInit( timeoutSeconds );
    while (1) {
	tv.tv_sec  = TimeoutGetRemaining();
	tv.tv_usec = 0;
	if (maxfd < 0) break;
	nfds = select( maxfd + 1, &readfds, &writefds, 0, &tv );
	if (nfds < 0 && errno == EINTR) {
	    continue;
	}
	if (nfds < 0) {
	    /* Serious error */
	    MPIU_Internal_sys_error_printf( "select", errno, 0 );
	    break;
	}
	if (nfds == 0) { 
	    /* Timeout from select */
	    return 0;
	}
	/* nfds > 0 */
	printf ("nfds = %d\n", nfds );
	for (fd=0; fd<=maxfd; fd++) {
	    if (FD_ISSET( fd, &writefds )) {
		handler = handlesByFD[fd].handler;
		if (handler) {
		    rc = (*handler)( fd, IO_WRITE, handlesByFD[fd].extra_data );
		}
		if (rc == 1) {
		    /* EOF? */
		    close(fd);
		    handlesByFD[fd].rdwr = 0;
		    FD_CLR(fd,&writefds);
		}
	    }
	    if (FD_ISSET( fd, &readfds )) {
		printf( "processing fd %d\n", fd );
		handler = handlesByFD[fd].handler;
		if (handler) {
		    rc = (*handler)( fd, IO_READ, handlesByFD[fd].extra_data );
		}
		if (rc == 1) {
		    /* EOF? */
		    close(fd);
		    handlesByFD[fd].rdwr = 0;
		    FD_CLR(fd,&readfds);
		}
	    }
	}
    }
}


static int end_time = -1;  /* Time of timeout in seconds */

void TimeoutInit( int seconds )
{
    if (seconds > 0) {
#ifdef HAVE_TIME
    time_t t;
    t = time( NULL );
    end_time = seconds + t;
#elif defined(HAVE_GETTIMEOFDAY)
    struct timeval tp;
    gettimeofday( &tp, NULL );
    end_time = seconds + tp.tv_sec;
#else
#   error 'No timer available'
#endif
    }
    else {
	end_time = -1;
    }
}

/* Return remaining time in seconds */
int TimeoutGetRemaining( void )
{
    int time_left;
    if (end_time < 0) {
	/* Return a large, positive number */
	return 1000000;
    }
    else {
#ifdef HAVE_TIME
    time_t t;
    t = time( NULL );
    time_left = end_time - t;
#elif defined(HAVE_GETTIMEOFDAY)
    struct timeval tp;
    gettimeofday( &tp, NULL );
    time_left = end_time - tp.tv_sec;
#else
#   error 'No timer available'
#endif
    }
    if (time_left < 0) time_left = 0;
    return time_left;
}

