/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "process.h"
/* Use the memory defintions from mpich2/src/include */
#define HAVE_SNPRINTF
#include "mpimem.h"
#include <errno.h>

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
- other - other

  Output Effects:
  The 'ProcessState' fields are allocated and filled in with the process id
  and other information.

  Callbacks:
  To 
  @*/
int MPIE_ForkProcesses( ProcessWorld *pWorld, char *envp[] )
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
	    pState = (ProcessState *)MPIU_Malloc( app->nProcess * sizeof(ProcessState) );
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

	    pid = fork();
	    if (pid < 0) {
		/* Error creating process */
		return -1;
	    }
	    if (pid == 0) {
		/* Child */
		/* exec the process (this call only returns if there is an 
		   error */
		rc = MPIE_ExecProgram( pState[i], envp );
		if (rc) {
		    MPIU_Internal_error_printf( "mpiexec fork failed\n" );
		    /* FIXME: kill children */
		    exit(1);
		}
	    }
	    else {
		/* Parent */
		pState[i].pid = pid;
	    }
	    
	}
	
	app = app->nextApp;
    }
    return i;
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
    client_env[j] = NULL;
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

