/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * This program provides a simple implementation of mpiexec that creates the
 * processes using a designated remote shell program.  This is intended as an 
 * example implementation; users are encouraged to try the mpd process
 * manager.  This implementation can be used to understand how to implement a
 * custom mpiexec for a special environment, such as a third-party process
 * manager.  This example may also be appropriate for a bproc implementation
 */

#include "remshellconf.h"

#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/wait.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

/* Include the definitions of the data structures, particularly the 
   process table */
#include "remshell.h"

/* Temporary definitions for memory management */
#ifndef MPIU_Malloc
#define MPIU_Malloc(a)    malloc((unsigned)(a))
#define MPIU_Calloc(a,b)  calloc((unsigned)(a),(unsigned)(b))
#define MPIU_Free(a)      free((void *)(a))
#ifdef HAVE_STRDUP
#ifdef NEEDS_STRDUP_DECL
extern char *strdup( const char * );
#endif
#define MPIU_Strdup(a)    strdup(a)
#else
/* Don't define MPIU_Strdup, provide it in safestr.c */
#endif
#endif

ProcessTable_t processTable;

/* Eventually, we'd like to move the PMI state into a separate file, along 
   with the PMI calls */
   
/* ----------------------------------------------------------------------- */
/* PMIState                                                                */
/* Many of the objects here are preallocated rather than being dynamically */
/* allocated as needed.  This ensures that no program fails because a      */
/* malloc fails.                                                           */
/* ----------------------------------------------------------------------- */
#define MAXGROUPS   256		/* max number of groups */
#define MAXKEYLEN    64		/* max length of key in keyval space */
#define MAXVALLEN   128		/* max length of value in keyval space */
#define MAXPAIRS   1024		/* max number of pairs in a keyval space */
#define MAXKVSS      16		/* max number of keyval spaces */

/* Each group has a size, name, and supports a barrier operation that
   is implemented by counting the number in the barrier */
typedef struct {
    int  groupsize;
    int  num_in_barrier;
    char kvsname[MAXNAMELEN];
} PMIGroup;

/* key-value pairs are used to communicate information between processes. */
typedef struct {
    char key[MAXKEYLEN];
    char val[MAXVALLEN];
} PMIKeyVal;

/* There can be multiple key-value spaces, each with its own set of 
   PMIKeyVal pairs */
typedef struct {
    int active;
    char kvsname[MAXNAMELEN];
    PMIKeyVal pairs[MAXPAIRS];
} PMIKeyValSpace;

/* */
typedef struct {
    int            nextnewgroup;
    int            kvsid;
    PMIGroup       grouptable[MAXGROUPS];
    PMIKeyValSpace kvstable[MAXKVSS];
} PMIState;

static PMIState pmi = { 0, 0, };   /* Arrays are uninitialized */

/* ----------------------------------------------------------------------- */
/* Debugging                                                               */
/* ----------------------------------------------------------------------- */
/* (this is exten in remshell.h) */
int debug = 1;
/* ----------------------------------------------------------------------- */
/* Prototypes                                                              */
/* ----------------------------------------------------------------------- */
void mpiexec_usage( const char * );
int mpiexecArgs( int, char *[], ProcessTable_t *,
		 int (*)( int, char *[], void *), void * );
int mpiexecPrintTable( ProcessTable_t * );
int mpiexecChooseHosts( ProcessTable_t * );
int mpiexecGetMyHostName( char myname[MAX_HOST_NAME+1] );

/* ----------------------------------------------------------------------- */

int main( int argc, char *argv[] )
{
    int rc = 0;
    int fdPMI, portnum;             /* fd and port for PMI messages */
    char myname[MAX_HOST_NAME+1];
    processTable.maxProcesses = MAXPROCESSES;
    processTable.nProcesses   = 0;

    /* Process the command line arguments to build the table of 
       processes to create */
    rc = mpiexecArgs( argc, argv, &processTable, 0, 0 );
    if (rc) return rc;

    if (processTable.nProcesses == 0) {
	MPIU_Error_printf( "No program specified" );
	return 1;
    }

    /* Determine the hosts to run on */
    rc = mpiexecChooseHosts( &processTable );
    if (rc) {
	MPIU_Error_printf( "Unable to choose hosts" );
	return 1;
    }

    if (debug) 
	mpiexecPrintTable( &processTable );

    /* Create the PMI INET socket */
    rc = mpiexecGetPort( &fdPMI, &portnum );
    if (debug) {
	DBG_PRINTF( "rc = %d, Using port %d and fd %d\n", rc, portnum, fdPMI );
    }
    if (rc) return rc;

    /* Optionally stage the executables */
    
    /* Start the remote shell processes ooncurrently */
    rc = mpiexecGetMyHostName( myname );
    if (rc) {
	MPIU_Internal_error_printf( "Could not get my hostname\n" );
	return 1;
    }
    rc = mpiexecStartProcesses( &processTable, myname, portnum );

    /* Poll on the active FDs and handle each input type */
    rc = mpiexecPollFDs( &processTable );

    /* Clean up and determine the return code.  Log any anomolous events */
    /* FIXME: NOT DONE */
    return rc;
}

/* ----------------------------------------------------------------------- */
/* Get a port for the PMI interface                                        */
/* Ports can be allocated within a requested range using the runtime       */
/* parameter value MPIEXEC_PORTRANGE, which has the format low:high,       */
/* where both low and high are positive integers.  Unless this program is  */
/* privaledged, the numbers must be greater than 1023.                     */
/* ----------------------------------------------------------------------- */
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
/* This is really IP!? */
#ifndef TCP
#define TCP 0
#endif
int mpiexecGetPort( int *fdout, int *portout )
{
    int                fd;
    struct sockaddr_in sa;
    int                optval = 1;
    int                portnum;
    char               *range_ptr;
    int                low_port, high_port;
    
    /* Get the low and high portnumber range.  zero may be used to allow
       the system to choose */
    range_ptr = getenv( "MPIEXEC_PORTRANGE" );
    if (!range_ptr) {
	/* Under cygwin we may want to use 1024 */
	low_port  = 0;
	high_port = 0;
    }
    else {
	MPIU_Error_printf( "ranges not supported for ports\n" );
	return 1;
    }

    for (portnum=low_port; portnum<=high_port; portnum++) {
	bzero( (void *)&sa, sizeof(sa) );
	sa.sin_family	   = AF_INET;
	sa.sin_port	   = htons( portnum );
	sa.sin_addr.s_addr = INADDR_ANY;
    
	fd = socket( AF_INET, SOCK_STREAM, TCP );
	if (fd < 0) {
	    return fd;
	}
    
	if (setsockopt( fd, IPPROTO_TCP, TCP_NODELAY, 
		    (char *)&optval, sizeof(optval) )) {
	    perror( "Error calling setsockopt:" );
	}
	
	if (bind( fd, (struct sockaddr *)&sa, sizeof(sa) ) < 0) {
	    close( fd );
	    if (errno != EADDRINUSE && errno != EADDRNOTAVAIL) {
		return -1;
	    }
	}
	else {
	    /* Success! We have a port.  */
	    break;
	}
    }
    
    /* Listen is a non-blocking call that enables connections */
#define MAX_PENDING_CONN 10
    listen( fd, MAX_PENDING_CONN );
    
    *fdout = fd;
    if (portnum == 0) {
	int sinlen = sizeof(sa);
	/* We asked for *any* port, so we need to find which
	   port we actually got */
	getsockname( fd, (struct sockaddr *)&sa, &sinlen );
	portnum = ntohs(sa.sin_port);
    }
    *portout = portnum;
    
    return 0;
}

#include <netdb.h>
int mpiexecGetMyHostName( char myname[MAX_HOST_NAME+1] )
{
    struct hostent     *hp;
    char *hostname = 0;
    /*
     * First, get network name if necessary
     */
    if (!hostname) {
	hostname = myname;
	gethostname( myname, MAX_HOST_NAME );
    }
    hp = gethostbyname( hostname );
    if (!hp) {
	return -1;
    }
    return 0;
}


/* ----------------------------------------------------------------------- */
/* Usage message                                                           */
/* ----------------------------------------------------------------------- */
void mpiexec_usage( const char *msg )
{
    if (msg) 
	MPIU_Error_printf( msg );
    MPIU_Usage_printf( "\
Usage: mpiexec -n <numprocs> -soft <softness> -host <hostname> \n\
               -wdir <working directory> -path <search path> \n\
               program args [ : -n <numprocs> ... program args ]\n" );
    exit( -1 );
}

/* ----------------------------------------------------------------------- */
/* Spawn processes                                                         */
/* This concurrently starts the processes specified in the process table.  */
/* stdin/out/err are redirected so that we can explicitly manage them.     */
/* Process are also started without waiting for any handshake.             */
/* ----------------------------------------------------------------------- */
int mpiexecStartProcesses( ProcessTable_t *ptable, char myname[], int port )
{
    int i;
    int pid;
    char port_as_string[1024];

    /* All processes use the same connection port back to the master 
       process */
    snprintf( port_as_string, 1024, "%s:%d", myname, port );

    for (i=0; i<ptable->nProcesses; i++) {
	int read_out_pipe[2], write_in_pipe[2], read_err_pipe[2];
	ProcessState *ps;
	char **myargv;
	int rshNargs, j, rc;

	ps = &ptable->table[i];

	/* Build the array to pass to exec before calling fork.  
	   This makes it easier to generate good error messages for the
	   user, and makes it easier to debug the code that creates the
	   argument vector */

	/* Note that each process has its OWN myargv */
	myargv = (char **)MPIU_Malloc( (ps->nArgs + 30) * sizeof(char *) );
	/* FIXME: remote shell command may have multiple args */
	rshNargs = mpiexecGetRemshellArgv( myargv, 30 );
	/* Look for %h (hostname) and %e (executable).  If not
	   found, then use the following.  
	   FIXME: Assume no %h and %e */
	myargv[rshNargs++] = (char *)(ps->hostname);
	/* FIXME: no option for user name (e.g., -l username) */
	/* FIXME: Do we start with -n, or do we let mpiexec handle that? */
	/* myargv[rshNargs++] = "-n"; */
	/* FIXME: this assumes a particular shell syntax (csh) */
	myargv[rshNargs++] = "setenv";
	myargv[rshNargs++] = "MPIEXEC_PORT";
	myargv[rshNargs++] = port_as_string;
	myargv[rshNargs++] = "\\;";
	myargv[rshNargs++] = (char *)(ps->exename);
	for (j=0; j<ps->nArgs; j++) 
	    myargv[rshNargs++] = (char *)(ps->args[j]);
	myargv[rshNargs++] = 0;
#define DEBUG
#ifdef DEBUG
	{ int k;
	  for (k=0; myargv[k]; k++) {
	      DBG_PRINTF( "%s ", myargv[k] );
	  }
	  DBG_PRINTF( "\n" );
	}
#endif
	
	/* Create the pipes for the stdin/out/err replacements */
	if (pipe(read_out_pipe)) return -1;
	if (pipe(write_in_pipe)) return -1;
	if (pipe(read_err_pipe)) return -1;

	if (debug) {
	    DBG_PRINTF( "About to fork process %d\n", i );
	}

	pid = fork();
	if (pid > 0) {
	    /* We are (and remain) the parent */
	    /* Close unused portion of pipes */
	    close(read_out_pipe[1]); 
	    close(read_err_pipe[1]);
	    close(write_in_pipe[0]);
 
	    /* Save the pipes that we will use to access the stdin/out/err
	       of the child */
	    ps->fdStdout = read_out_pipe[0];
	    ps->fdStderr = read_err_pipe[0];
	    ps->fdStdin  = write_in_pipe[1];
	}
	else if (pid == 0) {
	    char **myargv;
	    char port_as_string[1024];
	    int rshNargs, j, rc;

	    /* FIXME: we need to close the fdPMI */
	    close(read_out_pipe[0]);
	    close(write_in_pipe[1]);
	    close(read_err_pipe[0]);
	    
	    /* STDERR */
	    close(2);               
	    dup2(read_err_pipe[1],2);
	    close(read_err_pipe[1]); 

	    /* STDOUT */
	    close(1);               
	    dup2(read_out_pipe[1],1);
	    close(read_out_pipe[1]); 

	    /* STDIN */
	    close(0);                
	    dup2(write_in_pipe[0],0);
	    close(write_in_pipe[0]); 

	    rc = execvp( myargv[0], myargv );
	    if (rc) {
		MPIU_Error_printf( "Error from execvp: %d\n", errno );
	    }
	    /* should never return */
	    exit(-1);
	}
	else {
	    /* Error on fork */
	    /* FIXME */
	    MPIU_Error_printf( "Failure to create process number %d\n", i );
	    return -1;
	}
    }
}

/* ----------------------------------------------------------------------- */
/* Convert the remote shell command into argv format                       */
/* The command may be specified as a string with blanks separating the     */
/* arguments, either from the default, an environment variable, or         */
/* eventually the machines file (allowing different options for each host  */
/* or the command line.                                                    */
/* Returns the number of arguments                                         */
/* For example, this allows "ssh -2" as a command                          */
/* ----------------------------------------------------------------------- */
const char defaultRemoteshell[] = DEFAULT_REMOTE_SHELL;

int mpiexecGetRemshellArgv( char *argv[], int nargv )
{
    static char *(remshell[10]);
    static int  remargs = 0;
    int i;

    /* Convert the string for the remote shell command into an argv list */
    if (!remargs) {
	const char *rem = getenv( "MPIEXEC_REMSHELL" );
	char *next_parm;
	if (!rem) rem = defaultRemoteshell;
	
	/* Unpack the string into remshell.  Allow 10 tokens */
	while (rem) {
	    int len;
	    next_parm = strchr( rem, ' ' );
	    if (next_parm) 
		len = next_parm - rem;
	    else 
		len = strlen(rem);

	    remshell[remargs] = (char *)MPIU_Malloc( len + 1 ); /* Add the null */
	    MPIU_Strncpy( remshell[remargs], rem, len );
	    remshell[remargs][len] = 0;
	    remargs++;
	    if (next_parm) {
		rem = next_parm + 1;
		while (*rem == ' ') rem++;
		if (remargs == 10) {
		    /* FIXME */
		    MPIU_Error_printf( "Remote shell command is too complex\n" );
		    exit(1);
		}
	    }
	    else {
		rem = 0;
	    }
	}
    }

    /* remshell comtains the command.  Copy into argv and return the
       number of args.  We just copy *pointers* because any variable 
       fields will be replaced by the other commands */
    for (i=0; i<remargs; i++) 
	argv[i] = remshell[i];
    return remargs;
}
/* ----------------------------------------------------------------------- */
/* Useful debugging routines                                               */
/* ----------------------------------------------------------------------- */
int mpiexecPrintTable( ProcessTable_t *ptable )
{
    int i, j;

    for (i=0; i<ptable->nProcesses; i++) {
	ProcessState *ps = &ptable->table[i];
	DBG_PRINTF( "On %s run %s", ps->hostname, ps->exename );
	for (j=0; j<ps->nArgs; j++) {
	    DBG_PRINTF( " %s", ps->args[j] );
	}
	DBG_PRINTF( "\n" );
    }
}

/* ----------------------------------------------------------------------- */
/* Process any pending input from either the user or from the processes.   
   This simple mpiexec does not use any kind of aggregation tree, either 
   for creating processes or for handling output.  (In such cases,
   a different mpiexec and process manager should be used, such as mpd.

   Outline:
   for all fds, wait for poll (or select) to indicate that data is 
   available.  As a special case, don't accept data with the sink for the
   data is not available.  I.e., if stderr is not available for writing, 
   do not accept data from the fdStderr fd of any created process.
/* ----------------------------------------------------------------------- */
#include <sys/poll.h>

int mpiexecPollFDs( ProcessTable_t *ptable )
{
    struct pollfd *pollarray;
    int i, j, nfds, nprocess;

    /* Compute the array size needed for the poll operation */
    nprocess = ptable->nProcesses;
    nfds = 4 * nprocess + 3;
    pollarray = (struct pollfd *)MPIU_Malloc( nfds * sizeof(struct pollfd) );
    if (!pollarray) {
	MPIU_Internal_error_printf( "Unable to allocate poll array of size %d",
				    ptable->nProcesses );
	return 1;
    }

    /* Fill to poll array.  We'll exploit the fact that there are four
       fd's per process 
       Question: should we arrange these differently to make it easier to
       process any events?
    */
    j = 0;
    for (i=0; i<nprocess; i++) {
	pollarray[j].fd = ptable->table[i].fdStdin;
	pollarray[j].events = POLLOUT;
	j++;
	pollarray[j].fd = ptable->table[i].fdStdout;
	pollarray[j].events = POLLIN;
	j++;
	pollarray[j].fd = ptable->table[i].fdStderr;
	pollarray[j].events = POLLIN;
	j++;
	pollarray[j].fd = ptable->table[i].fdPMI;
	pollarray[j].events = POLLIN;
	j++;
    }
    /* Add the last three */
    pollarray[j].fd = 0; 
    pollarray[j].events = POLLIN;
    j++;
    pollarray[j].fd = 1; 
    pollarray[j].events = POLLOUT;
    j++;
    pollarray[j].fd = 2;
    pollarray[j].events = POLLOUT;

    while (1) {
	int timeout, rc;
	/* Compute the timeout until we must abort this run */
	/* (A negative value is infinite) */
	rc = poll( pollarray, nfds, timeout );

	/* loop through the entries, looking at the processes first */
	j = 0;
	for (i=0; i<nprocess; i++) {
	    if (pollarray[j].revents == POLLOUT) {
		;
	    }
	    
	}

	/* If the mpiexec stdout or stdin becomes full, turn off 
	   the check for data available from the processes on 
	   those fds */
	
	/* Similarly, do the same if the designated stdin process
	   is not accepting input (don't read any more) */
	
    }
}

/*
 * Provide a simple timeout capability.  Initialize the time with 
 * InitTimeout.  Call GetRemainingTime to get the time in seconds left.
 */
int end_time = -1;  /* Time of timeout in seconds */
void InitTimeout( int seconds )
{
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

/* Return remaining time in seconds */
int GetRemainingTime( void )
{
    int time_left;
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
    if (time_left < 0) time_left = 0;
    return time_left;
}
