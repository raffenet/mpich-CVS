/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
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
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/wait.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#if defined(USE_SIGNAL) || defined(USE_SIGACTION)
#include <signal.h>
#else
#error no signal choice
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include "pmutil.h"

#ifdef NEEDS_STRSIGNAL_DECL
extern char *strsignal(int);
#endif

/* 
 * Signal handler.  Detect a SIGCHLD exit.  The routines are
 *
 * setup_sigchild - Call to install the signal handler
 * handle_sigchild - This is the signal handler
 *
 * If a child exits with a non-zero return code, we may want to kill 
 * the other children.  In most cases, we'll want to kill the other 
 * children if a child dies on a signal.
 * Sometimes we do *not* want to kill the children; particularly when
 * we are debugging.   We also do not want to kill processes during a
 * normal exit, or if the MPI_Init/MPI_Init_thread routines were not called
 * (e.g., if we're running a non-MPI program).  Because of these 
 * considerations, we'll leave the decision to terminate a child process
 * to the code that calls these routines.
 *
 * Because this is a signal handler, it needs access to the process table
 * through a global variable, ptable.
 */


static ProcessTable *ptable = 0;
static int killOnAbort = 0;

static void setup_sigchild( void );
static int SetProcessExitStatus( ProcessTable *, pid_t, int );

/* Call this routine to initialize the sigchild and set the process table
   pointer */
void initPtableForSigchild( ProcessTable *t )
{
    ptable = t;
    setup_sigchild( );
}

/* inHandler and skipHandler are used to control the SIGCHLD handler and
   to avoid (or at least narrow) race conditions */
static volatile int inHandler = 0;
static volatile int skipHandler = 0;
/* nExited is used to keep track of how many processes have exited and
   to set the exitOrder field */
static int      nExited = 0;

/*
 * Note that signals are not queued.  Thus we must process all pending
 * processes, and not be concerned if we are invoked but we have already
 * waited on all processes.
 */
static void handle_sigchild( int sig )
{
    int prog_stat, pid;
    int foundChild = 0;

    /* Set a flag to indicate that we're in the handler, and check 
       to see if we should ignore the signal */
    inHandler = 1;
    if (skipHandler) {
	/* There is a small race condition here.  This should really
	   use a test-and-set primative. */
	inHandler = 0;
	return;
    }

    DBG_PRINTF( ("Entering sigchild handler\n") ); DBG_FFLUSH(stdout);
    if (debug) {
	DBG_FPRINTF( (stderr, "Waiting for any child on signal\n") );
	DBG_FFLUSH( stderr );
    }

    while (1) {
	/* Find out about any children that have exited */
	pid = waitpid( (pid_t)(-1), &prog_stat, WNOHANG );
    
	if (pid <= 0) {
	    /* Generate a debug message if we enter the handler but 
	       do not find a child */
	    if (debug && !foundChild) {
		DBG_FPRINTF( (stderr, "Did not find child process!\n") );
		DBG_FFLUSH( stderr );
	    }
	    break;
	}
	foundChild = 1;
	/* Receives a child failure or exit.  
	   If *failure*, kill the others */
	if (debug) {
	    DBG_FPRINTF( (stderr, "Found process %d in sigchld handler\n", pid ) );
	    DBG_FFLUSH( stderr );
	}
	/* Return value is 0 on success and non zero if the process 
	   was not found */
	SetProcessExitStatus( ptable, pid, prog_stat );
    }
    inHandler = 0;
}

#ifdef USE_SIGACTION
static void setup_sigchild( void )
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
static void setup_sigchild( void )
{
    /* Set new handler; ignore old choice */
    (void)signal( SIGCHLD, handle_sigchild );
}
#else
static void setup_sigchild( void )
{
    /* No way to set up sigchld */
}
#endif

void WaitForChildren( ProcessTable *ptable )
{
    pid_t pid;
    int   i, nactive, prog_stat;

    /* Tell the signal handler to ignore signals */
    skipHandler = 1;

    /* Wait for the handler to exit if it is running */
    while (inHandler) ;

    /* Determine the number of processes that we have left to wait on */
    nactive = 0;
    for (i=0; i<ptable->nProcesses; i++) {
	if (ptable->table[i].state != GONE) nactive++;
    }
    while (nactive) {
	pid = wait( &prog_stat );
	SetProcessExitStatus( ptable, pid, prog_stat );
	nactive --;
    }
}

int ComputeExitStatus( ProcessTable *ptable, int rule )
{
    int i, rc, prc;
    
    rc = 0;
    
    for (i=0; i<ptable->nProcesses; i++) {
	if (ptable->table[i].state == GONE) {
	    prc = ptable->table[i].status.exitStatus;
	    switch (rule) {
	    case 0:  /* Max */
		if (prc > rc) rc = prc;
		break;
	    case 1: /* Sum */
		rc += prc;
		break;
	    default: /* bitwise or */
		rc |= prc;
		break;
	    }
	}
    }
    return rc;
}


/* Send a given signal to all processes */
void SignalAllProcesses( ProcessTable *ptable, int sig, const char msg[] )
{
    int   i, rc;
    pid_t pid;

    for (i=0; i<=ptable->nProcesses; i++) {
	/* FIXME - GONE isn't the right test */
	if (ptable->table[i].state != GONE) {
	    pid = ptable->table[i].pid;
	    if (pid > 0) {
		if (debug) {
		    DBG_PRINTF( ("sig %d to %d\n", sig, pid) ); DBG_FFLUSH( stdout );
		}
		rc = kill( pid, sig );
		if (rc) {
		    /* Check for errors */
		    if (errno != ESRCH) {
			perror( msg );
		    }
		}
	    }
	    ptable->table[i].state = KILLED;
	}
    }
}

/*
 * Kill all processes.  This is called when (a) a child dies with a non-zero 
 * error code or with a signal *and* (b) the "kill-on-failure" feature is
 * selected (on by default).
 *
 * This is provided as a service function that can terminate the processes
 * in the process table.
 */
static int inKillChildren = 0;
void KillChildren( ProcessTable *ptable )
{
    /* DBG_FPRINTF( (stderr, "Entering kill children\n") ); */

    /* Indicate within KillChildren.  This has a slight race and 
     should use a test-and-set */
    if (inKillChildren) return;
    inKillChildren = 1;

    /* Loop through the processes and try to kill them; gently first,
     * then brutally 
     */

#if defined(HAVE_PTRACE) && defined(HAVE_PTRACE_CONT) && 0
    KillTracedProcesses( );
#endif

    SignalAllProcesses( ptable, SIGINT, "Could not kill with SIGINT" );

    /* We should wait here to give time for the processes to exit */
    sleep( 1 );
    SignalAllProcesses( ptable, SIGQUIT, "Could not kill with SIGQUIT" );

    /* We don't wait on the processes; a separate step handles that */
}

static int SetProcessExitStatus( ProcessTable *ptable, 
				 pid_t pid, int prog_stat )
{
    int   rc, sigval, i;

    rc = 0;
    if (WIFEXITED(prog_stat)) {
	rc = WEXITSTATUS(prog_stat);
    }
    sigval = 0;
    if (WIFSIGNALED(prog_stat)) {
	sigval = WTERMSIG(prog_stat);
    }
    /* Look up this pid in the exitstatus */
    for (i=0; i<ptable->nProcesses; i++) {
	if (ptable->table[i].pid == pid) {
	    if (debug) {
		DBG_FPRINTF( (stderr, "Found process %d in table in sigchld handler\n", pid) );
		DBG_FFLUSH( stderr );
	    }
	    ptable->table[i].state             = GONE;
	    /* Abnormal exit if either rc or sigval is set */
	    ptable->table[i].status.exitStatus = rc;
	    ptable->table[i].status.exitSig    = sigval;
	    ptable->table[i].status.exitOrder  = nExited++;
	    if (ptable->table[i].state <= COMMUNICATING &&
		ptable->table[i].state >= ALIVE) {
		/* We have an abnormal exit if it was an MPI job */
		if (sigval) 
		    ptable->table[i].status.exitReason = SIGNALLED;
		else 
		    ptable->table[i].status.exitReason = NOFINALIZE;
	    }
	    else {
		ptable->table[i].status.exitReason = NORMAL;
	    }
	    ptable->nActive--;
	    break;
	}
    }
    if (i == ptable->nProcesses) {
	/* Did not find the matching pid */
	;
	/* FIXME: we could store the pid and status, and use
	   it to handle the race condition where the process
	   exits before the table is initialized */
    }
    return 0;
}

/* Print out the reasons for failure for any processes that did not
   exit cleanly */
void PrintFailureReasons( FILE *fp, ProcessTable *ptable )
{
    int i;
    int rc, sig, order;
    exit_state_t reason;
    ProcessState *pstate = ptable->table;

    for (i=0; i<ptable->nProcesses; i++) {
	rc     = pstate[i].status.exitStatus;
	sig    = pstate[i].status.exitSig;
	order  = pstate[i].status.exitOrder;
	reason = pstate[i].status.exitReason;

	/* If signalled and we did not send the signal (INT or KILL)*/
	if (sig && (reason != KILLED || (sig != SIGKILL && sig != SIGINT))) {
#ifdef HAVE_STRSIGNAL
	    MPIU_Error_printf( 
		     "Return code = %d, signaled with %s\n", rc, 
		     strsignal(sig) );
#else
	    MPIU_Error_printf( 
		     "Return code = %d, signaled with %d\n", rc, sig );
#endif
	}
	else if (debug || rc) {
	    MPIU_Error_printf( "Return code = %d\n", rc );
	}
    }
}
