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
 * output of each line.
 *
 * An alternative to the code in this file might be to use "fd shipping",
 * but that is not portable or as flexible as the approach in this 
 * file.
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
	DBG_PRINTF( ("Reading from fd %d\n", fd) );
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
	MPIU_Strncpy( iosdata->leader, leader, MAXLEADER );
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
#include <sys/time.h>
/* handle input for all of the processes in a process table.  Returns
   after a single invocation of select returns, with the number of active
   processes.  */
int IOHandleLoop( ProcessTable *ptable, int *reason )
{
    int    i, j;
    fd_set readfds, writefds;
    int    nfds, maxfd, fd;
    ProcessState *pstate;
    int    nactive;
    struct timeval tv;

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
    if (maxfd == -1) { *reason = 1; return 0; }

    if (debug) {
	DBG_PRINTF( ("Found %d active fds\n", nactive) );
    }

    /* A null timeout is wait forever.  We set a timeout here */
    tv.tv_sec  = GetRemainingTime();
    nfds = select( maxfd+1, &readfds, &writefds, 0, &tv );
    if (nfds < 0) {
	/* It may be EINTR, in which case we need to recompute the active
	   fds */
	if (errno == EINTR) { *reason = 2; return nactive; }

	/* Otherwise, we have a problem */
	printf( "Error in select!\n" );
	perror( "Reason: " );
	/* FIXME */
    }
    if (nfds == 0) {
	/* This is a timeout from the select.  This is either a 
	   total time expired or a polling time limit.  */
	if (GetRemainingTime() <= 0) *reason = 2;
	else                         *reason = 3;  /* Must be poll timeout */
	/* Note that no poll timeout is implemented yet */
	return 0;
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
				    DBG_PRINTF( ("closing io handler %d on %d\n",
					    j, i) );
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
				    DBG_PRINTF( ("closing io handler %d on %d\n",
					    j, i) );
				}
				pstate[i].ios[j].fdstate = IO_FINISHED;
			    }
			}
		    }
		}
	    }
	}
    }
    *reason = 0;
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

/* Get the prefix to use on output lines from the environment. 
   This is a common routine so that the same environment variables
   and effects can be used by many versions of mpiexec
   
   The choices are these:
   MPIEXEC_PREFIX_DEFAULT set:
       stdout gets "rank>"
       stderr gets "rank(err)>"
   MPIEXEX_PREFIX_STDOUT and MPIEXEC_PREFIX_STDERR replace those
   choices.
   
   The value can contain %d for the rank and eventually %w for the
   number of MPI_COMM_WORLD (e.g., 0 for the original one and > 0 
   for any subsequent spawned processes.  

   FIXME: We may want an easy way to generate no output for %w if there is 
   only one comm_world, and to also suppress other characters, such as
      1>  (only one comm world; we are rank 1)
      [2]3> (at least 3 comm worlds, we're #2 and rank 3 in that)
*/
void GetPrefixFromEnv( int isErr, char value[], int maxlen, int rank,
		       int world )
{
    const char *envval = 0;
    int         useDefault = 0;

    /* Get the default, if any */
    if (getenv( "MPIEXEC_PREFIX_DEFAULT" )) useDefault = 1;


    if (isErr) 
	envval = getenv( "MPIEXEC_PREFIX_STDERR" );
    else 
	envval = getenv( "MPIEXEC_PREFIX_STDOUT" );

    if (!envval) {
	if (useDefault) {
	    if (isErr) 
		envval = "%d(err)> ";
	    else
		envval = "%d> ";
	}
    }
    value[0] = 0;
    if (envval) {
#define MAX_DIGITS 20	
	const char *pin = envval;
	char *pout = value;
	char digits[MAX_DIGITS];
	int  dlen;
	int  lenleft = maxlen-1;
	/* Convert %d and %w to the given values */
	while (lenleft > 0 && *pin) {
	    if (*pin == '%') {
		pin++;
		/* Get the control */
		switch (*pin) {
		case '%': *pout++ = '%'; lenleft--; break;
		case 'd': 
		    MPIU_Snprintf( digits, MAX_DIGITS, "%d", rank );
		    dlen = strlen( digits );
		    if (dlen < lenleft) {
			MPIU_Strnapp( pout, digits, maxlen );
			pout += dlen;
			lenleft -= dlen;
		    }
		    else {
			*pout++ = '*';
			lenleft--;
		    }
		    break;
		case 'w':
		    MPIU_Snprintf( digits, MAX_DIGITS, "%d", world );
		    dlen = strlen(digits);
		    if (dlen < lenleft) {
			MPIU_Strnapp( pout, digits, maxlen );
			pout += dlen;
			lenleft -= dlen;
		    }
		    else {
			*pout++ = '*';
			lenleft--;
		    }
		    break;
		default:
		    /* Ignore the control */
		    *pout++ = '%'; lenleft--; 
		    if (lenleft--) *pout++ = *pin;
		}
		pin++;
	    }
	    else {
		*pout++ = *pin++;
		lenleft--;
	    }
	}
	*pout = 0;
    }
}

/* ------------------------------------------------------------------------- */
/* 
 * Improved handling of stdin, out, and err
 *
 * For stdin, out, and err, we must move data from one fd to another.  This
 * is complicated by several requirements:
 * 1) stdout and stderr have many sources (the stdout/err of each forked 
 *    process) writing to a single sink (the stdout/err of the mpiexec process)
 * 2) stdin has one source but potentially many sinks 
 * 3) the handlers must not block, even if the sinks can no longer accept any
 *    data.
 * 4) We want the option to annotate the stdout/err, such as adding the rank
 *    to each line, or doing differential aggregation (e.g., processes 1-17,34
 *    are identical).
 *
 * A major source of complexity is that the actions that we take on one fd 
 * (the source) depend on the state of another fd (the sink).  To manage this
 * relationship between the fds, an fd that is a sink that is ready to 
 * process data maintains a FIFO of routines that are called to write to the
 * fd (we let the routine write directly because this is not an interface
 * for the casual user - a more robust interface would use additional copies
 * or routine calls to separate the access to the fd).
 * The stdout/err and stdin cases are slightly different, because the 
 * stdout/err is a many to one mode and stdin is (potentially) one to many, 
 * even though the most common case is one to one (stdin sent to the process
 * with rank 0 in MPI_COMM_WORLD).
 */

/* A circular buffer, used to hold a line of input */
typedef struct {
    char *first, *firstFree;   /* Pointers to the first character and the 
				  first free location in the buffer 
			          An empty buffer has first = firstFree */
    int  nleft;                /* Number of available chars from firstFree
				  to either first or end of buffer */
    char *buf;                 /* Pointer to the allocated buffer */
    int  bufSize;              /* Number of characters in the buffer */
} CircularBuffer;

/* Get readv/writev/iovec */
#include <sys/uio.h>

#ifndef HAVE_IOVEC_DEFINITION
struct iovec {
    void  *iov_base;
    size_t iov_len;
};
#endif

#define MAX_IOVS 32
/* An annotated buffer */
typedef struct {
    CircularBuffer cbuf;
    char   *lastSeen;                /* Last location seen 
					by the annotation routine in
					the cbuf */
    struct iovec   iovs[MAX_IOVS];   /* Used for inserting annotations*/
    int    cbufIncr[MAX_IOVS];       /* Tells how many characters through
					the cbuf to advanced when this
					iov element is consumed */
    int    nIOV;                     /* number of active iov's */
} AnnotatedBuffer;    

/* Data used to forward information from one fd to another */
typedef struct {
    AnnotatedBuffer abuf;       /* Holds the pending data */

    int state;
    int (*annotate)( AnnotatedBuffer *, void *);
	/* Routine to annotate the data */
    int (*notify)( AnnotatedBuffer *, void * ); 
        /* Routine to call when data available */
    void *annotateExtra;       /* Extra data passed to the annotate routine */
    void *notifyExtra;         /* Extra data passed to the notify routine */
} IOOutManyToOne;

/* This is a prototypical many-to-one handler (e.g., for 
   stdout or stderr) 
   Called when there is activity on fd, such as the stdout of a process.
   Steps:
   Read data into the buffer.
   Call the notify routine
       Based on the return from the notify routine,
       0) No data processed.  If buffer is full, return xxx
       <0) Error on processing.  Return error to caller
       >0) Some data processed
*/
static int IOManyToOneReader( int fd, void *extra )
{
    IOOutManyToOne *iodata = (IOOutManyToOne *)extra;
    int       n;
    int       err;

    if (debug) {
	DBG_PRINTF( ( "Reading from fd %d\n", fd ) );
    }
    n = read( fd, iodata->abuf.cbuf.first, iodata->abuf.cbuf.nleft );
    /* FIXME: what should we do on an error? */
    if (n <= 0) return 0;

    /* Update the circular buffer */
    iodata->abuf.cbuf.nleft -= n;
    iodata->abuf.cbuf.firstFree += n;
    if (iodata->abuf.cbuf.nleft == 0) {
	/* Either hit the end of the buffer or the first pointer */
	/* FIXME: handle at the first pointer */
	iodata->abuf.cbuf.firstFree = iodata->abuf.cbuf.buf;
	/* FIXME: we need to let the annotate routine know that
	   we've moved the firstFree pointer */
    }

    /* Annotate the incoming data if requested */
    /* We need to call the annotation routine AFTER reading data, 
       because we may want to check for newlines without the buffer.
       We use iov's to avoid copying the output (and the potential 
       for explosive growth in the required size for the annotation
       that we'd get a long prefix and a series of newlines).
    */
    if (iodata->annotate) {
	err = (*iodata->annotate)( &iodata->abuf, iodata->annotateExtra );
    }
    else {
	/* No annotation routine, so just place the entire buffer 
	   in the first entry */
	iodata->abuf.iovs[0].iov_base = iodata->abuf.cbuf.first;
	iodata->abuf.iovs[0].iov_len = iodata->abuf.cbuf.nleft;
	iodata->abuf.nIOV        = 1;
	iodata->abuf.cbufIncr[0] = 1;
    }

    /* Pass the data on to the output by using the notify routine */
    err = (*iodata->notify)( &iodata->abuf, iodata->notifyExtra );

    /* Check the output state to see if we should suppress reading 
       from our fd until data can be accepted by the notify routine */
    return n;
}

/* A prototypical notify function.
   Add this data to the specified output.  The choice of
   output is stored in the structure pointed at by the extra
   data.
   
   Note that this function may choose to output directly; however, 
   it must not block, so if it cannot write all output, it must
   queue this output.

   FIXME: Still needs a function to initialize and setup the extra data.
*/
typedef struct {
    int outfd;
} IONotifyData;

/* These are static because they are local to this file (the routines
   are set in the setup routine xxx) */
static int IOManyToOneNotify( AnnotatedBuffer *, void * );
static int IOManyToOneConsume( AnnotatedBuffer *, void * );
static int IOManyToOneReader( int, void * );
static int IOManyToOneWriter( int, void * );
static int IOManyToOneAnnotate( AnnotatedBuffer *, void * );

static int IOManyToOneNotify( AnnotatedBuffer *abuf, void *extra )
{
    /* FIXME: First check to see if the queue of pending writes
       is empty.  If it isn't, we must not write, instead queuing 
       the data */
    /* FIXME: If any data is left, we need to queue it */
    return IOManyToOneConsume( abuf, extra );
}

static int IOManyToOneConsume( AnnotatedBuffer *abuf, void *extra )
{
    int i, j, n, ntot;
    IONotifyData *iodata = (IONotifyData *)extra;
    
    n = writev( iodata->outfd, abuf->iovs, abuf->nIOV );

    if (n < 0) {
	/* FIXME: Check for buffer full (EAGAIN?) */
	/* Error */
	return n;
    }

    /* Figure out how much we wrote (all, we hope!) */
    ntot = 0;
    for (i=0; i<abuf->nIOV; i++) {
	if (abuf->iovs[i].iov_len > n) break;
	ntot = abuf->iovs[i].iov_len;
	/* Move the circular buffer pointers over this entry */
	if (abuf->cbufIncr[i]) {
	    abuf->cbuf.first += abuf->iovs[i].iov_len;
	    if (abuf->cbuf.first == abuf->cbuf.buf + abuf->cbuf.bufSize) {
		abuf->cbuf.first = abuf->cbuf.buf;
	    }
	}
    }

    if (i<abuf->nIOV) {
	/* We didn't manage to write everything.  We must compress the
	   iov as well as update the circular buffer pointer.  Because
	   we'll normally have a small IOV, compressing it is simpler 
	   than maintaining it as a circular list */
	for (j=0; j<i; j++) {
	    abuf->iovs[j]     = abuf->iovs[i+j];
	    abuf->cbufIncr[j] = abuf->cbufIncr[i+j];
	}
	abuf->nIOV -= i;

	/* Update the 0th entry for any bytes consumed from it */
	abuf->iovs[0].iov_base = (char*)(abuf->iovs[0].iov_base) + (n-ntot);
	abuf->iovs[0].iov_len -= (n-ntot);
	if (abuf->cbufIncr[0]) {
	    abuf->cbuf.first += n-ntot;
	    if (abuf->cbuf.first == abuf->cbuf.buf + abuf->cbuf.bufSize) 
		abuf->cbuf.first = abuf->cbuf.buf;
	}
    }

    return n;
}

/* 
   A prototypical annotation function.  This adds the
   specified label (stored in the structure pointed at by the extra data)
   after every newline (but only after a character has been seen)
   
   FIXME: Not tested.
   FIXME: Still needs an initialization function that 
   can set up the extra data.
*/
typedef struct {
    char *leader;
    int  leaderlen;
    int  newlineFlag;
} IOAnnotateData;

static int IOManyToOneAnnotate( AnnotatedBuffer *abuf, void *extra )
{
    char *p, *lastP;
    IOAnnotateData *iodata = (IOAnnotateData *)extra;
    int newlineFlag = iodata->newlineFlag, nseen;

    p = abuf->lastSeen;

    /* Search for a newline.  Because this is a circular buffer, this
       is slighly more complicated */
    if (abuf->cbuf.first < abuf->cbuf.firstFree) 
	lastP = abuf->cbuf.firstFree;
    else
	lastP = abuf->cbuf.buf + abuf->cbuf.bufSize;

    nseen = 0;
    while (p < lastP) {
	
	if (newlineFlag) {
	    /* newlineFlag lets us add the leader after a newline
	       only when there is data to follow the leader */
	    abuf->iovs[abuf->nIOV].iov_base = iodata->leader;
	    abuf->iovs[abuf->nIOV].iov_len  = iodata->leaderlen;
	    abuf->cbufIncr[abuf->nIOV++] = 0;
	    newlineFlag = 0;
	}
	nseen++;

	if (*p == '\n') {
	    /* Here is where we would break to insert an annotation */
	    abuf->iovs[abuf->nIOV].iov_base = abuf->lastSeen;
	    abuf->iovs[abuf->nIOV].iov_len  = nseen;
	    abuf->cbufIncr[abuf->nIOV++] = 1;
	    nseen = 0;
	}
	p++;
	if (p == abuf->cbuf.buf + abuf->cbuf.bufSize) {
	    /* Hit the end of the circular buffer.  Update both
	       the pointer and the end-of-buffer */
	    p = abuf->cbuf.buf;
	    lastP = abuf->cbuf.firstFree-1;
	}
	/* Leave some room for an iov (we don't need to fully 
	 annotate the buffer if there are too many newlines) */
	if (abuf->nIOV == MAX_IOVS - 2) break;
    }

    /* Remember our state */
    iodata->newlineFlag = newlineFlag;
    abuf->lastSeen      = p;

    return 0;
}

/* This is the routine that is called when it is possible to write 
   on an fd for the many-to-one function.  This routine
   manages the queue of pending writes.  Note that we 
   allow immediate access to the fd in the notify routine to bypass a
   separate queuing step.  We share a writer routine, IOManyToOneConsume.
*/
/* FIXME: MAX_PENDING_OUT should be max processes (including max universe 
   size */
#define MAX_PENDING_OUT 256
typedef struct {
    AnnotatedBuffer *abuf;
} IOQueueOut;
/* A circular list of entries.  Empty when first==firstFree=0 */
typedef struct {
    int        first, firstFree;
    IOQueueOut pending[MAX_PENDING_OUT];
} IOManyToOneOut;
static int IOManyToOneWriter( int fd, void *extra )
{
    IOManyToOneOut *iodata = (IOManyToOneOut *)extra;

    /* FIXME: Nothing to write, so we should not select on this fd */
    if (iodata->first == iodata->firstFree && iodata->first == 0) return 0;

    /* While there are queue entries,
       try to consume all of the data.  If all data read, remove that
       element */

    /* If no operations are pending, set a flag so that the Notifier routine
       can (attempt to) write directly to the output fd */

    return 0;
}

/* ------------------------------------------------------------------------- */
/* 
 * OneToMany .  These routines provide for stdin handling.
 * The initial implementation will direct stdin to a single fd, but the
 * design allows for directing copies of input to more than one destination.
 * This code is also somewhat simpler than the ManyToOne since there is
 * no need to annotate the data.  This allows use to use a single circular
 * buffer.  In the general case, the only complexity is keeping data around
 * until all selected destinations have read it.
 */
