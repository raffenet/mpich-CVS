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

/* Include definitions from the device which must exist before items in this
   file (mpiimple.h) can be defined.  NOTE: This include requires the device to
   copy mpidpre.h to the src/include directory in the build tree. */
#include "mpidpre.h"

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

/* Basic typedefs */
#ifndef HAVE_INT16_T 
/* Fix me (short may not be correct) */
typedef short int16_t;
#endif
#ifndef HAVE_INT32_T
/* Fix me (int may not be correct) */
typedef int int32_t;
#endif

#ifndef BOOL
typedef int BOOL;
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* Debugging and printf control */
/* Use these *only* for debugging output intended for the implementors
   and maintainers of MPICH.  Do *not* use these for any output that
   general users may normally see.  Use either the error code creation
   routines for error messages or msg_printf etc. for general messages 
   (msg_printf will go through gettext).  
*/
#define dbg_printf printf
#define dbg_fprintf fprintf
/* The following are temporary definitions */
#define msg_printf printf
#define msg_fprintf fprintf
#define err_printf printf
#define err_fprintf fprintf

/* Thread basics */
#ifdef MPICH_SINGLE_THREADED
typedef int MPID_Thread_key_t;
typedef int MPID_Thread_id_t;
typedef int MPID_Thread_lock_t;
#define MPID_Thread_lock( a )
#define MPID_Thread_unlock( a )
#define MPID_Thread_lock_init( a )
#define MPID_GetPerThread(p) p = &MPIR_Thread
#else /* Assumes pthreads for simplicity */
/* Eventually replace this with an include of a header file with the 
   correct definitions for the specified thread package. */
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
#error 'No Strdup available - need to provide one'
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
   and for the handles.  This is a 4 bit value.  0 is reserved for so 
   that all-zero handles can be flagged as an error. */
typedef enum { 
  MPID_COMM       = 0x1, 
  MPID_GROUP      = 0x2,
  MPID_DATATYPE   = 0x3,
  MPID_FILE       = 0x4,
  MPID_ERRHANDLER = 0x5,
  MPID_OP         = 0x6,
  MPID_INFO       = 0x7,
  MPID_WIN        = 0x8,
  MPID_KEYVAL     = 0x9,
  } MPID_Object_kind;
/* The above objects should correspond to MPI objects only. */
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

/* Handle block is between 1 and 1024 *elements* */
#define HANDLE_BLOCK_SIZE 256
/* Index size is bewtween 1 and 65536 *elements* */
#define HANDLE_BLOCK_INDEX_SIZE 1024

/* For direct, the remainder of the handle is the index into a predefined 
   block */
#define HANDLE_MASK 0x03FFFFFF
#define HANDLE_INDEX(a) ((a)& HANDLE_MASK)

/* ALL objects have the handle as the first value. */
/* Inactive (unused and stored on the appropriate avail list) objects 
   have MPIU_Handle_common as the head */
typedef struct {
    int  handle;
    void *next;   /* Free handles use this field to point to the next
		     free object */
} MPIU_Handle_common;

/* All *active* (in use) objects have the handle as the first value; objects
   with referene counts have the reference count as the second value.
   See MPIU_Object_add_ref and MPIU_Object_release_ref. */
typedef struct {
    int handle;
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
extern void *MPIU_Handle_obj_alloc( MPIU_Object_alloc_t * );
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
   
   We also need to know if the decremented value is zero so that we can see if
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
/* These are now internal to the handlemem package
void *MPIU_Handle_direct_init( void *, int, int, int );
void *MPIU_Handle_indirect_init( void *(**)[], int *, int, int, int, int );
int MPIU_Handle_free( void *((*)[]), int );
*/
/* Convert Handles to objects for MPI types that have predefined objects */
/* Question.  Should this do ptr=0 first, particularly if doing --enable-strict
   complication? */
#define MPID_Getb_ptr(kind,a,bmsk,ptr)					\
{									\
   switch (HANDLE_GET_KIND(a)) {					\
      case HANDLE_KIND_INVALID:						\
          ptr=0;							\
	  break;							\
      case HANDLE_KIND_BUILTIN:						\
          ptr=MPID_##kind##_builtin+((a)&(bmsk));			\
          break;							\
      case HANDLE_KIND_DIRECT:						\
          ptr=MPID_##kind##_direct+HANDLE_INDEX(a);			\
          break;							\
      case HANDLE_KIND_INDIRECT:					\
          ptr=((MPID_##kind*)						\
               MPIU_Handle_get_ptr_indirect(a,&MPID_##kind##_mem));	\
          break;							\
    }									\
}

/* Convert handles to objects for MPI types that do _not_ have any predefined
   objects */
/* Question.  Should this do ptr=0 first, particularly if doing --enable-strict
   complication? */
#define MPID_Get_ptr(kind,a,ptr)					\
{									\
   switch (HANDLE_GET_KIND(a)) {					\
      case HANDLE_KIND_INVALID:						\
          ptr=0;							\
	  break;							\
      case HANDLE_KIND_BUILTIN:						\
          ptr=0;                                                        \
          break;							\
      case HANDLE_KIND_DIRECT:						\
          ptr=MPID_##kind##_direct+HANDLE_INDEX(a);			\
          break;							\
      case HANDLE_KIND_INDIRECT:					\
          ptr=((MPID_##kind*)						\
               MPIU_Handle_get_ptr_indirect(a,&MPID_##kind##_mem));	\
          break;							\
    }									\
}

#define MPID_Comm_get_ptr(a,ptr)       MPID_Getb_ptr(Comm,a,0x03ffffff,ptr)
#define MPID_Group_get_ptr(a,ptr)      MPID_Getb_ptr(Group,a,0x03ffffff,ptr)
#define MPID_Datatype_get_ptr(a,ptr)   MPID_Getb_ptr(Datatype,a,0x000000ff,ptr)
#define MPID_Datatype_get_size(a)      (((a)&0xfc0000ff)>>8)
#define MPID_File_get_ptr(a,ptr)       MPID_Get_ptr(File,a,ptr)
#define MPID_Errhandler_get_ptr(a,ptr) MPID_Get_ptr(Errhandler,a,ptr)
#define MPID_Op_get_ptr(a,ptr)         MPID_Get_ptr(Op,a,ptr)
#define MPID_Info_get_ptr(a,ptr)       MPID_Get_ptr(Info,a,ptr)
#define MPID_Win_get_ptr(a,ptr)        MPID_Get_ptr(Win,a,ptr)
#define MPID_Request_get_ptr(a,ptr)    MPID_Get_ptr(Request,a,ptr)

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
#define MPID_Request_valid_ptr(ptr,err) MPID_Valid_ptr(Request,ptr,err)

/* Generic pointer test.  This is applied to any address, not just one from
   an MPI object.
   Currently unimplemented (returns success except for null pointers.
   With a little work, could check that the pointer is properly aligned,
   using something like 
   ((p) == 0 || ((char *)(p) & MPID_Alignbits[alignment] != 0)
   where MPID_Alignbits is set with a mask whose bits must be zero in a 
   properly aligned quantity.  For systems with no alignment rules, 
   all of these masks are zero, and this part of test can be eliminated.
 */
#define MPID_Pointer_is_invalid(p,alignment) ((p) == 0)

/* Parameter handling.  These functions have not been implemented yet.
   See src/util/param.[ch] */
typedef enum { MPIU_PARAM_FOUND = 0, 
               MPIU_PARAM_OK = 1, 
               MPIU_PARAM_ERROR = 2 } MPIU_Param_result_t;
int MPIU_Param_init( int *, char **[] );
int MPIU_Param_bcast( void );
int MPIU_Param_register( const char [], const char [], const char [] );
int MPIU_Param_get_int( const char [], int, int * );
int MPIU_Param_get_string( const char [], const char *, char ** );
void MPIU_Param_finalize( void );

/* Info */
typedef struct MPID_Info_s {
    int                handle;
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
  int                handle;
  volatile int       ref_count;
  MPID_Lang_t        language;
  MPID_Object_kind   kind;
  MPID_Errhandler_fn errfn;
  /* Other, device-specific information */
#ifdef MPID_DEV_ERRHANDLER_DECL
    MPID_DEV_ERRHANDLER_DECL
#endif
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
    int                  handle;
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

/* This structure is used to implement the group operations such as 
   MPI_Group_translate_ranks */
typedef struct {
    int          lpid, lrank;
} MPID_Group_pmap_t;

typedef struct {
    int          handle;
    volatile int ref_count;
    int          size;           /* Size of a group */
    int          *lrank_to_lpid; /* Array mapping a local rank to local 
				    process number */
    MPID_Group_pmap_t *lpid_to_lrank;
  /* Other, device-specific information */
#ifdef MPID_DEV_GROUP_DECL
    MPID_DEV_GROUP_DECL
#endif
} MPID_Group;

extern MPIU_Object_alloc_t MPID_Group_mem;
/* Preallocated group objects */
#define MPID_GROUP_N_BUILTIN 1
extern MPID_Group MPID_Group_builtin[MPID_GROUP_N_BUILTIN];
extern MPID_Group MPID_Group_direct[];

typedef struct MPIDI_VCRT * MPID_VCRT;
typedef struct MPIDI_VC   * MPID_VCR;

/* Communicators */
typedef struct MPID_Comm { 
    int           handle;        /* value of MPI_Comm for this structure */
    volatile int  ref_count;
    int16_t       context_id;    /* Assigned context id */
    int           remote_size;   /* Value of MPI_Comm_(remote)_size */
    int           rank;          /* Value of MPI_Comm_rank */
    MPID_VCRT     vcrt;          /* virtual connecton reference table */
    MPID_VCR     *vcr;           /* alias to the array of virtual connections in vcrt */
    MPID_List     attributes;    /* List of attributes */
    int           local_size;    /* Value of MPI_Comm_size for local group */
    MPID_Group   *local_group,   /* Groups in communicator. */
                 *remote_group;  /* The local and remote groups are the
				    same for intra communicators */
    char          name[MPI_MAX_OBJECT_NAME];  /* Required for MPI-2 */
    MPID_Errhandler *errhandler;  /* Pointer to the error handler structure */
    struct MPID_Collops_struct  *coll_fns; /* Pointer to a table of functions 
					      implementing the collective 
					      routines */
#ifndef MPICH_SINGLE_THREADED
    MPID_Thread_lock_t access_lock;
#endif
  /* Other, device-specific information */
#ifdef MPID_DEV_COMM_DECL
    MPID_DEV_COMM_DECL
#endif
} MPID_Comm;
extern MPIU_Object_alloc_t MPID_Comm_mem;
/* Preallocated comm objects */
#define MPID_COMM_N_BUILTIN 2
extern MPID_Comm MPID_Comm_builtin[MPID_COMM_N_BUILTIN];
extern MPID_Comm MPID_Comm_direct[];

#define MPID_CONTEXT_INTRA_PT2PT 0
#define MPID_CONTEXT_INTRA_COLL  1
#define MPID_CONTEXT_INTRA_FILE  2
#define MPID_CONTEXT_INTRA_WIN   3
#define MPID_CONTEXT_INTER_PT2PT 0
#define MPID_CONTEXT_INTER_COLLA 1
#define MPID_CONTEXT_INTER_COLLB 2
#define MPID_CONTEXT_INTER_COLL  3

/* Requests */
/* This currently defines a single structure type for all requests.  
   Eventually, we may want a union type, as used in MPICH-1 */
typedef enum { MPID_REQUEST_SEND, MPID_REQUEST_RECV, MPID_PREQUEST_SEND, 
	       MPID_PREQUEST_RECV, MPID_UREQUEST } MPID_Request_kind_t;
typedef struct MPID_Request {
    int          handle;
    volatile int ref_count;
    MPID_Request_kind_t kind;
    /* completion counter */
    volatile int cc;
    /* pointer to the completion counter */
    /* This is necessary for the case when an operation is described by a 
       list of requests */
    int volatile *cc_ptr;
    /* A comm is needed to find the proper error handler */
    MPID_Comm *comm;
    /* Status is needed for wait/test/recv */
    MPI_Status status;
    /* Persistent requests have their own, "real" requests */
    struct MPID_Request *active_request;
    /* Still missing: user-defined request support */
    /* Other, device-specific information */
#ifdef MPID_DEV_REQUEST_DECL
    MPID_DEV_REQUEST_DECL
#endif
} MPID_Request;
extern MPIU_Object_alloc_t MPID_Request_mem;
/* Preallocated request objects */
extern MPID_Request MPID_Request_direct[];

/* Windows */
typedef struct {
    int           handle;             /* value of MPI_Win for this structure */
    volatile int  ref_count;
    MPID_Errhandler *errhandler;  /* Pointer to the error handler structure */
    MPI_Aint    length;        
    int          disp_unit;      /* Displacement unit of *local* window */
    MPID_List    attributes;
    MPID_Comm    *comm;         /* communicator of window */
    char          name[MPI_MAX_OBJECT_NAME];  
  /* Other, device-specific information */
#ifdef MPID_DEV_WIN_DECL
    MPID_DEV_WIN_DECL
#endif
} MPID_Win;
extern MPIU_Object_alloc_t MPID_Win_mem;
/* Preallocated win objects */
extern MPID_Win MPID_Win_direct[];

/* Datatypes */

typedef struct MPID_Datatype_st { 
    int           handle;            /* value of MPI_Datatype for structure */
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
    int (*free_fn)( struct MPID_Datatype_st * ); /* Function to free this datatype */
    /* Other, device-specific information */
#ifdef MPID_DEV_DATATYPE_DECL
    MPID_DEV_DATATYPE_DECL
#endif
} MPID_Datatype;
extern MPIU_Object_alloc_t MPID_Datatype_mem;
/* Preallocated datatype objects */
#define MPID_DATATYPE_N_BUILTIN 33
extern MPID_Datatype MPID_Datatype_builtin[MPID_DATATYPE_N_BUILTIN];
extern MPID_Datatype MPID_Datatype_direct[];

/* Collective operations */
typedef struct MPID_Collops_struct {
    int ref_count;   /* Supports lazy copies */
    /* Contains pointers to the functions for the MPI collectives */
    int (*Barrier) (MPID_Comm *);
    int (*Bcast) (void*, int, MPID_Datatype *, int, MPID_Comm * );
    int (*Gather) (void*, int, MPID_Datatype *, void*, int, MPID_Datatype *, 
		   int, MPID_Comm *); 
    int (*Gatherv) (void*, int, MPID_Datatype *, void*, int *, int *, 
		    MPID_Datatype *, int, MPID_Comm *); 
    int (*Scatter) (void*, int, MPID_Datatype *, void*, int, MPID_Datatype *, 
		    int, MPID_Comm *);
    int (*Scatterv) (void*, int *, int *, MPID_Datatype *, void*, int, 
		    MPID_Datatype *, int, MPID_Comm *);
    int (*Allgather) (void*, int, MPID_Datatype *, void*, int, 
		      MPID_Datatype *, MPID_Comm *);
    int (*Allgatherv) (void*, int, MPID_Datatype *, void*, int *, int *, 
		       MPID_Datatype *, MPID_Comm *);
    int (*Alltoall) (void*, int, MPID_Datatype *, void*, int, MPID_Datatype *, 
			       MPID_Comm *);
    int (*Alltoallv) (void*, int *, int *, MPID_Datatype *, void*, int *, 
		     int *, MPID_Datatype *, MPID_Comm *);
    int (*Alltoallw) (void*, int *, int *, MPID_Datatype *, void*, int *, 
		     int *, MPID_Datatype *, MPID_Comm *);
    int (*Reduce) (void*, void*, int, MPID_Datatype *, MPI_Op, int, 
		   MPID_Comm *);
    int (*Allreduce) (void*, void*, int, MPID_Datatype *, MPI_Op, 
		      MPID_Comm *);
    int (*Reduce_scatter) (void*, void*, int *, MPID_Datatype *, MPI_Op, 
			   MPID_Comm *);
    int (*Scan) (void*, void*, int, MPID_Datatype *, MPI_Op, MPID_Comm * );
    int (*Exscan) (void*, void*, int, MPID_Datatype *, MPI_Op, MPID_Comm * );
    
} MPID_Collops;
        
/* Files */
typedef struct {
    int           handle;             /* value of MPI_File for this structure */
    volatile int  ref_count;
    MPID_Errhandler *errhandler;  /* Pointer to the error handler structure */
  /* Other, device-specific information */
#ifdef MPID_DEV_FILE_DECL
    MPID_DEV_FILE_DECL
#endif
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
    MPID_Comm         *comm_self;       /* Easy access to comm_self */
    MPID_Comm         *comm_parent;     /* Easy access to comm_parent */
    PreDefined_attrs  attrs;            /* Predefined attribute values */
    /* Communicator context ids.  Special data is needed for thread-safety */
    int context_id_mask[32];
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

/* Routine tracing (see --enable-timing for control of this) */
#ifdef HAVE_TIMING
/* Possible values for timing */
#define MPID_TIMING_KIND_OFF 0
#define MPID_TIMING_KIND_TIME 1
#define MPID_TIMING_KIND_LOG 2
#define MPID_TIMING_KIND_ALL 3
#define MPID_TIMING_KIND_RUNTIME 4

/* These next two include files contain the static state definitions */
#include "mpistates.h"
#include "mpisysstates.h"
extern void MPID_TimerStateBegin( int, MPID_Time_t * );
extern void MPID_TimerStateEnd( int, MPID_Time_t * );
#define MPID_MPI_STATE_DECLS  MPID_Time_t time_stamp
#define MPID_MPI_FUNC_EXIT(a) MPID_TimerStateBegin( a, &time_stamp )
#define MPID_MPI_FUNC_ENTER(a) MPID_TimerStateEnd( a, &time_stamp )
/* Statistics macros aren't defined yet */
/* All uses of these are protected by the symbol COLLECT_STATS, so they
   do not need to be defined in the non-HAVE_TIMING branch. */
#define MPID_STAT_BEGIN
#define MPID_STAT_END
#define MPID_STAT_ACC(statid,val)
#define MPID_STAT_ACC_RANGE(statid,rng)
#define MPID_STAT_ACC_SIMPLE(statid,val)
#define MPID_STAT_MISC(a) a
#else
#define MPID_MPI_STATE_DECLS
#define MPID_MPI_FUNC_EXIT(a)
#define MPID_MPI_FUNC_ENTER(a)
#endif

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

/*
 * Standardized general-purpose atomic update routines.  Some comments:
 * Setmax atomically implements *a_ptr = max(b,*a_ptr) .  This can
 * be implemented using compare-and-swap (form max, if new max is 
 * larger, compare-and-swap against old max.  if failure, restart).
 * Fetch_and_increment can be implemented in a similar way.
 * Implementations using LoadLink/StoreConditional are similar.
 *
 * Question: can we use the simple code for MPI_THREAD_SERIALIZED?
 * If not, do we want a separate set of definitions that can be used
 * in the code where serialized is ok.
 *
 * Currently, these are used in the routines to create new error classes
 * and codes.  Note that MPI object reference counts are handled with
 * their own routines.
 */
#if MPID_MAX_THREAD_LEVEL >= MPI_THREAD_FUNNELED
#define MPIR_Setmax(a_ptr,b) if (b>*(a_ptr)) { *(a_ptr) = b; }
#define MPIR_Fetch_and_increment(count_ptr,value_ptr) \
    { *value_ptr = *count_ptr; *count_ptr += 1; }
/* Here should go assembly language versions for various architectures */
#else
#define MPIR_Setmax(a_ptr,b) \
    {MPID_Thread_lock(&MPIR_Process.common_lock);\
    if (b > *(a_ptr)) *(a_ptr)=b;\
    MPID_Thread_unlock(&MPIR_Process.common_lock);}
#define MPIR_Fetch_and_increment(count_ptr,value_ptr) \
    {MPID_Thread_lock(&MPIR_Process.common_lock);\
    *value_ptr = *count_ptr; *count_ptr += 1; \
    MPID_Thread_unlock(&MPIR_Process.common_lock);}
#endif

/* Include definitions from the device which require items defined by this file
   (mpiimpl.h).  NOTE: This include requires the device to copy mpidpost.h to
   the src/include directory in the build tree. */
#include "mpidpost.h"

/*** ONLY FUNCTION DECLARATIONS BEYOND THIS POINT ***/

/* Bindings for internal routines */
void MPIR_Add_finalize( int (*)( void * ), void * );
int MPIR_Err_return_comm( MPID_Comm *, const char [], int );
int MPIR_Err_return_win( MPID_Win *, const char [], int );
int MPIR_Err_return_file( MPID_File *, const char [], int );
int MPIR_Err_create_code( int, const char [], ... );
void MPIR_Nest_incr( void );
void MPIR_Nest_decr( void );
int MPIR_Nest_value( void );

/* ADI Bindings */
int MPID_Init(int *, char ***, int, int *, int *, int *);
int MPID_Finalize(void);

int MPID_Open_port(MPID_Info *, char *);
int MPID_Close_port(char *);
int MPID_Comm_accept(char *, MPID_Info *, int, MPID_Comm *, MPID_Comm **);
int MPID_Comm_connect(char *, MPID_Info *, int, MPID_Comm *, MPID_Comm **);
int MPID_Comm_disconnect(MPID_Comm *);
int MPID_Comm_spawn_multiple(int, char *[], char* *[], int [], MPI_Info [],
			     int, MPID_Comm *, MPID_Comm **, int []);

int MPID_Send(const void *, int, MPI_Datatype, int, int, MPID_Comm *, int,
	       MPID_Request **);
int MPID_Isend(const void *, int, MPI_Datatype, int, int, MPID_Comm *, int,
	       MPID_Request **);
int MPID_Recv(void *, int, MPI_Datatype, int, int, MPID_Comm *, int,
	      MPI_Status *, MPID_Request **);
int MPID_Irecv(void *, int, MPI_Datatype, int, int, MPID_Comm *, int,
	       MPID_Request **);
#if !defined(MPID_Progress_start)
void MPID_Progress_start(void);
#endif
#if !defined(MPID_Progress_end)
void MPID_Progress_end(void);
#endif
#if !defined(MPID_Progress_test)
int MPID_Progress_test(void);
#endif
#if !defined(MPID_Progress_wait)
void MPID_Progress_wait(void);
#endif
#if !defined(MPID_Progress_poke)
void MPID_Progress_poke(void);
#endif

#if !defined(MPID_Request_free)
void MPID_Request_free(MPID_Request *);
#endif

int MPID_VCRT_Create(int size, MPID_VCRT *vcrt_ptr);
int MPID_VCRT_Add_ref(MPID_VCRT vcrt);
int MPID_VCRT_Release(MPID_VCRT vcrt);
int MPID_VCRT_Get_ptr(MPID_VCRT vcrt, MPID_VCR **vc_pptr);

#endif /* MPIIMPL_INCLUDED */
