/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
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
#if defined(USE_SIGNAL) || defined(USE_SIGACTION)
#include <signal.h>
#else
#error no signal choice
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include "remshell.h"

#ifdef USE_SIGCHLD_HANDLER
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


static ProcessTable_t *ptable = 0;
static int killOnAbort = 0;

/* Call this routine to initialize the sigchild and set the process table
   pointer */
void initPtableForSigchild( ProcessTable_t *t )
{
    ptable = t;
    setup_sigchild( );
}

/*
 * Note that signals are not queued.  Thus we must process all pending
 * processes, and not be concerned if we are invoked but we have already
 * waited on all processes.
 */
int handle_sigchild( int sig )
{
    int prog_stat, pid, rc, sigval, i;

    DBG_PRINTF( "Entering sigchild handler\n" ); fflush(stdout);
    if (debug) {
	DBG_FPRINTF( stderr, "Waiting for any child on signal\n" );
	fflush( stderr );
    }

    while (1) {
	/* Find out about any children that have exited */
	pid = waitpid( (pid_t)(-1), &prog_stat, WNOHANG );
    
	if (pid <= 0) {
	    if (debug) {
		DBG_FPRINTF( stderr, "Did not find child process!\n" );
		fflush( stderr );
	    }
	    break;
	}
	/* Receives a child failure or exit.  
	   If *failure*, kill the others */
	if (debug) {
	    DBG_FPRINTF( stderr, "Found process %d in sigchld handler\n", pid );
	    fflush( stderr );
	}
	rc = 0;
	if (WIFEXITED(prog_stat)) {
	    rc = WEXITSTATUS(prog_stat);
	}
	sigval = 0;
	if (WIFSIGNALED(prog_stat)) {
	    sigval = WTERMSIG(prog_stat);
	}
	if (sigval || rc) {
	    /* Look up this pid in the exitstatus */
	    for (i=0; i<ptable->nProcesses; i++) {
		if (ptable->table[i].pid == pid) {
		    if (debug) {
			DBG_FPRINTF( stderr, "Found process %d in table in sigchld handler\n", pid );
			fflush( stderr );
		    }
		    ptable->table[i].state      = GONE;
		    ptable->table[i].exitStatus = rc;
		    ptable->table[i].exitSig    = sigval;
		    ptable->nActive--;
		    break;
		}
	    }
	    if (i == ptable->nProcesses) {
		/* Did not find the matching pid */
		;
	    }
	    if (killOnAbort) 
		KillChildren();
	}
    }
}

#ifdef USE_SIGACTION
void setup_sigchild( void )
{
    struct sigaction oldact;

    /* Get the old signal action, reset the function and 
       if possible turn off the reset-handler-to-default bit, then
       set the new handler */
    sigaction( SIGCHLD, (struct sigaction *)0, &oldact );
    oldact.sa_handler = handle_sigchild;
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
void setup_sigchild( void )
{
    /* Set new handler; ignore old choice */
    (void)signal( SIGCHLD, handle_sigchild );
}
#else
void setup_sigchild( void )
{
}
#endif
#endif

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
