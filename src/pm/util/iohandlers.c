/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "pmutilconf.h"
#include "pmutil.h"
#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <errno.h>

/*
 * IO handlers for stdin, out, and err.
 * These are fairly simple, but allow useful control, such as labeling the
 * output of each line
 */
#define MAXCHARBUF 1024
#define MAXLEADER  32
typedef struct {
    char buf[MAXCHARBUF];
    int  curlen;
    int  outfd;
    char leader[MAXLEADER];
    int  leaderlen;
    int  outleader;
} IOOutData;

/* ----------------------------------------------------------------------- */
/* All too simple handler for stdout/stderr.  Note that this can block
   if the output fd is full (FIXME) 
   Return the number of bytes read, or < 0 if error. 
*/
int IOHandleStdOut( int fd, void *extra )
{
    IOOutData *iodata = (IOOutData *)extra;
    char      *p,*p1;
    int       n, nin;

    if (debug) {
	printf( "Reading from fd %d\n", fd );
    }
    n = read( fd, iodata->buf, MAXCHARBUF );
    if (n <= 0) return 0;

    /* Output the lines that have been read, including any partial data. */
    nin = n;
    p = iodata->buf;
    while (n > 0) {
	if (iodata->outleader) {
	    write( iodata->outfd, iodata->leader, iodata->leaderlen );
	    iodata->outleader = 0;
	}
	p1 = p;
	while (*p1 != '\n' && (int)(p1 - p) < n) p1++;
	write( iodata->outfd, p, (int)(p1 - p + 1) );
	if (*p1 == '\n') iodata->outleader = 1;
	n -= (p1 - p + 1);
	p = p1 + 1;
    }
    
    return nin;
}

/* FIXME: io handler for stdin */
/* Call this to read from fdSource and write to fdDest.  Prepend
   "leader" to all output. */
int IOSetupOutHandler( IOSpec *ios, int fdSource, int fdDest, char *leader )
{
    IOOutData *iosdata;
    
    iosdata = (IOOutData *)MPIU_Malloc( sizeof( IOOutData ) );
    iosdata->curlen    = 0;
    iosdata->outfd     = fdDest;
    iosdata->outleader = 1;
    if (*leader) {
	iosdata->leaderlen = strlen( leader );
	strncpy( iosdata->leader, leader, MAXLEADER );
    }
    else 
	iosdata->leaderlen = 0;
    ios->isWrite     = 0;   /* Because we are READING the stdout/err
			       of the process */
    ios->extra_state = (void *)iosdata;
    ios->handler     = IOHandleStdOut;
    ios->fd          = fdSource;
    ios->fdstate     = IO_PENDING; /* We always want to read from this process */

    return 0;
}

/* ---------------------------------------------------------------------- */
/* Process a collection of IO handlers.
 */
/* ---------------------------------------------------------------------- */
#include <sys/select.h>

/* handle input for all of the processes in a process table.  Returns
   after a single invocation of select returns, with the number of active
   processes.  */
int IOHandleLoop( ProcessTable *ptable )
{
    int    i, j;
    fd_set readfds, writefds;
    int    nfds, maxfd, fd;
    ProcessState *pstate;
    int    nactive;

    /* Setup the select sets */
    FD_ZERO( &readfds );
    FD_ZERO( &writefds );
    pstate  = ptable->table;
    maxfd   = -1;
    nactive = 0;
    for (i=0; i<ptable->nProcesses; i++) {
	/* if (pstate[i].state == GONE) continue; */
	/* printf( "state is %d\n", pstate[i].state ); */
	for (j=0; j<pstate[i].nIos; j++) {
	    fd = pstate[i].ios[j].fd;
	    if (pstate[i].ios[j].fdstate == IO_PENDING) {
		if (fd > maxfd)  maxfd = fd;
		if (pstate[i].ios[j].isWrite) {
		    FD_SET( fd, &writefds );
		    nactive++;
		}
		else {
		    FD_SET( fd, &readfds );
		    nactive++;
		}
	    }
	}
    }
    if (maxfd == -1) return 0;

    if (debug) {
	printf( "Found %d active fds\n", nactive );
    }

    /* A null timeout is wait forever.  
       FIXME: We need to set a limit on this (using the timeout) */
    nfds = select( maxfd+1, &readfds, &writefds, 0, 0 );
    if (nfds < 0) {
	/* It may be EINTR, in which case we need to recompute the active
	   fds */
	if (errno == EINTR) return nactive;

	/* Otherwise, we have a problem */
	printf( "Error in select!\n" );
	perror( "Reason: " );
	/* FIXME */
    }
    if (nfds > 0) {
	/* Find all of the fd's with activity */
	for (i=0; i<ptable->nProcesses; i++) {
	    /* FIXME: We may want to drain processes that have 
	       exited */
	    /*	    if (pstate[i].state == GONE) continue; */
	    for (j=0; j<pstate[i].nIos; j++) {
		if (pstate[i].ios[j].fdstate == IO_PENDING) {
		    int err;
		    fd = pstate[i].ios[j].fd;
		    if (pstate[i].ios[j].isWrite) {
			if (FD_ISSET( fd, &writefds )) {
			    err = (pstate[i].ios[j].handler)( fd, 
					    pstate[i].ios[j].extra_state );
			    if (err < 0) {
				if (debug) {
				    printf( "closing io handler %d on %d\n",
					    j, i );
				}
				pstate[i].ios[j].fdstate = IO_FINISHED;
			    }
			}
		    }
		    else {
			if (FD_ISSET( fd, &readfds )) {
			    err = (pstate[i].ios[j].handler)( fd, 
					    pstate[i].ios[j].extra_state );
			    if (err <= 0) {
				if (debug) {
				    printf( "closing io handler %d on %d\n",
					    j, i );
				}
				pstate[i].ios[j].fdstate = IO_FINISHED;
			    }
			}
		    }
		}
	    }
	}
    }
    return nactive;
}

/* 
 * This routine is used to close the open file descriptors prior to executing 
 * an exec.  This is normally used after a fork.  Use the first np entries
 * of pstate (this allows you to pass in any set of consequetive entries)
 */
void IOHandlersCloseAll( ProcessState *pstate, int np )
{
    int i, j;

    for (i=0; i<np; i++) {
	for (j=0; j<pstate[i].nIos; j++) {
	    close( pstate[i].ios[j].fd );
	}
    }
}
