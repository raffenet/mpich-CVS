/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2004 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* OWNER=gropp */

/* An example mpiexec program that simply forks new processes on the 
   same host.  This provides a simple way to run MPI programs without
   requiring any special services.

   This code also shows how to use the pmutil routines (in ../util) 
   to provide many of the services required by mpiexec 

   Steps:
   1. Read and process that command line.  Build a ProcessList.  (A ProcessList
   may have one entry for a request to create n separate processes)
   
   2. Convert the ProcessList into a ProcessTable.  In the forker mpiexec,
   this simply expands the requested number of processes into an 
   array with one entry per process.  These entries contain information
   on both the setup of the processes and the file descriptors used for
   stdin,out,err, and for the PMI calls.

   3. (Optionally) allow the forked processes to use a host:port to 
   contact this program, rather than just sharing a pipe.  This allows the
   forker to start other programs, such as debuggers.

   4. Establish a signal handler for SIGCHLD.  This will allow us to 
   get information about process termination; in particular, the exit
   status.

   5. Start the programs.

   6. Process input from the programs; send stdin given to this process 
   to the selected processes (usually rank 0 or everyone).  Handle all 
   PMI commands, including spawn.  Another "input" is the expiration of the
   specified timelimit for the run, if any.

   7. Process rundown commands and handle any abnormal termination.  

   8. Wait for any processes to exit; gather the exit status and reason
   for exit (if abnormal, such as signaled with SEGV or BUS)

   9. Release all resources and compute the exit status for this program
   (using one of several approaches, such as taking the maximum of the
   exit statuses).

*/

#include "forkerconf.h"
#include "pmutil.h"
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>
/* sys/socket for socketpair and parameters */
#include <sys/socket.h>
#include "simple_pmiutil.h"

/* mpimem.h contains prototypes for MPIU_Strncpy etc. */
#include "mpimem.h"

/* debug is a global debugging flag */
int debug = 0;

/* Forward references */
int forkProcesses( ProcessState *, int, char *[], int );
static void ExecProgram( int, int, int, ProcessState *, char *[] );
int getProcessTable( ProcessList *, int, ProcessTable * );

/* temp */
void mpiexecPrintProcessState( FILE *, ProcessState *, int );

/* Set a maximum number of separate process descriptions */
#define MAXPROCLIST 32

/* Note that envp is common but not standard */
int main( int argc, char *argv[], char *envp[] )
{
    ProcessList  proclist[MAXPROCLIST];
    int          nplist;
    ProcessTable ptable;
    int          rc;
    IOExitReason reason;

    /* Set defaults for any arguments that are options.  Also check the
       environment for special options, such as debugging */
    /* Simple test for debugging */
    if (getenv( "MPIEXEC_DEBUG" )) debug = 1;

    /* Handle the command line arguments.  Use the routine from pmutil
     to create a ProcessList */
    nplist = mpiexecArgs( argc, argv, proclist, MAXPROCLIST, 0, 0 );

    if (debug) mpiexecPrintProcessList( stdout, proclist, nplist );

    /* Convert the list to an array of states in a process table */
    ptable.nProcesses      = 0;
    ptable.maxProcesses    = mpiexecGetIntValue( "MPIEXEC_UNIVERSE_SIZE", 64 );
    /* A negative timeout is infinite */
    ptable.timeout_seconds = mpiexecGetIntValue( "MPIEXEC_TIMEOUT", -1 );
    ptable.nActive         = 0;
    ptable.table = (ProcessState *)MPIU_Malloc( 
	ptable.maxProcesses * sizeof(ProcessState) );
    if (!ptable.table) {
	MPIU_Internal_error_printf( "Unable to allocate memory for %d processes\n", ptable.maxProcesses );
    }

    if (debug) DBG_PRINTF( ("timeout_seconds = %d\n", ptable.timeout_seconds) );

    getProcessTable( proclist, nplist, &ptable );

    /* Initalize the various fields in the process state (e.g.,
       exit status, state, file descriptors ... */
    /* FIXME still to do */

    if (debug) 
	mpiexecPrintProcessState( stdout, ptable.table, ptable.nProcesses );

    /* Enable handling of sigchld.  After this, any exiting child
       will cause a signal handler to wait for the exit status and reason for
       that child (and any other children that have exited, in case signals
       were coalesed).  Note that POSIX does not allow SIG_IGN for SIGCHLD
    */
    initPtableForSigchild( &ptable );

    /* Set the timeout */
    InitTimeout( ptable.timeout_seconds );

    /* Now we can consider creating some processes */
    ptable.nActive = 0;
    PMIServInit( ptable.nProcesses );
    /* IO information is initialized as the processes are created */
    forkProcesses( ptable.table, ptable.nProcesses, envp, 0 );
    ptable.nActive += ptable.nProcesses;

    /* Process any I/O activity.  Exits when all processes exited 
       or when timeout reached */
    reason = 0;
    while (IOHandleLoop( &ptable, &reason ));

    if (reason == IO_TIMEOUT) {
	/* Exited due to timeout.  Generate an error message and
	   terminate the children */
	MPIU_Error_printf( "Timeout of %d minutes expired; job aborted\n",
			 ptable.timeout_seconds / 60 );
	KillChildren( &ptable );
    }

    /* Wait for all processes to exit and gather information on them.
       We do this through the Sigchild handler, since we wish to
       use the SIGCHLD handler, and we must turn off its processing before 
       we can use explicit waits.

       FIXME: This should wait a bounded length of time.
    */
    WaitForChildren( &ptable );

    /* Compute the return code (max for now) */
    /* Enhancement: allow different combiners (second argument) */
    rc = ComputeExitStatus( &ptable, 0 );

    /* Optionally provide detailed information about failed processes */
    PrintFailureReasons( stderr, &ptable );
    return( rc );
}

void mpiexec_usage( const char *msg )
{
    if (msg)
	MPIU_Error_printf( msg );
    MPIU_Usage_printf( "\
Usage: mpiexec -n <numprocs> -soft <softness> -host <hostname> \\\n\
               -wdir <working directory> -path <search path> \\\n\
               -file <filename> -configfile <filename> execname <args>\n" );
    exit( -1 );
}

/*
  Fork numprocs processes, with the current PMI groupid and kvsname 
  This is called for the initial processes and as the result of 
  an PMI "spawn" command 
  
  world indicates which comm world this is, starting from zero
 */
static int forkProcesses( ProcessState *pstate, int npstate, 
			  char *envp[], int world )
{
    pid_t pid;
    int   i;
    int   read_out_pipe[2];
    int   read_err_pipe[2];
    int   write_in_pipe[2];
    int   client_pipe_fds[2];

    for (i=0; i<npstate; i++) {
	/* Extract the information on the processes to spawn and their
	   environment and command lines from the ProcessState */
	
	pstate[i].pid   = PENDING;
	pstate[i].state = STARTING;

	/* Create the fd's that we will pass to the child */
	if (pipe(read_out_pipe)) return -1;
	if (pipe(read_err_pipe)) return -1;
	/* FIXME: Create the in pipe if requested (MPIEXEC_STDIN_DEST
	   setting, which is not yet implemented) */
	if (pipe(write_in_pipe)) return -1;

	/* Create the fd that we will use to communicate with the child 
	   for the pmi functions */
	socketpair( AF_UNIX, SOCK_STREAM, 0, client_pipe_fds );

	pid = fork();
	if (pid < 0) {
	    /* Error when forking */
	    MPIU_Internal_error_printf( "mpiexec fork failed\n" );
	    /* FIXME: kill children */
	    exit( 1 );
	}
	if (pid == 0) {
	    /* Close file descriptors that the exec'ed program
	       should not have (e.g., the PMI fd's; any fds for 
	       stdin/out/err.
	       FIXME: One standard should be to add FD_CLOEXEC to any
	       opened fd (POSIX) */
	    IOHandlersCloseAll( pstate, i );

	    /* Redirect stdout/err to the pipes */
	    close(read_out_pipe[0]); 
	    close(1);
	    dup2(read_out_pipe[1],1);
	    close(read_err_pipe[0]);
	    close(2);
	    dup2(read_err_pipe[1],2);

	    /* Close one side of the pipe pair */
	    close( client_pipe_fds[0] );

	    /* Switch to the program.  This routine is specific to
	       the forker mpiexec */
	    ExecProgram( i, npstate, client_pipe_fds[1], &pstate[i], envp );
	}
	else {
#           define MAX_LEADER 32
	    char leader[MAX_LEADER];
	    if (debug) {
		DBG_FPRINTF( (stderr, "Created process %d\n", (int)pid) );
	    }
	    /* Record this pid in the process state */
	    pstate[i].pid = pid;
	    /* Here is where we would close any extraneous fds */
	    close(read_out_pipe[1]); 
	    close(read_err_pipe[1]);
	    /* Close one side of the pipe pair */
	    close( client_pipe_fds[1] );
	    leader[0] = 0;
	    GetPrefixFromEnv( 0, leader, MAX_LEADER, i, world );
	    IOSetupOutHandler( &pstate[i].ios[0], read_out_pipe[0], 1, 
			       leader );
	    leader[0] = 0;
	    GetPrefixFromEnv( 1, leader, MAX_LEADER, i, world );
	    IOSetupOutHandler( &pstate[i].ios[1], read_err_pipe[0], 2, 
			       leader );
	    IOSetupPMIHandler( &pstate[i].ios[2], client_pipe_fds[0], 
			       &pstate[i], 0, npstate, i );
	    pstate[i].nIos = 3;
	    PMIServAddtoGroup( 0, i, pid, client_pipe_fds[0] );
	}
    }
    return i;
}

/*
 * exec the indicated program with the indicated environment
 */
#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif
#define MAX_CLIENT_ARG 50
#define MAX_CLIENT_ENV 200
static void ExecProgram( int rank, int numprocs, int pmifd, 
			 ProcessState *pstate, char *envp[] )
{
    int j, rc;

    /* Provide local variables in which to hold new environment variables. */
    char env_pmi_fd[MAXNAMELEN];
    char env_pmi_rank[MAXNAMELEN];
    char env_pmi_size[MAXNAMELEN];
    char env_pmi_debug[MAXNAMELEN];
    char env_appnum[MAXNAMELEN];
    char env_universesize[MAXNAMELEN];
    char pathstring[MAXPATHLEN+10];
    char *(client_env[MAX_CLIENT_ENV]);
    char *(client_arg[MAX_CLIENT_ARG]);

    /* Check to see if we should be traced */
    /* CheckIfTraced( ); */

    /* build environment for client */
    for ( j = 0; envp[j] && j < MAX_CLIENT_ENV-7; j++ )
	client_env[j] = envp[j]; /* copy mpiexec environment */

    if (j == MAX_CLIENT_ENV-7) {
	MPIU_Error_printf( "Environment is too large (max is %d)\n",
			   MAX_CLIENT_ENV-7);
	exit(-1);
    }
    if (pmifd >= 0) {
	MPIU_Snprintf( env_pmi_fd, MAXNAMELEN, "PMI_FD=%d" , pmifd );
	client_env[j++] = env_pmi_fd;
    }
    MPIU_Snprintf( env_pmi_rank, MAXNAMELEN, "PMI_RANK=%d", rank );
    client_env[j++] = env_pmi_rank;
    MPIU_Snprintf( env_pmi_size, MAXNAMELEN, "PMI_SIZE=%d", numprocs );
    client_env[j++] = env_pmi_size;
    MPIU_Snprintf( env_pmi_debug, MAXNAMELEN, "PMI_DEBUG=%d", debug );
    client_env[j++] = env_pmi_debug;
    /* FIXME: Get the correct appnum and universsize */
    MPIU_Snprintf( env_appnum, MAXNAMELEN, "MPI_APPNUM=%d", 0 );
    client_env[j++] = env_appnum;
    MPIU_Snprintf( env_universesize, MAXNAMELEN, "MPI_UNIVERSE_SIZE=%d", 0 );
    client_env[j++] = env_universesize;
    client_env[j] = NULL;
    for ( j = 0; client_env[j]; j++ )
	if (putenv( client_env[j] )) {
	    MPIU_Internal_sys_error_printf( "mpiexec", errno, 
			     "Could not set environment %s", client_env[j] );
	    exit( 1 );
	}
    
    /* change working directory if specified, replace argv[0], and exec client */
    if (pstate->spec.wdir) {
	rc = chdir( pstate->spec.wdir );
	if (rc < 0) {
	    MPIU_Error_printf( "Unable to set working directory to %s\n",
			       pstate->spec.wdir);
	    rc = chdir( getenv( "HOME" ) );
	    if (rc < 0) {
		MPIU_Error_printf( "Unable to set working directory to %s\n",
				   getenv( "HOME" ) );
		exit( 1 );
	    }
	}
    }

    /* Set up the command-line arguments */
    client_arg[0] = (char *)pstate->spec.exename;
    for (j=0; j<pstate->spec.nArgs; j++) {
	client_arg[1+j] = (char *)pstate->spec.args[j];
    }
    /* The argv array is null-terminated */
    client_arg[1+j] = 0;

    /* pathname argument should be used here */
    if (pstate->spec.path) {
	/* Set up the search path */
	MPIU_Snprintf( pathstring, sizeof(pathstring)-1, "PATH=%s", 
		       pstate->spec.path );
	putenv( pathstring );
    }
    rc = execvp( pstate->spec.exename, client_arg );

    if ( rc < 0 ) {
	MPIU_Internal_sys_error_printf( "mpiexec", errno, 
					"mpiexec could not exec %s\n", 
					pstate->spec.exename );
	exit( 1 );
    }
}

/*
 * Convert the ProcessList into an array of process states.  
 * In the general case,
 * the mpiexec program will use a resource manager to provide this function;
 * the resource manager may use a list of host names or query a sophisticated
 * resource management system.  Since the forker process manager runs all
 * processes on the same host, this function need only expand the 
 * process list into a process table.
 *
 * 
 * Updates the ProcessTable with the new processes, and updates the
 * number of processes.  All processses are added to the end of the
 * current array.  Only the "spec" part of the state element is initialized
 *
 * Return value is the number of processes added, or a negative value
 * if an error is encountered.
 *
 * We use a state array so that we can convert any plist into an array
 * of states.  This allows use to use spawn during an mpiexec run.
 *
 * This routine could also check for inconsistent arguments, such as
 * a hostname that is not the calling host, or an architecture that does
 * not match the calling host's architecture.
 *
 * TODO: How do we handle the UNIVERSE_SIZE in this assignment (we 
 * need at least one process from each appnum; that is, from each
 * requested set of processes.
 *
 */
int getProcessTable( ProcessList *plist, int nplist, ProcessTable *ptable )
{
    int          i, ntotal, j, npstate;
    int          navail;
    ProcessState *pstate;

    /* First, count up the number of processes */
    ntotal = 0;
    navail = ptable->maxProcesses;
    for (i=0; i<nplist; i++) {
	if (plist[i].np > 0) {
	    ntotal += plist[i].np;
	    navail -= plist[i].np;
	}
	else if (plist[i].soft.nelm > 0) {
	    /* Find the largest soft within the remaining universe size */
	    int softmaxnp = 0;
	    int softnp, *tuple;
	    
	    for (j=0; j<plist[i].soft.nelm; j++) {
		int start, end, stride;

		/* For each tuple, determine the max for this tuple */
		tuple  = plist[i].soft.tuples[j];
		start  = tuple[0];
		end    = tuple[1];
		stride = tuple[2];
		
		softnp = start;
		if (stride > 0) {
		    softnp = start + stride * ( (start-end)/stride );
		    while (softnp > start && softnp > navail) 
			softnp -= stride;
		}
		else if (stride < 0) {
		    softnp = start;
		    while (softnp > navail && softnp+stride >= end) {
			softnp += stride;
		    }
		}
		if (softnp > softmaxnp) softmaxnp = softnp;
	    }
	    ntotal += softmaxnp;
	    navail -= softmaxnp;
	    /* Remember the computed value */
	    plist[i].np = softmaxnp;
	}
    }

    /* Fill in the process state (just the process spec part) */
    npstate = ptable->nProcesses;
    if (npstate + ntotal >= ptable->maxProcesses) {
	/* */
	MPIU_Error_printf( "Requested %d more processes but universe size is %d and %d already used\n", ntotal, ptable->maxProcesses, npstate );
	return -1;
    }
    pstate = &ptable->table[0];
    for (i=0; i<nplist; i++) {
	for (j=0; j<plist[i].np; j++) {
	    pstate[npstate + j].spec = plist[i].spec;
	}
	npstate += plist[i].np;
    }
    ptable->nProcesses += ntotal;
    return ntotal;
}

/* temporary */
void mpiexecPrintProcessState( FILE *fp, ProcessState *pstate, int npstate )
{
    int i, j;
    ProcessSpec *pspec;
    SoftSpec    *sspec;
    
    for (i=0; i<npstate; i++) {
	pspec = &pstate[i].spec;
	DBG_FPRINTF( (fp, "ProcessState[%d]:\n", i) );
	DBG_FPRINTF( (fp, "\
    exename   = %s\n\
    hostname  = %s\n\
    arch      = %s\n\
    path      = %s\n\
    wdir      = %s\n", 
		 pspec->exename  ? pspec->exename : "<NULL>", 
		 pspec->hostname ? pspec->hostname : "<NULL>", 
		 pspec->arch     ? pspec->arch     : "<NULL>", 
		 pspec->path     ? pspec->path     : "<NULL>", 
		 pspec->wdir     ? pspec->wdir     : "<NULL>") );
	DBG_FPRINTF( (fp, "    args:\n") );
	for (j=0; j<pspec->nArgs; j++) {
	    DBG_FPRINTF( (fp, "        %s\n", pspec->args[j]) );
	}
    }
    
}
