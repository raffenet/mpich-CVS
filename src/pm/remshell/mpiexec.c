/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* OWNER=gropp */
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

/* Include mpi error definitions */
#include "mpierrs.h"

#ifdef HAVE_SNPRINTF
#define MPIU_Snprintf snprintf
#endif

/* Debug definitions */
#define DEBUG_ARGS

ProcessTable_t processTable;

/* ----------------------------------------------------------------------- */
/* Debugging                                                               */
/* ----------------------------------------------------------------------- */
/* (this is extern in remshell.h) */
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
int mpiexecGetPort( int *, int * );
int mpiexecStartProcesses( ProcessTable_t *, char [], int );
int mpiexecGetRemshellArgv( char *[], int );
int mpiexecPollFDs( ProcessTable_t *, int );
int mpiexecPrintTable( ProcessTable_t * );

/* ----------------------------------------------------------------------- */

int main( int argc, char *argv[] )
{
    int rc = 0;
    int fdPMI=-1, portnum;             /* fd and port for PMI messages */
    char myname[MAX_HOST_NAME+1];

    processTable.maxProcesses	 = MAXPROCESSES;
    processTable.nProcesses	 = 0;
    processTable.nActive	 = 0;
    processTable.timeout_seconds = -1;

    /* Process the command line arguments to build the table of 
       processes to create */
    rc = mpiexecArgs( argc, argv, &processTable, 0, 0 );
    if (rc) return rc;

    if (processTable.nProcesses == 0) {
	MPIU_Error_printf( "No program specified\n" );
	return 1;
    }

    /* Initialiaze the timeout handling (get the current time and remember
       the timelimit) */
    InitTimeout( processTable.timeout_seconds );

    /* Determine the hosts to run on */
    rc = mpiexecChooseHosts( &processTable );
    if (rc) {
	MPIU_Error_printf( "Unable to choose hosts\n" );
	return 1;
    }

    if (debug) 
	mpiexecPrintTable( &processTable );

    /* Create the PMI INET socket.
       The simple PMI implementation reads and writes to an fd; this 
       fd may be a pipe or a socket (this mpiexec program uses a socket) */
    rc = mpiexecGetPort( &fdPMI, &portnum );
    if (debug) {
	DBG_PRINTF( "rc = %d, Using port %d and fd %d\n", rc, portnum, fdPMI );
    }
    if (rc) return rc;

    /* Optionally stage the executables */
#ifdef USE_MPI_STAGE_EXECUTABLES
    if (stageExes) {
	mpiexecStageExes( &processTable );
    }
#endif    

    /* Get ths host name for the PMI socket contact address (combine 
       with the port number above) */
    rc = mpiexecGetMyHostName( myname );
    if (rc) {
	MPIU_Internal_error_printf( "Could not get my hostname\n" );
	return 1;
    }

    /* Pass the process table to the file that contains the SIGCHLD handler.
       It will use this to process any child that exits */
    initPtableForSigchild( &processTable );

    /* Start the remote shell processes ooncurrently */
    rc = mpiexecStartProcesses( &processTable, myname, portnum );
    if (rc) {
	MPIU_Error_printf( "Failure while creating processes" );
    }

    /* Poll on the active FDs and handle each input type */
    rc = mpiexecPollFDs( &processTable, fdPMI );

    /* Clean up and determine the return code.  Log any anomolous events */
    rc = mpiexecEndAll( &processTable );

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
	/* Under cygwin we may want to use 1024 as a low port number */
	/* a low and high port of zero allows the system to choose 
	   the port value */
	low_port  = 0;
	high_port = 0;
    }
    else {
	/* FIXME: Look for n:m format */
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
#define MAX_ID_STR 10
#define MAX_PORT_STR 1024
int mpiexecStartProcesses( ProcessTable_t *ptable, char myname[], int port )
{
    int i;
    int pid;
    char port_as_string[MAX_PORT_STR];
    char id_as_string[MAX_ID_STR];

    /* All processes use the same connection port back to the master 
       process */
    snprintf( port_as_string, 1024, "%s:%d", myname, port );

    PMIServInit( ptable->nProcesses );
    for (i=0; i<ptable->nProcesses; i++) {
	int read_out_pipe[2], write_in_pipe[2], read_err_pipe[2],
	    pmi_pipe[2];
	int useRemshell = 1;
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

	/* Allow the special case of either local host or this host.
	   If selected, just fork to create the process.  This allows
	   us to bypass the potentially expensive step of invoking a 
	   remote shell program and also allows most of this code to
	   be tested with with local processes */
	if (strcmp(ps->hostname,myname) == 0 ||
	    strcmp(ps->hostname,"localhost") == 0) {
	    if (debug) {
		DBG_PRINTF( "Using fork instead of remote shell\n" );
	    }
	    useRemshell = 0;
	}
	if (useRemshell) {
	    /* Because the remote shell program may have multiple arguments,
	       we use a routine to update myargv */
	    rshNargs = mpiexecGetRemshellArgv( myargv, 30 );
	    /* Look for %h (hostname) and %e (executable).  If not
	       found, then use the following.  
	       FIXME: Assume no %h and %e.  Further, assume 
	       executable at the end */
	    myargv[rshNargs++] = (char *)(ps->hostname);
	    /* FIXME: no option for user name (e.g., -l username) */
	    /* FIXME: Do we start with -n, or do we let mpiexec handle that? */
	    /* myargv[rshNargs++] = "-n"; */
	    /* FIXME: this assumes a particular shell syntax (csh) */
	    /* FIXME: Some (all) GNU versions of remote shell are broken 
	       because then interpret all arguments as intended for 
	       the remote shell program, including those for the 
	       remote command.  We may need to either escape any user
	       argument that begins with a dash (e.g., \- instead of -)
	       for this kind of remshell program; this must be a runtime
	       decision so that we can all alternate choices of remote shell
	       program */
	    myargv[rshNargs++] = "setenv";
	    myargv[rshNargs++] = "PMI_PORT";
	    myargv[rshNargs++] = port_as_string;
	    myargv[rshNargs++] = ";";
	    myargv[rshNargs++] = "setenv";
	    myargv[rshNargs++] = "PMI_ID";
	    MPIU_Snprintf( id_as_string, MAX_ID_STR, "%d", i );
	    myargv[rshNargs++] = id_as_string;
	    myargv[rshNargs++] = ";";
	}
	else {
	    rshNargs = 0;
	    socketpair( AF_UNIX, SOCK_STREAM, 0, pmi_pipe );
	}
	myargv[rshNargs++] = (char *)(ps->exename);
	/* Some broken versions of remote shell programs look at options
	   passed to the *program*!  Unfortunately, the default inetutils
	   rsh command, used in many Linux distributions, has this bug.
	   To fix this, we'll need to consider escaping some of the 
	   arguments to keep the broken remshell program from seeing them. */
	for (j=0; j<ps->nArgs; j++) 
	    myargv[rshNargs++] = (char *)(ps->args[j]);
	myargv[rshNargs++] = 0;
#ifdef DEBUG_ARGS
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
	    /* Increment the number of created processes */
	    ptable->nActive++;
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

	    /* We could add an option to use a pipe for this one when
	       we don't use a remote shell */
	    if (useRemshell) {
		ps->fdPMI    = -1;   /* No socket until attached */
	    }
	    else {
		ps->fdPMI    = pmi_pipe[0];
		/* register this process in the PMI group */
		/* FIXME: Adds to group 0 only */
		PMIServAddtoGroup( 0, i, pid, ps->fdPMI );
		PMIServSetupEntry( ps->fdPMI, 0, ptable->nProcesses, i, 
				   &ps->pmientry );
		close( pmi_pipe[1] );
	    }

	    ps->pid      = pid;
	    ps->state    = UNINITIALIZED;

	    /* We add this process to the PMI process group when it checks 
	       in */
	}
	else if (pid == 0) {
	    /* FIXME: we need to close the fdPMI */
	    close(read_out_pipe[0]);
	    close(write_in_pipe[1]);
	    close(read_err_pipe[0]);

	    /* Redirect stdin/out/err to pipes that mpiexec will handle
	       (reading from stdout/err and writing to stdin) */
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

	    if (!useRemshell) {
		/* Use environment variables to pass information to the
		   process, including the fd used for the PMI calls.
		   Note that putenv retains the argument passed into it
		   (it becomes part of the environment), so separate 
		   strings must be allocated for each process and for each
		   environment variable */
		char envvar1[65], envvar2[65], envvar3[65];

		MPIU_Snprintf( envvar1, 64, "PMI_RANK=%d", i );
		if (putenv( envvar1 )) {
		    MPIU_Internal_error_printf( "Could not set environment PMI_RANK" );	    }
		MPIU_Snprintf( envvar2, 64, "PMI_SIZE=%d", ptable->nProcesses );
		if (putenv( envvar2 )) {
		    MPIU_Internal_error_printf( "Could not set environment PMI_SIZE" );
		}
		MPIU_Snprintf( envvar3, 64, "PMI_FD=%d", pmi_pipe[1] );
		if (putenv( envvar3 )) {
		    MPIU_Internal_error_printf( "Could not set environment PMI_FD" );
		}
		close( pmi_pipe[0] );
		/* Could also set PMI_DEBUG */
	    }
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
    return 0;
}

/* ----------------------------------------------------------------------- */
/* Convert the remote shell command into argv format                       */
/* The command may be specified as a string with blanks separating the     */
/* arguments, either from the default, an environment variable, or         */
/* eventually the machines file (allowing different options for each host  */
/* or the command line.                                                    */
/* Returns the number of arguments                                         */
/* For example, this allows "ssh -2" as a command                          */
/* Allow the environment variable MPIEXEC_REMSHELL to set the remote shell */
/* program to use                                                          */
/* ----------------------------------------------------------------------- */
const char defaultRemoteshell[] = DEFAULT_REMOTE_SHELL;

#define MAX_REMSHELL_ARGS 10
int mpiexecGetRemshellArgv( char *argv[], int nargv )
{
    static char *(remshell[MAX_REMSHELL_ARGS]);
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

	    remshell[remargs] = (char *)MPIU_Malloc( len + 1 ); 
	    MPIU_Strncpy( remshell[remargs], rem, len );
	    remshell[remargs][len] = 0;
	    remargs++;
	    if (next_parm) {
		rem = next_parm + 1;
		while (*rem == ' ') rem++;
		if (remargs >= MAX_REMSHELL_ARGS) {
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
    return 0;
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

   Design:
   For each entry in the pollarray (the array passed to the poll system 
   call), there is a parallel array that describes what to do when there 
   is activity on that socket.  The entries in this array are structures
   that contain the fd, a function used to process the fd, a pointer to the
   controlling process, and the state of the fd.

   A major challenge is what to do when a fd closes.  The algorithm is
   this
       If the process has not indicated that it is exiting, mark the
       process as exiting.  Note unannounced termination
       
       Close the fd

   Process all pending fds from the poll request.

   If any fds closed, 
       Decide if all processes should be signalled and killed
           (We may want to establish a small timeout window here in
	   which processes are allowed to exit)
   
       Recreate the pollarray from the remaining fds, if desired.
       
   Old design:
   The old design tried to exploit the fact that the fd's came in groups
   of 4 for most processes.  The key problem was "most"; handling some
   of the special cases made the code overly complex.  By using an extra
   level of indirection as above, we avoid these problems.

   We use the following mapping from the process table to the array
   of poll fds:
   
   For process i
   4*i   = processes stdin
   4*i+1 = processes stdout
   4*i+2 = processes stderr
   4*i+3 = processes pmi

   The last 4 fds are for mpiexec itself.  To make these eaiser to find,
   the index "mpiexecFdIdx gives the first of these:
   mpiexecFdIdx    = mpiexec stdin
   mpiexecFdIdx +1 = mpiexec stdout
   mpiexecFdIdx +2 = mpiexec stderr
   mpiexecFdIdx +3 = mpiexec pmi connection port

   An alternate approach is to attach to each fd a handler function.
   
   Another alternative is to keep an array that maps from the pollarray 
   (in groups of 4) to the process table.  This would allow us to compress
   the pollarray as processes are removed.
*/   
/* ----------------------------------------------------------------------- */
#include <sys/poll.h>

typedef struct {
    int fd;            /* Corresponding fd */
    int processIdx;    /* Index of process in ptable; -1 for mpiexec */
    int (*handleIO)( int, int, void * );
    void *extraData;   /* Passed to the handleIO function */
    int state; } fdHandle_t;

/* Handler function prototypes */
int mpiexecStdin( int fd, int idx, void *extra );
int mpiexecStdout( int fd, int idx, void *extra );
int mpiexecStderr( int fd, int idx, void *extra );
int mpiexecGetPMIsock( int, int, void * );
int mpiexecHandleOutput( int, int, void * );
int mpiexecHandleStdin( int, int, void * );

int mpiexecPollFDs( ProcessTable_t *ptable, int fdPMI )
{
    struct pollfd *pollarray;
    int i, j, nfds, nprocess;
    int resetPollarray = 1;
    int activeNfds = 0;
    fdHandle_t *handlearray;

    /* Compute the array size needed for the poll operation */
    nprocess = ptable->nProcesses;
    nfds     = 4 * nprocess + 4;
    pollarray = (struct pollfd *)MPIU_Malloc( nfds * sizeof(struct pollfd) );
    if (!pollarray) {
	MPIU_Internal_error_printf( "Unable to allocate poll array of size %d",
				    ptable->nProcesses );
	return 1;
    }
    handlearray = (fdHandle_t *)MPIU_Malloc( nfds * sizeof(fdHandle_t) );
    if (!handlearray) {
	MPIU_Internal_error_printf( "Unable to allocate fdhandle array of size %d",
				    ptable->nProcesses );
	return 1;
    }

    /* Loop until we exit or timeout */
    while (1) {
	int timeout, rc;

	/* Compute the timeout until we must abort this run */
	/* (A negative value is infinite) */
	timeout = GetRemainingTime();

	DBG_PRINTF( "About to poll...\n" ); fflush(stdout);
	/* 
	 * Fill in poll array.  Initialize all of the fds.
	 */
	if (resetPollarray) {
	    activeNfds = mpiexecSetupPollArray( ptable, 
						pollarray, handlearray, fdPMI );
	    resetPollarray = 0;
	    /* If only the mpiexec fds are set, exit */
	    DBG_PRINTF( "activeNfds = %d\n", activeNfds ) ; fflush(stdout); 
	    if (activeNfds == 1) {
		DBG_PRINTF( "fd left = %d\n", pollarray[0].fd );
	    }
	    /* FIXME: if all processes have exited, then we can close the
	       PMIServer socket.  Need to think about the state machine
	       for processes.  Should a PMI_FINALIZED return be handled?
	       In general, should process returns from the handlers
	       including PMI_UPDATETABLE */
	    if (activeNfds <= 0) break;
	}

	/* Exit if no active processes */
	if (ptable->nActive == 0) break;

	/* poll's timeout is in milliseconds */
	timeout = timeout * 1000;
	do {
	    rc = poll( pollarray, activeNfds, timeout );
	} while (rc == -1 && errno == EINTR);

	/* rc = 0 is a timeout, with nothing read */
	if (rc == 0) {
	    DBG_PRINTF( "rc = 0, timeout = %d\n", timeout );
	    break;
	}

	/* loop through the entries */
	for (j=0; j<activeNfds; j++) {
	    if (pollarray[j].revents & (POLLOUT | POLLIN)) {
		rc = handlearray[j].handleIO( handlearray[j].fd, 
					      handlearray[j].processIdx,
					      handlearray[j].extraData );
		if (rc != 0) {
		    /* State change */
		    handlearray[j].state = rc;
		    resetPollarray = 1;
		}
		/* FIXME use rc to update the state */
	    }
	    else if (pollarray[j].revents & (POLLERR | POLLHUP | POLLNVAL) ) {
		/* Error condition.  Likely that the socket 
		   has disconnected.  Look at the client state to 
		   understand how to handle */
		rc = mpiexecCloseProcess( handlearray[j].fd, 
					  handlearray[j].processIdx,
					  ptable );
		resetPollarray = 1;
	    }
	}

	/* If the mpiexec stdout or stdin becomes full, turn off 
	   the check for data available from the processes on 
	   those fds */
	
	/* Similarly, do the same if the designated stdin process
	   is not accepting input (don't read any more) */
	
    }
    DBG_PRINTF( "Exiting poll...\n" ); fflush(stdout);
    return 0;
}

/* This routine sets up the pollarray, given the process table. */
int mpiexecSetupPollArray( ProcessTable_t *ptable, struct pollfd pollarray[], 
			   fdHandle_t handlearray[], int fdPMI )
{
    int j, i;
    int nprocess = ptable->nProcesses;

    /* 
     * Fill in poll array.  Initialize all of the fds.
     */
    j = 0;

    /* Start with the four fd's used by mpiexec to connect to 
       stdin/out/err and the socket to create connections on */
#if 0
/* Debug-ignore for now, just listen to the process */
    /* stdin FROM the environment (e.g., term, shell) */
    pollarray[j].fd	      = 0; 
    pollarray[j].events	      = POLLIN;
    handlearray[j].fd	      = 0;
    handlearray[j].processIdx = -1;
    handlearray[j].handleIO   = mpiexecStdin;
    handlearray[j].extraData  = ptable;
    handlearray[j].state      = 0;
    j++;
    /* stdout TO the environment (e.g., term, shell) */
    pollarray[j].fd = 1; 
    pollarray[j].events = POLLOUT;
    handlearray[j].fd	      = 1;
    handlearray[j].processIdx = -1;
    handlearray[j].handleIO   = mpiexecStdout;
    handlearray[j].extraData  = ptable;
    handlearray[j].state      = 0;
    j++;
    /* stderr TO the environment (e.g., term, shell) */
    pollarray[j].fd = 2; 
    pollarray[j].events = POLLOUT;
    handlearray[j].fd	      = 2;
    handlearray[j].processIdx = -1;
    handlearray[j].handleIO   = mpiexecStderr;
    handlearray[j].extraData  = ptable;
    handlearray[j].state      = 0;
    j++;
#endif
    /* socket used to create PMI sockets (not the pmi socket for each 
       process; those are in the process table below) */
    pollarray[j].fd	      = fdPMI;
    pollarray[j].events	      = POLLIN;
    handlearray[j].fd	      = fdPMI;
    handlearray[j].processIdx = -1;
    handlearray[j].handleIO   = mpiexecGetPMIsock;
    handlearray[j].extraData  = ptable;
    handlearray[j].state      = 0;
    j++;

    /* Only include fd's that are positive; fds for closed pipes/sockets
       are set to -1 */
    for (i=0; i<nprocess; i++) {
#if 0
	/* Stdin TO the process (if enabled) */
	if (ptable->table[i].fdStdin >= 0) {
	    pollarray[j].fd	      = ptable->table[i].fdStdin;
	    pollarray[j].events	      = POLLOUT;
	    handlearray[j].fd	      = pollarray[j].fd;
	    handlearray[j].processIdx = i;
	    handlearray[j].handleIO   = mpiexecHandleStdin;
	    handlearray[j].extraData  = &ptable->table[i].stdinBuf;
	    handlearray[j].state      = 0;
	    j++;
	}
#endif
	/* Stdout FROM the process */
	if (ptable->table[i].fdStdout >= 0) {
	    pollarray[j].fd	      = ptable->table[i].fdStdout;
	    pollarray[j].events	      = POLLIN;
	    handlearray[j].fd	      = pollarray[j].fd;
	    handlearray[j].processIdx = i;
	    handlearray[j].handleIO   = mpiexecHandleOutput;
	    handlearray[j].extraData  = &ptable->table[i].stdoutBuf;
	    handlearray[j].state      = 0;
	    j++;
	}
	/* Stderr FROM the process */
	if (ptable->table[i].fdStderr >= 0) {
	    pollarray[j].fd	      = ptable->table[i].fdStderr;
	    pollarray[j].events	      = POLLIN;
	    handlearray[j].fd	      = pollarray[j].fd;
	    handlearray[j].processIdx = i;
	    handlearray[j].handleIO   = mpiexecHandleOutput;
	    handlearray[j].extraData  = &ptable->table[i].stderrBuf;
	    handlearray[j].state      = 0;
	    j++;
	}

	/* PMI requests from process */
	if (ptable->table[i].fdPMI >= 0) {
	    pollarray[j].fd	      = ptable->table[i].fdPMI;
	    pollarray[j].events	      = POLLIN;
	    handlearray[j].fd	      = pollarray[j].fd;
	    handlearray[j].processIdx = i;
	    handlearray[j].handleIO   = PMIServHandleInputFd;
	    handlearray[j].extraData  = &ptable->table[i].pmientry;
	    handlearray[j].state      = 0;
	    j++;
	}

    }

    return j;
}

/* ------------------------------------------------------------------------ */
/* On some systems (SGI IRIX 6), process exit sometimes kills all processes
 * in the process GROUP.  This code attempts to fix that.  
 * We DON'T do it if stdin (0) is connected to a terminal, because that
 * disconnects the process from the terminal.
 */
/* ------------------------------------------------------------------------ */
void CreateNewSession( void )
{
#if defined(HAVE_SETSID) && defined(HAVE_ISATTY) && defined(USE_NEW_SESSION) &&\
    defined(HAVE_GETSID)
if (!isatty(0) && getsid(0) != getpid()) {
    pid_t rc;
    rc = setsid();
    if (rc < 0) {
	MPIU_Internal_error_printf( "Could not create new process group\n" );
	}
    }
#endif
}

/* ------------------------------------------------------------------------ */
/* These routines handle the stdin/err/out for the mpiexec process.
   To start with, these are empty (we'll make the routines that talk to 
   the processes write directly 
*/
int mpiexecStdin( int fd, int idx, void *extra )
{
    return 0;
}
int mpiexecStdout( int fd, int idx, void *extra )
{
    return 0;
}
int mpiexecStderr( int fd, int idx, void *extra )
{
    return 0;
}
/* ------------------------------------------------------------------------ */
/*
 * Handle the stdin, stdout, stderr.  This is very simple, with 
 * any remaining data moved to the beginning of the buffer.  A 
 * more sophisticated system could use a circular buffer, though that makes
 * the code to read/write more complex.
 */

/* Read from the fd and write to stdout */
int mpiexecHandleOutput( int fd, int pidx, void *extra )
{
    int n, nout;
    charbuf *buf = (charbuf *)extra;

    DBG_PRINTF( "Reading from %d to %d\n", fd, buf->destfd );
    buf->destfd = 1;
    buf->firstchar = buf->buf;
    buf->nleft = MAXCHARBUF;
    /* Restart any interrupted system call */
    do {
	n = read( fd, buf->firstchar, buf->nleft );
    }
    while (n < 0 && errno == EINTR);
    DBG_PRINTF ("Read %d chars\n", n );
    if (n >= 0) {
	buf->nleft -= n;
	/* Try to write to outfd */
	nout = write( buf->destfd, buf->buf, MAXCHARBUF - buf->nleft );
	
	if (nout >= 0) {
	    if (nout == MAXCHARBUF - buf->nleft ) {
		/* Everything was sent */
	    }
	    else {
		/* Move the data to the front of the buffer */
		/* FIXME */
	    }
	}
	else {
	    /* Unless EAGAIN we have a problem */
	}
    }
    else {
	perror( "Error from read: " );
    }
    return 0;
}
int mpiexecHandleStdin( int fd, int pidx, void *extra )
{
    int n;
    charbuf *buf = (charbuf *)extra;

    /* Attempt to write the awaiting data to the fd */
    
    /* Restart any interrupted system call */
    do {
	n = write( fd, buf->firstchar, buf->nleft );
    }
    while (n < 0 && errno == EINTR);

    if (n >= 0) {
	buf->firstchar += n;
	buf->nleft -= n;
	if (buf->nleft == 0) {
	    /* reset */
	    buf->firstchar = buf->buf;
	}
    }
    else {
	/* Error writing.  EAGAIN is ok; others are real errors */
    }
    return 0;
}

/* Respond to a connection request by 
   creating a new socket.
   Finding the corresponding process in the process table
   Initializing the table entry and sending the 
   startup handshake
 */
int mpiexecGetPMIsock( int fd, int pidx, void *extra )
{
    int    newfd;
    struct sockaddr sock;
    int    addrlen;
    int    id;
    ProcessTable_t *ptable = (ProcessTable_t *)extra;

    /*  */
    DBG_PRINTF( "Beginning accept on %d\n", fd ); fflush(stdout);
 
    /* Get the new socket */
    newfd = accept( fd, &sock, &addrlen );
    if (newfd < 0) return newfd;

    /* */
    DBG_PRINTF( "accept succeeded with fd %d\n", newfd ); fflush(stdout);

#ifdef FOO
    /* Mark this fd as non-blocking */
    flags = fcntl( newfd, F_GETFL, 0 );
    if (flags >= 0) {
	flags |= O_NDELAY;
	fcntl( newfd, F_SETFL, flags );
    }
#endif

    /* Find the matching process.  Do this by reading from the socket and 
       getting the id value that the process was created with. */
    id = PMI_Init_port_connection( newfd );
    if (id >= 0) {
	/* find the matching entry */
	ProcessState *ps = &ptable->table[id];
	
	ps->fdPMI = newfd;

	/* Now, initialize the connection */
	PMIServAddtoGroup( 0, id, ps->pid, ps->fdPMI );
	PMIServSetupEntry( newfd, 0, ptable->nProcesses, id, 
				   &ps->pmientry );
	PMI_Init_remote_proc( newfd, &ps->pmientry,
			      id, ptable->nProcesses, debug );

    }

    /* Return 1 to indicate that we need to recompute the 
       poll array of active fd's */
    return 1;
}

/* An error or hangup occured for this process.  Close the fd
   and mark the process as exiting.  Processes are *NOT* marked as
   exited until they are waited on in the sigchld handler */
int mpiexecCloseProcess( int fd, int pidx, void *extra )
{
    ProcessTable_t *ptable = extra;
    if (pidx >= 0) {
	if (ptable->table[pidx].state < EXITING) 
	    ptable->table[pidx].state = EXITING;
    }
    close (fd);  /* Ignore any errors */

    /* Find this fd in the process entry and set it to -1 */
    if (ptable->table[pidx].fdStdin == fd) {
	ptable->table[pidx].fdStdin = -1;
    }
    if (ptable->table[pidx].fdStdout == fd) {
	ptable->table[pidx].fdStdout = -1;
    }
    if (ptable->table[pidx].fdStderr == fd) {
	ptable->table[pidx].fdStderr = -1;
    }
    if (ptable->table[pidx].fdPMI == fd) {
	ptable->table[pidx].fdPMI = -1;
    }

    /* Should we attempt to close any other fds on this process? 
       No, let them exit normally. */

    return 0;
}
