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

  @*/
int MPIE_ForkProcesses( ProcessWorld *pWorld, char *envp[],
			int (*preamble)(void*), void *preambleData,
			int (*postfork)(void*,void*,ProcessState*), 
			void *postforkData,
			int (*postamble)(void*,void*,ProcessState*), 
			void *postambleData
			)
{
    pid_t         pid;
    ProcessApp   *app;
    ProcessState *pState;
    int          wRank = 0;    /* Rank in this comm world of the process */
    int          i, rc;

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

	for (i=0; i<app->nProcess; i++) {
	    pState[i].app    = app;
	    pState[i].wRank                 = wRank++;
	    pState[i].status                = PROCESS_UNINITIALIZED;
	    pState[i].exitStatus.exitReason = EXIT_NOTYET;
	    pState[i].pid                   = -1;

	    /* Execute any preamble */
	    if (preamble) {
		rc = (*preamble)( preambleData );
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
		pState[i].pid = pid;

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
    return i;
}

/*@
  MPIE_ProcessGetExitStatus - Return an integer exit status based on the
  return status of all processes in the process universe.
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
		if (pState->exitStatus.exitStatus > rc) {
		    rc = pState->exitStatus.exitStatus;
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
    int j, rc;
    ProcessApp *app;

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

    app = pState->app;

    /* build environment for client */
    for ( j = 0; envp[j] && j < MAX_CLIENT_ENV-7; j++ )
	client_env[j] = envp[j]; /* copy mpiexec environment */

    if (j == MAX_CLIENT_ENV-7) {
	MPIU_Error_printf( "Environment is too large (max is %d)\n",
			   MAX_CLIENT_ENV-7);
	exit(-1);
    }
    /*    if (pmifd >= 0) {
	MPIU_Snprintf( env_pmi_fd, MAXNAMELEN, "PMI_FD=%d" , pmifd );
	client_env[j++] = env_pmi_fd;
    }
    */
    MPIU_Snprintf( env_pmi_rank, MAXNAMELEN, "PMI_RANK=%d", pState->wRank );
    client_env[j++] = env_pmi_rank;
    MPIU_Snprintf( env_pmi_size, MAXNAMELEN, "PMI_SIZE=%d", app->nProcess );
    client_env[j++] = env_pmi_size;
    /*    MPIU_Snprintf( env_pmi_debug, MAXNAMELEN, "PMI_DEBUG=%d", debug );
	  client_env[j++] = env_pmi_debug; */
    /* FIXME: Get the correct universsize */
    MPIU_Snprintf( env_appnum, MAXNAMELEN, "MPI_APPNUM=%d", app->myAppNum );
    client_env[j++] = env_appnum;
    MPIU_Snprintf( env_universesize, MAXNAMELEN, "MPI_UNIVERSE_SIZE=%d", 0 );
    client_env[j++] = env_universesize;
    client_env[j]   = 0;
    for ( j = 0; client_env[j]; j++ )
	if (putenv( client_env[j] )) {
	    MPIU_Internal_sys_error_printf( "mpiexec", errno, 
			     "Could not set environment %s", client_env[j] );
	    exit( 1 );
	}
    
    /* change working directory if specified, replace argv[0], and exec client */
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

    world = pUniv.worlds;
    while (world) {
	app = world->apps;
	while (app) {
	    pState = app->pState;
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
/* SIGNALS and CHILDRED                                                      */
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
	MPIE_SetupSigChld();
#endif
	return;
    }

    DBG_PRINTF( ("Entering sigchild handler\n") ); DBG_FFLUSH(stdout);
#if 0
    if (debug) {
	DBG_FPRINTF( (stderr, "Waiting for any child on signal\n") );
	DBG_FFLUSH( stderr );
    }
#endif

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
#if 0
	    if (debug && !foundChild) {
		DBG_FPRINTF( (stderr, "Did not find child process!\n") );
		DBG_FFLUSH( stderr );
	    }
#endif
	    break;
	}
	foundChild = 1;
	/* Receives a child failure or exit.  
	   If *failure*, kill the others */
#if 0
	if (debug) {
	    DBG_FPRINTF( (stderr, "Found process %d in sigchld handler\n", pid ) );
	    DBG_FFLUSH( stderr );
	}
#endif
	pState = MPIE_FindProcessByPid( pid );
	if (pState) {
	    MPIE_ProcessSetExitStatus( pState, prog_stat );
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
    MPIE_SetupSigChld();
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
     MPIE_SetupSigChld();
     pUniv.worlds = 0;
}

#ifdef USE_SIGACTION
/*@
  MPIE_ProcessInit - Initialize the support for process creation

  Notes:
  The major chore of this routine is to set the 'SIGCHLD' signal handler
  @*/
void MPIE_SetupSigChld( void )
{
    struct sigaction oldact;

    /* Get the old signal action, reset the function and 
       if possible turn off the reset-handler-to-default bit, then
       set the new handler */
    sigaction( SIGCHLD, (struct sigaction *)0, &oldact );
    oldact.sa_handler = (void (*)(int))handle_sigchild;
#ifdef SA_RESETHAND
    /* Note that if this feature is not supported, there is a race
       condition in the handling of signals, and the OS is fundementally
       flawed */
    oldact.sa_flags   = oldact.sa_flags & ~(SA_RESETHAND);
#endif
    sigaddset( &oldact.sa_mask, SIGCHLD );
    sigaction( SIGCHLD, &oldact, (struct sigaction *)0 );
}
#elif defined(USE_SIGNAL)
void MPIE_SetupSigChld( void )
{
    /* Set new handler; ignore old choice */
    (void)signal( SIGCHLD, handle_sigchild );
}
#else
void MPIE_SetupSigChld( void )
{
    /* No way to set up sigchld */
#error "Unknown signal handling!  This routine must set a signal for SIGCHLD"
}
#endif


#if 0
int MPIE_WaitProcesses( ProcessUniverse *pUniv )
{
    ProcessWorld *world;
    ProcessApp   *app;
    ProcessState *pState;
    int           i, nactive;
    pid_t         pid;
    int           prog_stat;

    /* Determine the number of processes that we have left to wait on */
    nactive = 0;
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

    while (nactive > 0) {
	pid = wait( &prog_stat );
	pState = MPIE_FindProcessByPid( pUniv, pid );
	if (pState) {
	    int rc = 0, sigval = 0;
	    if (WIFEXITED(prog_stat)) {
		rc = WEXITSTATUS(prog_stat);
	    }
	    if (WIFSIGNALED(prog_stat)) {
		sigval = WTERMSIG(prog_stat);
	    }
	    pState->exitStatus.exitStatus = rc;
	    pState->exitStatus.exitSig    = sigval;
	    if (sigval) 
		pState->exitStatus.exitReason = EXIT_SIGNALLED;
	    else {
		if (pState->status >= PROCESS_ALIVE &&
		    pState->status <= PROCESS_COMMUNICATING) 
		    pState->exitStatus.exitReason = EXIT_NOFINALIZE;
		else 
		    pState->exitStatus.exitReason = EXIT_NORMAL;
	    }
	    nactive --;
	}
    }
}

#endif
