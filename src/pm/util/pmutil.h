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

/* 
   We must buffer the stdin, out, and err output.  A minimal memory approach
   would be to provide buffers only for the 3 fd's at mpiexec; that is, 
   only provide buffers for fd's 0, 1, and 2.  
   Instead, we will provide buffering for each created process for 
   each of the fds used by the spawned process.  This allows us greater
   control over stdin funnelling to the processes, and it allows various
   approaches for handling output, such as sending output only when full
   lines are available, and labeling lines as coming from particular
   processes.

   Each of these buffers is described by the charbuf structure:
*/
#define MAXCHARBUF 1024
typedef struct {
    char buf[MAXCHARBUF];  /* buffer for text */
    char *firstchar;       /* pointer to first free character in buffer */
    int  nleft;            /* number of characters left to transfer */
    int  destfd;           /* dest or src fd (the "other" fd used with
			      this buffer) */
} charbuf;

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
typedef struct { 
    int            fdStdin, fdStdout, fdStderr, /* fds for std io */
	           fdPMI;                      /* fds for process management */

    client_state_t state;            /* state of process */

    pid_t         pid;               /* pid of process */
    
    ProcessSpec   spec;

    charbuf        stdinBuf;         /* data awaiting processes stdin */
    charbuf        stdoutBuf;        /* data from processes stdout */
    charbuf        stderrBuf;        /* data from processes stderr */

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

/* Functions to process arguments */
int mpiexecRMProcessArg( int argc, char *argv[], void *extra );
int getIntValue( const char [], int );
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
