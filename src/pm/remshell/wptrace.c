/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* OWNER=gropp */
#include "remshellconf.h"

/* #undef HAVE_PTRACE */

#if defined(HAVE_PTRACE) && defined(HAVE_PTRACE_CONT) 
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ptrace.h>
/* 
 * Ptrace to control execution for handling failures.
 * Ptrace causes the process to stop on any signal (except SIGKILL).
 * fork();
 * IGNORE = 0;
 * ptrace( PTRACE_TRACEME, IGNORE, IGNORE);
 * exec...
 *
 * The parent can use
 * ptrace( PTRACE_CONT, pid, 0 );
 * to cause the process to continue.  PTRACE_KILL to kill, 
 * PTRACE_ATTACH and DETACH for processes not started with TRACEME.
 *
 * wait returns status: 
 * WIFSTOPPED
 * WSTOPSIG
 * option value of WUNTRACED
 *
 * When using this option, it may be necessary to timeout the select
 * on PMI messages more often, perform a nonblocking wait on processes
 * and look for ones that are stopped.
 *
 * Functions to write:
 *    CheckForStopped - Checks for a stopped process and executes
 *    the requested routine (which probably executes a simple command)
 * 
 *    RunOnStopped - Runs a command on the stopped process
 *    
 */
#define MAX_COMMAND_LEN 1024
static char commandOnStopped[MAX_COMMAND_LEN];
static int onStopped = 0;
/* Set the default command */
void SetDefaultCommandOnStopped( void )
{
    char *p = getenv( "MPIEXEC_ONSIG" );
    if (p) 
	MPIU_Strncpy( commandOnStopped, p, MAX_COMMAND_LEN );
}
/* Eventually allow the polling interval to be set by environment/cmdline */
int InitHandleStopped( void ) { return 1; }  /* Make 10 for general use */

/* Set the command to be used on stopped processes */
void SetCommandOnStopped( const char cmd[] )
{
    MPIU_Strncpy( commandOnStopped, cmd, MAX_COMMAND_LEN );
    onStopped = 1;

    /* Check for special cases */
    if (strncmp( commandOnStopped, "traceback", 9 ) == 0) {
	/* FIXME: gdb only reads command from a file! */
	MPIU_Strncpy( commandOnStopped, 
		"gdb -batch -n -x gettb %e %p", MAX_COMMAND_LEN );
    }
    else if (strncmp( commandOnStopped, "gdb", 3 ) == 0) {
	MPIU_Strncpy( commandOnStopped, 
		      "xterm -e \"gdb %e %p\"", MAX_COMMAND_LEN );
    }

}
/*
 * Run the specified command on the given pid.  The following sequences
 * are handled specially:
 * %e - replace with the name of the executable
 * %p - replace with the pid of the process
 * e.g., the command
 *    xterm -e "gdb %e %p" &
 * runs an xterm that runs gdb on the stopped process, in the background.
 */
void RunOnStopped( const char execname[], pid_t pid ) 
{ 
    char c; 
    char fullcommand[MAX_COMMAND_LEN+1]; 
    char *pout, *pin;

    /* Form the full command from the command string */
    pout = fullcommand;
    pin  = commandOnStopped;
    
    while ((c = *pin++) != 0 && (pout - fullcommand) < MAX_COMMAND_LEN) {
	if (c == '%') {
	    if (*pin == 'e') {
		char *eptr = execname;
		pin++;
		/* Replace with the executable name */
		while (*eptr && (pout - fullcommand) < MAX_COMMAND_LEN) {
		    *pout++ = *eptr++;
		}
	    }
	    else if (*pin == 'p') {
		char pidchars[12], *pptr = pidchars;
		pin++;
		/* Replace with the pid */
		snprintf( pidchars, 12, "%d", (int)pid );
		while (*pptr && (pout - fullcommand) < MAX_COMMAND_LEN) {
		    *pout++ = *pptr++;
		}
	    }
	    else {
		*pout++ = c;
		*pout++ = *pin++;
	    }
	}
	else {
	    *pout++ = c;
	}
    }
    if (pout - fullcommand >= MAX_COMMAND_LEN) {
	/* Error - command is too long */
	return;
    }
    /* Add trailing null */
    *pout = 0;

    /* Run this command string in the background and orphaned, but with
       stdout still directed to us */
    /* FIXME: system isn't robust enough for what we want */
    MPIU_Msg_printf( "Running %s\n", fullcommand );
    /* We need to detach before we can run a command that itself wishes to
       use ptrace.  There isn't a good way to do this, but we try
       using PTRACE_DETACH.  What we do use is SIGTSTP, which 
       will often leave the process stopped so that the next command
       can find it.
    */
    ptrace( PTRACE_DETACH, pid, 0, SIGTSTP );
    system( fullcommand );
    /* We could re-attach the process here.  If we don't, we can no
       longer wait on the process.  Instead, we might reattach but turn off
       the handling of events. */
    /* ptrace( PTRACE_ATTACH, pid, 0, 0 ); */
}

/* See if we want to set ptrace for this process.  Putting this into a routine
   allows us to have more complex criteria */
void CheckIfTraced( void )
{
    int rc;
    if (onStopped) {
	rc = ptrace( PTRACE_TRACEME, 0, 0, 0 );
	if (rc < 0) {
	    perror( "Error from ptrace(PTRACE_TRACEME):" );
	}
    }
}

void CheckForStopped( const char execname[] )
{
    pid_t pid;
    int   sig;
    int   client_stat;

    /* ? WUNTRACED */
    while (1) {
	pid = waitpid( -1, &client_stat, WNOHANG );
	if (!pid) return;  /* no stopped process */
	if (WIFSTOPPED(client_stat)) {
	    sig = WSTOPSIG(client_stat);

	    if (sig == SIGTRAP) {
		/* Ignore these signals */
		ptrace( PTRACE_CONT, pid, 0, 0 );
	    }
	    else if (onStopped) {
		/*printf( "Signal is %d %s\n", sig, strsignal(sig) );*/
		/* FIXME: Find this pid in the list of processes; get the 
		   executable name */
		RunOnStopped( execname, pid );
	    }
	}
	else {
	    /* Handle a process exit */
	    /* FIXME: look up pid and see if the process has
	       finalized */
	    HandleWaitStatus( pid, client_stat, NORMAL, 0 );

	    num_exited++;
	    
	}
    }
}
void KillTracedProcesses( void )
{
    int   i;
    pid_t pid;

    for (i=0; i<=maxfdentryInUse; i++) {
	if (fdtable[i].active) {
	    pid = fdtable[i].pid;
	    if (pid > 0) {
		ptrace( PTRACE_KILL, pid, 0, 0 );
	    }
	}
    }
}
#else
/* Dummy routines if ptrace is not available */
int InitHandleStopped( void ) { return 0; }
void SetDefaultCommandOnStopped( void ) {}
void CheckIfTraced( void ) {}
void CheckForStopped( const char cmd[] ) {}
void SetCommandOnStopped( const char cmd[] ) {}
void KillTracedProcesses( void ){}
#endif
