/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIIMPL_INCLUDED
#define MPIIMPL_INCLUDED

/* Include the mpi definitions */
#include "mpi.h"

/* Data computed by configure.  This is included *after* mpi.h because we
   do not want mpi.h to depend on any other files or configure flags */
#include "mpichconf.h"

/* Include the implementation definitions (e.g., error reporting, thread
   portability) */
/* ... to do ... */

/* Thread types */
/* Temporary */
typedef int MPID_Thread_key_t;
typedef int MPID_Thread_id_t;

/* Communicator type */
typedef void MPID_Comm;

/* Per process data */
typedef enum { MPICH_PRE_INIT=0, MPICH_WITHIN_MPI=1,
               MPICH_POST_FINALIZED=2 } MPIR_MPI_State_t;
typedef struct {
    MPIR_MPI_State_t  initialized;      /* Is MPI initalized? */
    MPID_Thread_key_t thread_key;       /* Id for perthread data */
    MPID_Thread_id_t  master_thread;    /* Thread that started MPI */
    int               do_error_checks;  /* runtime error check control */
    MPID_Comm         *comm_world;      /* Easy access to comm_world for
                                           error handler */
} MPICH_PerProcess_t;
extern MPICH_PerProcess_t MPIR_Process;

#define MPID_MPI_FUNC_EXIT(a)
#define MPID_MPI_FUNC_ENTER(a)

/* Error checking (see --enable-error-checking for control of this) */
#ifdef HAVE_ERROR_CHECKING

#if HAVE_ERROR_CHECKING == all
#define MPID_BEGIN_ERROR_CHECKS
#define MPID_END_ERROR_CHECKS
#elif HAVE_ERROR_CHECKING == runtime
#define MPID_BEGIN_ERROR_CHECKS if (MPIR_Process.do_error_checks) {
#define MPID_END_ERROR_CHECKS }
#else
#error "Unknown value for error checking"
#endif

#else
#define MPID_BEGIN_ERROR_CHECKS
#define MPID_END_ERROR_CHECKS
#endif /* HAVE_ERROR_CHECKING */


#endif /* MPIIMPL_INCLUDED */
