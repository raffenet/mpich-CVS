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

/* 
 * This routine can be called to handle the result of a wait.  
 * This is in a separate routine so that it can be used anywhere
 * waitpid or wait are called.
 */
void HandleWaitStatus( ProcessState *ps, int client_stat, 
		       int has_finalized ) 
{
    /* Get the status of the exited process */
    if (WIFEXITED(client_stat)) {
	/* true if the process exited normally */
	ps->exitStatus = WEXITSTATUS(client_stat);
    }
    else {
	ps->exitStatus = -1; /* For unknown, since valid
				returns in 0-255 */
    }
	
    if (WIFSIGNALED(client_stat)) {
	ps->exitSig        = WTERMSIG(client_stat);
	/* ps->exitReason     = sigstate; */
	/* Add to ptable the number aborted ? */
    }
    else {
	ps->exitSig    = 0;
	ps->exitReason = has_finalized ? NORMAL : NOFINALIZE;

    }
}

/*
 * Wait on the process in fdentry[idx].  Do a blocking wait if 
 * requested.  If sigstate is not "NORMAL", set the exit state for 
 * the process to this value if it exits with a signal.  This is used
 * to separate processes that died because mpiexec sent them a signal
 * from processes that died because they received a signal from a 
 * different source (e.g., SIGFPE or SIGSEGV)
 */
int waitOnProcess( ProcessState *ps, int blocking )
{
    int client_stat, rc, has_finalized;
    pid_t pid;

    /* Careful here: we may want to use WNOHANG; wait a little, then
       do something like kill the process */
    if (debug) {
	DBG_FPRINTF( stderr, "Waiting on status of process %d\n",
		 ps->pid );
	fflush( stderr );
    }
    pid = ps->pid;
    if (pid <= 0) return -1;

    if (blocking)
	rc = waitpid( pid, &client_stat, 0 );
    else {
	rc = waitpid( pid, &client_stat, WNOHANG );
	if (rc == 0) return 0;
    }
    if (rc < 0) {
	MPIU_Internal_error_printf( "Error waiting for process!" );
	perror( "Reason: " );
	return 0;
    }
    if (debug) {
	DBG_FPRINTF( stderr, "Wait on %d completed\n", pid );
	fflush( stderr );
    }

    has_finalized = ps->state == FINALIZED;
    HandleWaitStatus( ps, client_stat, has_finalized );

    /* Add to ptable the number exited? */
    /* (We need to know when we're done */

    return 0;
}

int mpiexecEndAll( ProcessTable_t *ptable )
{
    int i;
    ProcessState *ps;

    for (i=0; i<ptable->nProcesses; i++) {
	ps = &ptable->table[i];

	if (ps->state != GONE) {
	    /* Check on the process.  If not dead, kill it */
	}
    }
}
