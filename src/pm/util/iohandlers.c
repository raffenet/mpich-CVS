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
 * Handling I/O for mpiexec.
 *
 * Handling of I/O from the processes started by mpiexec requires care.
 * We want to have the following properties
 * 1) Stdout and stderr from the processes go to stdout/err of mpiexec
 * 2) Stdin from mpiexec is routed to one (or more) processes (by 
 *    default, the process with rank 0 in MPI_COMM_WORLD)
 * 3) All operations are non-blocking, even if the stdout/err of mpiexec
 *    is blocked
 * 4) stdout/err output can be "annotated", for example, with the rank in
 *    MPI_COMM_WORLD of the process that generated the data.
 * 5) Support MPIEXEC_TIMEOUT
 * 6) No busy waiting, which implies that all fds, both those associated
 *    with IO and those with everything else (such as the PMI fd), are
 *    handled in a single select/poll .
 * 7) A consequence of 1,2, and 3 is that certain fds are linked; that is
 *    data read from one fd is written to another.  Requirement 3 and 6 
 *    
 *
 * These requirements suggest a handler-based implementation.  The following
 * routines are used:
 *
 * IOHandleLoop - Using the "ios" entries in the ProcessTable, construct the
 * set of active fds (note that fds may be temporarily ignored if they are
 * read fd's for which no buffer space exists or write fd's for which there
 * is not data to write).  For each active fd, the associated handler
 * routine is called; this routine is passed the fd and a void pointer to
 * extra state that was initialized when the handler was set.  The return 
 * value indicates the new state of this fd.  The values are
 *    IOHANDLER_OK_TO_READ - ready to read
 *    IOHANDLER_STALL      - don't use the fd for now (stalled on input/output)
 *    IOHANDLER_CLOSED     - fd has been closed
 *    IOHANDLER_ERR        - Error in fd (still need subcases)
 * All success values are positve; errors are negative.
 *
 * IOHandlersCloseAll - Close all of the fds associated with a process table.
 * This is necessary since a fork copies access to the fds; if the fds are
 * not closed, then the processes can become confused about which fds
 * are still in use.  In addition, if many processes are created, the
 * pool of available fds may be exhausted.
 *
 * GetPrefixFromEnv - This is a special service routine that determines
 * the annotation prefix to use on output to stdout and stderr.  
 * (The name should probably be changed.)
 *
 * The other routines in this file provide sample IO handlers (a PMI handler
 * is provided in pmiserv.c; see IOSetupPMIHandler).  These are
 *
 * IOSetupOutHandler - setup stdout/err handling, with annotation of output
 * IOSetupInHandler  - setup stdin handling, with communication to a single
 *                     process
 * IOSetupOutSimple  - A basic form of stdout/err handler that only uses the
 *                     IOHandleLoop for reading (it writes directly to 
 *                     stdout/err).  This should not be used normally, but it
 *                     is included to show a simplified IO handler; it can
 *                     also be used while debugging.
 * IOSetupInSimple   - a similar handler for input; this sends all data
 *                     to process 0.
 * 
 * Note that both IOSetupOutSimple and IOSetupInSimple defined handler that
 * can block, causing failure.  These should not be used in production 
 * applications.
 *
 * For the greatest flexibility, actions are usually performed by invoking
 * a function rather than trying to perform the action directly.  This makes
 * it easier to separate the routines that read data from one fd from the
 * routines that write that data to another fd.  In some cases, these routines
 * are very short, but since this code should not be performance-critical,
 * this gives us better control over the actions.
 *
 * The description of the specific handlers is given with the handlers.
 *
 * All routines that are handlers are declared static since they are
 * only used within this file.
 */

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

/* ------------------------------------------------------------------------- */
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
 *
 * The routines are:
 * IOSetupOutHandler - Setup the output handler
 * IOSetupInHandler  - Setup the input handler
 *
 * Internal routines are:
 * IOManyToOneReader - Used for stdout/err from the processes, this
 *   reads from the given fd, optionally calls an annotator, and 
 *   queues the data for writing.  In the mpiexec case, 
 *   this handler is attached to the stdout or stderr of the created
 *   processes.
 * IoManyToOneWriter - The counterpart of IOManyToOneReader, this
 *   handles any queued output when it is possible to write.
 * IOManyToOneNotify - Used by IOManyToOneReader to write data to the 
 *   final output.  This
 *   routine is invoked by IOManyToOneReader, and handles enqueing the
 *   data to the output fd.
 * IOManyToOneConsume - Used by IOManyToOneNotify to consume data that
 *   was read by IOManyToOneReader.
 * IOManyToOneAnnotate - Used by IOManyToOneReader to annotate the data,
 *   for example, by prefixing lines with the rank in MPI_COMM_WORLD
 *   of the process that was the source of the data.
 * IOManyToOneNotifyReader - Used by IOManyToOneWriter to inform a
 *   read fd that data has been processed.
 */

/* FIXME - These routines are not yet complete */

/* A circular buffer, used to hold a line of input */
typedef struct {
    char *first, *firstFree;   /* Pointers to the first character and the 
				  first free location in the buffer 
			          An empty buffer has first = firstFree */
    int  nleft;                /* Number of available chars from firstFree
				  to either first or end of buffer (this 
			          is the amount that we can read with 
			          a single operation) */
    char *buf;                 /* Pointer to the allocated buffer */
    int  bufSize;              /* Number of characters in the buffer */
} CircularBuffer;

/* Get readv/writev/iovec */
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

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

/* Declarations of internal routines */
static int IOManyToOneReader( int, void *);

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

   FIXME: We need to enumerate the success states.  We don't need to return
   the amount of data read, so positive values can be used for the possible
   states. 
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
    iodata->abuf.cbuf.nleft     -= n;
    iodata->abuf.cbuf.firstFree += n;
    if (iodata->abuf.cbuf.nleft == 0) {
	/* Either hit the end of the buffer or the first pointer */
	/* FIXME: handle at the first pointer */
	iodata->abuf.cbuf.firstFree = iodata->abuf.cbuf.buf;
	/* FIXME: we need to let the annotate routine know that
	   we've moved the firstFree pointer (do we, or should it
	   assume that?) */
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
	/* FIXME: what if err? */
    }
    else {
	/* No annotation routine, so just place the entire buffer 
	   in the first entry */
	iodata->abuf.iovs[0].iov_base = iodata->abuf.cbuf.first;
	iodata->abuf.iovs[0].iov_len  = iodata->abuf.cbuf.nleft;
	iodata->abuf.nIOV             = 1;
	iodata->abuf.cbufIncr[0]      = 1;
    }

    /* Pass the data on to the output by using the notify routine.
       This routine may be able to write the data directly, or it
       may have to queue the data.  The buffer is not emptied unless it
       is written */
    err = (*iodata->notify)( &iodata->abuf, iodata->notifyExtra );
    /* FIXME: err? One value is queued, not written, in which case
       we should stall input on this fd */

    /* Check the output state to see if we should suppress reading 
       from our fd until data can be accepted by the notify routine */
    /* FIXME: return state, not count */
    /* FIXME: In particular, if buffer is full, indicate that :
       IOHANDLER_BUFFER_FULL? (or IOHANDLER_STALLED)
       IOHANDLER_OK
       and negative values
       IOHANDLER_ERR_CLOSED
       IOHANDLER_ERR_MEM
    */
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
/* Should probably be maxproc */
#define MAX_ABUFS 256
/* The IONotifyData is shared between the reader and the writers.  
   FIXME: we may need to invoke a routine when an annotateBuffer is
   completely consumed, e.g., to indicate that a fd is available for
   reading again */
typedef struct {
    int outfd;
    int first, firstFree; /* head and first free of FIFO queue of pending
			     operations.  first=firstFree if queue is
			     empty */
    int maxQueue;         /* size of queue */
    AnnotatedBuffer *(abufs[MAX_ABUFS]);    /* FIFO Queue of pending
					       output data */
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
    IONotifyData *iodata = (IONotifyData *)extra;
    int          err;

    /* FIXME: First check to see if the queue of pending writes
       is empty.  If it isn't, we must not write, instead queuing 
       the data */
    if (iodata->first != iodata->firstFree) {
	/* There is data queued to write */
	/* FIXME: test for queue overflow */
	iodata->abufs[iodata->firstFree++] = abuf;
	if (iodata->firstFree >= iodata->maxQueue) iodata->firstFree = 0;
	err = 0 /* FIXME IOHANDLER_STALLED */;
    }
    else {
	err = IOManyToOneConsume( abuf, extra );
	/* FIXME: If any data is left, we need to queue it */
    }

    return err;
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
   This routine is invoked when it is possible to write on the output fd.
   Normally, this is invoked because the output fd was blocked but is
   now available for writing
 */
static int IOManyToOneWriter( int fd, void *extra ) 
{
    IONotifyData *iodata = (IONotifyData *)extra;
    int err = 0;

    while (iodata->first != iodata->firstFree) {
	err = IOManyToOneConsume( iodata->abufs[iodata->first], extra );
	if (err == 0) {
	    iodata->first = (iodata->first + 1) % iodata->maxQueue;
	    /* FIXME: mark that fd as now readable */
	}
	else {
	    /* Unable to write any more */
	    /* FIXME: indicate stalled */
	    break;
	}
    } 
    
    return 0;
}

/* ------------------------------------------------------------------------- */
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

/* Finally, here is the initialization routine */
#define MAX_CBUF_SIZE 1024
int IOSetupOutHandler( IOSpec *ios, int fdSource, int fdDest, char *leader )
{
    IOOutManyToOne *iosdata;
    
    iosdata = (IOOutManyToOne *)MPIU_Malloc( sizeof( IOOutManyToOne ) );
    /* FIXME: test iosdata */

    iosdata->abuf.cbuf.first     = 0;
    iosdata->abuf.cbuf.firstFree = 0;
    iosdata->abuf.cbuf.nleft     = MAX_CBUF_SIZE;
    iosdata->abuf.cbuf.bufSize   = MAX_CBUF_SIZE;
    iosdata->abuf.cbuf.buf       = (char *)MPIU_Malloc( MAX_CBUF_SIZE );
    /* FIXME: test buf */

    iosdata->abuf.lastSeen       = 0;

    iosdata->annotate = IOManyToOneAnnotate;
    iosdata->annotateExtra = 0;   /* FIXME */

    iosdata->notify   = IOManyToOneNotify;
    iosdata->notifyExtra = 0;     /* FIXME */

#if 0    
    iosdata->outfd     = fdDest;
    iosdata->outleader = 1;
    if (*leader) {
	iosdata->leaderlen = strlen( leader );
	if (iosdata->leaderlen > MAXLEADER) {
	    /* FIXME: error in leader */
	    ;
	}
	else {
	    MPIU_Strncpy( iosdata->leader, leader, MAXLEADER );
	}
    }
    else 
	iosdata->leaderlen = 0;

    ios->isWrite     = 0;   /* Because we are READING the stdout/err
			       of the process */
    ios->extra_state = (void *)iosdata;
    ios->handler     = IOHandleStdOut;
    ios->fd          = fdSource;
    ios->fdstate     = IO_PENDING; /* We always want to read from this process */
#endif
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

/* FIXME: These routines (and their internal versions) need to be written */
/* ----------------------------------------------------------------------- */
/* Very simple model handlers.  Note that these read and write directly
   to stdout/err/in, and hence can block under certain circumstances.  See
   the more complex (but more correct) handlers above for a better model
   of the IO handlers.  These are provided as simple examples and to 
   provide basic handlers that can be used while debugging. */
/* ----------------------------------------------------------------------- */

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

static int IOHandleStdOut( int, void * );

/* ----------------------------------------------------------------------- */
/* All too simple handler for stdout/stderr.  Note that this can block
   if the output fd is full (FIXME) 
   Return the number of bytes read, or < 0 if error. 
*/
static int IOHandleStdOut( int fd, void *extra )
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
int IOSetupOutSimple( IOSpec *ios, int fdSource, int fdDest, char *leader )
{
    IOOutData *iosdata;
    
    iosdata = (IOOutData *)MPIU_Malloc( sizeof( IOOutData ) );
    /* FIXME: test iosdata */
    iosdata->curlen    = 0;
    iosdata->outfd     = fdDest;
    iosdata->outleader = 1;
    if (*leader) {
	iosdata->leaderlen = strlen( leader );
	if (iosdata->leaderlen > MAXLEADER) {
	    /* FIXME: error in leader */
	    ;
	}
	else {
	    MPIU_Strncpy( iosdata->leader, leader, MAXLEADER );
	}
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

