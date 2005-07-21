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
   contact this program, rather than just sharing a pipe.  This also allows the
   forker to start other programs, such as debuggers.  To enable this feature,
   both the compile-time definition MPIEXEC_ALLOW_PORT and the environment 
   variable MPIEXEC_USE_PORT must be set.

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
#include "env.h"
#include "simple_pmiutil.h"

/* mpimem.h contains prototypes for MPIU_Strncpy etc. */
#include "mpimem.h"

typedef struct { PMISetup pmiinfo; IOLabelSetup labelinfo; } SetupInfo;

/* Forward declarations */
int mypreamble( void *, ProcessState * );
int mypostfork( void *, void *, ProcessState* );
int mypostamble( void *, void *, ProcessState* );
int myspawn( ProcessWorld *, void * );

/* Set printFailure to 1 to get an explanation of the failure reason
   for each process when a process fails */
static int printFailure = 0;
/* Set usePort to 1 if a host:port should be used insted of inheriting 
   an FD to a socketpair.  Meaningful only if code is compilete with 
   -DMPIEXEC_ALLOW_PORT */
static int usePort = 0;

/* Note that envp is common but not standard */
int main( int argc, char *argv[], char *envp[] )
{
    int          rc;
    int          erc = 0;  /* Other (exceptional) return codes */
    int          reason, signaled = 0;
    SetupInfo    s;

    /* MPIE_ProcessInit initializes the global pUniv */
    MPIE_ProcessInit();
    /* Set a default for the universe size */
    pUniv.size  = 64;

    /* Set defaults for any arguments that are options.  Also check the
       environment for special options, such as debugging.  Set 
       some defaults in pUniv */
    MPIE_CheckEnv( &pUniv, 0, 0 );
    IOLabelCheckEnv( );

    /* Handle the command line arguments.  Use the routine from util/cmnargs.c
       to fill in the universe */
    MPIE_Args( argc, argv, &pUniv, 0, 0 );
    /* If there were any soft arguments, we need to handle them now */
    rc = MPIE_InitWorldWithSoft( &pUniv.worlds[0], pUniv.size );

    if (MPIE_Debug) MPIE_PrintProcessUniverse( stdout, &pUniv );

    DBG_PRINTF( ("timeout_seconds = %d\n", pUniv.timeout) );

#ifdef MPIEXEC_ALLOW_PORT
    /* Establish a host:port for use with the created processes.
       In this code, the major use is to illustrate the use of a port for 
       connecting to processes rather than an FD that is inherited through the
       fork and exec.  This could allow gforker to support the "singleton"
       init, allowing an MPI process to contact a waiting mpiexec that 
       would serve as a process manager.  This option is not implemented */
    if (getenv("MPIEXEC_USE_PORT")) {
	s.pmiinfo.portName = (char *)malloc( 1024 );
	if (!s.pmiinfo.portName) {
	    MPIU_Error_printf( "Failed to allocate storage for portName" );
	}
	if (PMIServSetupPort( &pUniv, s.pmiinfo.portName, 1024 )) {
	    MPIU_Error_printf( "Failed to setup a host:port\n" );
	}
	else {
	    usePort = 1;
	}
    }
    else {
	s.pmiinfo.portName = 0;
    }
#endif

    PMIServInit(myspawn,&s);
    s.pmiinfo.pWorld = &pUniv.worlds[0];
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
    /* For now, always print if a process died on an uncaught signal */
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

/* ------------------------------------------------------------------------- */
/* Redirect stdout and stderr to a handler */
int mypreamble( void *data, ProcessState *pState )
{
    SetupInfo *s = (SetupInfo *)data;

    IOLabelSetupFDs( &s->labelinfo );
    /* In the inherit-FD case, this routine creates the FD that will
       be used for PMI commands between the mpiexec process and the
       fork/exec'ed process */
    PMISetupSockets( usePort, &s->pmiinfo );

    /* Tell MPIE_ForkProcesses not to include the PMI environment if
       we are using a port, and use the PMI_PORT and ID instead */
    if (usePort) pState->initWithEnv = 0;
    
    return 0;
}
/* Close one side of each pipe pair and replace stdout/err with the pipes */
int mypostfork( void *predata, void *data, ProcessState *pState )
{
    SetupInfo *s = (SetupInfo *)predata;

    IOLabelSetupInClient( &s->labelinfo );
    PMISetupInClient( usePort, &s->pmiinfo );

    return 0;
}
/* Close one side of the pipe pair and register a handler for the I/O */
int mypostamble( void *predata, void *data, ProcessState *pState )
{
    SetupInfo *s = (SetupInfo *)predata;

    IOLabelSetupFinishInServer( &s->labelinfo, pState );
    PMISetupFinishInServer( usePort, &s->pmiinfo, pState );

    return 0;
}

int myspawn( ProcessWorld *pWorld, void *data )
{
    SetupInfo    *s = (SetupInfo *)data;
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
    MPIE_Putenv( pWorld, "PMI_SPAWNED=1" );
    MPIE_ForkProcesses( pWorld, 0, mypreamble, s,
			mypostfork, 0, mypostamble, 0 );
    return 0;
}
