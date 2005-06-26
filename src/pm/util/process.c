/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "pmutilconf.h"
#ifdef NEEDS_POSIX_FOR_SIGACTION
#define _POSIX_SOURCE
#endif

#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "pmutil.h"
#include "ioloop.h"
#include "process.h"
/* Use the memory defintions from mpich2/src/include */
#include "mpimem.h"
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include <sys/wait.h>
#if defined(USE_SIGNAL) || defined(USE_SIGACTION)
#include <signal.h>
#else
#error no signal choice
#endif
#ifdef NEEDS_STRSIGNAL_DECL
extern char *strsignal(int);
#endif

/* There is only one universe */
ProcessUniverse pUniv;

/* This is the home of the common debug flag */
int MPIE_Debug = 0;

/* Local, forward references */
static void MPIE_InstallSigHandler( int sig, void (*handler)(int) );

/*
  Fork numprocs processes, with the current PMI groupid and kvsname 
  This is called for the initial processes and as the result of 
  an PMI "spawn" command 
  
  world indicates which comm world this is, starting from zero
 */
/*@
  MPIE_ForkProcesses - Create new processes for a comm_world

  Input Parameters:
+ pWorld - Pointer to a 'ProcessWorld' structure, containing one or more
           'ProcessApp' structures.  Each 'ProcessApp' specifies how many
	   processes to create.
. envp   - Environment to provide to each process
. preamble - Routine executed before the fork for each created process
. preambldData - Data passed to the preamble function
- other - other

  Output Effects:
  The 'ProcessState' fields are allocated and filled in with the process id
  and other information.

  Callbacks:
+ preamble - called before fork
. postfork - called after fork, but before exec, in the forked child
- postamble - called after fork in the parent (the process that executed
     the fork).  

  A typical use of the 'preamble' function is to open a file descriptor
  that is then inherited by the forked process.  The 'postfork' and
  'postamble' can close any unneeded file descriptors

  Returns:
  The number of created processes.

  @*/
int MPIE_ForkProcesses( ProcessWorld *pWorld, char *envp[],
			int (*preamble)(void*,ProcessState*), 
			void *preambleData,
			int (*postfork)(void*,void*,ProcessState*), 
			void *postforkData,
			int (*postamble)(void*,void*,ProcessState*), 
			void *postambleData
			)
{
    pid_t         pid;
    ProcessApp   *app;
    ProcessState *pState=0;
    int          wRank = 0;    /* Rank in this comm world of the process */
    int          i, rc;
    int          nProcess = 0;
    static       int UniqId = 0; /* A unique ID for each forked process, 
				    up to 2 billion */

    app = pWorld->apps;
    while (app) {
	/* Allocate process state if necessary */
	if (!app->pState) {
	    pState = (ProcessState *)MPIU_Malloc( 
		app->nProcess * sizeof(ProcessState) );
	    if (!pState) {
		return -1;
	    }
	    app->pState = pState;
	}
	pState = app->pState;

	for (i=0; i<app->nProcess; i++) {
	    pState[i].app                   = app;
	    pState[i].wRank                 = wRank++;
	    pState[i].id                    = UniqId++;
	    pState[i].initWithEnv           = 1;  /* Default is to use env
						    to initialize connection */
	    pState[i].status                = PROCESS_UNINITIALIZED;
	    pState[i].exitStatus.exitReason = EXIT_NOTYET;
	    pState[i].pid                   = -1;

	    /* Execute any preamble */
	    if (preamble) {
		rc = (*preamble)( preambleData, &pState[i] );
	    }

	    pid = fork();
	    if (pid < 0) {
		/* Error creating process */
		return -1;
	    }
	    if (pid == 0) {
		/* Child */
		/* exec the process (this call only returns if there is an 
		   error */
		if (postfork) {
		    rc = (*postfork)( preambleData, postforkData, &pState[i] );
		}
		rc = MPIE_ExecProgram( &pState[i], envp );
		if (rc) {
		    MPIU_Internal_error_printf( "mpiexec fork failed\n" );
		    /* FIXME: kill children */
		    exit(1);
		}
	    }
	    else {
		/* Parent */
		nProcess++;
		/* Add this to the live processes in the Universe */
		pUniv.nLive++;

		pState[i].pid    = pid;
		pState[i].status = PROCESS_ALIVE;

		if (postamble) {
		    rc = (*postamble)( preambleData, postambleData, 
				       &pState[i] );
		    if (rc) {
			MPIU_Internal_error_printf( 
				      "mpiexec postamble failed\n" );
			/* FIXME: kill children */
			exit(1);
		    }
		}
	    }
	}
	
	app = app->nextApp;
    }
    return nProcess;
}

/*@
  MPIE_ProcessGetExitStatus - Return an integer exit status based on the
  return status of all processes in the process universe; returns the
  maximum value seen
  @*/
int MPIE_ProcessGetExitStatus( void )
{
    ProcessWorld *world;
    ProcessApp   *app;
    ProcessState *pState;
    int          i, rc = 0;

    world = pUniv.worlds;
    while (world) {
	app = world->apps;
	while (app) {
	    pState = app->pState;
	    for (i=0; i<app->nProcess; i++) {
		if (pState[i].exitStatus.exitStatus > rc) {
		    rc = pState[i].exitStatus.exitStatus;
		}
	    }
	    app = app->nextApp;
	}
	world = world->nextWorld;
    }
    return rc;
}
/*
 * exec the indicated program with the indicated environment
 */
#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif
#define MAX_CLIENT_ARG 50
#define MAX_CLIENT_ENV 200

#define MAXNAMELEN 1024
int MPIE_ExecProgram( ProcessState *pState, char *envp[] )
{
    int j, rc, nj;
    ProcessApp *app;

    /* Provide local variables in which to hold new environment variables. */
    char env_pmi_rank[MAXNAMELEN];
    char env_pmi_id[MAXNAMELEN];
    char env_pmi_size[MAXNAMELEN];
    char env_pmi_debug[MAXNAMELEN];
    char env_appnum[MAXNAMELEN];
    char env_universesize[MAXNAMELEN];
    char pathstring[MAXPATHLEN+10];
    char *(client_env[MAX_CLIENT_ENV]);
    char *(client_arg[MAX_CLIENT_ARG]);

    app = pState->app;

    /* build environment for client. */
    j = MPIE_EnvSetup( pState, envp, client_env, MAX_CLIENT_ENV-7 );
    if (j < 0) {
	MPIU_Error_printf( "Failure setting up environment\n" );
    }
    nj = j;  /* nj is the first entry of client_env that will be set by
		this routine */
#if 0
    if (envp) {
	for ( j = 0; envp[j] && j < MAX_CLIENT_ENV-7; j++ )
	    client_env[j] = envp[j]; /* copy mpiexec environment */
    }
    else {
	j = 0;
    }
#endif
    if (j == MAX_CLIENT_ENV-7) {
	MPIU_Error_printf( "Environment is too large (max is %d)\n",
			   MAX_CLIENT_ENV-7);
	exit(-1);
    }

    if (pState->initWithEnv) {
	MPIU_Snprintf( env_pmi_rank, MAXNAMELEN, "PMI_RANK=%d", 
		       pState->wRank );
	client_env[j++] = env_pmi_rank;
	MPIU_Snprintf( env_pmi_size, MAXNAMELEN, "PMI_SIZE=%d", 
		       app->pWorld->nProcess );
	client_env[j++] = env_pmi_size;
	MPIU_Snprintf( env_pmi_debug, MAXNAMELEN, "PMI_DEBUG=%d", MPIE_Debug );
	client_env[j++] = env_pmi_debug; 
    }
    else {
	/* We must also communicate the ID to the process.  
	   This id is saved in the pState so that we can match it 
	   when it comes back to us (it is the same as the rank 
	   in the simple case) */
	MPIU_Snprintf( env_pmi_id, sizeof(env_pmi_id), "PMI_ID=%d",
		       pState->id );
	client_env[j++] = env_pmi_id;
    }

    MPIU_Snprintf( env_appnum, MAXNAMELEN, "MPI_APPNUM=%d", app->myAppNum );
    client_env[j++] = env_appnum;
    MPIU_Snprintf( env_universesize, MAXNAMELEN, "MPI_UNIVERSE_SIZE=%d", 
		   pUniv.size );
    client_env[j++] = env_universesize;
    
    client_env[j]   = 0;
    for ( j = nj; client_env[j]; j++ )
	if (putenv( client_env[j] )) {
	    MPIU_Internal_sys_error_printf( "mpiexec", errno, 
			     "Could not set environment %s", client_env[j] );
	    exit( 1 );
	}
    
    /* change working directory if specified, replace argv[0], 
       and exec client */
    if (app->wdir) {
	rc = chdir( app->wdir );
	if (rc < 0) {
	    MPIU_Error_printf( "Unable to set working directory to %s\n",
			       app->wdir);
	    rc = chdir( getenv( "HOME" ) );
	    if (rc < 0) {
		MPIU_Error_printf( "Unable to set working directory to %s\n",
				   getenv( "HOME" ) );
		exit( 1 );
	    }
	}
    }

    /* Set up the command-line arguments */
    client_arg[0] = (char *)app->exename;
    for (j=0; j<app->nArgs; j++) {
	client_arg[1+j] = (char *)app->args[j];
    }
    /* The argv array is null-terminated */
    client_arg[1+j] = 0;

    /* pathname argument should be used here */
    if (app->path) {
	/* Set up the search path */
	MPIU_Snprintf( pathstring, sizeof(pathstring)-1, "PATH=%s", 
		       app->path );
	putenv( pathstring );
    }
    rc = execvp( app->exename, client_arg );

    if ( rc < 0 ) {
	MPIU_Internal_sys_error_printf( "mpiexec", errno, 
					"mpiexec could not exec %s\n", 
					app->exename );
	exit( 1 );
    }
    return 0;
}

/*@
  MPIE_FindProcessByPid - Given a pid, return a pointer to the process state

  Notes:
  This searches through all processes in the process universe.  Returns
  null if no corresponding process is found.

  @*/
ProcessState *MPIE_FindProcessByPid( pid_t pid )
{
    ProcessWorld *world;
    ProcessApp   *app;
    ProcessState *pState;
    int           i;
    int           np = 100000; /* FIXME: Should initialize with the
				  number of created processes */

    world = pUniv.worlds;
    while (world) {
	app = world->apps;
	while (app) {
	    pState = app->pState;
	    np -= app->nProcess;
	    if (np < 0) {
		/* This is a panic exit, used becaue we may call this
		   from within the signal handler */
		return 0;
	    }
	    for (i=0; i<app->nProcess; i++) {
		if (pState[i].pid == pid) return &pState[i];
	    }
	    app = app->nextApp;
	}
	world = world->nextWorld;
    }
    return 0;
}

/*
 * Given a pointer to a process state structure, and the process status
 * returned by a 'wait' call, set the fields in the process state to
 * indicate the reason for the process exit
 */
static int nExited = 0;
void MPIE_ProcessSetExitStatus( ProcessState *pState, int prog_stat )
{
    int rc = 0, sigval = 0;
    if (WIFEXITED(prog_stat)) {
	rc = WEXITSTATUS(prog_stat);
	    }
    if (WIFSIGNALED(prog_stat)) {
	sigval = WTERMSIG(prog_stat);
    }
    pState->exitStatus.exitStatus = rc;
    pState->exitStatus.exitSig    = sigval;
    pState->exitStatus.exitOrder  = nExited++;
    if (sigval) 
	pState->exitStatus.exitReason = EXIT_SIGNALLED;
    else {
	if (pState->status >= PROCESS_ALIVE &&
	    pState->status <= PROCESS_COMMUNICATING) 
	    pState->exitStatus.exitReason = EXIT_NOFINALIZE;
	else 
	    pState->exitStatus.exitReason = EXIT_NORMAL;
    }
}

/* ------------------------------------------------------------------------- */
/* SIGNALS and CHILDREN                                                      */
/* ------------------------------------------------------------------------- */

/*
 * POSIX requires a SIGCHLD handler (SIG_IGN is invalid for SIGCHLD in
 * POSIX).  Thus, we have to perform the waits within the signal handler.
 * Because there may be a race condition with recording the pids in the 
 * ProcessState structure, we provide an "unexpected child" structure to
 * hold information about processes that are not yet registered.  A clean
 * up handler records the state of those processes when we're ready to 
 * exit.
 *
 * Because signals are not queued, this handler processes all completed
 * processes.  
 *
 * We must perform the wait in the handler because if we do not, we loose 
 * the exit status information (it is no longer available after the
 * signal handler exits).
 */
/* inHandler and skipHandler are used to control the SIGCHLD handler and
   to avoid (or at least narrow) race conditions */
static volatile int inHandler = 0;
/* skip handler is set if we are planning to wait for processes with the
   "wait" system call in a separate routine.  */
static volatile int skipHandler = 0;

/* The "unexpected" structure is used to record any processes that
   exit before we've recorded their pids.  This is unlikely, but may
   happen if a process fails to exec (e.g., the fork succeeds but the 
   exec immediately fails).  This ensures that we can handle SIGCHLD
   events without loosing information about the child processes */
#define MAXUNEXPECTEDPIDS 1024
static struct {
    pid_t  pid;
    int    stat;
} unexpectedExit[MAXUNEXPECTEDPIDS];
static int nUnexpected = 0;
	 
static void handle_sigchild( int sig )
{
    int prog_stat, pid;
    int foundChild = 0;
    ProcessState *pState;

    /* Set a flag to indicate that we're in the handler, and check 
       to see if we should ignore the signal */
    if (inHandler) return;
    inHandler = 1;

    if (skipHandler) {
	/* Someone else wants to wait on the processes, so we
	   return now. */
	inHandler = 0;
#ifndef SA_RESETHAND
	/* If we can't clear the "reset handler bit", we must 
	   re-install the handler here */
	MPIE_InstallSigHandler( SIGCHLD, handle_sigchild );
#endif
	return;
    }

    DBG_PRINTF( ("Entering sigchild handler\n") );
    DBG_EPRINTF((stderr, "Waiting for any child on signal\n") );

    /* Since signals may be coallesced, we process all children that
       have exited */
    while (1) {
	/* Find out about any children that have exited */
	pid = waitpid( (pid_t)(-1), &prog_stat, WNOHANG );
    
	if (pid <= 0) {
	    /* Note that it may not be an error if no child 
	       found, depending on various race conditions.  
	       Thus, we allow this case to happen without 
	       generating an error message */
	    /* Generate a debug message if we enter the handler but 
	       do not find a child */
	    DBG_EPRINTFCOND(MPIE_Debug && !foundChild,
			    (stderr, "Did not find child process!\n") );
	    /* We need to reset errno since otherwise a system call being
	       used in the main thread might see this errno and 
	       mistakenly decide that it suffered an error */
	    errno = 0;
	    break;
	}
	foundChild = 1;
	/* Receives a child failure or exit.  
	   If *failure*, kill the others */
	DBG_PRINTF(("Found process %d in sigchld handler\n", pid ) );
	pState = MPIE_FindProcessByPid( pid );
	if (pState) {
	    MPIE_ProcessSetExitStatus( pState, prog_stat );
	    pState->status = PROCESS_GONE;
	    if (pState->exitStatus.exitReason != EXIT_NORMAL) {
		/* Not a normal exit.  We may want to abort all 
		   remaining processes */
		MPIE_OnAbend( &pUniv );
	    }
	    /* Let the universe know that there are fewer processes */
	    pUniv.nLive--;
	    if (pUniv.nLive == 0) {
		/* Invoke any special code for handling all processes
		   exited (e.g., terminate a listener socket) */
		if (pUniv.OnNone) { (*pUniv.OnNone)(); }
	    }
	}
	else {
	    /* Remember this process id and exit status for later */
	    unexpectedExit[nUnexpected].pid  = pid;
	    unexpectedExit[nUnexpected].stat = prog_stat;
	}
    }
#ifndef SA_RESETHAND
    /* If we can't clear the "reset handler bit", we must 
       re-install the handler here */
    MPIE_InstallSigHandler( SIGCHLD, handle_sigchild );
#endif
    inHandler = 0;
}

/*@
  MPIE_ProcessInit - Initialize the support for process creation

  Notes:
  The major chore of this routine is to set the 'SIGCHLD' signal handler
  @*/
void MPIE_ProcessInit( void )
{
    MPIE_InstallSigHandler( SIGCHLD, handle_sigchild );
    pUniv.worlds = 0;
    pUniv.nLive  = 0;
    pUniv.OnNone = 0;
}

/*
 * Wait upto timeout seconds for all processes to exit.  
 * Because we are using a SIGCHLD handler to get the exit reason and
 * status from exiting children, this routine waits for those
 * signal handlers to return.  (POSIX requires a SIGCHLD handler, and leaving
 * the signal handler in charge avoids race conditions and possible loss
 * of information).
 */
int MPIE_WaitForProcesses( ProcessUniverse *pUniv, int timeout )
{
    ProcessWorld *world;
    ProcessApp   *app;
    ProcessState *pState;
    int           i, nactive;

    /* Determine the number of processes that we have left to wait on */
    TimeoutInit( timeout );
    nactive = 0;
    do {
	world = pUniv->worlds;
	while (world) {
	    app = world->apps;
	    while (app) {
		pState = app->pState;
		for (i=0; i<app->nProcess; i++) {
		    if (pState[i].status != PROCESS_GONE &&
			pState[i].pid > 0) nactive++;
		}
		app = app->nextApp;
	    }
	    world = world->nextWorld;
	}
    } while (nactive > 0 && TimeoutGetRemaining() > 0);

    return 0;
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

/*@
  MPIE_InitWorldWithSoft - Initialize a process world from any 
  soft specifications

  Input Parameter:
. maxnp - The maximum number of processes to allow.

  Input/Output Parameter:
. world - Process world.  On return, the 'ProcessState' fields for
  any soft specifications have been initialized 
  @*/
int MPIE_InitWorldWithSoft( ProcessWorld *world, int maxnp )
{
    ProcessApp *app;
    int        minNeeded, maxNeeded;
    int        j;

    /* Compute the number of available processes */
    maxnp -= world->nProcess;

    /* Compute the number of requested processes */
    minNeeded = maxNeeded = 0;
    app       = world->apps;
    while (app) {
	if (app->soft.nelm > 0 && app->nProcess == 0) {
	    /* Found a soft spec */
	    for (j=0; j<app->soft.nelm; j++) {
		int *tuple, start, end, stride;
		tuple = app->soft.tuples[j];
		start  = tuple[0];
		end    = tuple[1];
		stride = tuple[2];
		
		if (stride > 0) {
		    minNeeded += start;
		    maxNeeded += start + stride * ( (start-end)/stride );
		}
		else if (stride < 0) {
		    minNeeded += start + stride * ( (end-start)/stride );
		    maxNeeded += start;
		}
	    }
	}
	app = app->nextApp;
    }

    if (minNeeded > maxnp) {
	/* Requested more than there are available */
	return 1;
    }
    if (maxNeeded > maxnp) {
	/* Must take fewer than the maximum.  Take the minimum for now */
	app       = world->apps;
	while (app) {
	    if (app->soft.nelm > 0 && app->nProcess == 0) {
		/* Found a soft spec */
		for (j=0; j<app->soft.nelm; j++) {
		    int *tuple, start, end, stride;
		    tuple = app->soft.tuples[j];
		    start  = tuple[0];
		    end    = tuple[1];
		    stride = tuple[2];
		    
		    if (stride > 0) {
			app->nProcess = start;
		    }
		    else if (stride < 0) {
			app->nProcess = 
			    start + stride * ( (end-start)/stride );
		    }
		}
	    }
	    app = app->nextApp;
	}
	/* If we wanted to get closer to the maximum number, we could
	   iterative all stride to each set until we reached the limit.
	   But this isn't necessary to conform to the standard */
    }
    else {
	/* Take the maximum */
	app = world->apps;
	while (app) {
	    if (app->soft.nelm > 0 && app->nProcess == 0) {
		/* Found a soft spec */
		for (j=0; j<app->soft.nelm; j++) {
		    int *tuple, start, end, stride;
		    tuple = app->soft.tuples[j];
		    start  = tuple[0];
		    end    = tuple[1];
		    stride = tuple[2];
		    
		    /* Compute the "real" end */
		    if (stride > 0) {
			app->nProcess = 
			    start + stride * ( (start-end)/stride );
		    }
		    else if (stride < 0) {
			app->nProcess = start;
		    }
		}
	    }
	    app = app->nextApp;
	}
    }
    return 0;
}

/* ------------------------------------------------------------------------ */
/* Routines to deliver signals to every process in a world                  */
/* ------------------------------------------------------------------------ */

/*@
  MPIE_SignalWorld - Send a signal to every process in a world 

  @*/
int MPIE_SignalWorld( ProcessWorld *world, int signum )
{
    ProcessApp   *app;
    ProcessState *pState;
    int           np, i;
    
    app = world->apps;
    while (app) {
	pState = app->pState;
	np     = app->nProcess;
	for (i=0; i<np; i++) {
	    pid_t pid;
	    pid = pState[i].pid;
	    if (pid > 0 && pState[i].status != PROCESS_GONE) {
		/* Ignore error returns */
		kill( pid, signum );
	    }
	}
	app = app->nextApp;
    }
    return 0;
}
    

/*@
  MPIE_KillWorld - Kill all of the processes in a world
 @*/
int MPIE_KillWorld( ProcessWorld *world )
{
    MPIE_SignalWorld( world, SIGINT );

    /* We should wait here to give time for the processes to exit */
    sleep( 1 );
    MPIE_SignalWorld( world, SIGQUIT );

    return 0;
}

/*@
  MPIE_KillUniverse - Kill all of the processes in a universe
  @*/
int MPIE_KillUniverse( ProcessUniverse *pUniv )
{
    ProcessWorld *world;

    world = pUniv->worlds;
    while (world) {
	MPIE_KillWorld( world );
	world = world->nextWorld;
    }
    return 0;
}

/* Print out the reasons for failure for any processes that did not
   exit cleanly */
void MPIE_PrintFailureReasons( FILE *fp )
{
    int                i;
    int                rc, sig, order;
    ProcessExitState_t exitReason;
    ProcessWorld      *world;
    ProcessApp        *app;
    ProcessState      *pState;
    int                worldnum, wrank;

    world = pUniv.worlds;
    while (world) {
	worldnum = world->worldNum;
	app = world->apps;
	while (app) {
	    pState = app->pState;
	    for (i=0; i<app->nProcess; i++) {
		wrank      = pState[i].wRank;
		rc         = pState[i].exitStatus.exitStatus;
		sig        = pState[i].exitStatus.exitSig;
		order      = pState[i].exitStatus.exitOrder;
		exitReason = pState[i].exitStatus.exitReason;

		/* If signalled and we did not send the signal (INT or KILL)*/
		if (sig && (exitReason != EXIT_KILLED || 
			    (sig != SIGKILL && sig != SIGINT))) {
#ifdef HAVE_STRSIGNAL
		    MPIU_Error_printf( 
				      "[%d]%d:Return code = %d, signaled with %s\n", 
				      worldnum, wrank, rc, strsignal(sig) );
#else
		    MPIU_Error_printf( 
				      "[%d]%d:Return code = %d, signaled with %d\n", 
				      worldnum, wrank, rc, sig );
#endif
		}
		else if (MPIE_Debug || rc) {
		    MPIU_Error_printf( "[%d]%d:Return code = %d\n", 
				       worldnum, wrank, rc );
		}
	    }
	    app    = app->nextApp;
	}
	world = world->nextWorld;
    }
}

/*
  
 */
static void handle_forwardsig( int sig )
{
    ProcessWorld *world;

    world = pUniv.worlds;
    while (world) {
	MPIE_SignalWorld( world, sig );
	world = world->nextWorld;
    }
    return;
}

int MPIE_ForwardSignal( int sig )
{
    MPIE_InstallSigHandler( sig, handle_forwardsig );
    return 0;
}

/*
 * This routine contains the action to take on an abnormal exit from
 * a managed procese.  The normal action is to kill all of the other processes 
 */
int MPIE_OnAbend( ProcessUniverse *p )
{
    if (!p) p = &pUniv;
    MPIE_KillUniverse( p );
    return 0;
}

int MPIE_ForwardCommonSignals( void )
{
    MPIE_ForwardSignal( SIGINT );
    MPIE_ForwardSignal( SIGQUIT );
    MPIE_ForwardSignal( SIGTERM );
#ifdef SIGSTOP
    MPIE_ForwardSignal( SIGSTOP );
#endif
#ifdef SIGCONT
    MPIE_ForwardSignal( SIGCONT );
#endif
    /* Do we want to forward usr1 and usr2? */
    return 0;
}

/*
  Install a signal handler
*/
static void MPIE_InstallSigHandler( int sig, void (*handler)(int) )
{
#ifdef USE_SIGACTION
    struct sigaction oldact;

    /* Get the old signal action, reset the function and 
       if possible turn off the reset-handler-to-default bit, then
       set the new handler */
    sigaction( sig, (struct sigaction *)0, &oldact );
    oldact.sa_handler = (void (*)(int))handler;
#ifdef SA_RESETHAND
    /* Note that if this feature is not supported, there is a race
       condition in the handling of signals, and the OS is fundementally
       flawed */
    oldact.sa_flags   = oldact.sa_flags & ~(SA_RESETHAND);
#endif
    sigaddset( &oldact.sa_mask, sig );
    sigaction( sig, &oldact, (struct sigaction *)0 );
#elif defined(USE_SIGNAL)
    /* Set new handler; ignore old choice */
    (void)signal( sig, handler );
#else
    /* No way to set up sigchld */
#error "Unknown signal handling!"
#endif
}
