/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "pmutilconf.h"
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
/* ctype is needed for isspace and isascii (isspace is only defined for 
   values on which isascii returns true). */
#include <ctype.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include "pmutil.h"
#include "process.h"
#include "pmiserv.h"
#include "ioloop.h"

#ifndef MAX_PENDING_CONN
#define MAX_PENDING_CONN 10
#endif
#ifndef MAX_HOST_NAME
#define MAX_HOST_NAME 1024
#endif

/* ----------------------------------------------------------------------- */
/* Get a port for the PMI interface                                        */
/* Ports can be allocated within a requested range using the runtime       */
/* parameter value MPIEXEC_PORTRANGE, which has the format low:high,       */
/* where both low and high are positive integers.  Unless this program is  */
/* privaledged, the numbers must be greater than 1023.                     */
/* Return -1 on error                                                      */
/* ----------------------------------------------------------------------- */
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
/* This is really IP!? */
#ifndef TCP
#define TCP 0
#endif
int PMIServGetPort( int *fdout, char *portString, int portLen )
{
    int                fd = -1;
    struct sockaddr_in sa;
    int                optval = 1;
    int                portnum;
    char               *range_ptr;
    int                low_port=0, high_port=0;

    /* Under cygwin we may want to use 1024 as a low port number */
    /* a low and high port of zero allows the system to choose 
       the port value */
    
    /* Get the low and high portnumber range.  zero may be used to allow
       the system to choose */
    range_ptr = getenv( "MPIEXEC_PORTRANGE" );
    if (range_ptr) {
	char *p;
	/* Look for n:m format */
	p = range_ptr;
	while (*p && isspace(*p)) p++;
	while (*p && isdigit(*p)) low_port = 10 * low_port + (*p++ - '0');
	if (*p == ':') {
	    while (*p && isdigit(*p)) high_port = 10 * high_port + (*p++ - '0');
	}
	if (!*p) {
	    MPIU_Error_printf( "Invalid character %c in MPIEXEC_PORTRANGE\n", 
			       *p );
	    return -1;
	}
    }

    for (portnum=low_port; portnum<=high_port; portnum++) {
	bzero( (void *)&sa, sizeof(sa) );
	sa.sin_family	   = AF_INET;
	sa.sin_port	   = htons( portnum );
	sa.sin_addr.s_addr = INADDR_ANY;
    
	fd = socket( AF_INET, SOCK_STREAM, TCP );
	if (fd < 0) {
	    /* Failure; return immediately */
	    return fd;
	}
    
	if (setsockopt( fd, IPPROTO_TCP, TCP_NODELAY, 
		    (char *)&optval, sizeof(optval) )) {
	    MPIU_Internal_sys_error_printf( "setsockopt", errno, 0 );
	}
	
	if (bind( fd, (struct sockaddr *)&sa, sizeof(sa) ) < 0) {
	    close( fd );
	    fd = -1;
	    if (errno != EADDRINUSE && errno != EADDRNOTAVAIL) {
		return -1;
	    }
	}
	else {
	    /* Success! We have a port.  */
	    break;
	}
    }
    
    if (fd < 0) {
	/* We were unable to find a usable port */
	return -1;
    }

    /* Listen is a non-blocking call that enables connections */
    listen( fd, MAX_PENDING_CONN );
    
    *fdout = fd;
    if (portnum == 0) {
	int sinlen = sizeof(sa);
	/* We asked for *any* port, so we need to find which
	   port we actually got */
	getsockname( fd, (struct sockaddr *)&sa, &sinlen );
	portnum = ntohs(sa.sin_port);
    }

    /* Create the port string */
    {
	char hostname[MAX_HOST_NAME+1];
    MPIE_GetMyHostName( hostname, sizeof(hostname) );
    MPIU_Snprintf( portString, portLen, "%s:%d", hostname, portnum );
    }
    
    return 0;
}

#include <netdb.h>
int MPIE_GetMyHostName( char myname[MAX_HOST_NAME+1], int namelen )
{
    struct hostent     *hp;
    char *hostname = 0;
    /*
     * First, get network name if necessary
     */
    if (!hostname) {
	hostname = myname;
	gethostname( myname, namelen );
    }
    hp = gethostbyname( hostname );
    if (!hp) {
	return -1;
    }
    return 0;
}

/* IO Handler for the listen socket
   Respond to a connection request by creating a new socket, which is
   then registered.
   Initialize the startup handshake.
 */
int PMIServAcceptFromPort( int fd, int rdwr, void *data )
{
    int    newfd;
    struct sockaddr sock;
    int    addrlen;
    int    id;
    ProcessWorld *pWorld = (ProcessWorld *)data;
    ProcessApp   *app;

    /* Get the new socket */
    MPIE_SYSCALL(newfd,accept,( fd, &sock, &addrlen ));
    if (newfd < 0) return newfd;

#ifdef FOO
    /* Mark this fd as non-blocking */
    flags = fcntl( newfd, F_GETFL, 0 );
    if (flags >= 0) {
	flags |= O_NDELAY;
	fcntl( newfd, F_SETFL, flags );
    }
#endif

    /* Find the matching process.  Do this by reading from the socket and 
       getting the id value with which process was created. */
    id = PMI_Init_port_connection( newfd );
    if (id >= 0) {
	/* find the matching entry */
	ProcessState *pState;
	int           nSoFar = 0;
	PMIProcess   *pmiprocess;

	app = pWorld->apps;
	while (app) {
	    if (app->nProcess < id - nSoFar) {
		/* Found the matching app */
		pState = app->pState + (id - nSoFar);
		break;
	    }
	    else {
		nSoFar += app->nProcess;
	    }
	    app = app->nextApp;
	}
	
	/* Now, initialize the connection */
#if 0
	PMIServAddtoGroup( 0, id, ps->pid, ps->fdPMI );
	PMIServSetupEntry( newfd, 0, ptable->nProcesses, id, 
				   &ps->pmientry );
	PMI_Init_remote_proc( newfd, &ps->pmientry,
			      id, ptable->nProcesses, debug );
#endif
	/* See PMISetupFinishInServer for similar code */
	pmiprocess = PMISetupNewProcess( newfd, pState );
	MPIE_IORegister( newfd, IO_READ, PMIServHandleInput, pmiprocess );
    }
    else {
    }

    /* Return success. */
    return 0;
}
