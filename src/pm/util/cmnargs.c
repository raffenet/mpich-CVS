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
#include <ctype.h>

/* ----------------------------------------------------------------------- */
/* Process options                                                         */
/* The process options steps loads up the processTable with entries,       */
/* including the hostname, for each process.  If no host is specified,     */
/* one will be provided in a subsequent step                               */
/* ----------------------------------------------------------------------- */
/*
-n num - number of processes
-host hostname
-arch architecture name
-wdir working directory (cd to this one BEFORE running executable?)
-path pathlist - use this to find the executable
-soft comma separated triplets (ignore for now)
-file name - implementation-defined specification file
-configfile name - file containing specifications of host/program, 
   one per line, with # as a comment indicator, e.g., the usual
   mpiexec input, but with ":" replaced with a newline.  That is,
   the configfile contains lines with -soft, -n etc.
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
static int mpiexecParseSoftspec( const char *, SoftSpec * );
static int ReadConfigFile( const char *, ProcessList *, int );

int mpiexecArgs( int argc, char *argv[], ProcessList *plist, int nplist, 
		 int (*ProcessArg)( int, char *[], void *), void *extraData )
{
    int         i;
    int         appnum=0;
    int         np=-1;     /* These 6 values are set by command line options */
    const char *host=0;    /* These are the defaults.  When a program name */
    const char *arch=0;    /* is seen, the values in these variables are */
    const char *wdir=0;    /* used to initialize the ProcessState entries */
    const char *path=0;    /* we use np == -1 to detect both -n and -soft */
    const char *soft=0;
    const char *exename=0;
    int        indexOfFirstArg=-1;
    int        curplist = 0; /* Index of current ProcessList element */
    int        optionArgs = 0; /* Keep track of where we got 
				 the options */
    int        optionCmdline = 0;

    /* Get values from the environment first.  Command line options
       override the environment */
    for (i=1; i<argc; i++) {
	if ( strncmp( argv[i], "-n",  strlen( argv[i] ) ) == 0 ||
	     strncmp( argv[i], "-np", strlen( argv[i] ) ) == 0 ) {
	    np = getInt( i+1, argc, argv );
	    optionArgs = 1;
	    i++;
	}
	else if ( strncmp( argv[i], "-soft", 6 ) == 0 ) {
	    if ( i+1 < argc )
		soft = argv[++i];
	    else {
		mpiexec_usage( "Missing argument to -soft\n" );
	    }
	    optionArgs = 1;
	}
	else if ( strncmp( argv[i], "-host", 6 ) == 0 ) {
	    if ( i+1 < argc )
		host = argv[++i];
	    else 
		mpiexec_usage( "Missing argument to -host\n" );
	    optionArgs = 1;
	}
	else if ( strncmp( argv[i], "-arch", 6 ) == 0 ) {
	    if ( i+1 < argc )
		arch = argv[++i];
	    else
		mpiexec_usage( "Missing argument to -arch\n" );
	    optionArgs = 1;
	}
	else if ( strncmp( argv[i], "-wdir", 6 ) == 0 ) {
	    if ( i+1 < argc )
		wdir = argv[++i];
	    else
		mpiexec_usage( "Missing argument to -wdir\n" );
	    optionArgs = 1;
	}
	else if ( strncmp( argv[i], "-path", 6 ) == 0 ) {
	    if ( i+1 < argc )
		path = argv[++i];
	    else
		mpiexec_usage( "Missing argument to -path\n" );
	    optionArgs = 1;
	}
	else if ( strncmp( argv[i], "-configfile", 12 ) == 0) {
	    if ( i+1 < argc ) {
		/* Ignore the other command line arguments */
		ReadConfigFile( argv[++i], plist, nplist );
	    }
	    else
		mpiexec_usage( "Missing argument to -configfile\n" );
	    optionCmdline = 1;
	} 
	else if (argv[i][0] != '-') {
	    exename = argv[i];

	    /* if the executable name is relative to the current
	       directory, convert it to an absolute name.  
	       FIXME: Make this optional (MPIEXEC_EXEPATH_ABSOLUTE?) */
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

	    if (soft) {
		/* Set the np to 0 to indicate valid softspec */
		plist[curplist].np = 0;
		if (np > 0) {
		    /* FIXME: Could warn about np and soft? */
		    ;
		}
		mpiexecParseSoftspec( soft, &plist[curplist].soft );
	    }
	    else {
		if (np == -1) np = 1;
		plist[curplist].np = np;
		plist[curplist].soft.nelm = 0;
		plist[curplist].soft.tuples = 0;
	    }
	    plist[curplist].appnum = appnum;
	    curplist++;
	    appnum++;

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
    if (optionArgs && optionCmdline) {
	MPIU_Error_printf( "-configfile may not be used with other options\n" );
	return -1;
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
int mpiexecGetIntValue( const char name[], int default_val )
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
 * Process a "soft" specification.  Returns the maximum of the 
 * number of requested processes, or -1 on error
 *   Format is in pseudo BNF:
 *  soft -> element[,element]
 *  element -> number | range
 *  range   -> number:number[:number]
 */
static int mpiexecParseSoftspec( const char *str, SoftSpec *sspec )
{
    const char *p = str, *p1, *p2;
    int s, e, incr;
    int nelm;
    int maxproc = 1;
    /* First, count the number of commas to preallocate the SoftSpec 
       tuples array */
    nelm = 1;
    p1 = p;
    while ( (p1 = strchr(p1,',')) != NULL ) {
	nelm ++;
	p1++;
    }
    sspec->nelm   = nelm;
    sspec->tuples = (int (*)[3]) MPIU_Malloc( nelm * sizeof(int [3]));

    nelm = 0;
    while ( *p ) {
	p1 = strchr(p,',');
	if (!p1) {
	    /* Use the rest of the string */
	    p1 = p + strlen(p);
	}
	/* Extract the element between p and p1-1 */
	/* FIXME: handle sign, invalid input */
	s = 0; e = 0; incr = 1;
	p2 = p;
	while (p2 < p1 && *p2 != ':') {
	    s += 10 * s + (*p2 - '0');
	    p2++;
	}
	if (*p2 == ':') {
	    /* Keep going */
	    p2++;
	    while (p2 < p1 && *p2 != ':') {
		e += 10 * e + (*p2 - '0');
		p2++;
	    }
	    if (*p2 == ':') {
		/* Keep going */
		p2++;
		incr = 0;
		while (p2 < p1 && *p2 != ':') {
		    incr += 10 * incr + (*p2 - '0');
		    p2++;
		}
	    }
	}
	else {
	    e = s; 
	}

	/* Save the results */
	sspec->tuples[nelm][0] = s;
	sspec->tuples[nelm][1] = e;
	sspec->tuples[nelm][2] = incr;

	/* FIXME: handle negative increments, and e not s + k incr */
	if (e > maxproc) maxproc = e;
	nelm++;

	p = p1;
	if (*p == ',') p++;
    }
    return maxproc;
}

/*
 * Read a file of mpiexec arguments, with a newline between groups.
 * Initialize the values in plist, and return the number of entries.
 * Return -1 on error.
 */
#define MAXLINEBUF 2048
#define MAXARGV    256
static int LineToArgv( char *buf, char *(argv[]), int maxargc );

static int ReadConfigFile( const char *filename, 
			   ProcessList *plist, int nplist)
{
    FILE *fp = 0;
    int curplist = 0;
    char linebuf[MAXLINEBUF];
    char *(argv[MAXARGV]);       /* A kind of dummy argv */
    int  argc, newplist;

    fp = fopen( filename, "r" );
    if (!fp) {
	MPIU_Error_printf( "Unable to open configfile %s\n", filename );
	return -1;
    }

    /* Read until we get eof */
    while (fgets( linebuf, MAXLINEBUF, fp )) {
	/* Convert the line into an argv array */
	argc = LineToArgv( linebuf, argv, MAXARGV );
	
	/* Process this argv.  We can use the same routine as for the
	   command line (this allows slightly more than the standard
	   requires for configfile, but the extension (allowing :)
	   is not prohibited by the standard */
	newplist = mpiexecArgs( argc, argv, 
				&plist[curplist], nplist - curplist, 0, 0 );
	if (newplist > 0) 
	    curplist += newplist;
	else 
	    /* An error occurred */
	    break;
    }

    fclose( fp );
    return curplist;
}

/* 
   Convert a line into an array of pointers to the arguments, which are
   all null-terminated.  The argument values copy the values in linebuf 
   so that the line buffer may be reused.
*/
static int LineToArgv( char *linebuf, char *(argv[]), int maxargv )
{
    int argc = 0;
    char *p;

    p = linebuf;
    while (*p) {
	while (isspace(*p)) p++;
	if (argc >= maxargv) {
	    MPIU_Error_printf( "Too many arguments in configfile line\n" );
	    return -1;
	}
	argv[argc] = p;
	/* Skip over the arg and insert a null at end */
	while (*p && !isspace(*p)) p++;

	/* Convert the entry into a copy */
	argv[argc] = MPIU_Strdup( argv[argc] );
	argc++;
	*p++ = 0;
    }
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
	DBG_FPRINTF( fp, "ProcessList[%d] for %d processes:\n", i, plist[i].np );
	DBG_FPRINTF( fp, "\
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
	DBG_FPRINTF( fp, "    args:\n" );
	for (j=0; j<pspec->nArgs; j++) {
	    DBG_FPRINTF( fp, "        %s\n", pspec->args[j] );
	}
	if (sspec->nelm > 0) {
	    DBG_FPRINTF( fp, "    Soft spec with %d tuples\n", sspec->nelm );
	    for (j=0; j<sspec->nelm; j++) {
		DBG_FPRINTF( fp, "        %d:%d:%d\n", 
			 sspec->tuples[j][0],
			 sspec->tuples[j][1],
			 sspec->tuples[j][2] );
	    }
	}
	else {
	    DBG_FPRINTF( fp, "    No soft spec\n" );
	}
    }
}
