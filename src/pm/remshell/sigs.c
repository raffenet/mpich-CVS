/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "remshellconf.h"

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
 */

#if defined(USE_SIGNAL) || defined(USE_SIGACTION)
#include <signal.h>
#else
#error no signal choice
#endif

int handle_sigchild( int sig )
{
    int prog_stat, pid, rc, sigval, i;

    if (debug) {
	DBG_FPRINTF( stderr, "Waiting for any child on signal\n" );
	fflush( stderr );
    }
    pid = waitpid( (pid_t)(-1), &prog_stat, WNOHANG );
    
    if (pid > 0) {
	/* Receives a child failure or exit.  If *failure*, kill the others */
	if (debug) {
	    DBG_FPRINTF( stderr, "Found process %d\n", pid );
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
	    for (i=0; i<maxfdentryInUse ; i++) {
		if (fdtable[i].pid == pid) {
		    if (debug) {
			DBG_FPRINTF( stderr, "Found process %d\n", pid );
			fflush( stderr );
		    }
		    fdtable[i].active   = 0;
		    exitstatus[i].rc  = rc;
		    exitstatus[i].sig = sigval;
		    break;
		}
	    }
	    if (i == numprocs) {
		/* Did not find the matching pid */
		;
	    }
	    if (killOnAbort) 
		KillChildren();
	}
    }
    else {
	if (debug) {
	    DBG_FPRINTF( stderr, "Did not find child process!\n" );
	    fflush( stderr );
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
void SignalAllProcesses( int sig, const char msg[] )
{
    int   i, rc;
    pid_t pid;

    for (i=0; i<=maxfdentryInUse; i++) {
	if (fdtable[i].active) {
	    pid = fdtable[i].pid;
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
void KillChildren( void )
{
    int i, pid, rc;

    /* DBG_FPRINTF( stderr, "Entering kill children\n" ); */

    /* Indicate within KillChildren */
    if (inKillChildren) return;
    inKillChildren = 1;

    /* Loop through the processes and try to kill them; gently first,
     * then brutally 
     */
    
    KillTracedProcesses( );

    SignalAllProcesses( SIGINT, "Could not kill with sigint" );

    /* We should wait here to give time for the processes to exit */
    
    sleep( 1 );
    SignalAllProcesses( SIGQUIT, "Could not kill with sigquit" );
    
    /* Try to wait for the processes */
    for (i=0; i<=maxfdentryInUse; i++) {
	if (fdtable[i].active) {
	    pid = fdtable[i].pid;
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
