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
 * we are debugging.
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

static volatile int inHandler = 0;
static volatile int skipHandler = 0;
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
	inHandler = 0;
	return;
    }

    DBG_PRINTF( "Entering sigchild handler\n" ); fflush(stdout);
    if (debug) {
	DBG_FPRINTF( stderr, "Waiting for any child on signal\n" );
	fflush( stderr );
    }

    while (1) {
	/* Find out about any children that have exited */
	pid = waitpid( (pid_t)(-1), &prog_stat, WNOHANG );
    
	if (pid <= 0) {
	    /* Generate a debug message if we enter the handler but 
	       do not find a child */
	    if (debug && !foundChild) {
		DBG_FPRINTF( stderr, "Did not find child process!\n" );
		fflush( stderr );
	    }
	    break;
	}
	foundChild = 1;
	/* Receives a child failure or exit.  
	   If *failure*, kill the others */
	if (debug) {
	    DBG_FPRINTF( stderr, "Found process %d in sigchld handler\n", pid );
	    fflush( stderr );
	}
	/* Return value is 0 on success and non zero if the process 
	   was not found */
	SetProcessExitStatus( ptable, pid, prog_stat );
	    /*	    if (killOnAbort) 
		    KillChildren(); */
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
#ifdef FOO
/* Send a given signal to all processes */
void SignalAllProcesses( ProcessTable_t *ptable, int sig, const char msg[] )
{
    int   i, rc;
    pid_t pid;

    for (i=0; i<=ptable->nProcesses; i++) {
	/* FIXME - GONE isn't the right test */
	if (ptable->table[i].state != GONE) {
	    pid = ptable->table[i].pid;
	    if (pid > 0) {
		if (debug) {
		    DBG_PRINTF( "sig %d to %d\n", sig, pid ); fflush( stdout );
		}
		rc = kill( pid, sig );
		if (rc) {
		    /* Check for errors */
		    if (errno != ESRCH) {
			perror( msg );
		    }
		}
	    }
	}
    }
}

/*
 * Kill all processes.  This is called when (a) a child dies with a non-zero 
 * error code or with a signal *and* (b) the "kill-on-failure" feature is
 * selected (on by default).
 */
static inKillChildren = 0;
void KillChildren( ProcessTable_t *ptable )
{
    int i, pid, rc;

    /* DBG_FPRINTF( stderr, "Entering kill children\n" ); */

    /* Indicate within KillChildren */
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
    
    /* Try to wait for the processes */
    for (i=0; i<=ptable->nProcesses; i++) {
	/* FIXME - is GONE the right test? */
	if (ptable->table[i].state != GONE) {
	    pid = ptable->table[i].pid;
	    if (pid > 0) {
		if (debug) {
		    DBG_PRINTF( "Wait on %d\n", pid ); fflush( stdout );
		}
		/* Nonblocking wait */
		rc = waitOnProcess( i, 0, KILLED );
	    }
	}
    }
}
#endif


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
		DBG_FPRINTF( stderr, "Found process %d in table in sigchld handler\n", pid );
		fflush( stderr );
	    }
	    ptable->table[i].state             = GONE;
	    /* Abnormal exit if either rc or sigval is set */
	    ptable->table[i].status.exitStatus = rc;
	    ptable->table[i].status.exitSig    = sigval;
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
