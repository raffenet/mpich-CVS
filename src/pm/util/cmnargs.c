/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "pmutilconf.h"
#include "pmutil.h"
#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

/* ----------------------------------------------------------------------- */
/* Process options                                                         */
/* The process options steps loads up the processTable with entries,       */
/* including the hostname, for each process.  If no host is specified,     */
/* one will be provided in a subsequent step                               */
/* ----------------------------------------------------------------------- */
/*
-host hostname
-arch architecture name
-wdir working directory (cd to this one BEFORE running executable?)
-path pathlist - use this to find the executable
-soft comma separated triplets (ignore for now)
-file name - implementation-defined specification file
-configfile name - file containing specifications of host/program, 
   one per line, with # as a comment indicator, e.g., the usual
   mpiexec input, but with ":" replaced with a newline.
*/

/*
 * MPIExecArgs processes the arguments for mpiexec.  For any argument that 
 * is not recognized, it calls the ProcessArg routine, which returns the
 * number of arguments that should be skipped.  The void * pointer in
 * the call to ProcessArg is filled with the "extraData" pointer.  If
 * ProcessArg is null, then any unrecognized argument causes mpiexec to 
 * print a help message and exit.
 *
 * In addition the the arguments specified by the MPI standard, 
 * -np is accepted as a synonym for -n and -hostfile is allowed
 * to specify the available hosts.
 *
 * The implementation understands the ":" notation to separate out 
 * different executables.  Since no ordering of these arguments is implied,
 * other than that the executable comes last, we store the values until
 * we see an executable.
 */
static int getInt( int, int, char *[] );
int getIntValue( const char [], int );

int mpiexecArgs( int argc, char *argv[], ProcessList *plist, int nplist, 
		 int (*ProcessArg)( int, char *[], void *), void *extraData )
{
    int         i, j;
    int         np=1;      /* These 6 values are set by command line options */
    const char *host=0;    /* These are the defaults.  When a program name */
    const char *arch=0;    /* is seen, the values in these variables are */
    const char *wdir=0;    /* used to initialize the ProcessState entries */
    const char *path=0;
    const char *soft=0;
    const char *exename=0;
    /*char pathname[PATH_MAX]; 
      char wdirname[MAXNAMELEN]; */
    int        indexOfFirstArg=-1;
    int         curplist = 0; /* Index of current ProcessList element */

    /* Get values from the environment first.  Command line options
       override the environment */
    for (i=1; i<argc; i++) {
	if ( strncmp( argv[i], "-n",  strlen( argv[i] ) ) == 0 ||
	     strncmp( argv[i], "-np", strlen( argv[i] ) ) == 0 ) {
	    np = getInt( i+1, argc, argv );
	    i++;
	}
	else if ( strncmp( argv[i], "-soft", 6 ) == 0 )
	    if ( i+1 < argc )
		soft = argv[++i];
	    else {
		mpiexec_usage( "Missing argument to -soft\n" );
	    }
	else if ( strncmp( argv[i], "-host", 6 ) == 0 )
	    if ( i+1 < argc )
		host = argv[++i];
	    else 
		mpiexec_usage( "Missing argument to -host\n" );		    
	else if ( strncmp( argv[i], "-arch", 6 ) == 0 )
	    if ( i+1 < argc )
		arch = argv[++i];
	    else
		mpiexec_usage( "Missing argument to -arch\n" );		    
	else if ( strncmp( argv[i], "-wdir", 6 ) == 0 )
	    if ( i+1 < argc )
		wdir = argv[++i];
	    else
		mpiexec_usage( "Missing argument to -wdir\n" );		    
	else if ( strncmp( argv[i], "-path", 6 ) == 0 )
	    if ( i+1 < argc )
		path = argv[++i];
	    else
		mpiexec_usage( "Missing argument to -path\n" );		    
	else if (argv[i][0] != '-') {
	    exename = argv[i];

	    /* if the executable name is relative to the current
	       directory, convert it to an absolute name.  FIXME */
	    /* We may not want to do this, if the idea is that that
	       executable should be found in the PATH at the destionation */
	    /* wd = getwd( curdir ) */

	    /* Skip arguments until we hit either the end of the args
	       or a : */
	    i++;
	    indexOfFirstArg = i;
	    while (i < argc && argv[i][0] != ':') i++;
	    if (i == indexOfFirstArg) { 
		/* There really wasn't an argument */
		indexOfFirstArg = -1;
	    }
	    
	    /* Check that we still have room */
	    if (curplist >= nplist) {
		mpiexec_usage( "Too many processes requested\n" );
		return -1;
	    }

	    plist[curplist].spec.exename  = exename;
	    plist[curplist].spec.hostname = host;
	    plist[curplist].spec.arch     = arch;
	    plist[curplist].spec.path     = path;
	    plist[curplist].spec.wdir     = wdir;
	    if (indexOfFirstArg > 0) {
		plist[curplist].spec.args     =
		    (const char **)(argv + indexOfFirstArg);
		plist[curplist].spec.nArgs    = i - indexOfFirstArg;
	    }
	    else {
		plist[curplist].spec.args     = 0;
		plist[curplist].spec.nArgs    = 0;
	    }

	    plist[curplist].np = np;
	    /* Fixme: handle soft options */
	    plist[curplist].soft.nelm = 0;
	    plist[curplist].soft.tuples = 0;

	    curplist++;

	    /* Now, clear all of the values for the next set */
	    host = arch = wdir = path = soft = exename = 0;
	    indexOfFirstArg = -1;
	    np              = 1;
	}
	else {
	    /* Use the callback routine to handle any unknown arguments
	       before the program name */
	    int incr = 0;
	    if (ProcessArg) {
		incr = ProcessArg( argc, argv, extraData );
	    }
	    if (incr) {
		/* increment by one less because the for loop will also
		   increment i */
		i += (incr-1);
	    }
	    else {
		MPIU_Error_printf( "invalid mpiexec argument %s\n", argv[i] );
		mpiexec_usage( NULL );
		return -1;
	    }
	}
    }
    return curplist;
}

/* Return the int-value of the given argument.  If there is no 
   argument, or it is not a valid int, exit with an error message */
static int getInt( int argnum, int argc, char *argv[] )
{
    char *p;
    long i;

    if (argnum < argc) {
	p = argv[argnum];
	i = strtol( argv[argnum], &p, 0 );
	if (p == argv[argnum]) {
	    MPIU_Error_printf( "Invalid parameter value %s to argument %s\n",
		     argv[argnum], argv[argnum-1] );
	    mpiexec_usage( NULL );
	    /* Does not return */
	}
	return (int)i;
    }
    else {
	MPIU_Error_printf( "Missing argument to %s\n", argv[argnum-1] );
	mpiexec_usage( NULL );
	/* Does not return */
    }
    /* Keep compiler happy */
    return 0;
}

/* 
 * Try to get an integer value from the enviroment.  Return the default
 * if the value is not available or invalid
 */
int getIntValue( const char name[], int default_val )
{
    const char *env_val;
    int  val = default_val;

    env_val = getenv( name );
    if (env_val) {
#ifdef HAVE_STRTOL
	char *invalid_char; /* Used to detect invalid input */
	val = (int) strtol( env_val, &invalid_char, 0 );
	if (*invalid_char != '\0') val = default_val;
#else
	val = atoi( env_val );
#endif
    }
    return val;
}

/*
 * Debugging routine used to print out the results from mpiexecArgs
 */
void mpiexecPrintProcessList( FILE *fp, ProcessList *plist, int nplist )
{
    int i, j;
    ProcessSpec *pspec;
    SoftSpec    *sspec;
    
    for (i=0; i<nplist; i++) {
	pspec = &plist[i].spec;
	sspec = &plist[i].soft;
	fprintf( fp, "ProcessList[%d] for %d processes:\n", i, plist[i].np );
	fprintf( fp, "\
    exename   = %s\n\
    hostname  = %s\n\
    arch      = %s\n\
    path      = %s\n\
    wdir      = %s\n", 
		 pspec->exename  ? pspec->exename : "<NULL>", 
		 pspec->hostname ? pspec->hostname : "<NULL>", 
		 pspec->arch     ? pspec->arch     : "<NULL>", 
		 pspec->path     ? pspec->path     : "<NULL>", 
		 pspec->wdir     ? pspec->wdir     : "<NULL>" );
	fprintf( fp, "    args:\n" );
	for (j=0; j<pspec->nArgs; j++) {
	    fprintf( fp, "        %s\n", pspec->args[j] );
	}
	if (sspec->nelm > 0) {
	    fprintf( fp, "    Soft spec with %d tuples\n", sspec->nelm );
	    for (j=0; j<sspec->nelm; j++) {
		fprintf( fp, "        %d:%d:%d\n", 
			 sspec->tuples[j][0],
			 sspec->tuples[j][1],
			 sspec->tuples[j][2] );
	    }
	}
	else {
	    fprintf( fp, "    No soft spec\n" );
	}
    }
}
