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

#include <stdio.h>
/* 
   Include the implementation definitions (e.g., error reporting, thread
   portability)
   More detailed documentation is containing in the MPICH2 and ADI3 manuals.
 */
/* ... to do ... */

/* key used by spawners and spawnees to get the port by which they can connect to each other */
#define MPICH_PARENT_PORT_KEY     "MPI_Parent_port"
/* key used to tell comm_accept that it doesn't need to transfer bnr databases */
#define MPICH_BNR_SAME_DOMAIN_KEY "BNR_SAME_DOMAIN"
/* key used to inform spawned processes that their parent is mpiexec and not another mpi application */
#define MPICH_EXEC_IS_PARENT_KEY  "MPIEXECSpawned"

/* Basic typedefs */
#ifndef HAVE_INT16_T 
/* Fix me (short may not be correct) */
typedef short int16_t;
#endif
#ifndef HAVE_INT32_T
/* Fix me (int may not be correct) */
typedef int int32_t;
#endif

/* Thread basics */
#ifdef MPICH_SINGLE_THREADED
typedef int MPID_Thread_key_t;
typedef int MPID_Thread_id_t;
typedef int MPID_Thread_lock_t;
#define MPID_GetPerThread(p) p = &MPIR_Thread
#else /* Assumes pthreads for simplicity */
#if defined HAVE_PTHREAD_CREATE
#include <pthread.h>
typedef pthread_key_t MPID_Thread_key_t;
typedef pthread_t MPID_Thread_id_t;
typedef pthread_mutex_t MPID_Thread_lock_t;
#define MPID_GetPerThread(p) {\
     p = (MPICH_PerThread_t*)pthread_getspecific( MPIR_Process.thread_key ); \
     if (!p) { p = MPIU_Calloc( 1, sizeof(MPICH_PerThread_t ) );\
               pthread_setspecific( MPIR_Process.thread_key, p );}}
#define MPID_Thread_lock( a ) pthread_mutex_lock( a )
#define MPID_Thread_unlock( a ) pthread_mutex_unlock( a )
#define MPID_Thread_lock_init( a ) pthread_mutex_init( a, 0 )
#else
#error No Thread Package Chosen
#endif
#endif

/* Memory allocation */
#ifdef USE_MEMORY_TRACING
#define MPIU_Malloc(a)    MPIU_trmalloc((unsigned)(a),__LINE__,__FILE__)
#define MPIU_Calloc(a,b)  \
    MPIU_trcalloc((unsigned)(a),(unsigned)(b),__LINE__,__FILE__)
#define MPIU_Free(a)      MPIU_trfree(a,__LINE__,__FILE__)
#define MPIU_Strdup(a)    MPIU_trstrdup(a,__LINE__,__FILE__)
/* Define these as invalid C to catch their use in the code */
#define malloc(a)         'Error use MPIU_Malloc'
#define calloc(a,b)       'Error use MPIU_Calloc'
#define free(a)           'Error use MPIU_Free'
#if defined(strdup) || defined(__strdup)
#undef strdup
#endif
#define strdup(a)         'Error use MPIU_Strdup'
#else
#define MPIU_Malloc(a)    malloc((unsigned)(a))
#define MPIU_Calloc(a,b)  calloc((unsigned)(a),(unsigned)(b))
#define MPIU_Free(a)      free((void *)(a))
#ifdef HAVE_STRDUP
#define MPIU_Strdup(a)    strdup(a)
#else
#define MPIU_Strdup(a)    ?????
#endif
#endif
void MPIU_trinit ( int );
void *MPIU_trmalloc ( unsigned int, int, const char * );
void MPIU_trfree ( void *, int, const char * );
int MPIU_trvalid ( const char * );
void MPIU_trspace ( int *, int * );
void MPIU_trid ( int );
void MPIU_trlevel ( int );
void MPIU_trpush ( int );
void MPIU_trpop (void);
void MPIU_trDebugLevel ( int );
void *MPIU_trstrdup( const char *, int, const char * );
void *MPIU_trcalloc ( unsigned, unsigned, int, const char * );
void *MPIU_trrealloc ( void *, int, int, const char * );
void MPIU_TrSetMaxMem ( int );
void MPIU_trdump ( FILE * );
void MPIU_trSummary ( FILE * );
void MPIU_trdumpGrouped ( FILE * );

/* Memory allocation stack */
#define MAX_MEM_STACK 16
typedef struct { int n_alloc; void *ptrs[MAX_MEM_STACK]; } MPIU_Mem_stack;
#define MALLOC_STK(n,a) {a=MPIU_Malloc(n);\
               if (memstack.n_alloc >= MAX_MEM_STACK) abort(implerror);\
               memstack.ptrs[memstack.n_alloc++] = a;}
#define FREE_STK     {int i; for (i=memstack.n_alloc-1;i>=0;i--) {\
               MPIU_Free(memstack.ptrs[i]);}}
#define MALLOC_STK_INIT memstack.n_alloc = 0
#define MALLOC_STK_DECL MPIU_Mem_stack memstack

/* Known language bindings */
typedef enum { MPID_LANG_C, MPID_LANG_FORTRAN, 
	       MPID_LANG_CXX, MPID_LANG_FORTRAN90 } MPID_Lang_t;

/* Known MPI object types.  These are used for both the error handlers 
   and for the handles.  This is a 4 bit value */
typedef enum { 
  MPID_COMM       = 0x0, 
  MPID_GROUP      = 0x1,
  MPID_DATATYPE   = 0x2,
  MPID_FILE       = 0x3,
  MPID_ERRHANDLER = 0x4,
  MPID_OP         = 0x5,
  MPID_INFO       = 0x6,
  MPID_WIN        = 0x7,
  MPID_KEYVAL     = 0x8,
  } MPID_Object_kind;
#define HANDLE_MPI_KIND_SHIFT 26
#define HANDLE_GET_MPI_KIND(a) ( ((a)&0x3c000000) >> HANDLE_MPI_KIND_SHIFT )

/* Handle types.  These are really 2 bits */
#define HANDLE_KIND_INVALID  0x0
#define HANDLE_KIND_BUILTIN  0x1
#define HANDLE_KIND_DIRECT   0x2
#define HANDLE_KIND_INDIRECT 0x3
/* Mask assumes that ints are at least 4 bytes */
#define HANDLE_KIND_MASK 0xc0000000
#define HANDLE_KIND_SHIFT 30
#define HANDLE_GET_KIND(a) (((a)&HANDLE_KIND_MASK)>>HANDLE_KIND_SHIFT)
#define HANDLE_SET_KIND(a,kind) ((a)|((kind)<<HANDLE_KIND_SHIFT))

/* For indirect, the remainder of the handle has a block and index */
#define HANDLE_INDIRECT_SHIFT 16
#define HANDLE_BLOCK(a) (((a)& 0x03FF0000) >> HANDLE_INDIRECT_SHIFT)
#define HANDLE_BLOCK_INDEX(a) ((a) & 0x0000FFFF)

/* For direct, the remainder of the handle is the index into a predefined 
   block */
#define HANDLE_MASK 0x03FFFFFF
#define HANDLE_INDEX(a) ((a)& HANDLE_MASK)

/* Handle block is between 1 and 1024 *elements* */
#define HANDLE_BLOCK_SIZE 256
/* Index size is bewtween 1 and 65536 *elements* */
#define HANDLE_BLOCK_INDEX_SIZE 1024

#define PREDEFINED_HANDLE(name,index) \
     (HANDLE_KIND_DIRECT << HANDLE_KIND_SHIFT) | \
     (MPID_##name << HANDLE_MPI_KIND_SHIFT) | index

/* ALL objects have the id as the first value. */
/* Inactive (unused and stored on the appropriate avail list) objects 
   have MPIU_Handle_common as the head */
typedef struct {
    int  id;
    void *next;   /* Free handles use this field to point to the next
		     free object */
} MPIU_Handle_common;

/* All *active* (in use) objects have the id as the first value; objects
   with referene counts have the reference count as the second value.
   See MPIU_Object_add_ref and MPIU_Object_release_ref. */
typedef struct {
    int id;
    volatile int ref_count;
} MPIU_Handle_head;

/* This type contains all of the data, except for the direct array,
   used by the object allocators. */
typedef struct {
    MPIU_Handle_common *avail;          /* Next available object */
    int                initialized;     /* */
    void              *(*indirect)[];   /* Pointer to indirect object blocks */
    int                indirect_size;   /* Number of allocated indirect blocks */
    MPID_Object_kind   kind;            /* Kind of object this is for */
    int                size;            /* Size of an individual object */
    void               *direct;         /* Pointer to direct block, used 
					   for allocation */
    int                direct_size;     /* Size of direct block */
} MPIU_Object_alloc_t;
extern void *MPIU_Handle_obj_new( MPIU_Object_alloc_t * );
extern void MPIU_Handle_obj_free( MPIU_Object_alloc_t *, void * );
void *MPIU_Handle_get_ptr_indirect( int, MPIU_Object_alloc_t * );

/* This isn't quite right, since we need to distinguish between multiple 
   user threads and multiple implementation threads.
 */
#ifdef MPICH_SINGLE_THREADED
#define MPID_Object_add_ref(objptr) \
    ((MPIU_Handle_head*)(objptr))->ref_count++
#define MPID_Object_release_ref(objptr,newval_ptr) \
    *(newval)=--((MPIU_Handle_head*)(objptr))->ref_count
#else
/* These can be implemented using special assembly language operations
   on most processors.  If no such operation is available, then each
   object, in addition to the ref_count field, must have a thread-lock. 
   
   We also need the old value when decrementing so that we can see if
   we must deallocate the object.  This fetch and decrement must be 
   atomic so that multiple threads don't decide that they were 
   responsible for setting the value to zero.
 */
#define MPID_Object_add_ref(objptr) \
    {MPID_Thread_lock(&(objptr)->mutex);(objptr)->ref_count++;\
    MPID_Thread_unlock(&(objptr)->mutex);}
#define MPID_Object_release_ref(objptr,newval_ptr) \
    {MPID_Thread_lock(&(objptr)->mutex);*(newval_ptr)=--(objptr)->ref_count;\
    MPID_Thread_unlock(&(objptr)->mutex);}
#endif

/* Routines to initialize handle allocations */
void *MPIU_Handle_direct_init( void *, int, int, int );
void *MPIU_Handle_indirect_init( void *(**)[], int *, int, int, int, int );
int MPIU_Handle_free( void *((*)[]), int );

/* Handles conversion */
/* Question.  Should this do ptr=0 first, particularly if doing --enable-strict
   complication? */
#define MPID_Get_ptr(kind,a,ptr) \
   switch (HANDLE_GET_KIND(a)) {\
      case HANDLE_KIND_INVALID: ptr=0; break;\
      case HANDLE_KIND_BUILTIN: ptr=0;break;\
      case HANDLE_KIND_DIRECT: ptr=MPID_##kind##_direct+HANDLE_INDEX(a);break;\
      case HANDLE_KIND_INDIRECT: \
      ptr=(MPID_##kind*)MPIU_Handle_get_ptr_indirect(a,&MPID_##kind##_mem);break;\
     }
#define MPID_Comm_get_ptr(a,ptr) MPID_Get_ptr(Comm,a,ptr)
#define MPID_Group_get_ptr(a,ptr) MPID_Get_ptr(Group,a,ptr)
#define MPID_Datatype_get_ptr(a,ptr) MPID_Get_ptr(Datatype,a,ptr)
#define MPID_File_get_ptr(a,ptr) MPID_Get_ptr(File,a,ptr)
#define MPID_Errhandler_get_ptr(a,ptr) MPID_Get_ptr(Errhandler,a,ptr)
#define MPID_Op_get_ptr(a,ptr) MPID_Get_ptr(Op,a,ptr)
#define MPID_Info_get_ptr(a,ptr) MPID_Get_ptr(Info,a,ptr)
#define MPID_Win_get_ptr(a,ptr) MPID_Get_ptr(Win,a,ptr)

/* Valid pointer checks */
/* This test is lame.  Should eventually include cookie test 
   and in-range addresses */
#define MPID_Valid_ptr(kind,ptr,err) \
  {if (!(ptr)) { err = MPIR_Err_create_code( MPI_ERR_OTHER, "**nullptr" ); } }

#define MPID_Info_valid_ptr(ptr,err) MPID_Valid_ptr(Info,ptr,err)
#define MPID_Comm_valid_ptr(ptr,err) MPID_Valid_ptr(Comm,ptr,err)
#define MPID_Datatype_valid_ptr(ptr,err) MPID_Valid_ptr(Datatype,ptr,err)
#define MPID_Group_valid_ptr(ptr,err) MPID_Valid_ptr(Group,ptr,err)
#define MPID_Win_valid_ptr(ptr,err) MPID_Valid_ptr(Win,ptr,err)
#define MPID_Op_valid_ptr(ptr,err) MPID_Valid_ptr(Op,ptr,err)
#define MPID_Errhandler_valid_ptr(ptr,err) MPID_Valid_ptr(Errhandler,ptr,err)
#define MPID_File_valid_ptr(ptr,err) MPID_Valid_ptr(File,ptr,err)

/* Info */
typedef struct MPID_Info_s {
    int                id;
    struct MPID_Info_s *next;
    char               *key;
    char               *value;
} MPID_Info;
extern MPIU_Object_alloc_t MPID_Info_mem;
/* Preallocated info objects */
extern MPID_Info MPID_Info_direct[];

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
} MPID_List;

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
    int          id;
    volatile int ref_count;
    int          size;           /* Size of a group */
    int          *lrank_to_lpid; /* Array mapping a local rank to local 
				    process number */
  /* other, device-specific information */
} MPID_Group;

extern MPIU_Object_alloc_t MPID_Group_mem;
/* Preallocated group objects */
extern MPID_Group MPID_Group_direct[];

/* Communicators */
typedef struct { 
    int           id;            /* value of MPI_Comm for this structure */
    volatile int ref_count;
    int16_t       context_id;    /* Assigned context id */
    int           size;          /* Value of MPI_Comm_(remote)_size */
    int           rank;          /* Value of MPI_Comm_rank */
    MPID_List     attributes;    /* List of attributes */
    MPID_Group    *local_group,  /* Groups in communicator. */
                  *remote_group; /* The local and remote groups are the
				    same for intra communicators */
    char          name[MPI_MAX_OBJECT_NAME];  /* Required for MPI-2 */
    MPID_Errhandler *errhandler;  /* Pointer to the error handler structure */
#ifndef MPICH_SINGLE_THREADED
    MPID_Thread_lock_t access_lock;
#endif
  /* other, device-specific information */
    int           is_singlemethod; /* An example, device-specific field,
				      this is used in a multi-method
				      device to indicate that all processes
				      in this communicator belong to the
				      same method */
} MPID_Comm;
extern MPIU_Object_alloc_t MPID_Comm_mem;
/* Preallocated comm objects */
extern MPID_Comm MPID_Comm_direct[];

/* Windows */
typedef struct {
    int           id;             /* value of MPI_Win for this structure */
    volatile int  ref_count;
    MPID_Errhandler *errhandler;  /* Pointer to the error handler structure */
    char          name[MPI_MAX_OBJECT_NAME];  /* Required for MPI-2 */
} MPID_Win;
extern MPIU_Object_alloc_t MPID_Win_mem;
/* Preallocated win objects */
extern MPID_Win MPID_Win_direct[];

/* Datatypes */

typedef struct MPID_Datatype_st { 
    int           id;            /* value of MPI_Datatype for structure */
    volatile int  ref_count;
    int           is_contig;     /* True if data is contiguous (even with 
                                    a (count,datatype) pair) */
    int           is_perm;       /* True if datatype is a predefined type */
    struct MPID_Dataloop_st *opt_loopinfo;  /* "optimized" loopinfo.  
				    Filled in at create
                                    time; not touched by MPI calls.  This will
                                    be for the homogeneous case until further
                                    notice */

    int           size;          /* Q: maybe this should be in the dataloop? */
    MPI_Aint      extent;        /* MPI-2 allows a type to be created by
                                    resizing (the extent of) an existing 
                                    type */
    MPI_Aint      ub, lb,        /* MPI-1 upper and lower bounds */
                  true_ub, true_lb; /* MPI-2 true upper and lower bounds */
    int           alignsize;     /* size of datatype to align (affects pad) */
    /* The remaining fields are required but less frequently used, and
       are placed after the more commonly used fields */
    int loopsize; /* size of loops for this datatype in bytes; derived value */
    int           combiner;      /* MPI call that was used to create this
				    datatype */
    struct MPID_Dataloop_st *loopinfo; /* Original loopinfo, used when 
					  creating and when getting contents */
    int           has_mpi1_ub;   /* The MPI_UB and MPI_LB are sticky */
    int           has_mpi1_lb;
    int           is_permanent;  /* */
    int           is_committed;  /* */

    int           loopinfo_depth; /* Depth of dataloop stack needed
                                     to process this datatype.  This 
                                     information is used to ensure that
                                     no datatype is constructed that
                                     cannot be processed (see MPID_Segment) */
    /* int opt_loopinfo_depth ??? */

    MPID_List     attributes;    /* MPI-2 adds datatype attributes */

    int32_t       cache_id;      /* These are used to track which processes */
    /* MPID_Lpidmask mask; */         /* have cached values of this datatype */

    char          name[MPI_MAX_OBJECT_NAME];  /* Required for MPI-2 */

    /* The following is needed to efficiently implement MPI_Get_elements */
    int           n_elements;   /* Number of basic elements in this datatype */
    MPI_Aint      element_size; /* Size of each element or -1 if elements are
				   not all the same size */

    /* other, device-specific information */
} MPID_Datatype;
extern MPIU_Object_alloc_t MPID_Datatype_mem;
/* Preallocated datatype objects */
extern MPID_Datatype MPID_Datatype_direct[];

typedef struct {
    int           id;             /* value of MPI_File for this structure */
    volatile int  ref_count;
    MPID_Errhandler *errhandler;  /* Pointer to the error handler structure */
} MPID_File;
extern MPIU_Object_alloc_t MPID_File_mem;
/* Preallocated file objects */
extern MPID_File MPID_File_direct[];

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

typedef struct {
    int              nest_count;   /* For layered MPI implementation */
    int              op_errno;     /* For errors in predefined MPI_Ops */
#ifdef HAVE_TIMING
    MPID_Stateinfo_t timestamps[MPICH_MAX_STATES];  /* per thread state info */
#endif
} MPICH_PerThread_t;

#ifdef MPICH_SINGLE_THREADED
extern MPICH_PerThread_t MPIR_Thread;
#define MPID_Comm_thread_lock( ptr )
#define MPID_Comm_thread_unlock( ptr )
#else
#define MPID_Comm_thread_lock() MPID_Thread_lock( &ptr->access_lock)
#define MPID_Comm_thread_unlock() MPID_Thread_unlock( &ptr->access_lock )
#endif

/* Per process data */
typedef enum { MPICH_PRE_INIT=0, MPICH_WITHIN_MPI=1,
               MPICH_POST_FINALIZED=2 } MPIR_MPI_State_t;

typedef struct {
    int appnum;          /* Application number provided by mpiexec (MPI-2) */
    int host;            /* host */
    int io;              /* standard io allowed */
    int lastusedcode;    /* last used error code (MPI-2) */
    int tag_ub;          /* Maximum message tag */
    int universe;        /* Universe size from mpiexec (MPI-2) */
    int wtime_is_global; /* Wtime is global over processes in COMM_WORLD */
} PreDefined_attrs;

typedef struct OpenPortNode {
    char port_name[MPI_MAX_PORT_NAME];
    int bfd;
    struct OpenPortNode *next;
} OpenPortNode_t;

typedef struct {
    MPIR_MPI_State_t  initialized;      /* Is MPI initalized? */
    int               thread_provided;  /* Provided level of thread support */
    MPID_Thread_key_t thread_key;       /* Id for perthread data */
    MPID_Thread_id_t  master_thread;    /* Thread that started MPI */
    MPID_Thread_lock_t allocation_lock; /* Used to lock around 
                                           list-allocations */
    MPID_Thread_lock_t common_lock;     /* General purpose common lock */
    int               do_error_checks;  /* runtime error check control */
    MPID_Comm         *comm_world;      /* Easy access to comm_world for
                                           error handler */
    PreDefined_attrs  attrs;            /* Predefined attribute values */
#ifdef HAVE_QUERYPERFORMANCECOUNTER
    double            timer_frequency;  /* High performance counter frequency */
#endif
    char              bnr_dbname[100];
    MPI_Comm          comm_parent;
    OpenPortNode_t    *port_list;
} MPICH_PerProcess_t;
extern MPICH_PerProcess_t MPIR_Process;

/* Record the level of thread support */
extern int MPID_THREAD_LEVEL;

#ifdef MPICH_SINGLE_THREADED
#define MPID_MAX_THREAD_LEVEL MPI_THREAD_FUNNELED
#else
#define MPID_MAX_THREAD_LEVEL MPI_THREAD_MULTIPLE
#endif

/* Allocation locks */
#if MPID_MAX_THREAD_LEVEL >= MPI_THREAD_FUNNELED
#define MPID_Allocation_lock()
#define MPID_Allocation_unlock()
#else
/* A more sophisticated version of these would handle the case where 
   the library supports MPI_THREAD_MULTIPLE but the user only asked for
   MPI_THREAD_FUNNELLED */
#define MPID_Allocation_lock() MPID_Thread_lock( &MPIR_Process.allocation_lock )
#define MPID_Allocation_unlock() MPID_Thread_unlock( &MPIR_Procss.allocation_lock )
#endif

/* Routine tracing */
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
int MPIR_Err_return_win( MPID_Win *, const char [], int );
int MPIR_Err_return_file( MPID_File *, const char [], int );
int MPIR_Err_create_code( int, const char [], ... );
void MPIR_Nest_incr( void );
void MPIR_Nest_decr( void );
int MPIR_Nest_value( void );
int MM_Open_port(MPID_Info *, char *);
int MM_Close_port(char *);
int MM_Accept(MPID_Info *, char *);
int MM_Connect(MPID_Info *, char *);
int MM_Send(int, char *, int);
int MM_Recv(int, char *, int);
int MM_Close(int);
#endif /* MPIIMPL_INCLUDED */
