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

/* Include nested mpi (NMPI) definitions */
#include "nmpi.h"

/* There are a few definitions that must be made *before* the mpichconf.h 
   file is included.  These include the definitions of the error levels */
#define MPICH_ERROR_MSG_NONE 0
#define MPICH_ERROR_MSG_CLASS 1
#define MPICH_ERROR_MSG_GENERIC 2
#define MPICH_ERROR_MSG_ALL 8

/* Data computed by configure.  This is included *after* mpi.h because we
   do not want mpi.h to depend on any other files or configure flags */
#include "mpichconf.h"

/* Include definitions from the device which must exist before items in this
   file (mpiimpl.h) can be defined.  NOTE: This include requires the device to
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

/* This allows us to keep names local to a single file when we can use
   weak symbols */
#ifdef  USE_WEAK_SYMBOLS
#define PMPI_LOCAL static
#else
#define PMPI_LOCAL 
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
#define BOOL int
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
typedef enum MPIU_dbg_state_t
{
    MPIU_DBG_STATE_NONE = 0,
    MPIU_DBG_STATE_UNINIT = 1,
    MPIU_DBG_STATE_STDOUT = 2,
    MPIU_DBG_STATE_MEMLOG = 4,
}
MPIU_dbg_state_t;
int MPIU_dbg_printf(char *str, ...);
#if defined(MPICH_DBG_OUTPUT)
extern MPIU_dbg_state_t MPIUI_dbg_state;
#define MPIU_DBG_PRINTF(e)			\
{						\
    if (MPIUI_dbg_state != MPIU_DBG_STATE_NONE)	\
    {						\
	MPIU_dbg_printf e;			\
    }						\
}
#else
#define MPIU_DBG_PRINTF(e)
#endif
void MPIU_dump_dbg_memlog_to_stdout(void);
void MPIU_dump_dbg_memlog_to_file(FILE * fp);
/* The follow is temporarily provided for backward compatibility.  Any code
   using dbg_printf should be updated to use MPIU_DBG_PRINTF. */
#define dbg_printf MPIU_dbg_printf
/* The following are temporary definitions */
int msg_printf(char *str, ...);
#define msg_fprintf fprintf
int err_printf(char *str, ...);
#define err_fprintf fprintf

#define MPIU_Assert assert

#include "mpiimplthread.h"

/* Memory allocation */
/* style: allow:malloc:2 sig:0 */
/* style: allow:free:2 sig:0 */
/* style: allow:strdup:2 sig:0 */
/* style: allow:calloc:2 sig:0 */
/* style: define:__strdup:1 sig: 0 */
/* style: define:strdup:1 sig: 0 */

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
typedef struct MPIU_Mem_stack { int n_alloc; void *ptrs[MAX_MEM_STACK]; } MPIU_Mem_stack;
#define MALLOC_STK(n,a) {a=MPIU_Malloc(n);\
               if (memstack.n_alloc >= MAX_MEM_STACK) abort(implerror);\
               memstack.ptrs[memstack.n_alloc++] = a;}
#define FREE_STK     {int i; for (i=memstack.n_alloc-1;i>=0;i--) {\
               MPIU_Free(memstack.ptrs[i]);}}
#define MALLOC_STK_INIT memstack.n_alloc = 0
#define MALLOC_STK_DECL MPIU_Mem_stack memstack

/* Message printing */
int MPIU_usage_printf( char *str, ... );
int MPIU_error_printf( char *str, ... );

/* Known language bindings */
typedef enum MPID_Lang_t { MPID_LANG_C, MPID_LANG_FORTRAN, 
               MPID_LANG_CXX, MPID_LANG_FORTRAN90 } MPID_Lang_t;

/* Known MPI object types.  These are used for both the error handlers 
   and for the handles.  This is a 4 bit value.  0 is reserved for so 
   that all-zero handles can be flagged as an error. */
typedef enum MPID_Object_kind { 
  MPID_COMM       = 0x1, 
  MPID_GROUP      = 0x2,
  MPID_DATATYPE   = 0x3,
  MPID_FILE       = 0x4,
  MPID_ERRHANDLER = 0x5,
  MPID_OP         = 0x6,
  MPID_INFO       = 0x7,
  MPID_WIN        = 0x8,
  MPID_KEYVAL     = 0x9,
  MPID_ATTR       = 0xa,
  MPID_REQUEST    = 0xb,
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
typedef struct MPIU_Handle_common {
    int  handle;
    void *next;   /* Free handles use this field to point to the next
                     free object */
} MPIU_Handle_common;

/* All *active* (in use) objects have the handle as the first value; objects
   with referene counts have the reference count as the second value.
   See MPIU_Object_add_ref and MPIU_Object_release_ref. */
typedef struct MPIU_Handle_head {
    int handle;
    volatile int ref_count;
} MPIU_Handle_head;

/* This type contains all of the data, except for the direct array,
   used by the object allocators. */
typedef struct MPIU_Object_alloc_t {
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
#define MPIU_Object_add_ref(objptr) \
    ((MPIU_Handle_head*)(objptr))->ref_count++
#define MPIU_Object_release_ref(objptr,inuse_ptr) \
    *(inuse_ptr)=--((MPIU_Handle_head*)(objptr))->ref_count
#else
/* These can be implemented using special assembly language operations
   on most processors.  If no such operation is available, then each
   object, in addition to the ref_count field, must have a thread-lock. 
   
   We also need to know if the decremented value is zero so that we can see if
   we must deallocate the object.  This fetch and decrement must be 
   atomic so that multiple threads don't decide that they were 
   responsible for setting the value to zero.
 */
#if USE_ATOMIC_UPDATES
#ifdef HAVE_PENTIUM_GCC_ASM
#define MPID_Atomic_incr( count_ptr ) \
   __asm__ __volatile__ ( "lock; incl %0" \
                         : "=m" (*count_ptr) :: "memory", "cc" )

#define MPID_Atomic_decr_flag( count_ptr, flag ) \
   __asm__ __volatile__ ( "lock; decl %0 ; setnz %1" \
                         : "=m" (*count_ptr) , "=q" (flag) :: "memory", "cc" )
#else
#abort "Atomic updates specified but no code for this platform"
#endif
#define MPIU_Object_add_ref(objptr) \
    MPID_Atomic_incr(&((objptr)->ref_count))
#define MPIU_Object_release_ref(objptr,inuse_ptr) \
    { int flag; 
      MPID_Atomic_decr_flag(&((objptr)->ref_count),flag); *inuse_ptr = flag; }
#else
#define MPIU_Object_add_ref(objptr) \
    {MPID_Thread_lock(&(objptr)->mutex);(objptr)->ref_count++;\
    MPID_Thread_unlock(&(objptr)->mutex);}
#define MPIU_Object_release_ref(objptr,isuse_ptr) \
    {MPID_Thread_lock(&(objptr)->mutex);*(inuse_ptr)=--(objptr)->ref_count;\
    MPID_Thread_unlock(&(objptr)->mutex);}
#endif
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
#define MPID_Getb_ptr(kind,a,bmsk,ptr)                                  \
{                                                                       \
   switch (HANDLE_GET_KIND(a)) {                                        \
      case HANDLE_KIND_BUILTIN:                                         \
          ptr=MPID_##kind##_builtin+((a)&(bmsk));                       \
          break;                                                        \
      case HANDLE_KIND_DIRECT:                                          \
          ptr=MPID_##kind##_direct+HANDLE_INDEX(a);                     \
          break;                                                        \
      case HANDLE_KIND_INDIRECT:                                        \
          ptr=((MPID_##kind*)                                           \
               MPIU_Handle_get_ptr_indirect(a,&MPID_##kind##_mem));     \
          break;                                                        \
      case HANDLE_KIND_INVALID:                                         \
      default:								\
          ptr=0;							\
          break;							\
    }                                                                   \
}

/* Convert handles to objects for MPI types that do _not_ have any predefined
   objects */
/* Question.  Should this do ptr=0 first, particularly if doing --enable-strict
   complication? */
#define MPID_Get_ptr(kind,a,ptr)					\
{									\
   switch (HANDLE_GET_KIND(a)) {					\
      case HANDLE_KIND_DIRECT:						\
          ptr=MPID_##kind##_direct+HANDLE_INDEX(a);			\
          break;							\
      case HANDLE_KIND_INDIRECT:					\
          ptr=((MPID_##kind*)						\
               MPIU_Handle_get_ptr_indirect(a,&MPID_##kind##_mem));	\
          break;							\
      case HANDLE_KIND_INVALID:						\
      case HANDLE_KIND_BUILTIN:						\
      default:								\
          ptr=0;							\
          break;							\
     }									\
}

#define MPID_Comm_get_ptr(a,ptr)       MPID_Getb_ptr(Comm,a,0x03ffffff,ptr)
#define MPID_Group_get_ptr(a,ptr)      MPID_Getb_ptr(Group,a,0x03ffffff,ptr)
#define MPID_Datatype_get_ptr(a,ptr)   MPID_Getb_ptr(Datatype,a,0x000000ff,ptr)
#define MPID_Datatype_get_basic_size(a) (((a)&0x0000ff00)>>8)
#define MPID_File_get_ptr(a,ptr)       MPID_Get_ptr(File,a,ptr)
#define MPID_Errhandler_get_ptr(a,ptr) MPID_Get_ptr(Errhandler,a,ptr)
#define MPID_Op_get_ptr(a,ptr)         MPID_Getb_ptr(Op,a,0x000000ff,ptr)
#define MPID_Info_get_ptr(a,ptr)       MPID_Get_ptr(Info,a,ptr)
#define MPID_Win_get_ptr(a,ptr)        MPID_Get_ptr(Win,a,ptr)
#define MPID_Request_get_ptr(a,ptr)    MPID_Get_ptr(Request,a,ptr)
#define MPID_Keyval_get_ptr(a,ptr)     MPID_Get_ptr(Keyval,a,ptr)


/* Note: Probably there is some clever way to build all of these from a macro.
 */
#define MPID_Datatype_get_size_macro(a,__size)				\
do {									\
    void *ptr;								\
    switch (HANDLE_GET_KIND(a)) {					\
        case HANDLE_KIND_DIRECT:					\
            ptr = MPID_Datatype_direct+HANDLE_INDEX(a);			\
            __size = ((MPID_Datatype *) ptr)->size;			\
            break;							\
        case HANDLE_KIND_INDIRECT:					\
            ptr = ((MPID_Datatype *)					\
		   MPIU_Handle_get_ptr_indirect(a,&MPID_Datatype_mem));	\
            __size = ((MPID_Datatype *) ptr)->size;			\
            break;							\
        case HANDLE_KIND_BUILTIN:					\
            __size = MPID_Datatype_get_basic_size(a);			\
            break;							\
        case HANDLE_KIND_INVALID:					\
        default:							\
	    __size = 0;							\
	    break;							\
 									\
    }									\
} while (0)

#define MPID_Datatype_get_loopdepth_macro(a,__depth)                    \
do {                                                                    \
    void *ptr;                                                          \
    switch (HANDLE_GET_KIND(a)) {                                       \
        case HANDLE_KIND_DIRECT:                                        \
            ptr = MPID_Datatype_direct+HANDLE_INDEX(a);                 \
            __depth = ((MPID_Datatype *) ptr)->loopinfo_depth;          \
            break;                                                      \
        case HANDLE_KIND_INDIRECT:                                      \
            ptr = ((MPID_Datatype *)                                    \
		   MPIU_Handle_get_ptr_indirect(a,&MPID_Datatype_mem)); \
            __depth = ((MPID_Datatype *) ptr)->loopinfo_depth;          \
            break;                                                      \
        case HANDLE_KIND_INVALID:                                       \
        case HANDLE_KIND_BUILTIN:                                       \
        default:                                                        \
            __depth = 0;                                                \
            break;                                                      \
    }                                                                   \
} while (0)
#define MPID_Datatype_get_loopptr_macro(a,__lptr)                       \
do {                                                                    \
    void *ptr;                                                          \
    switch (HANDLE_GET_KIND(a)) {                                       \
        case HANDLE_KIND_DIRECT:                                        \
            ptr = MPID_Datatype_direct+HANDLE_INDEX(a);                 \
            __lptr = ((MPID_Datatype *) ptr)->opt_loopinfo;             \
            break;                                                      \
        case HANDLE_KIND_INDIRECT:                                      \
            ptr = ((MPID_Datatype *)                                    \
		   MPIU_Handle_get_ptr_indirect(a,&MPID_Datatype_mem)); \
            __lptr = ((MPID_Datatype *) ptr)->opt_loopinfo;             \
            break;                                                      \
        case HANDLE_KIND_INVALID:                                       \
        case HANDLE_KIND_BUILTIN:                                       \
        default:                                                        \
            __lptr = 0;                                                 \
            break;                                                      \
    }                                                                   \
} while (0)
        
#define MPID_Datatype_get_extent_macro(a,__extent)                      \
do {                                                                    \
    void *ptr;                                                          \
    switch (HANDLE_GET_KIND(a)) {                                       \
        case HANDLE_KIND_DIRECT:                                        \
            ptr = MPID_Datatype_direct+HANDLE_INDEX(a);                 \
            __extent = ((MPID_Datatype *) ptr)->extent;                 \
            break;                                                      \
        case HANDLE_KIND_INDIRECT:                                      \
            ptr = ((MPID_Datatype *)                                    \
		   MPIU_Handle_get_ptr_indirect(a,&MPID_Datatype_mem)); \
            __extent = ((MPID_Datatype *) ptr)->extent;                 \
            break;                                                      \
        case HANDLE_KIND_INVALID:                                       \
        case HANDLE_KIND_BUILTIN:                                       \
        default:                                                        \
            __extent = MPID_Datatype_get_basic_size(a);  /* same as size */   \
            break;                                                      \
    }                                                                   \
} while (0)

/* Valid pointer checks */
/* This test is lame.  Should eventually include cookie test 
   and in-range addresses */
#define MPID_Valid_ptr(kind,ptr,err) \
  {if (!(ptr)) { err = MPIR_Err_create_code( MPI_ERR_OTHER, "**nullptr", 0 ); } }

#define MPID_Info_valid_ptr(ptr,err) MPID_Valid_ptr(Info,ptr,err)
#define MPID_Comm_valid_ptr(ptr,err) MPID_Valid_ptr(Comm,ptr,err)
#define MPID_Datatype_valid_ptr(ptr,err) MPID_Valid_ptr(Datatype,ptr,err)
#define MPID_Group_valid_ptr(ptr,err) MPID_Valid_ptr(Group,ptr,err)
#define MPID_Win_valid_ptr(ptr,err) MPID_Valid_ptr(Win,ptr,err)
#define MPID_Op_valid_ptr(ptr,err) MPID_Valid_ptr(Op,ptr,err)
#define MPID_Errhandler_valid_ptr(ptr,err) MPID_Valid_ptr(Errhandler,ptr,err)
#define MPID_File_valid_ptr(ptr,err) MPID_Valid_ptr(File,ptr,err)
#define MPID_Request_valid_ptr(ptr,err) MPID_Valid_ptr(Request,ptr,err)
#define MPID_Keyval_valid_ptr(ptr,err) MPID_Valid_ptr(Keyval,ptr,err)

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
typedef enum MPIU_Param_result_t { MPIU_PARAM_FOUND = 0, 
               MPIU_PARAM_OK = 1, 
               MPIU_PARAM_ERROR = 2 } MPIU_Param_result_t;
int MPIU_Param_init( int *, char *[], const char [] );
int MPIU_Param_bcast( void );
int MPIU_Param_register( const char [], const char [], const char [] );
int MPIU_Param_get_int( const char [], int, int * );
int MPIU_Param_get_string( const char [], const char *, char ** );
void MPIU_Param_finalize( void );

/* Info */
typedef struct MPID_Info {
    int                handle;
    struct MPID_Info   *next;
    char               *key;
    char               *value;
} MPID_Info;
extern MPIU_Object_alloc_t MPID_Info_mem;
/* Preallocated info objects */
extern MPID_Info MPID_Info_direct[];

/* Error Handlers */
typedef union MPID_Errhandler_fn {
   void (*C_Comm_Handler_function) ( MPI_Comm *, int *, ... );
   void (*F77_Handler_function) ( MPI_Fint *, MPI_Fint *, ... );
   void (*C_Win_Handler_function) ( MPI_Win *, int *, ... );
   void (*C_File_Handler_function) ( MPI_File *, int *, ... );
} MPID_Errhandler_fn;

typedef struct MPID_Errhandler {
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
extern MPIU_Object_alloc_t MPID_Errhandler_mem;
/* Preallocated errhandler objects */
extern MPID_Errhandler MPID_Errhandler_direct[];

/* Keyvals and attributes */
typedef union MPID_Copy_function {
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

typedef union MPID_Delete_function {
  int  (*C_CommDeleteFunction)  ( MPI_Comm, int, void *, void * );
  void (*F77_DeleteFunction)( MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *, 
                              MPI_Fint * );
  void (*F90_DeleteFunction)( MPI_Fint *, MPI_Fint *, MPI_Aint *, MPI_Aint *, 
                              MPI_Fint * );
  int  (*C_FileDeleteFunction)  ( MPI_File, int, void *, void * );
  int  (*C_TypeDeleteFunction)  ( MPI_Datatype, int, void *, void * );
  
} MPID_Delete_function;

typedef struct MPID_Keyval {
    int                  handle;
    volatile int         ref_count;
    MPID_Lang_t          language;
    MPID_Object_kind     kind;
    void                 *extra_state;
    MPID_Copy_function   copyfn;
    MPID_Delete_function delfn;
  /* other, device-specific information */
#ifdef MPID_DEV_KEYVAL_DECL
    MPID_DEV_KEYVAL_DECL
#endif
} MPID_Keyval;

/* Attributes need no ref count or handle, but since we want to use the
   common block allocator for them, we must provide those elements 
*/
typedef struct MPID_Attribute {
    int          handle;
    volatile int ref_count;
    MPID_Keyval  *keyval;           /* Keyval structure for this attribute */
    struct MPID_Attribute *next;    /* Pointer to next in the list */
    long        pre_sentinal;       /* Used to detect user errors in accessing
				       the value */
    void *      value;              /* Stored value */
    long        post_sentinal;      /* Like pre_sentinal */
    /* other, device-specific information */
#ifdef MPID_DEV_ATTR_DECL
    MPID_DEV_ATTR_DECL
#endif
} MPID_Attribute;

/*---------------------------------------------------------------------------
 * Groups are *not* a major data structure in MPICH-2.  They are provided
 * only because they are required for the group operations (e.g., 
 * MPI_Group_intersection) and for the scalable RMA synchronization
 *---------------------------------------------------------------------------*/
/* This structure is used to implement the group operations such as 
   MPI_Group_translate_ranks */
typedef struct MPID_Group_pmap_t {
    int          lrank;     /* Local rank in group (between 0 and size-1) */
    int          lpid;      /* local process id, from VCONN */
    int          next_lpid; /* Index of next lpid (in lpid order) */
    int          flag;      /* marker, used to implement group operations */
} MPID_Group_pmap_t;

typedef struct MPID_Group {
    int          handle;
    volatile int ref_count;
    int          size;           /* Size of a group */
    int          rank;           /* rank of this process relative to this 
				    group */
    int          idx_of_first_lpid;
    MPID_Group_pmap_t *lrank_to_lpid; /* Array mapping a local rank to local 
					 process number */
    /* We may want some additional data for the RMA syncrhonization calls */
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

typedef enum MPID_Comm_kind_t { MPID_INTRACOMM = 0, MPID_INTERCOMM = 1 } MPID_Comm_kind_t;
/* Communicators */
typedef struct MPID_Comm { 
    int           handle;        /* value of MPI_Comm for this structure */
    volatile int  ref_count;
    int16_t       context_id;    /* Assigned context id */
    int           remote_size;   /* Value of MPI_Comm_(remote)_size */
    int           rank;          /* Value of MPI_Comm_rank */
    MPID_VCRT     vcrt;          /* virtual connecton reference table */
    MPID_VCR     *vcr;           /* alias to the array of virtual connections in vcrt */
    MPID_Attribute *attributes;    /* List of attributes */
    int           local_size;    /* Value of MPI_Comm_size for local group */
    MPID_Group   *local_group,   /* Groups in communicator. */
                 *remote_group;  /* The local and remote groups are the
                                    same for intra communicators */
    MPID_Comm_kind_t comm_kind;  /* MPID_INTRACOMM or MPID_INTERCOMM */
    char          name[MPI_MAX_OBJECT_NAME];  /* Required for MPI-2 */
    MPID_Errhandler *errhandler;  /* Pointer to the error handler structure */
    struct MPID_Collops  *coll_fns; /* Pointer to a table of functions 
                                              implementing the collective 
                                              routines */
#ifndef MPICH_SINGLE_THREADED
    MPID_Thread_lock_t access_lock;
#endif
#ifdef MPID_HAS_HETERO
    int is_hetero;
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
typedef enum MPID_Request_kind_t { MPID_REQUEST_SEND, MPID_REQUEST_RECV, MPID_PREQUEST_SEND, 
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
    /* Persistent requests have their own "real" requests.  Receive requests
       have partnering send requests when src=dest. etc. */
    struct MPID_Request *partner_request;
    /* User-defined request support */
    MPI_Grequest_cancel_function *cancel_fn;
    MPI_Grequest_free_function *free_fn;
    MPI_Grequest_query_function *query_fn;
    void *grequest_extra_state;
    
    /* Other, device-specific information */
#ifdef MPID_DEV_REQUEST_DECL
    MPID_DEV_REQUEST_DECL
#endif
} MPID_Request;
extern MPIU_Object_alloc_t MPID_Request_mem;
/* Preallocated request objects */
extern MPID_Request MPID_Request_direct[];

/* Windows */
typedef struct MPID_Win {
    int           handle;             /* value of MPI_Win for this structure */
    volatile int  ref_count;
    MPID_Errhandler *errhandler;  /* Pointer to the error handler structure */
    MPI_Aint    length;        
    int          disp_unit;      /* Displacement unit of *local* window */
    MPID_Attribute *attributes;
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

typedef struct MPID_Datatype { 
    int           handle;            /* value of MPI_Datatype for structure */
    volatile int  ref_count;
    int           is_contig;     /* True if data is contiguous (even with 
                                    a (count,datatype) pair) */
    struct MPID_Dataloop *opt_loopinfo;  /* "optimized" loopinfo.  
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
    struct MPID_Dataloop *loopinfo; /* Original loopinfo, used when 
                                          creating and when getting contents */
    int           has_mpi1_ub;   /* The MPI_UB and MPI_LB are sticky */
    int           has_mpi1_lb;
    int           is_permanent;  /* True if datatype is a predefined type */
    int           is_committed;  /* */

    int           loopinfo_depth; /* Depth of dataloop stack needed
                                     to process this datatype.  This 
                                     information is used to ensure that
                                     no datatype is constructed that
                                     cannot be processed (see MPID_Segment) */
    /* int opt_loopinfo_depth ??? */

    MPID_Attribute   *attributes;    /* MPI-2 adds datatype attributes */

    int32_t       cache_id;      /* These are used to track which processes */
    /* MPID_Lpidmask mask; */         /* have cached values of this datatype */

    char          name[MPI_MAX_OBJECT_NAME];  /* Required for MPI-2 */

    /* The following is needed to efficiently implement MPI_Get_elements */
    int           n_elements;   /* Number of basic elements in this datatype */
    MPI_Aint      element_size; /* Size of each element or -1 if elements are
                                   not all the same size */
    int (*free_fn)( struct MPID_Datatype * ); /* Function to free this datatype */
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

/* Reduction and accumulate operations */
/*E
  MPID_Op_kind - Enumerates types of MPI_Op types

  Notes:
  These are needed for implementing 'MPI_Accumulate', since only predefined
  operations are allowed for that operation.  

  A gap in the enum values was made allow additional predefined operations
  to be inserted.  This might include future additions to MPI or experimental
  extensions (such as a Read-Modify-Write operation).

  Module:
  Collective-DS
  E*/
typedef enum MPID_Op_kind { MPID_OP_MAX=1, MPID_OP_MIN=2, MPID_OP_SUM=3, MPID_OP_PROD=4, 
	       MPID_OP_LAND=5, MPID_OP_BAND=6, MPID_OP_LOR=7, MPID_OP_BOR=8,
	       MPID_OP_LXOR=9, MPID_OP_BXOR=10, MPID_OP_MAXLOC=11, 
               MPID_OP_MINLOC=12, MPID_OP_REPLACE=13, 
               MPID_OP_USER_NONCOMMUTE=32, MPID_OP_USER=33 }
  MPID_Op_kind;

/*S
  MPID_User_function - Definition of a user function for MPI_Op types.

  Notes:
  This includes a 'const' to make clear which is the 'in' argument and 
  which the 'inout' argument, and to indicate that the 'count' and 'datatype'
  arguments are unchanged (they are addresses in an attempt to allow 
  interoperation with Fortran).  It includes 'restrict' to emphasize that 
  no overlapping operations are allowed.

  We need to include a Fortran version, since those arguments will
  have type 'MPI_Fint *' instead.  We also need to add a test to the
  test suite for this case; in fact, we need tests for each of the handle
  types to ensure that the transfered handle works correctly.

  This is part of the collective module because user-defined operations
  are valid only for the collective computation routines and not for 
  RMA accumulate.

  Yes, the 'restrict' is in the correct location.  C compilers that 
  support 'restrict' should be able to generate code that is as good as a
  Fortran compiler would for these functions.

  We should note on the manual pages for user-defined operations that
  'restrict' should be used when available, and that a cast may be 
  required when passing such a function to 'MPI_Op_create'.

  Question:
  Should each of these function types have an associated typedef?

  Should there be a C++ function here?

  Module:
  Collective-DS
  S*/
typedef union MPID_User_function {
    void (*c_function) ( const void *, void *, 
			 const int *, const MPI_Datatype * ); 
    void (*f77_function) ( const void *, void *,
			  const MPI_Fint *, const MPI_Fint * );
} MPID_User_function;

/*S
  MPID_Op - MPI_Op structure

  Notes:
  All of the predefined functions are commutative.  Only user functions may 
  be noncummutative, so there are two separate op types for commutative and
  non-commutative user-defined operations.

  Operations do not require reference counts because there are no nonblocking
  operations that accept user-defined operations.  Thus, there is no way that
  a valid program can free an 'MPI_Op' while it is in use.

  Module:
  Collective-DS
  S*/
typedef struct MPID_Op {
     int                handle;      /* value of MPI_Op for this structure */
     volatile int       ref_count;
     MPID_Op_kind       kind;
     MPID_Lang_t        language;
     MPID_User_function function;
  } MPID_Op;
#define MPID_OP_N_BUILTIN 14
extern MPID_Op MPID_Op_builtin[MPID_OP_N_BUILTIN];
extern MPID_Op MPID_Op_direct[];
extern MPIU_Object_alloc_t MPID_Op_mem;

/* Collective operations */
typedef struct MPID_Collops {
    int ref_count;   /* Supports lazy copies */
    /* Contains pointers to the functions for the MPI collectives */
    int (*Barrier) (MPID_Comm *);
    int (*Bcast) (void*, int, MPI_Datatype, int, MPID_Comm * );
    int (*Gather) (void*, int, MPI_Datatype, void*, int, MPI_Datatype, 
                   int, MPID_Comm *); 
    int (*Gatherv) (void*, int, MPI_Datatype, void*, int *, int *, 
                    MPI_Datatype, int, MPID_Comm *); 
    int (*Scatter) (void*, int, MPI_Datatype, void*, int, MPI_Datatype, 
                    int, MPID_Comm *);
    int (*Scatterv) (void*, int *, int *, MPI_Datatype, void*, int, 
                    MPI_Datatype, int, MPID_Comm *);
    int (*Allgather) (void*, int, MPI_Datatype, void*, int, 
                      MPI_Datatype, MPID_Comm *);
    int (*Allgatherv) (void*, int, MPI_Datatype, void*, int *, int *, 
                       MPI_Datatype, MPID_Comm *);
    int (*Alltoall) (void*, int, MPI_Datatype, void*, int, MPI_Datatype, 
                               MPID_Comm *);
    int (*Alltoallv) (void*, int *, int *, MPI_Datatype, void*, int *, 
                     int *, MPI_Datatype, MPID_Comm *);
    int (*Alltoallw) (void*, int *, int *, MPI_Datatype *, void*, int *, 
                     int *, MPI_Datatype *, MPID_Comm *);
    int (*Reduce) (void*, void*, int, MPI_Datatype, MPI_Op, int, 
                   MPID_Comm *);
    int (*Allreduce) (void*, void*, int, MPI_Datatype, MPI_Op, 
                      MPID_Comm *);
    int (*Reduce_scatter) (void*, void*, int *, MPI_Datatype, MPI_Op, 
                           MPID_Comm *);
    int (*Scan) (void*, void*, int, MPI_Datatype, MPI_Op, MPID_Comm * );
    int (*Exscan) (void*, void*, int, MPI_Datatype, MPI_Op, MPID_Comm * );
    
} MPID_Collops;

#define MPIR_BARRIER_TAG 1

/* Files */
typedef struct MPID_File {
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
typedef struct MPID_Stateinfo_t {
    MPID_Time_t stamp;
    int count;
} MPID_Stateinfo_t;
#define MPICH_MAX_STATES 512

/* Thread types */
/* Temporary; this will include "mpichthread.h" eventually */

typedef struct MPICH_PerThread_t {
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
#define MPID_Common_thread_lock()
#define MPID_Common_thread_unlock()
#else
#define MPID_Comm_thread_lock() MPID_Thread_lock( &ptr->access_lock)
#define MPID_Comm_thread_unlock() MPID_Thread_unlock( &ptr->access_lock )
#define MPID_Common_thread_lock() MPID_Thread_lock( &MPIR_PerProcess.common_lock )
#define MPID_Common_thread_unlock() MPID_Thread_unlock( &MPIR_PerProcess.common_lock )
#endif

/* Per process data */
typedef enum MPIR_MPI_State_t { MPICH_PRE_INIT=0, MPICH_WITHIN_MPI=1,
               MPICH_POST_FINALIZED=2 } MPIR_MPI_State_t;

typedef struct PreDefined_attrs {
    int appnum;          /* Application number provided by mpiexec (MPI-2) */
    int host;            /* host */
    int io;              /* standard io allowed */
    int lastusedcode;    /* last used error code (MPI-2) */
    int tag_ub;          /* Maximum message tag */
    int universe;        /* Universe size from mpiexec (MPI-2) */
    int wtime_is_global; /* Wtime is global over processes in COMM_WORLD */
} PreDefined_attrs;

typedef struct MPICH_PerProcess_t {
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
    /* Attribute dup functions.  Here for lazy initialization */
    int (*comm_attr_dup)( MPID_Comm *, MPID_Attribute **new_attr );
    int (*comm_attr_free)( MPID_Comm *, MPID_Attribute *attr_p );
    int (*type_attr_dup)( MPID_Datatype *, MPID_Attribute **new_attr );
    int (*type_attr_free)( MPID_Datatype *, MPID_Attribute *attr_p );
    int (*win_attr_free)( MPID_Win *, MPID_Attribute *attr_p );
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

/* set up the timing macros */
#include "mpitimerimpl.h"

/* Error checking (see --enable-error-checking for control of this) */
#ifdef HAVE_ERROR_CHECKING

#define MPID_ERROR_LEVEL_ALL 1
#define MPID_ERROR_LEVEL_RUNTIME 2

#if HAVE_ERROR_CHECKING == MPID_ERROR_LEVEL_ALL
#define MPID_BEGIN_ERROR_CHECKS
#define MPID_END_ERROR_CHECKS
#define MPID_ELSE_ERROR_CHECKS
#elif HAVE_ERROR_CHECKING == MPID_ERROR_LEVEL_RUNTIME
#DEFINE MPID_BEGIN_ERROR_CHECKS IF (MPIR_Process.do_error_checks) {
#define MPID_ELSE_ERROR_CHECKS }else{
#define MPID_END_ERROR_CHECKS }
#else
#error "Unknown value for error checking"
#endif

#else
#define MPID_BEGIN_ERROR_CHECKS
#define MPID_END_ERROR_CHECKS
#endif /* HAVE_ERROR_CHECKING */

/* 
 *  Standardized error checking macros.  These provide the correct tests for
 *  common tests.  These set err with the encoded error value.
 */
#define MPIR_ERRTEST_INITIALIZED(err) \
  if (MPIR_Process.initialized != MPICH_WITHIN_MPI) {\
      err = MPIR_Err_create_code( MPI_ERR_OTHER, "**initialized", 0 ); }
#define MPIR_ERRTEST_SEND_TAG(tag,err) \
  if ((tag) < 0 || (tag) > MPIR_Process.attrs.tag_ub) {\
      err = MPIR_Err_create_code( MPI_ERR_TAG, "**tag", "**tag %d", tag);}
#define MPIR_ERRTEST_RECV_TAG(tag,err) \
  if ((tag) < MPI_ANY_TAG || (tag) > MPIR_Process.attrs.tag_ub) {\
      err = MPIR_Err_create_code( MPI_ERR_TAG, "**tag", "**tag %d", tag );}
#define MPIR_ERRTEST_SEND_RANK(comm_ptr,rank,err) \
  if ((rank) < MPI_PROC_NULL || (rank) >= (comm_ptr)->remote_size) {\
      err = MPIR_Err_create_code( MPI_ERR_RANK, "**rank", "**rank %d", rank );}
#define MPIR_ERRTEST_RECV_RANK(comm_ptr,rank,err) \
  if ((rank) < MPI_ANY_SOURCE || (rank) >= (comm_ptr)->remote_size) {\
      err = MPIR_Err_create_code( MPI_ERR_RANK, "**rank", "**rank %d", rank );}
#define MPIR_ERRTEST_COUNT(count,err) \
    if ((count) < 0) {\
        err = MPIR_Err_create_code( MPI_ERR_COUNT, "**countneg", "**countneg %d", count );}
#define MPIR_ERRTEST_ALIAS(ptr1,ptr2,err) \
    if ((ptr1)==(ptr2) && (ptr1) != MPI_BOTTOM) {\
        err = MPIR_Err_create_code( MPI_ERR_BUFFER, "**bufalias", 0 );}
#define MPIR_ERRTEST_ARGNULL(arg,arg_name,err) \
   if (!arg) {\
       err = MPIR_Err_create_code( MPI_ERR_ARG, "**nullptr", "**nullptr %s", arg_name ); } 
#define MPIR_ERRTEST_INTRA_ROOT(comm_ptr,root,err) \
  if ((root) <= MPI_PROC_NULL || (root) >= (comm_ptr)->local_size) {\
      err = MPIR_Err_create_code( MPI_ERR_ROOT, "**root", "**root %d", root );}


/* The following are placeholders.  We haven't decided yet whether these
   should take a handle or pointer, or if they should take a handle and return 
   a pointer if the handle is valid.  These need to be rationalized with the
   MPID_xxx_valid_ptr and MPID_xxx_get_ptr.

   [BRT] They should not take a handle and return a pointer if they will be
   placed inside of a #ifdef HAVE_ERROR_CHECKING block.  Personally, I think
   the macros should take handles.  We already have macros for validating
   pointers to various objects.
*/
#define MPIR_ERRTEST_OP(op,err)
#define MPIR_ERRTEST_GROUP(group,err)
#define MPIR_ERRTEST_COMM(comm,err)					   \
{									   \
    if (HANDLE_GET_MPI_KIND(comm) != MPID_COMM ||			   \
	(HANDLE_GET_KIND(comm) == HANDLE_KIND_INVALID &&		   \
	 comm != MPI_COMM_NULL))					   \
    {									   \
	mpi_errno = MPIR_Err_create_code( MPI_ERR_TYPE, "**comm", 0 );	   \
    }									   \
    if (comm == MPI_COMM_NULL)						   \
    {									   \
	mpi_errno = MPIR_Err_create_code( MPI_ERR_TYPE, "**commnull", 0 ); \
    }									   \
}
#define MPIR_ERRTEST_REQUEST(request,err)
#define MPIR_ERRTEST_DATATYPE(count, datatype,err)			    \
{									    \
    if (HANDLE_GET_MPI_KIND(datatype) != MPID_DATATYPE ||		    \
	(HANDLE_GET_KIND(datatype) == HANDLE_KIND_INVALID &&		    \
	 datatype != MPI_DATATYPE_NULL))				    \
    {									    \
	mpi_errno = MPIR_Err_create_code( MPI_ERR_TYPE, "**dtype", 0 );	    \
    }									    \
    if (count > 0 && datatype == MPI_DATATYPE_NULL)			    \
    {									    \
	mpi_errno = MPIR_Err_create_code( MPI_ERR_TYPE, "**dtypenull", 0 ); \
    }									    \
}
#define MPIR_ERRTEST_ERRHANDLER(errhandler,err)

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

#ifndef HAVE_DEV_COMM_HOOK
#define MPID_Dev_comm_create_hook( a )
#define MPID_Dev_comm_destroy_hook( a )
#endif


/* Bindings for internal routines */
void MPIR_Add_finalize( int (*)( void * ), void * );
int MPIR_Err_return_comm( MPID_Comm *, const char [], int );
int MPIR_Err_return_win( MPID_Win *, const char [], int );
int MPIR_Err_return_file( MPID_File *, const char [], int );
int MPIR_Err_create_code( int, const char [], ... );
#ifdef MPICH_SINGLE_THREADED
#define MPIR_Nest_incr() MPIR_Thread.nest_count++
#define MPIR_Nest_decr() MPIR_Thread.nest_count--
#define MPIR_Nest_value() MPIR_Thread.nest_count
#else
void MPIR_Nest_incr( void );
void MPIR_Nest_decr( void );
int MPIR_Nest_value( void );
#endif
void MPIR_Wait(MPID_Request *);
int MPIR_Test(MPID_Request *);

/* ADI Bindings */
int MPID_Init(int *, char ***, int, int *, int *, int *);
int MPID_Finalize(void);
int MPID_Abort( MPID_Comm *, int );

int MPID_Open_port(MPID_Info *, char *);
int MPID_Close_port(char *);
int MPID_Comm_accept(char *, MPID_Info *, int, MPID_Comm *, MPID_Comm **);
int MPID_Comm_connect(char *, MPID_Info *, int, MPID_Comm *, MPID_Comm **);
int MPID_Comm_disconnect(MPID_Comm *);
int MPID_Comm_spawn_multiple(int, char *[], char* *[], int [], MPI_Info [],
                             int, MPID_Comm *, MPID_Comm **, int []);

int MPID_Send(const void *, int, MPI_Datatype, int, int, MPID_Comm *, int,
	      MPID_Request **);
int MPID_Rsend(const void *, int, MPI_Datatype, int, int, MPID_Comm *, int,
               MPID_Request **);
int MPID_Ssend(const void *, int, MPI_Datatype, int, int, MPID_Comm *, int,
               MPID_Request **);
int MPID_Isend(const void *, int, MPI_Datatype, int, int, MPID_Comm *, int,
               MPID_Request **);
int MPID_Irsend(const void *, int, MPI_Datatype, int, int, MPID_Comm *, int,
		MPID_Request **);
int MPID_Issend(const void *, int, MPI_Datatype, int, int, MPID_Comm *, int,
		MPID_Request **);
int MPID_Recv(void *, int, MPI_Datatype, int, int, MPID_Comm *, int,
              MPI_Status *, MPID_Request **);
int MPID_Irecv(void *, int, MPI_Datatype, int, int, MPID_Comm *, int,
               MPID_Request **);

int MPID_Probe(int, int, MPID_Comm *, int, MPI_Status *);
int MPID_Iprobe(int, int, MPID_Comm *, int, int *, MPI_Status *);

void MPID_Cancel_send(MPID_Request *);
void MPID_Cancel_recv(MPID_Request *);

int MPID_Win_create(void *, MPI_Aint, int, MPID_Info *, MPID_Comm *,
                    MPID_Win **);
int MPID_Win_fence(int, MPID_Win *);
int MPID_Put(void *, int, MPI_Datatype, int, MPI_Aint, int,
            MPI_Datatype, MPID_Win *); 
int MPID_Get(void *, int, MPI_Datatype, int, MPI_Aint, int,
            MPI_Datatype, MPID_Win *);
int MPID_Win_free(MPID_Win **); 


void MPID_Progress_start(void);
void MPID_Progress_end(void);
int MPID_Progress_test(void);
void MPID_Progress_wait(void);
void MPID_Progress_poke(void);

MPID_Request * MPID_Request_create(void);
void MPID_Request_set_completed(MPID_Request *);
void MPID_Request_release(MPID_Request *);

void MPID_Errhandler_free(MPID_Errhandler *errhan_ptr);

int MPID_VCRT_create(int size, MPID_VCRT *vcrt_ptr);
int MPID_VCRT_add_ref(MPID_VCRT vcrt);
int MPID_VCRT_release(MPID_VCRT vcrt);
int MPID_VCRT_get_ptr(MPID_VCRT vcrt, MPID_VCR **vc_pptr);

int MPID_VCR_dup(MPID_VCR orig_vcr, MPID_VCR * new_vcr);
int MPID_VCR_release(MPID_VCR vcr);
int MPID_VCR_get_lpid(MPID_VCR vcr, int * lpid_ptr);

int MPID_Get_processor_name(char *, int *);


/* Include definitions from the device which require items defined by this file
   (mpiimpl.h).  NOTE: This include requires the device to copy mpidpost.h to
   the src/include directory in the build tree. */
#include "mpidpost.h"

/* thresholds to switch between long and short vector algorithms for
   collective operations */ 
#define MPIR_BCAST_SHORT_MSG          16384
#define MPIR_BCAST_MIN_PROCS          8
#define MPIR_ALLTOALL_SHORT_MSG       1024
#define MPIR_REDUCE_SCATTER_SHORT_MSG 0  /* temporarily because the short message algo needs derived datatypes */
#define MPIR_SCATTER_SHORT_MSG        2048  /* for intercommunicator scatter */
#define MPIR_GATHER_SHORT_MSG         2048  /* for intercommunicator scatter */

/* Tags for point to point operations which implement collective operations */
#define MPIR_BARRIER_TAG               1
#define MPIR_BCAST_TAG                 2
#define MPIR_GATHER_TAG                3
#define MPIR_GATHERV_TAG               4
#define MPIR_SCATTER_TAG               5
#define MPIR_SCATTERV_TAG              6
#define MPIR_ALLGATHER_TAG             7
#define MPIR_ALLGATHERV_TAG            8
#define MPIR_ALLTOALL_TAG              9
#define MPIR_ALLTOALLV_TAG            10
#define MPIR_REDUCE_TAG               11
#define MPIR_USER_REDUCE_TAG          12
#define MPIR_USER_REDUCEA_TAG         13
#define MPIR_ALLREDUCE_TAG            14
#define MPIR_USER_ALLREDUCE_TAG       15
#define MPIR_USER_ALLREDUCEA_TAG      16
#define MPIR_REDUCE_SCATTER_TAG       17
#define MPIR_USER_REDUCE_SCATTER_TAG  18
#define MPIR_USER_REDUCE_SCATTERA_TAG 19
#define MPIR_SCAN_TAG                 20
#define MPIR_USER_SCAN_TAG            21
#define MPIR_USER_SCANA_TAG           22
#define MPIR_LOCALCOPY_TAG            23
#define MPIR_EXSCAN_TAG               24
#define MPIR_ALLTOALLW_TAG            25

/* These functions are used in the implementation of collective
   operations. They are wrappers around MPID send/recv functions. They do
   sends/receives by setting the context offset to
   MPID_INTRA_CONTEXT_COLL. */
int MPIC_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag,
              MPI_Comm comm);
int MPIC_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
              MPI_Comm comm, MPI_Status *status);
int MPIC_Sendrecv(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  int dest, int sendtag, void *recvbuf, int recvcount,
                  MPI_Datatype recvtype, int source, int recvtag,
                  MPI_Comm comm, MPI_Status *status);
int MPIR_Localcopy(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int recvcount, MPI_Datatype recvtype);

void MPIR_MAXF  ( void *, void *, int *, MPI_Datatype * ) ;
void MPIR_MINF  ( void *, void *, int *, MPI_Datatype * ) ;
void MPIR_SUM  ( void *, void *, int *, MPI_Datatype * ) ;
void MPIR_PROD  ( void *, void *, int *, MPI_Datatype * ) ;
void MPIR_LAND  ( void *, void *, int *, MPI_Datatype * ) ;
void MPIR_BAND  ( void *, void *, int *, MPI_Datatype * ) ;
void MPIR_LOR  ( void *, void *, int *, MPI_Datatype * ) ;
void MPIR_BOR  ( void *, void *, int *, MPI_Datatype * ) ;
void MPIR_LXOR  ( void *, void *, int *, MPI_Datatype * ) ;
void MPIR_BXOR  ( void *, void *, int *, MPI_Datatype * ) ;
void MPIR_MAXLOC  ( void *, void *, int *, MPI_Datatype * ) ;
void MPIR_MINLOC  ( void *, void *, int *, MPI_Datatype * ) ;

#define MPIR_PREDEF_OP_COUNT 12
extern MPI_User_function *MPIR_Op_table[];

#ifndef MPIR_MIN
#define MPIR_MIN(a,b) (((a)>(b))?(b):(a))
#endif
#ifndef MPIR_MAX
#define MPIR_MAX(a,b) (((b)>(a))?(b):(a))
#endif

/* Fortran logicals */
#ifndef _CRAY
extern MPI_Fint MPIR_F_TRUE, MPIR_F_FALSE;
#define MPIR_TO_FLOG(a) ((a) ? MPIR_F_TRUE : MPIR_F_FALSE)
/* 
   Note on true and false.  This code is only an approximation.
   Some systems define either true or false, and allow some or ALL other
   patterns for the other.  This is just like C, where 0 is false and 
   anything not zero is true.  Modify this test as necessary for your
   system.
 */
#define MPIR_FROM_FLOG(a) ( (a) == MPIR_F_TRUE ? 1 : 0 )

#else
/* CRAY Vector processors only; these are defined in /usr/include/fortran.h 
   Thanks to lmc@cray.com */
#define MPIR_TO_FLOG(a) (_btol(a))
#define MPIR_FROM_FLOG(a) ( _ltob(&(a)) )    /*(a) must be a pointer */
#endif

int MPIR_Bcast (void *buffer, int count, MPI_Datatype datatype, int
                root, MPID_Comm *comm_ptr);
int MPIR_Gather (void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                 void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                 int root, MPID_Comm *comm_ptr);
int MPIR_Gatherv (void *sendbuf, int sendcnt, MPI_Datatype sendtype, 
                  void *recvbuf, int *recvcnts, int *displs,
                  MPI_Datatype recvtype, int root, MPID_Comm
                  *comm_ptr); 
int MPIR_Reduce (void *sendbuf, void *recvbuf, int count, MPI_Datatype
                 datatype, MPI_Op op, int root, MPID_Comm *comm_ptr); 
int MPIR_Scatterv (void *sendbuf, int *sendcnts, int *displs,
                   MPI_Datatype sendtype, void *recvbuf, int recvcnt,
                   MPI_Datatype recvtype, int root, MPID_Comm
                   *comm_ptr );

#endif /* MPIIMPL_INCLUDED */
