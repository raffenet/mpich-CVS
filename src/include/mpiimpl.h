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

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

/* 
   Include the implementation definitions (e.g., error reporting, thread
   portability)
   More detailed documentation is containing in the MPICH2 and ADI3 manuals.
 */
/* ... to do ... */

/* Basic typedefs */
#ifndef HAVE_INT16_T 
/* Fix me (short may not be correct) */
typedef short int16_t;
#endif

/* Known language bindings */
typedef enum { MPID_LANG_C, MPID_LANG_FORTRAN, 
	       MPID_LANG_CXX, MPID_LANG_FORTRAN90 } MPID_Lang_t;
/* Known MPI object types */
typedef enum { 
  MPID_COMM=1, MPID_WIN=2, MPID_FILE=4, MPID_DATATYPE=8 } MPID_Object_kind;

/* Error Handlers */
typedef union {
   void (*C_Comm_Handler_function) ( MPI_Comm *, int *, ... );
   void (*F77_Handler_function) ( MPI_Fint *, MPI_Fint *, ... );
   void (*C_Win_Handler_function) ( MPI_Win *, int *, ... );
   void (*C_File_Handler_function) ( MPI_File *, int *, ... );
} MPID_Errhandler_fn;

typedef struct {
  int                id;
  volatile int       ref_count;
  MPID_Lang_t        language;
  MPID_Object_kind   kind;
  MPID_Errhandler_fn errfn;
  /* Other, device-specific information */
} MPID_Errhandler;

/* Lists and attributes */
typedef struct MPID_List_elm {
    struct MPID_List_elm *next;
    void   *item;
    /* other, device-specific information */
}MPID_List;

typedef union {
  int  (*C_CommCopyFunction)( MPI_Comm, int, void *, void *, void *, int * );
  void (*F77_CopyFunction)  ( MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *, 
			      MPI_Fint *, MPI_Fint *, MPI_Fint * );
  void (*F90_CopyFunction)  ( MPI_Fint *, MPI_Fint *, MPI_Aint *, MPI_Aint *,
			      MPI_Aint *, MPI_Fint *, MPI_Fint * );
  int (*C_FileCopyFunction) ( MPI_Comm, int, void *, void *, void *, int * );
  int (*C_TypeCopyFunction) ( MPI_Datatype, int, 
			      void *, void *, void *, int * );
  /* The C++ function is the same as the C function */
} MPID_Copy_function;
typedef union {
  int  (*C_DeleteFunction)  ( MPI_Comm, int, void *, void * );
  void (*F77_DeleteFunction)( MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *, 
			      MPI_Fint * );
  void (*F90_DeleteFunction)( MPI_Fint *, MPI_Fint *, MPI_Aint *, MPI_Aint *, 
			      MPI_Fint * );
  int  (*C_FileDeleteFunction)  ( MPI_File, int, void *, void * );
  int  (*C_TypeDeleteFunction)  ( MPI_Datatype, int, void *, void * );
  
} MPID_Delete_function;
typedef struct {
    int                  id;
    volatile int         ref_count;
    MPID_Lang_t          language;
    MPID_Object_kind     kind;
    void                 *extra_state;
    MPID_Copy_function   copyfn;
    MPID_Delete_function delfn;
  /* other, device-specific information */
} MPID_Keyval;
typedef struct {
    void *      value;              /* Stored value */
    MPID_Keyval *keyval;            /* Keyval structure for this attribute */
    /* other, device-specific information */
} MPID_Attribute;

typedef struct {
    volatile int ref_count;
    int          size;           /* Size of a group */
    int          *lrank_to_lpid; /* Array mapping a local rank to local 
				    process number */
  /* other, device-specific information */
} MPID_Group;

/* Communicators */
typedef struct { 
    volatile int ref_count;
    int16_t       context_id;    /* Assigned context id */
    int           size;          /* Value of MPI_Comm_(remote)_size */
    int           rank;          /* Value of MPI_Comm_rank */
    int           id;            /* value of MPI_Comm for this structure */
    MPID_List     attributes;    /* List of attributes */
    MPID_Group    *local_group,  /* Groups in communicator. */
                  *remote_group; /* The local and remote groups are the
				    same for intra communicators */
    char          name[MPI_MAX_OBJECT_NAME];  /* Required for MPI-2 */
    MPID_Errhandler *errhandler;  /* Pointer to the error handler structure */
  /* other, device-specific information */
    int           is_singlemethod; /* An example, device-specific field,
				      this is used in a multi-method
				      device to indicate that all processes
				      in this communicator belong to the
				      same method */
} MPID_Comm;

/* Time stamps */
/* Get the timer definitions.  The source file for this include is
   src/mpi/timer/mpichtimer.h.in */
#include "mpichtimer.h"
typedef struct {
    MPID_Time_t stamp;
    int count;
} MPID_Stateinfo_t;
#define MPICH_MAX_STATES 512

/* Thread types */
/* Temporary; this will include "mpichthread.h" eventually */
typedef int MPID_Thread_key_t;
typedef int MPID_Thread_id_t;

typedef struct {
    int              nest_count;   /* For layered MPI implementation */
    int              op_errno;     /* For errors in predefined MPI_Ops */
#ifdef HAVE_TIMING
    MPID_Stateinfo_t timestamps[MPICH_MAX_STATES];  /* per thread state info */
#endif
} MPICH_PerThread_t;

#ifdef MPICH_SINGLE_THREADED
extern MPICH_PerThread_t MPIR_Thread;
#define MPID_GetPerThread(p) p = &MPIR_Thread
#else /* Assumes pthreads for simplicity */
#define MPID_GetPerThread(p) {\
     p = pthread_getspecific( MPIR_Process.thread_key ); \
     if (!p) { p = MPID_Calloc( 1, sizeof(MPID_PerThread_t ) );\
               pthread_setspecific( MPIR_Process.thread_key, p );}}
#endif


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

#define MPID_ERROR_LEVEL_ALL 1
#define MPID_ERROR_LEVEL_RUNTIME 2

#if HAVE_ERROR_CHECKING == MPID_ERROR_LEVEL_ALL
#define MPID_BEGIN_ERROR_CHECKS
#define MPID_END_ERROR_CHECKS
#elif HAVE_ERROR_CHECKING == MPID_ERROR_LEVEL_RUNTIME
#DEFINE MPID_BEGIN_ERROR_CHECKS IF (MPIR_Process.do_error_checks) {
#define MPID_END_ERROR_CHECKS }
#else
#error "Unknown value for error checking"
#endif

#else
#define MPID_BEGIN_ERROR_CHECKS
#define MPID_END_ERROR_CHECKS
#endif /* HAVE_ERROR_CHECKING */

/* Bindings for internal routines */
void MPIR_Add_finalize( int (*)( void * ), void * );
int MPIR_Err_return_comm( MPID_Comm *, const char [], int );
int MPIR_Err_create_code( int, const char [], ... );
void MPIR_Nest_incr( void );
void MPIR_Nest_decr( void );
int MPIR_Nest_value( void );
#endif /* MPIIMPL_INCLUDED */
