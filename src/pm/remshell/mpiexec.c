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

/*
 * Define Data structures.
 *
 * ProcessState - Each process has one of these entries, stored in any
 * array, and ordered by rank in comm_world (for the initial processes)
 *
 * PMIState - mpiexec provides the process management interface (PMI)
 * services.  PMIState contains the information needed for this,
 * other than the fds used to communicate with each created process's
 * PMI interface.  
 */

#define MAXNAMELEN  256		/* max length of vairous names */
#ifndef PATH_MAX
#define PATH_MAX 2048		/* max length of PATH */
#endif
#define MAXPROCESSES     64	/* max number of processes to manage */

#ifndef MAX_HOST_NAME
#define MAX_HOST_NAME 512
#endif

/* ----------------------------------------------------------------------- */
/* ProcessState                                                            */
/* ----------------------------------------------------------------------- */
typedef enum { UNINITIALIZED=-1, 
	       UNKNOWN, ALIVE, COMMUNICATING, FINALIZED, EXITING, GONE } 
    client_state_t;

/* Record the return value from each process */
typedef enum { NORMAL,     /* Normal exit (possibly with nonzero status) */
	       SIGNALLED,  /* Process died on an uncaught signal (e.g., 
			      SIGSEGV) */
	       NOFINALIZE, /* Process exited without calling finalize */
	       ABORTED,    /* Process killed by mpiexec after a PMI abort */
	       KILLED      /* Process was killed by mpiexec */ 
             } exit_state_t;

typedef struct { 
    int            fdStdin, fdStdout, fdStderr, /* fds for std io */
	fdPMI;                       /* fds for process management */
    client_state_t state;            /* state of process */
    const char    *exename;          /* Executable to run */
    const char    **args;            /* Pointer into the array of args */
    const char    *hostname;         /* Host for process */
    const char    *arch;             /* Architecture type */
    const char    *path;             /* Search path for executables */
    const char    *wdir;             /* Working directory */
    int            nArgs;            /* Number of args (list is *not* null
					terminated) */
    int            rank;             /* rank in comm_world (or universe) */
    int            pmiGroup;         /* PMI group index (into array of 
					PMI Groups */
    int            pmiKVS;           /* PMI kvs index (into array of 
					key-value-spaces) */
    exit_state_t   exitReason;       /* how/why did the process exit */
    int            exitSig;          /* exit signal, if any */
    int            exitStatus;       /* exit statue */
    int            exitOrder;        /* indicates order in which processes
					exited */
} ProcessState;

/* We put both the array and the count of processes into a single
   structure to make it easier to pass this information to other routines */
typedef struct {
    int          nProcesses;         /* Number of processes created */
    int          maxProcesses;
    ProcessState table[MAXPROCESSES];
} ProcessTable_t;

ProcessTable_t processTable;

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

    /* Process the command line arguments to build the table of 
       processes to create */
    rc = mpiexecArgs( argc, argv, &processTable, 0, 0 );
    if (rc) return rc;

    /* Determine the hosts to run on */
    rc = mpiexecChooseHosts( &processTable );

    mpiexecPrintTable( &processTable );

    /* Create the PMI INET socket */
    rc = mpiexecGetPort( &fdPMI, &portnum );
    printf( "rc = %d, Using port %d and fd %d\n", rc, portnum, fdPMI );
    if (rc) return rc;

    /* Optionally stage the executables */
    
    /* Start the remote shell processes ooncurrently */
    rc = mpiexecStartProcesses( &processTable, myname, portnum );

    /* Poll on the active FDs and handle each input type */

    /* Clean up and determine the return code.  Log any anomolous events */

    return rc;
}

/* ----------------------------------------------------------------------- */
/* Process options                                                         */
/* The process options steps loads up the processTable with entries,       */
/* including the hostname, for each process.  If no host is specified,     */
/* one will be provided in a subsequent step
/* ----------------------------------------------------------------------- */
/*
-host hostname
-arch architecture name
-wdir working directory (cd to this one BEFORE running executable?)
-path pathlist - use this to find the executable
-file filename - use this to specify other info
-soft comma separated triplets (ignore for now)
*/
/*
 * MPIExecArgs processes the arguments for mpiexec.  For any argument that 
 * is not recognized, it calls the ProcessArg routine, which returns the
 * number of arguments that should be skipped.  The void * pointer in
 * the call to ProcessArg is filled with the "extraData" pointer.  If
 * ProcessArg is null, then any unrecognized argument causes mpiexec to 
 * print a help message and exit.
 *
 * In addition the the arguments specified by the MPI standard, 
 * -np is accepted as a synonym for -n and -hostfile is allowed
 * to specify the available hosts.
 *
 * The implementation understands the ":" notation to separate out 
 * different executables.  Since no ordering of these arguments is implied,
 * other than that the executable comes last, we store the values until
 * we see an executable.
 */
static int getInt( int, int, char *[] );

int mpiexecArgs( int argc, char *argv[], ProcessTable_t *ptable,
		 int (*ProcessArg)( int, char *[], void *), void *extraData )
{
    int         i, j;
    int         np=1;      /* These 6 values are set by command line options */
    const char *host=0;    /* These are the defaults.  When a program name */
    const char *arch=0;    /* is seen, the values in these variables are */
    const char *wdir=0;    /* used to initialize the ProcessState entries */
    const char *path=0;
    const char *soft=0;
    const char *exename=0;
    int        indexOfFirstArg=-1;

    for (i=1; i<argc; i++) {
	if ( strncmp( argv[i], "-n",  strlen( argv[i] ) ) == 0 ||
	     strncmp( argv[i], "-np", strlen( argv[i] ) ) == 0 ) {
	    np = getInt( i+1, argc, argv );
	    i++;
	}
	else if ( strncmp( argv[i], "-soft", 6 ) == 0 )
	    if ( i+1 < argc )
		soft = argv[++i];
	    else
		mpiexec_usage( "Missing argument to -soft\n" );
	else if ( strncmp( argv[i], "-host", 6 ) == 0 )
	    if ( i+1 < argc )
		host = argv[++i];
	    else
		mpiexec_usage( "Missing argument to -host\n" );		    
	else if ( strncmp( argv[i], "-arch", 6 ) == 0 )
	    if ( i+1 < argc )
		arch = argv[++i];
	    else
		mpiexec_usage( "Missing argument to -arch\n" );		    
	else if ( strncmp( argv[i], "-wdir", 6 ) == 0 )
	    if ( i+1 < argc )
		wdir = argv[++i];
	    else
		mpiexec_usage( "Missing argument to -wdir\n" );		    
	else if ( strncmp( argv[i], "-path", 6 ) == 0 )
	    if ( i+1 < argc )
		path = argv[++i];
	    else
		mpiexec_usage( "Missing argument to -path\n" );		    
	else if (argv[i][0] != '-') {
	    exename = argv[i];
	    /* Skip arguments until we hit either the end of the args
	       or a : */
	    i++;
	    indexOfFirstArg = i;
	    while (i < argc && argv[i][0] != ':') i++;
	    if (i == indexOfFirstArg) { 
		/* There really wasn't an argument */
		indexOfFirstArg = -1;
	    }
	    
	    if (ptable->nProcesses + np > ptable->maxProcesses) {
		mpiexec_usage( "Too many processes requested\n" );
	    }

	    /* Now, we are ready to update the process table */
	    for (j=0; j<np; j++) {
		ProcessState *ps;
		ps = &ptable->table[ptable->nProcesses + j];
		ps->fdStdin = ps->fdStdout = ps->fdStderr = -1;
		ps->fdPMI = -1;
		ps->state = UNINITIALIZED;
		ps->exename = exename;
		if (indexOfFirstArg > 0) {
		    ps->args = (const char **)(argv + indexOfFirstArg);
		    ps->nArgs = i - indexOfFirstArg;
		}
		else {
		    ps->args = 0;
		    ps->nArgs = 0;
		}
		ps->hostname = host;
		ps->arch = arch;
		ps->path = path;
		ps->wdir = wdir;
		ps->rank = ptable->nProcesses + i;
		ps->pmiGroup = -1;
		ps->pmiKVS = -1;
		ps->exitReason = 0;
		ps->exitSig = 0;
		ps->exitStatus = 0;
		ps->exitOrder = -1;
	    }
	    ptable->nProcesses += np;

	    /* Now, clear all of the values for the next set */
	    host = arch = wdir = path = soft = exename = 0;
	    indexOfFirstArg = -1;
	    np              = 1;
	}
	else {
	    int incr = 0;
	    if (ProcessArg) {
		incr = ProcessArg( argc, argv, extraData );
	    }
	    if (incr) {
		/* increment by one less because the for loop will also
		   increment i */
		i += (incr-1);
	    }
	    else {
		fprintf( stderr, "invalid mpiexec argument %s\n", argv[i] );
		mpiexec_usage( NULL );
	    }
	}
    }
    return 0;
}

/* Return the int-value of the given argument.  If there is no 
   argument, or it is not a valid int, exit with an error message */
static int getInt( int argnum, int argc, char *argv[] )
{
    char *p;
    long i;

    if (argnum < argc) {
	p = argv[argnum];
	i = strtol( argv[argnum], &p, 0 );
	if (p == argv[argnum]) {
	    char msg[1024];
	    sprintf( "Invalid parameter value %s to argument %s\n",
		     argv[argnum], argv[argnum-1] );
	    mpiexec_usage( msg );
	    /* Does not return */
	}
	return (int)i;
    }
    else {
	char msg[1024];
	sprintf( msg, "Missing argument to %s\n", argv[argnum-1] );
	mpiexec_usage( msg );
	/* Does not return */
    }
}

/* ----------------------------------------------------------------------- */
/* Determine the hosts                                                     */
/*                                                                         */
/* For each requested process that does not have an assigned host yet,     */
/* use information from a machines file to fill in the choices             */
/* ----------------------------------------------------------------------- */
/* This structure is used as part of the code to assign machines to 
   processes */
typedef struct {
    int nHosts; 
    char **hname;
} MachineTable;
MachineTable *mpiexecReadMachines( const char *, int );

int mpiexecChooseHosts( ProcessTable_t *ptable )
{
    int i, j, k, nNeeded=0;
    const char *arch;
    MachineTable *mt;

    /* First, determine how many processes require host names */
    for (i=0; i<ptable->nProcesses; i++) {
	if (!ptable->table[i].hostname) nNeeded++;
    }
    if (nNeeded == 0) return 0;

    /* Read the appropriate machines file.  There may be multiple files, 
       one for each requested architecture.  We'll read one machine file
       at a time, filling in all of the processes for each particular 
       architecture */
    while (nNeeded) {
	for (i=0; i<ptable->nProcesses; i++) {
	    if (!ptable->table[i].hostname) break;
	}
	/* Read the machines file for this architecture.  Use the
	   default architecture if none selected */
	arch = ptable->table[i].arch;
	mt = mpiexecReadMachines( arch, nNeeded );
	if (!mt) {
	    /* FIXME : needs an error message */
	    exit(-1);
	}
	/* Assign machines to all processes with this arch */
	k = 0;
	for (; i<ptable->nProcesses; i++) {
	    if (ptable->table[i].arch == arch &&
		!ptable->table[i].hostname) {
		ptable->table[i].hostname = mt->hname[k++];
		if (k >= mt->nHosts) k = 0;
		nNeeded--;
	    }
	}
    }
    return 0;
}

#define MAXLINE 256
const char defaultMachinesPath[] = DEFAULT_MACHINES_PATH;

MachineTable *mpiexecReadMachines( const char *arch, int nNeeded )
{
    FILE *fp;
    char buf[MAXLINE+1];
    char machinesfile[PATH_MAX];
    char dirname[PATH_MAX];
    char *p;
    const char *path=getenv("MPIEXEC_MACHINES_PATH");
    MachineTable *mt;
    int len, nFound = 0;
    
    /* Try to open the machines file.  arch may be null, in which 
       case we open the default file */
    /* FIXME: need path and external designation of file names */
    /* Partly done */
    if (!path) path = defaultMachinesPath;

    while (path) {
	char *next_path;
	/* Get next path member */
	next_path = strchr( path, ':' );
	if (next_path) 
	    len = next_path - path;
	else
	    len = strlen(path);
	
	/* Copy path into the file name */
	strncpy( dirname, path, len );

	dirname[len] = 0;

	/* Construct the final path name */
	if (arch) {
	    sprintf( machinesfile, "%s/machines.%s", dirname, arch );
	}
	else {
	    strcpy( machinesfile, dirname );
	    strcat( machinesfile, "/machines" );
	}
	fp = fopen( machinesfile, "r" );
	if (fp) break;  /* Found one */
    }
	
    if (!fp) {
	fprintf( stderr, "Could not open machines file %s\n", machinesfile );
	return 0;
    }
    mt = (MachineTable *)malloc( sizeof(MachineTable) );
    if (!mt) {
	fprintf( stderr, "Internal error: could not allocate machine table\n" );
	return 0;
	}
    
    mt->hname = (char **)malloc( nNeeded * sizeof(char *) );
    if (!mt->hname) {
	return 0;
    }
    while (nNeeded) {
	if (!fgets( buf, MAXLINE, fp )) break;
	printf( "line: %s\n", buf );
	/* Skip comment lines */
	p = buf;
	while (isascii(*p) && isspace(*p)) p++;
	if (*p == '#') continue;
	
	len = strlen( p );
	if (p[len-1] == '\n') p[--len] = 0;
	if (p[len-1] == '\r') p[--len] = 0;   /* Handle DOS files */
	mt->hname[nFound] = (char *)malloc( len + 1 );
	if (!mt->hname[nFound]) return 0;
	strncpy( mt->hname[nFound], p, len+1 );
	nFound++;
	nNeeded--;
    }
    mt->nHosts = nFound;
    return mt;	
}
/* ----------------------------------------------------------------------- */
/* Get a port for the PMI interface                                        */
/* Ports can be allocated within a requested range
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
	fprintf( stderr, "ranges not supported for ports\n" );
	return 1;
    }

    for (portnum=low_port; portnum<=high_port; portnum++) {
	bzero( &sa, sizeof(sa) );
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
	fputs( msg, stderr );
    fprintf( stderr, "\
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

    for (i=0; i<ptable->nProcesses; i++) {
	int read_out_pipe[2], write_in_pipe[2], read_err_pipe[2];
	ProcessState *ps;

	ps = &ptable->table[i];

	/* Create the pipes for the stdin/out/err replacements */
	if (pipe(read_out_pipe)) return -1;
	if (pipe(write_in_pipe)) return -1;
	if (pipe(read_err_pipe)) return -1;

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
	    int rshNargs, j;

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

	    /* Now we are ready to run the program. */
	    myargv = (char **)malloc( (ps->nArgs + 30) * sizeof(char *) );
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
	    sprintf( port_as_string, "%s:%d", myname, port );
	    myargv[rshNargs++] = port_as_string;
	    myargv[rshNargs++] = "\\;";
	    myargv[rshNargs++] = (char *)(ps->exename);
	    for (j=0; j<ps->nArgs; j++) 
		myargv[rshNargs++] = (char *)(ps->args[j]);
	    myargv[rshNargs++] = 0;
#ifdef DEBUG
	    { int k;
	    for (k=0; myargv[k]; k++) {
		printf( "%s ", myargv[k] );
	    }
	    printf( "\n" );
	    }
#endif
	    execvp( myargv[0], myargv );
	    /* never returns */
	}
	else {
	    /* Error on fork */
	    /* FIXME */
	    fprintf( stderr, "Failure to create process number %d\n", i );
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

	    remshell[remargs] = (char *)malloc( len + 1 ); /* Add the null */
	    strncpy( remshell[remargs], rem, len );
	    remshell[remargs][len] = 0;
	    remargs++;
	    if (next_parm) {
		rem = next_parm + 1;
		while (*rem == ' ') rem++;
		if (remargs == 10) {
		    /* FIXME */
		    fprintf( stderr, "Remote shell command is too complex\n" );
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
	printf( "On %s run %s", ps->hostname, ps->exename );
	for (j=0; j<ps->nArgs; j++) {
	    printf( " %s", ps->args[j] );
	}
	printf( "\n" );
    }
}

