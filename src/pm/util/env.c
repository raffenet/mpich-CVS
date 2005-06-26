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
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "mpimem.h"
#include "process.h"
#include "env.h"

/*
 * 
 */

/* 
 * This routine may be called by MPIE_Args to handle any environment arguments
 * Returns the number of arguments to skip (0 if argument is not recognized
 * as an environment control)
 */
int MPIE_ArgsCheckForEnv( int argc, char *argv[], ProcessWorld *pWorld,
			  EnvInfo **appEnv )

{
    int      i, incr=0;
    EnvInfo *env;
    char    *cmd;

    if ( strncmp( argv[0], "-env",  4) == 0) {
	if (!*appEnv) {
	    env = (EnvInfo *)MPIU_Malloc( sizeof(EnvInfo) );
	    env->includeAll = 1;
	    env->envPairs   = 0;
	    env->envNames   = 0;
	    *appEnv         = env;
	}
	else 
	    env = *appEnv;
	cmd = argv[0] + 4;
    }
    else if (strncmp( argv[0], "-genv", 5 ) == 0) {
	if (!pWorld->genv) {
	    env = (EnvInfo *)MPIU_Malloc( sizeof(EnvInfo) );
	    env->includeAll = 1;
	    env->envPairs   = 0;
	    env->envNames   = 0;
	    pWorld->genv    = env;
	}
	env = pWorld->genv;
	cmd = argv[0] + 5;
    }
    else 
	return 0;

    /* genv and env commands have the same form, just affect different
       env structures.  We handle this by identifying which structure,
       then checkout the remaining command */
    if (!cmd[0]) {
	/* A basic name value command */
	EnvData *p;
	p             = (EnvData *)MPIU_Malloc( sizeof(EnvData) );
	p->name       = (const char *)MPIU_Strdup( argv[1] );
	p->value      = (const char *)MPIU_Strdup( argv[2] );
	p->envvalue   = 0;
	p->nextData   = env->envPairs;
	env->envPairs = p;
	
	incr = 3;
    }
    else if (strcmp( cmd, "none" ) == 0) {
	env->includeAll = 0;
	incr = 1;
    }
    else if (strcmp( cmd, "list" ) == 0) {
	/* argv[1] has a list of names, separated by commas */
	EnvData *p;
	char    *lPtr = argv[1], *name;
	int      namelen;
	
	while (*lPtr) {
	    name = lPtr++;
	    while (*lPtr && *lPtr != ',') lPtr++;
	    namelen = lPtr - name;
	    p             = (EnvData *)MPIU_Malloc( sizeof(EnvData) );
	    p->value      = 0;
	    p->name       = (const char *)MPIU_Malloc( namelen + 1 );
	    p->envvalue   = 0;
	    for (i=0; i<namelen; i++) ((char *)p->name)[i] = name[i];
	    ((char *)p->name)[namelen] = 0;

	    p->nextData   = env->envNames;
	    env->envNames = p;
	    if (*lPtr == ',') lPtr++;
	}		
	incr = 2;
    }
    else {
	/* Unrecognized env argument. */
	incr = 0;
    }

    return incr;
}

/*
  Setup the environment of a process for a given process state.  
  This handles the options for the process world and app 

  Input Arguments:
  pState - process state structure
  envp   - Base (pre-existing) environment
  maxclient - size of client_envp array

  Output Arguments:
  client_envp - 

  Side Effects:
  If envnone or genvnone was selected, the environment variables in envp
  will be removed with unsetenv().

  Returns the number of items set in client_envp, or -1 on error.
 */
int MPIE_EnvSetup( ProcessState *pState, const char *envp[],
		   char *client_envp[], int maxclient )
{
    ProcessWorld *pWorld;
    ProcessApp   *app;
    EnvInfo      *env;
    EnvData      *wPairs = 0,*wNames = 0, *aPairs = 0, *aNames = 0;
    int          includeAll = 1, j;
    int          irc = 0;
    int          debug = 1;

    app    = pState->app;
    pWorld = app->pWorld;

    /* Get the world defaults */
    env        = pWorld->genv;
    if (env) {
	includeAll = env->includeAll;
	wPairs     = env->envPairs;
	wNames     = env->envNames;
    }

    /* Get the app values (overrides includeAll) */
    env        = app->env;
    if (env) {
	if (includeAll) {
	    /* Let the local env set envnone (there is no way to undo 
	       -genvnone) */
	    includeAll = env->includeAll;
	}
	aPairs     = env->envPairs;
	aNames     = env->envNames;
    }

    if (includeAll) {
	for (j=0; envp[j] && j < maxclient; j++) {
	    putenv( (char *)envp[j] );
	    client_envp[j] = (char *)envp[j];
	}
	irc = j;
    }
    else {
	for (j=0; envp[j]; j++) {
	    unsetenv( (char *)envp[j] );
	}
	irc = 0;
    }

    while (wPairs) {
	if (putenv( (char *)(wPairs->envvalue) )) {
	    irc = -1;
	    if (debug) perror( "putenv(wPairs) failed: " );
	}
	wPairs = wPairs->nextData;
    }

    while (wNames) {
	if (putenv( (char *)(wNames->envvalue) )) {
	    irc = -1;
	    if (debug) perror( "putenv(wNames) failed: " );
	}
	wNames = wNames->nextData;
    }

    while (aPairs) {
	if (putenv( (char *)(aPairs->envvalue) )) {
	    irc = -1;
	    if (debug) perror( "putenv(aPairs) failed: " );
	}
	aPairs = aPairs->nextData;
    }

    while (aNames) {
	if (putenv( (char *)(aNames->envvalue) )) {
	    irc = -1;
	    if (debug) {
		perror( "putenv(aNames) failed: " );
	    }
	}
	aNames = aNames->nextData;
    }

    return irc;
}

/*
  Initialize the environment data
  
  Builds the envvalue version of the data, using the given data.
  if getValue is true, get the value for the name with getenv .
 */
int MPIE_EnvInitData( EnvData *elist, int getValue )
{
    const char *value;
    char       *str;
    int        slen, rc;

    while (elist) {
	if (getValue) {
	    value = (const char *)getenv( elist->name );
	}
	else {
	    value = elist->value;
	}
	if (!value) {
	    /* Special case for an empty value */
	    value = "";
	}
	slen = strlen( elist->name ) + strlen(value) + 2;
	str  = (char *)MPIU_Malloc( slen );
	if (!str) {
	    return 1;
	}
	MPIU_Strncpy( str, elist->name, slen );
	if (value && *value) {
	    rc = MPIU_Strnapp( str, "=", slen );
	    rc += MPIU_Strnapp( str, value, slen );
	    if (rc) {
		return 1;
	    }
	}
	elist->envvalue = (const char *)str;

	elist = elist->nextData;
    }
    return 0;
}
