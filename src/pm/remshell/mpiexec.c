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

#include "remshellconf.h"
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>

#include "pmutil.h"
#include "process.h"
#include "cmnargs.h"
#include "pmiserv.h"
#include "ioloop.h"
#include "labelout.h"
#include "rm.h"
#include "simple_pmiutil.h"

/* mpimem.h contains prototypes for MPIU_Strncpy etc. */
#include "mpimem.h"

typedef struct { PMISetup pmiinfo; IOLabelSetup labelinfo; } SetupInfo;

/* Forward declarations */
int mypreamble( void *, ProcessState* );
int mypostfork( void *, void *, ProcessState* );
int mypostamble( void *, void *, ProcessState* );
int myspawn( ProcessWorld *, void * );

/* Set printFailure to 1 to get an explanation of the failure reason
   for each process when a process fails */
static int printFailure = 0;

#ifndef MAX_PORT_STRING
#define MAX_PORT_STRING 1024
#endif

/* Note that envp is common but not standard */
int main( int argc, char *argv[], char *envp[] )
{
    int          rc, erc, signaled;
    int          reason;
    SetupInfo    s;
    int          fdPMI;
    char         portString[MAX_PORT_STRING];

    MPIE_ProcessInit();
    /* Set a default for the universe size */
    pUniv.size = 64;

    /* Set defaults for any arguments that are options.  Also check the
       environment for special options, such as debugging.  Set 
       some defaults in pUniv */
    MPIE_CheckEnv( &pUniv, 0, 0 );
    IOLabelCheckEnv( );

    /* Handle the command line arguments.  Use the routine from util/cmnargs.c
       to fill in the universe */
    MPIE_Args( argc, argv, &pUniv, 0, 0 );
    rc = MPIE_InitWorldWithSoft( &pUniv.worlds[0], pUniv.size );
    /* If there were any soft arguments, we need to handle them now */
    rc = MPIE_ChooseHosts( &pUniv.worlds[0], MPIE_ReadMachines, 0 );

    if (MPIE_Debug) MPIE_PrintProcessUniverse( stdout, &pUniv );

    DBG_PRINTF( ("timeout_seconds = %d\n", pUniv.timeout) );

    /* Get the common port for creating PMI connections to the created
       processes */
    PMIServGetPort( &fdPMI, portString, sizeof(portString) );
    MPIE_IORegister( fdPMI, IO_READ, PMIServAcceptFromPort, 0 );

#ifdef USE_MPI_STAGE_EXECUTABLES
    /* Hook for later use in staging executables */
    if (?stageExes) {
	rc = MPIE_StageExecutables( &pUniv.worlds[0] );
	if (!rc) ...;
    }
#endif    

    PMIServInit(myspawn,0);
    PMISetupNewGroup( pUniv.worlds[0].nProcess, 0 );
    MPIE_ForwardCommonSignals();
    MPIE_ForkProcesses( &pUniv.worlds[0], envp, mypreamble, &s,
			mypostfork, 0, mypostamble, 0 );
    reason = MPIE_IOLoop( pUniv.timeout );

    if (reason == IOLOOP_TIMEOUT) {
	/* Exited due to timeout.  Generate an error message and
	   terminate the children */
	MPIU_Error_printf( "Timeout of %d minutes expired; job aborted\n",
			 pUniv.timeout / 60 );
	erc = 1;
	MPIE_KillUniverse( &pUniv );
    }

    /* Wait for all processes to exit and gather information on them.
       We do this through the SIGCHLD handler. We also bound the length
       of time that we wait to 2 seconds.
    */
    MPIE_WaitForProcesses( &pUniv, 2 );

    /* Compute the return code (max for now) */
    rc = MPIE_ProcessGetExitStatus( &signaled );

    /* Optionally provide detailed information about failed processes */
    if ( (rc && printFailure) || signaled) 
	MPIE_PrintFailureReasons( stderr );

    /* If the processes exited normally (or were already gone) but we
       had an exceptional exit, such as a timeout, use the erc value */
    if (!rc && erc) rc = erc;

    return( rc );
}

void mpiexec_usage( const char *msg )
{
    if (msg)
	MPIU_Error_printf( msg );
    MPIU_Usage_printf( "Usage: mpiexec %s\n", MPIE_ArgDescription() );
    exit( -1 );
}

/* Redirect stdout and stderr to a handler */
int mypreamble( void *data, ProcessState *pState )
{
    SetupInfo *s = (SetupInfo *)data;

    IOLabelSetupFDs( &s->labelinfo );
    PMISetupSockets( 0, &s->pmiinfo );
    
    return 0;
}
/* Close one side of each pipe pair and replace stdout/err with the pipes */
int mypostfork( void *predata, void *data, ProcessState *pState )
{
    SetupInfo *s = (SetupInfo *)predata;

    IOLabelSetupInClient( &s->labelinfo );
    PMISetupInClient( 1, &s->pmiinfo );

    return 0;
}
int myexec( void *predata, void *data, ProcessState *pState, 
	    char *argv[] )
{
}

/* Close one side of the pipe pair and register a handler for the I/O */
int mypostamble( void *predata, void *data, ProcessState *pState )
{
    SetupInfo *s = (SetupInfo *)predata;

    IOLabelSetupFinishInServer( &s->labelinfo, pState );
    PMISetupFinishInServer( 1, &s->pmiinfo, pState );

    return 0;
}

int myspawn( ProcessWorld *pWorld, void *data )
{
    SetupInfo    s;
    ProcessWorld *p, **pPtr;

    p = pUniv.worlds;
    pPtr = &(pUniv.worlds);
    while (p) {
	pPtr = &p->nextWorld;
	p    = *pPtr;
    }
    *pPtr = pWorld;

    /* Fork Processes may call a routine that is passed s but not pWorld;
       this makes sure that all routines can access the current world */
    s->pmiinfo.pWorld = pWorld;

    /* FIXME: This should be part of the PMI initialization in the clients */
    putenv( "PMI_SPAWNED=1" );

    MPIE_ForkProcesses( pWorld, 0, mypreamble, &s,
			mypostfork, 0, mypostamble, 0 );
    return 0;
}

/* Temp test for the replacement for the simple "spawn == fork" */

/*
 * Approach:
 * Processes are created using a remote shell program. This requires
 * changing the command line from
 *
 *  a.out args ...
 * 
 * to 
 *
 * remshell-program remshell-args /bin/sh -c PMI_PORT=string && 
 *            export PMI_PORT && PMI_ID=rank-in-world && export PMI_ID &&
 *            a.out args
 *
 * (the export PMI_PORT=string syntax is not valid in all versions of sh)
 *
 * Using PMI_ID ensures that we correctly identify each process (this was
 * a major problem in the setup used by the p4 device in MPICH1).  
 * Using environment variables instead of command line arguments keeps
 * the commaand line clean.  
 *
 * Two alternatives should be considered
 * 1) Use an intermediate manager.  This would allow us to set up the
 *    environment as well:
 *    remshell-program remshell-args manager -port string
 *    One possibilty for the manager is the mpd manager
 * 2) Use the secure server (even the same one as in MPICH1); then 
 *    there is no remote shell command.
 * 
 * We can handle the transformation of the command line by adding a
 * to the postfork routine; this is called after the fork but before the
 * exec, and it can change the command line by making a copy of the app 
 * structure, changing the command line, and setting the pState structure 
 * to point to this new app (after the fork, these changes are visable only
 * to the forked process).
 *
 * Enhancements: 
 * Allow the code to avoid the remote shell if the process is being created 
 * on the local host. 
 *
 * Handle the user of -l username and -n options to remshell
 * (-n makes stdin /dev/null, necessary for backgrounding).
 * (-l username allows login to hosts where the user's username is 
 * different)
 *
 * Provide an option to add a backslash before any - to deal with the
 * serious bug in the GNU inetutils remote shell programs that process
 * *all* arguments on the remote shell command line, even those for the
 * *program*!
 *
 * To best support the errcodes return from MPI_Comm_spawn, 
 * we need a way to communicate the array of error codes back to the
 * spawn and spawn multiple commands.  Query: how is that done in 
 * PMI?  
 * 
 */

/* ----------------------------------------------------------------------- */
/* Convert the remote shell command into argv format                       */
/* The command may be specified as a string with blanks separating the     */
/* arguments, either from the default, an environment variable, or         */
/* eventually the machines file (allowing different options for each host  */
/* or the command line.                                                    */
/* Returns the number of arguments                                         */
/* For example, this allows "ssh -2" as a command                          */
/* Allow the environment variable MPIEXEC_REMSHELL to set the remote shell */
/* program to use                                                          */
/* ----------------------------------------------------------------------- */
const char defaultRemoteshell[] = DEFAULT_REMOTE_SHELL;

#define MAX_REMSHELL_ARGS 10
int MPIE_GetRemshellArgv( char *argv[], int nargv )
{
    static char *(remshell[MAX_REMSHELL_ARGS]);
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

	    remshell[remargs] = (char *)MPIU_Malloc( len + 1 ); 
	    MPIU_Strncpy( remshell[remargs], rem, len );
	    remshell[remargs][len] = 0;
	    remargs++;
	    if (next_parm) {
		rem = next_parm + 1;
		while (*rem == ' ') rem++;
		if (remargs >= MAX_REMSHELL_ARGS) {
		    /* FIXME */
		    MPIU_Error_printf( "Remote shell command is too complex\n" );
		    exit(1);
		}
	    }
	    else {
		rem = 0;
	    }
	}
    }

    /* remshell contains the command.  Copy into argv and return the
       number of args.  We just copy *pointers* because any variable 
       fields will be replaced by the other commands */
    for (i=0; i<remargs; i++) 
	argv[i] = remshell[i];
    return remargs;
}
