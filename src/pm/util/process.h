/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <sys/types.h>

/*
Data structures and routines for managing processs

The data structures for managing processes following the hierarchy implied
by MPI-2 design, particularly the ":" syntax of mpiexec and the
arguments to MPI_Comm_spawn_multiple.

From the top, we have:

ProcessUniverse - All processes, consisting of a list of ProcessWorld
    ProcessWorld - All processes in an MPI_COMM_WORLD, containing an
                   list of apps (corresponding to the MPI_APPNUM
                   attribute)
        ProcessApp - A group of processes sharing the same executable
                     command line, and other parameters (such as
                     working dir), and an array of process descriptions
            ProcessState - Information for a specific process,
                     including host, exit status, pid, MPI rank, and state
                 ProcessExitStatus - 
The I/O for processes is handled separately.  I/O handlers for each fd
include an "extra-data" pointer that can store pointers to the process
structures as necessary.
 */

/* ProcessSoftSpec is the "soft" specification of desired processor counts.
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
} ProcessSoftSpec;

/* Record the return value from each process */
typedef enum { EXIT_NOTYET,     /* Not exited */
               EXIT_NORMAL,     /* Normal exit (possibly with nonzero status)*/
	       EXIT_SIGNALLED,  /* Process died on an uncaught signal (e.g., 
			           SIGSEGV) */
	       EXIT_NOFINALIZE, /* Process exited without calling finalize */
	       EXIT_ABORTED,    /* Process killed by mpiexec after a 
	                           PMI abort */
	       EXIT_KILLED      /* Process was killed by mpiexec */ 
             } ProcessExitState_t;

typedef enum { PROCESS_UNINITIALIZED=-1, /* Before process created */
	       /*PROCESS_UNKNOWN,*/      /* Process started but in unkown 
					    state */
	       /*PROCESS_STARTING,*/       /* Process is starting */
	       PROCESS_ALIVE,          /* Process is (expected to be) alive */
	       PROCESS_COMMUNICATING,  /* Process is alive and using PMI */
	       PROCESS_FINALIZED,      /* Process is alive but has indicated 
					  that it has called PMI_Finalize */
	       /*PROCESS_EXITING, */       /* Process expected to exit */
	       PROCESS_GONE            /* Process has exited and its status 
					  collected */
} ProcessStatus_t;

typedef struct {
    ProcessExitState_t  exitReason;       /* how/why did the process exit */
    int                 exitSig;          /* exit signal, if any */
    int                 exitStatus;       /* exit statue */
    int                 exitOrder;        /* indicates order in which processes
			   	 	     exited */
} ProcessExitStatus;

typedef struct ProcessState {
    const char        *hostname;         /* Host for process */
    int               wRank;             /* Rank in COMM_WORLD */
    pid_t             pid;               /* pid for process */
    ProcessStatus_t   status;            /* what state the process is in */
    ProcessExitStatus exitStatus;        /* Exit status */
    struct ProcessApp *app;              /* Pointer to "parent" app */
} ProcessState;

typedef struct ProcessApp {
    int           myAppNum;          /* Appnum of this group */
    const char    *exename;          /* Executable to run */
    const char    *arch;             /* Architecture type */
    const char    *path;             /* Search path for executables */
    const char    *wdir;             /* Working directory */
    const char    *hostname;         /* Default host (can be overridded
					by each process in an App) */
    const char    **args;            /* Pointer into the array of args */
    int            nArgs;            /* Number of args (list is *not* null
					terminated) */
    ProcessSoftSpec soft;            /* "soft" spec, if any */
    int            nProcess;         /* Number of processes in this app */
    ProcessState   *pState;          /* Array of process states */

    struct ProcessApp *nextApp;      /* Next App */
} ProcessApp;

typedef struct ProcessWorld {
    int        nApps;                /* Number of Apps in this world */
    int        nProcess;             /* Number of processes in this world */
    ProcessApp *apps;                /* Array of Apps */
    struct ProcessWorld *nextWorld;
} ProcessWorld;

typedef struct ProcessUniverse {
    ProcessWorld *worlds;
} ProcessUniverse;

    
int MPIE_ForkProcesses( ProcessWorld *, char *[] );
