/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/* ----------------------------------------------------------------------- */
/* A simple resource manager.  
 * This file implements a simple resource manager.  By implementing the 
 * interfaces in this file, other resource managers can be used.
 *
 * The interfaces are:
 * int mpiexecRMChooseHosts( ProcessTable_t *ptable )
 *    Set the host field for each of the processes
 *
 * int mpiexecRMProcessArg( int argc, char *argv[], void *extra )
 *    Called by the top-level argument processor for unrecognized arguments;
 *    allows the resource manager to use the command line.  If no command
 *    line options are allowed, this routine simply returns zero.
 *
 */
/* ----------------------------------------------------------------------- */

#include "pmutilconf.h"

#include <stdio.h>
#include <ctype.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "pmutil.h"

/* ----------------------------------------------------------------------- */
/* Determine the hosts                                                     */
/*                                                                         */
/* For each requested process that does not have an assigned host yet,     */
/* use information from a machines file to fill in the choices             */
/* ----------------------------------------------------------------------- */
/* This structure is used as part of the code to assign machines to 
   processes */
typedef struct {
    int nHosts; 
    char **hname;
} MachineTable;

static MachineTable *mpiexecReadMachines( const char *, int );

/* Choose the hosts for the processes in the ProcessList.  In
   addition, expand the list into a table with one entry per process.
*/
int mpiexecChooseHosts( ProcessList *plist, int nplist, 
			ProcessTable *ptable )
{
    int i, k, nNeeded=0, ntest;
    const char *arch;
    MachineTable *mt;

    /* First, determine how many processes require host names */
    for (i=0; i<ptable->nProcesses; i++) {
	if (!ptable->table[i].hostname) nNeeded++;
    }
    if (nNeeded == 0) return 0;

    /* Read the appropriate machines file.  There may be multiple files, 
       one for each requested architecture.  We'll read one machine file
       at a time, filling in all of the processes for each particular 
       architecture */
    /* ntest is used to ensure that we exit this loop in the case that 
       there are no machines of the requested architecture */
    ntest = ptable->nProcesses;
    while (nNeeded && ntest--) {
	for (i=0; i<ptable->nProcesses; i++) {
	    if (!ptable->table[i].hostname) break;
	}
	/* Read the machines file for this architecture.  Use the
	   default architecture if none selected */
	arch = ptable->table[i].arch;
	mt = mpiexecReadMachines( arch, nNeeded );
	if (!mt) {
	    /* FIXME : needs an error message */
	    /* By default, run on local host? */
	    if (1) {
		for (; i<ptable->nProcesses; i++) {
		    if ((!arch || 
			 (strcmp( ptable->table[i].arch, arch )== 0)) &&
			!ptable->table[i].hostname) {
			ptable->table[i].hostname = "localhost";
			nNeeded--;
		    }
		}
		continue;
	    }
	    return 1;
	}
	if (mt->nHosts == 0) {
	    if (arch) {
		MPIU_Error_printf( "No machines specified for %s\n", arch );
	    }
	    else {
		MPIU_Error_printf( "No machines specified\n" );
	    }
		
	    return 1;
	}
	/* Assign machines to all processes with this arch */
	k = 0;
	/* Start from the first process that needs this arch */
	for (; i<ptable->nProcesses; i++) {
	    if ((!arch || (strcmp( ptable->table[i].arch, arch )== 0)) &&
		!ptable->table[i].hostname) {
		ptable->table[i].hostname = mt->hname[k++];
		if (k >= mt->nHosts) k = 0;
		nNeeded--;
	    }
	}
	/* We can't free the machines table because we made references
	   to storage (hostnames) in the table */
    }
    return nNeeded != 0;   /* Return nonzero on failure */
}

#define MAXLINE 256
static const char defaultMachinesPath[] = DEFAULT_MACHINES_PATH;

/* Read the associate machines file for the given architecture, returning
   a table with nNeeded machines.  The format of this file is

   # comments
   hostname
   
   Eventually, we'll allow
   hostname [ : nproc [ : login ] ]

   The files are for the format:

   path/machines.<arch>
   or
   path/machines 
   (if no arch is specified)
   
*/
static MachineTable *mpiexecReadMachines( const char *arch, int nNeeded )
{
    FILE *fp=0;
    char buf[MAXLINE+1];
    char machinesfile[PATH_MAX];
    char dirname[PATH_MAX];
    char *p;
    const char *path=getenv("MPIEXEC_MACHINES_PATH");
    MachineTable *mt;
    int len, nFound = 0;
    
    /* Try to open the machines file.  arch may be null, in which 
       case we open the default file */
    /* FIXME: need path and external designation of file names */
    /* Partly done */
    if (!path) path = defaultMachinesPath;

    while (path) {
	char *next_path;
	/* Get next path member */
	next_path = strchr( path, ':' );
	if (next_path) 
	    len = next_path - path;
	else
	    len = strlen(path);
	
	/* Copy path into the file name */
	MPIU_Strncpy( dirname, path, len );

	dirname[len] = 0;

	/* Construct the final path name */
	if (arch) {
	    MPIU_Snprintf( machinesfile, PATH_MAX, 
			   "%s/machines.%s", dirname, arch );
	}
	else {
	    MPIU_Strncpy( machinesfile, dirname, PATH_MAX );
	    MPIU_Strnapp( machinesfile, "/machines", PATH_MAX );
	}
	if (debug) {
	    DBG_PRINTF( "Attempting to open %s\n", machinesfile );
	}
	fp = fopen( machinesfile, "r" );
	if (fp) break;  /* Found one */

	if (next_path) 
	    path = next_path + 1;
	else
	    path = 0;
    }
	
    if (!fp) {
	MPIU_Error_printf( "Could not open machines file %s\n", machinesfile );
	return 0;
    }
    mt = (MachineTable *)MPIU_Malloc( sizeof(MachineTable) );
    if (!mt) {
	MPIU_Internal_error_printf( "Could not allocate machine table\n" );
	return 0;
    }
    
    /* This may be larger than needed if the machines file has
       fewer entries than nNeeded */
    mt->hname = (char **)MPIU_Malloc( nNeeded * sizeof(char *) );
    if (!mt->hname) {
	return 0;
    }
    while (nNeeded) {
	if (!fgets( buf, MAXLINE, fp )) {
	    break;
	}
	if (debug) {
	    DBG_PRINTF( "line: %s", buf );
	}
	/* Skip comment lines */
	p = buf;
	p[MAXLINE] = 0;
	while (isascii(*p) && isspace(*p)) p++;
	if (*p == '#') continue;
	
	len = strlen( p );
	if (p[len-1] == '\n') p[--len] = 0;
	if (p[len-1] == '\r') p[--len] = 0;   /* Handle DOS files */
	mt->hname[nFound] = (char *)MPIU_Malloc( len + 1 );
	if (!mt->hname[nFound]) return 0;
	MPIU_Strncpy( mt->hname[nFound], p, len+1 );
	nFound++;
	nNeeded--;
    }
    mt->nHosts = nFound;
    return mt;	
}

int mpiexecRMProcessArg( int argc, char *argv[], void *extra )
{
  return 0;
}
