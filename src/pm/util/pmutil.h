/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef PMUTIL_H_INCLUDED
#define PMUTIL_H_INCLUDED
/*
 * Define Data structures.
 *
 * ProcessList  - An array of "mpiexec-style" arguments.  Each entry
 * describes a program to be run on one or more systems, with one or
 * more processes.  This is used for command-lines from mpiexec, 
 * data from a -configfile argument to mpiexec, or the arguments
 * to MPI_Comm_spawn (or a single entry from MPI_Comm_spawn_multiple).
 *
 * ProcessState - Each process has one of these entries, stored in any
 * array, and ordered by rank in comm_world (for the initial processes)
 *
 * PMIState - mpiexec provides the process management interface (PMI)
 * services.  PMIState contains the information needed for this,
 * other than the fds used to communicate with each created process's
 * PMI interface.  
 */

#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "pmiserv.h"

#define MAXNAMELEN  256		/* max length of vairous names */
#ifndef PATH_MAX
#define PATH_MAX 2048		/* max length of PATH */
#endif
#define MAXPROCESSES     64	/* max number of processes to manage */

#ifndef MAX_HOST_NAME
#define MAX_HOST_NAME 512
#endif

/* ----------------------------------------------------------------------- */
/* ProcessState                                                            */
/* ----------------------------------------------------------------------- */
typedef enum { UNINITIALIZED=-1, 
	       UNKNOWN, STARTING, ALIVE, COMMUNICATING, FINALIZED, EXITING, GONE } 
    client_state_t;

/* Record the return value from each process */
typedef enum { NORMAL,     /* Normal exit (possibly with nonzero status) */
	       SIGNALLED,  /* Process died on an uncaught signal (e.g., 
			      SIGSEGV) */
	       NOFINALIZE, /* Process exited without calling finalize */
	       ABORTED,    /* Process killed by mpiexec after a PMI abort */
	       KILLED      /* Process was killed by mpiexec */ 
             } exit_state_t;

typedef int IOHandler( int, void * );
typedef enum { IO_PENDING, IO_QUIET, IO_FINISHED } FDState;
typedef struct {
    int       fd;          /* fd for this IO */
    int       isWrite;     /* true if writing, else reading. No bidirection (?) */
    FDState   fdstate;     /* state of the fd */
    IOHandler *handler;    /* Routine to call to handle IO on this FD */
    void      *extra_state;/* Point to extra state to pass to the handler */
} IOSpec;

/*---------------------------------------------------------------------------
 * Specifying a collection of processes.
 ----------------------------------------------------------------------------*/
/* 
   A ProcessSpec gives the environment, arguments, and other parameters
   with which to start a process.  
 */
typedef struct {
    const char    *exename;          /* Executable to run */
    const char    **args;            /* Pointer into the array of args */
    const char    *hostname;         /* Host for process */
    const char    *arch;             /* Architecture type */
    const char    *path;             /* Search path for executables */
    const char    *wdir;             /* Working directory */
    int            nArgs;            /* Number of args (list is *not* null
					terminated) */
} ProcessSpec;

/* SoftSpec is the "soft" specification of desired processor counts.
   Format is in pseudo BNF:
   soft -> element[,element]
   element -> number | range
   range   -> number:number[:number]

   These are stored as 3 element tuples containing first:last:stride.
   A single number is first:first:1, e.g., 17 is represented as 17:17:1.
*/
typedef struct {
    int nelm;                        /* Number of tuples */
    int (*tuples)[3];                /* The tuples (allocated) */
} SoftSpec;

/* A ProcessList is a structure that contains information on the 
   environment and arguments for a Process (ProcessSpec), along
   with the number of processes to create with that ProcessSpec 
*/
typedef struct {
    ProcessSpec spec;                /* Specifications for this process */
    int         np;                  /* Number of processes */
    SoftSpec    soft;                /* The "soft" specification, if any */
} ProcessList;

typedef struct {
    exit_state_t   exitReason;       /* how/why did the process exit */
    int            exitSig;          /* exit signal, if any */
    int            exitStatus;       /* exit statue */
    int            exitOrder;        /* indicates order in which processes
					exited */
} ProcessExitStatus;

/* PENDING is a special pid for a process that may have been forked but 
   for who we do not yet have a pid */

#define PENDING -2
/* Record each process, including the fd's used to handle standard input
   and output, and any process-specific details about each process, 
   such as the working directory and specific requirements about the 
   host */
#define MAXIOS 4
typedef struct { 
    IOSpec       ios[MAXIOS];        /* Structures for the handling of
					file descriptors such as stdin
					and the PMI interface */
    int          nIos;               /* Number of defined ios entries for
					this process */

    client_state_t state;            /* state of process */

    pid_t         pid;               /* pid of process */
    
    ProcessSpec   spec;

    int            rank;             /* rank in comm_world (or universe) */

    PMI_Process    pmientry;         /* PMI information.  This allows
					the pmi implementation to 
					define whatever it needs */
    ProcessExitStatus status;        /* return code of the process when
					it exited, and the reason */

} ProcessState;

/* We put both the array and the count of processes into a single
   structure to make it easier to pass this information to other routines */
typedef struct {
    int          nProcesses;         /* Number of processes created */
    int          maxProcesses;       /* Maximum number of processes available;
					set to MAXPROCESSES */
    int          timeout_seconds;    /* Maximum time to run (from start);
					-1 is infinity */
    int          nActive;            /* Number of active processes. */
    ProcessState *table;             /* Entries for each process */
} ProcessTable;

/* 
   ---------------------------------------------------------------------------
   Function prototypes 
   ---------------------------------------------------------------------------
 */
#include <stdio.h>

/* Functions to process arguments */
int mpiexecArgs( int argc, char *argv[], ProcessList *plist, int nplist, 
		 int (*ProcessArg)( int, char *[], void *), void *extraData );
int mpiexecRMProcessArg( int argc, char *argv[], void *extra );
void mpiexecPrintProcessList( FILE *fp, ProcessList *plist, int nplist );
int getIntValue( const char [], int );
extern void mpiexec_usage( const char * );

/* timeout */
void InitTimeout( int seconds );
int GetRemainingTime( void );

/* 
   ---------------------------------------------------------------------------
   Miscellaneous
   ---------------------------------------------------------------------------
 */

/* Temporary debug value */
extern int debug;

/* Temporary debug definitions */
#define DBG_PRINTF printf
#define DBG_FPRINTF fprintf

/* Use the memory defintions from mpich2/src/include */
#include "mpimem.h"

#endif
