/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIIMPL_INCLUDED
#define MPIIMPL_INCLUDED

/*
 * This file is the temporary home of most of the definitions used to 
 * implement MPICH.  We will eventually divide this file into logical
 * pieces once we are certain of the relationships between the components.
 */

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

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#else
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
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

/* 
   If we do not have the GCC attribute, then make this empty.  We use
   the GCC attribute to improve error checking by the compiler, particularly 
   for printf/sprintf strings 
*/
#ifdef HAVE_GCC_ATTRIBUTE
#define ATTRIBUTE(a) __attribute__(a)
#else
#define ATTRIBUTE(a)
#endif


/*
 * Basic utility macros
 */
#define MPIU_QUOTE(A) MPIU_QUOTE2(A)
#define MPIU_QUOTE2(A) #A


/* 
   Include the implementation definitions (e.g., error reporting, thread
   portability)
   More detailed documentation is contained in the MPICH2 and ADI3 manuals.
 */
/* ... to do ... */
#include "mpitypedefs.h"

#include "mpiimplthread.h"
#include "mpiatomic.h"

#include "mpiutil.h"

/* Include definitions from the device which must exist before items in this
   file (mpiimpl.h) can be defined. */
#include "mpidpre.h"


/* ------------------------------------------------------------------------- */
/* mpidebug.h */
/* ------------------------------------------------------------------------- */
/* Debugging and printf control */
/* Use these *only* for debugging output intended for the implementors
   and maintainers of MPICH.  Do *not* use these for any output that
   general users may normally see.  Use either the error code creation
   routines for error messages or MPIU_msg_printf etc. for general messages 
   (MPIU_msg_printf will go through gettext).  

   FIXME: Document all of these macros
*/
typedef enum MPIU_dbg_state_t
{
    MPIU_DBG_STATE_NONE = 0,
    MPIU_DBG_STATE_UNINIT = 1,
    MPIU_DBG_STATE_STDOUT = 2,
    MPIU_DBG_STATE_MEMLOG = 4,
    MPIU_DBG_STATE_FILE = 8
}
MPIU_dbg_state_t;
int MPIU_dbg_init(int rank);
int MPIU_dbg_printf(char *str, ...) ATTRIBUTE((format(printf,1,2)));
int MPIU_dbglog_printf(char *str, ...) ATTRIBUTE((format(printf,1,2)));
int MPIU_dbglog_vprintf(char *str, va_list ap);
#if defined(MPICH_DBG_OUTPUT)
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
extern MPIU_dbg_state_t MPIUI_dbg_state;
extern FILE * MPIUI_dbg_fp;
#define MPIU_dbglog_flush()				\
{							\
    if (MPIUI_dbg_state & MPIU_DBG_STATE_STDOUT)	\
    {							\
	fflush(stdout);					\
    }							\
}
void MPIU_dump_dbg_memlog_to_stdout(void);
void MPIU_dump_dbg_memlog_to_file(char *filename);
void MPIU_dump_dbg_memlog(FILE * fp);
/* The follow is temporarily provided for backward compatibility.  Any code
   using dbg_printf should be updated to use MPIU_DBG_PRINTF. */
#define dbg_printf MPIU_dbg_printf

/* For unconditional debug output, use the following */
#define DBG_PRINTF printf
#define DBG_FPRINTF fprintf

/* Define MPICH_DEBUG_INTERNAL to compile in optional debugging
   tests and routines.  These are enabled with the appropriate 
   value for MPICH_DEBUG_ITEM, an environment variable that contains
   a string.  This string is matched in the routine
   MPIR_IDebug .

   MPICH_DEBUG_INTERNAL is selected with a suitably high level choice of the
   --enable-g option to configure.
 */
#ifdef MPICH_DEBUG_INTERNAL
extern int MPIR_IDebug( const char * );
#endif

/* ------------------------------------------------------------------------- */
/* end of mpidebug.h */
/* ------------------------------------------------------------------------- */

/* Provisional routine (src/util/dbg/dbg_control.c) to test that the
   environment value of "MPICH_DEBUG_ITEM" matches the input argument */
int MPIR_IDebug( const char * );

/* Routines for memory management */
#include "mpimem.h"

/* ------------------------------------------------------------------------- */
/* mpimsg.h */
/* ------------------------------------------------------------------------- */
/* Message printing */
int MPIU_Usage_printf( char *str, ... ) ATTRIBUTE((format(printf,1,2)));
int MPIU_Msg_printf( char *str, ... ) ATTRIBUTE((format(printf,1,2)));
int MPIU_Error_printf( char *str, ... ) ATTRIBUTE((format(printf,1,2)));
int MPIU_Internal_error_printf( char *str, ... ) ATTRIBUTE((format(printf,1,2)));
int MPIU_Internal_sys_error_printf( const char *, int, const char *str, ... ) ATTRIBUTE((format(printf,3,4)));

/* ------------------------------------------------------------------------- */
/* end of mpimsg.h */
/* ------------------------------------------------------------------------- */

/*TDSOverview.tex
  
  MPI has a number of data structures, most of which are represented by 
  an opaque handle in an MPI program.  In the MPICH implementation of MPI, 
  these handles are represented
  as integers; this makes implementation of the C/Fortran handle transfer 
  calls (part of MPI-2) easy.  
 
  MPID objects (again with the possible exception of 'MPI_Request's) 
  are allocated by a common set of object allocation functions.
  These are 
.vb
    void *MPIU_Handle_obj_create( MPIU_Object_alloc_t *objmem )
    void MPIU_Handle_obj_destroy( MPIU_Object_alloc_t *objmem, void *object )
.ve
  where 'objmem' is a pointer to a memory allocation object that knows 
  enough to allocate objects, including the
  size of the object and the location of preallocated memory, as well 
  as the type of memory allocator.  By providing the routines to allocate and
  free the memory, we make it easy to use the same interface to allocate both
  local and shared memory for objects (always using the same kind for each 
  type of object).

  The names create/destroy were chosen because they are different from 
  new/delete (C++ operations) and malloc/free.  
  Any name choice will have some conflicts with other uses, of course.

  Reference Counts:
  Many MPI objects have reference count semantics.  
  The semantics of MPI require that many objects that have been freed by the 
  user 
  (e.g., with 'MPI_Type_free' or 'MPI_Comm_free') remain valid until all 
  pending
  references to that object (e.g., by an 'MPI_Irecv') are complete.  There
  are several ways to implement this; MPICH uses `reference counts` in the
  objects.  To support the 'MPI_THREAD_MULTIPLE' level of thread-safety, these
  reference counts must be accessed and updated atomically.  
  A reference count for
  `any` object can be incremented (atomically) 
  with 'MPIU_Object_add_ref(objptr)'
  and decremented with 'MPIU_Object_release_ref(objptr,newval_ptr)'.  
  These have been designed so that then can be implemented as inlined 
  macros rather than function calls, even in the multithreaded case, and
  can use special processor instructions that guarantee atomicity to 
  avoid thread locks.
  The decrement routine sets the value pointed at by 'inuse_ptr' to 0 if 
  the postdecrement value of the reference counter is zero, and to a non-zero
  value otherwise.  If this value is zero, then the routine that decremented 
  the
  reference count should free the object.  This may be as simple as 
  calling 'MPIU_Handle_obj_destroy' (for simple objects with no other allocated
  storage) or may require calling a separate routine to destroy the object.
  Because MPI uses 'MPI_xxx_free' to both decrement the reference count and 
  free the object if the reference count is zero, we avoid the use of 'free'
  in the MPID routines.

  The 'inuse_ptr' approach is used rather than requiring the post-decrement
  value because, for reference-count semantics, all that is necessary is
  to know when the reference count reaches zero, and this can sometimes
  be implemented more cheaply that requiring the post-decrement value (e.g.,
  on IA32, there is an instruction for this operation).

  Question:
  Should we state that this is a macro so that we can use a register for
  the output value?  That avoids a store.  Alternately, have the macro 
  return the value as if it was a function?

  Structure Definitions:
  The structure definitions in this document define `only` that part of
  a structure that may be used by code that is making use of the ADI.
  Thus, some structures, such as 'MPID_Comm', have many defined fields;
  these are used to support MPI routines such as 'MPI_Comm_size' and
  'MPI_Comm_remote_group'.  Other structures may have few or no defined
  members; these structures have no fields used outside of the ADI.  
  In C++ terms,  all members of these structures are 'private'.  

  For the initial implementation, we expect that the structure definitions 
  will be designed for the multimethod device.  However, all items that are
  specific to a particular device (including the multi-method device) 
  will be placed at the end of the structure;
  the document will clearly identify the members that all implementations
  will provide.  This simplifies much of the code in both the ADI and the 
  implementation of the MPI routines because structure member can be directly
  accessed rather than using some macro or C++ style method interface.
  
 T*/

/* Known language bindings */
/*E
  MPID_Lang_t - Known language bindings for MPI

  A few operations in MPI need to know what language they were called from
  or created by.  This type enumerates the possible languages so that
  the MPI implementation can choose the correct behavior.  An example of this
  are the keyval attribute copy and delete functions.

  Module:
  Attribute-DS
  E*/
typedef enum MPID_Lang_t { MPID_LANG_C 
#ifdef HAVE_FORTRAN_BINDING
			   , MPID_LANG_FORTRAN
			   , MPID_LANG_FORTRAN90
#endif
#ifdef HAVE_CXX_BINDING
			   , MPID_LANG_CXX
#endif
} MPID_Lang_t;

/* Macros for the MPI handles (e.g., the object that encodes an
   MPI_Datatype) */
#include "mpihandlemem.h"

/* ------------------------------------------------------------------------- */
/* mpiobjref.h */
/* ------------------------------------------------------------------------- */
/* This isn't quite right, since we need to distinguish between multiple 
   user threads and multiple implementation threads.
 */
#define MPICH_DEBUG_MAX_REFCOUNT 64

#ifdef MPICH_SINGLE_THREADED
#ifdef MPICH_DEBUG_HANDLES
#define MPIU_Object_set_ref(objptr,val)												\
{																\
    if (1) {															\
        MPIU_DBG_PRINTF(("set %p (0x%08x) refcount to %d in %s:%d\n", (objptr), (objptr)->handle, val, __FILE__, __LINE__));	\
    }																\
    ((MPIU_Handle_head*)(objptr))->ref_count = val;										\
}

/*M
   MPIU_Object_add_ref - Increment the reference count for an MPI object

   Synopsis:
.vb
    MPIU_Object_add_ref( MPIU_Object *ptr )
.ve   

   Input Parameter:
.  ptr - Pointer to the object.

   Notes:
   In an unthreaded implementation, this function will usually be implemented
   as a single-statement macro.  In an 'MPI_THREAD_MULTIPLE' implementation,
   this routine must implement an atomic increment operation, using, for 
   example, a lock on datatypes or special assembly code such as 
.vb
   try-again:
      load-link          refcount-address to r2
      add                1 to r2
      store-conditional  r2 to refcount-address
      if failed branch to try-again:
.ve
   on RISC architectures or
.vb
   lock
   inc                   refcount-address or
.ve
   on IA32; "lock" is a special opcode prefix that forces atomicity.  This 
   is not a separate instruction; however, the GNU assembler expects opcode
   prefixes on a separate line.

   Module: 
   MPID_CORE

   Question:
   This accesses the 'ref_count' member of all MPID objects.  Currently,
   that member is typed as 'volatile int'.  However, for a purely polling,
   thread-funnelled application, the 'volatile' is unnecessary.  Should
   MPID objects use a 'typedef' for the 'ref_count' that can be defined
   as 'volatile' only when needed?  For now, the answer is no; there isn''t
   enough to be gained in that case.
M*/
#define MPIU_Object_add_ref(objptr)												\
{																\
    ((MPIU_Handle_head*)(objptr))->ref_count++;											\
    if (1) {															\
	MPIU_DBG_PRINTF(("incr %p (0x%08x) refcount in %s:%d, count=%d\n",							\
			 (objptr), (objptr)->handle, __FILE__, __LINE__, (objptr)->ref_count));					\
    }																\
    if (((MPIU_Handle_head*)(objptr))->ref_count > MPICH_DEBUG_MAX_REFCOUNT){							\
        MPIU_DBG_PRINTF(("Invalid refcount in %p (0x%08x) incr at %s:%d\n", (objptr), (objptr)->handle, __FILE__, __LINE__));	\
    }																\
}

/*M
   MPIU_Object_release_ref - Decrement the reference count for an MPI object

   Synopsis:
.vb
   MPIU_Object_release_ref( MPIU_Object *ptr, int *inuse_ptr )
.ve

   Input Parameter:
.  objptr - Pointer to the object.

   Output Parameter:
.  inuse_ptr - Pointer to the value of the reference count after decrementing.
   This value is either zero or non-zero. See below for details.
   
   Notes:
   In an unthreaded implementation, this function will usually be implemented
   as a single-statement macro.  In an 'MPI_THREAD_MULTIPLE' implementation,
   this routine must implement an atomic decrement operation, using, for 
   example, a lock on datatypes or special assembly code such as 
.vb
   try-again:
      load-link          refcount-address to r2
      sub                1 to r2
      store-conditional  r2 to refcount-address
      if failed branch to try-again:
      store              r2 to newval_ptr
.ve
   on RISC architectures or
.vb
      lock
      dec                   refcount-address 
      if zf store 0 to newval_ptr else store 1 to newval_ptr
.ve
   on IA32; "lock" is a special opcode prefix that forces atomicity.  This 
   is not a separate instruction; however, the GNU assembler expects opcode
   prefixes on a separate line.  'zf' is the zero flag; this is set if the
   result of the operation is zero.  Implementing a full decrement-and-fetch
   would require more code and the compare and swap instruction.

   Once the reference count is decremented to zero, it is an error to 
   change it.  A correct MPI program will never do that, but an incorrect one 
   (particularly a multithreaded program with a race condition) might.  

   The following code is `invalid`\:
.vb
   MPID_Object_release_ref( datatype_ptr );
   if (datatype_ptr->ref_count == 0) MPID_Datatype_free( datatype_ptr );
.ve
   In a multi-threaded implementation, the value of 'datatype_ptr->ref_count'
   may have been changed by another thread, resulting in both threads calling
   'MPID_Datatype_free'.  Instead, use
.vb
   MPID_Object_release_ref( datatype_ptr, &inUse );
   if (!inuse) 
       MPID_Datatype_free( datatype_ptr );
.ve

   Module: 
   MPID_CORE
  M*/
#define MPIU_Object_release_ref(objptr,inuse_ptr)										\
{																\
    *(inuse_ptr)=--((MPIU_Handle_head*)(objptr))->ref_count;									\
    if (1) {															\
	MPIU_DBG_PRINTF(("decr %p (0x%08x) refcount in %s:%d, count=%d\n",							\
			 (objptr), (objptr)->handle, __FILE__, __LINE__, (objptr)->ref_count));					\
    }																\
    if (((MPIU_Handle_head*)(objptr))->ref_count > MPICH_DEBUG_MAX_REFCOUNT){							\
        MPIU_DBG_PRINTF(("Invalid refcount in %p (0x%08x) decr at %s:%d\n", (objptr), (objptr)->handle, __FILE__, __LINE__));	\
    }																\
}
#else
#define MPIU_Object_set_ref(objptr,val) \
    {((MPIU_Handle_head*)(objptr))->ref_count = val;}
#define MPIU_Object_add_ref(objptr) \
    {((MPIU_Handle_head*)(objptr))->ref_count++;}
#define MPIU_Object_release_ref(objptr,inuse_ptr) \
    {*(inuse_ptr)=--((MPIU_Handle_head*)(objptr))->ref_count;}
#endif
#else
#if defined(USE_ATOMIC_UPDATES)
#define MPIU_Object_set_ref(objptr,val) \
    {((MPIU_Handle_head*)(objptr))->ref_count = val;}
#define MPIU_Object_add_ref(objptr) \
    {MPID_Atomic_incr(&((objptr)->ref_count));}
#define MPIU_Object_release_ref(objptr,inuse_ptr)		\
{								\
    int nzflag__;						\
    MPID_Atomic_decr_flag(&((objptr)->ref_count),nzflag__);	\
    *inuse_ptr = nzflag__;					\
}
#else
#define MPIU_Object_set_ref(objptr,val) \
    {((MPIU_Handle_head*)(objptr))->ref_count = val;}
#define MPIU_Object_add_ref(objptr)		\
{						\
    MPID_Common_thread_lock();			\
    (objptr)->ref_count++;			\
    MPID_Common_thread_unlock();		\
}
#define MPIU_Object_release_ref(objptr,inuse_ptr)	\
{							\
    MPID_Common_thread_lock();				\
    *(inuse_ptr)=--(objptr)->ref_count;			\
    MPID_Common_thread_unlock();			\
}
#endif
#endif

/* ------------------------------------------------------------------------- */
/* mpiobjref.h */
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
/* Should the following be moved into mpihandlemem.h ?*/
/* ------------------------------------------------------------------------- */

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

/* FIXME: the masks should be defined with the handle definitions instead
   of inserted here as literals */
#define MPID_Comm_get_ptr(a,ptr)       MPID_Getb_ptr(Comm,a,0x03ffffff,ptr)
#define MPID_Group_get_ptr(a,ptr)      MPID_Getb_ptr(Group,a,0x03ffffff,ptr)
#define MPID_File_get_ptr(a,ptr)       MPID_Get_ptr(File,a,ptr)
#define MPID_Errhandler_get_ptr(a,ptr) MPID_Getb_ptr(Errhandler,a,0x3,ptr)
#define MPID_Op_get_ptr(a,ptr)         MPID_Getb_ptr(Op,a,0x000000ff,ptr)
#define MPID_Info_get_ptr(a,ptr)       MPID_Get_ptr(Info,a,ptr)
#define MPID_Win_get_ptr(a,ptr)        MPID_Get_ptr(Win,a,ptr)
#define MPID_Request_get_ptr(a,ptr)    MPID_Get_ptr(Request,a,ptr)
/* Keyvals have a special format. This is roughly MPID_Get_ptrb, but
   the handle index is in a smaller bit field.  In addition, 
   there is no storage for the builtin keyvals.  
   For the indirect case, we mask off the part of the keyval that is
   in the bits normally used for the indirect block index.
*/
#define MPID_Keyval_get_ptr(a,ptr)     \
{                                                                       \
   switch (HANDLE_GET_KIND(a)) {                                        \
      case HANDLE_KIND_BUILTIN:                                         \
          ptr=0;                                                        \
          break;                                                        \
      case HANDLE_KIND_DIRECT:                                          \
          ptr=MPID_Keyval_direct+((a)&0x3fffff);                        \
          break;                                                        \
      case HANDLE_KIND_INDIRECT:                                        \
          ptr=((MPID_Keyval*)                                           \
             MPIU_Handle_get_ptr_indirect((a)&0xfc3fffff,&MPID_Keyval_mem)); \
          break;                                                        \
      case HANDLE_KIND_INVALID:                                         \
      default:								\
          ptr=0;							\
          break;							\
    }                                                                   \
}

/* Valid pointer checks */
/* This test is lame.  Should eventually include cookie test 
   and in-range addresses */
#define MPID_Valid_ptr(kind,ptr,err) \
  {if (!(ptr)) { err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, \
                                             "**nullptrtype", "**nullptrtype %s", #kind ); } }
#define MPID_Valid_ptr_class(kind,ptr,errclass,err) \
  {if (!(ptr)) { err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, errclass, \
                                             "**nullptrtype", "**nullptrtype %s", #kind ); } }

#define MPID_Info_valid_ptr(ptr,err) MPID_Valid_ptr_class(Info,ptr,MPI_ERR_INFO,err)
/* Check not only for a null pointer but for an invalid communicator,
   such as one that has been freed.  Let's try the ref_count as the test
   for now */
#define MPID_Comm_valid_ptr(ptr,err) {                      \
     MPID_Valid_ptr_class(Comm,ptr,MPI_ERR_COMM,err);       \
     if ((ptr) && (ptr)->ref_count == 0) {                      \
        err = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_COMM,"**comm", 0);ptr=0;}}
#define MPID_Group_valid_ptr(ptr,err) MPID_Valid_ptr_class(Group,ptr,MPI_ERR_GROUP,err)
#define MPID_Win_valid_ptr(ptr,err) MPID_Valid_ptr_class(Win,ptr,MPI_ERR_WIN,err)
#define MPID_Op_valid_ptr(ptr,err) MPID_Valid_ptr_class(Op,ptr,MPI_ERR_OP,err)
#define MPID_Errhandler_valid_ptr(ptr,err) MPID_Valid_ptr_class(Errhandler,ptr,MPI_ERR_ARG,err)
#define MPID_File_valid_ptr(ptr,err) MPID_Valid_ptr_class(File,ptr,MPI_ERR_FILE,err)
#define MPID_Request_valid_ptr(ptr,err) MPID_Valid_ptr_class(Request,ptr,MPI_ERR_REQUEST,err)
#define MPID_Keyval_valid_ptr(ptr,err) MPID_Valid_ptr_class(Keyval,ptr,MPI_ERR_KEYVAL,err)

/* FIXME: 
   Generic pointer test.  This is applied to any address, not just one from
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
/* Fixme: The following MPID_ALIGNED_xxx values are temporary.  They 
   need to be computed by configure and included in the mpichconf.h file.
   Note that they cannot be set conservatively (i.e., as sizeof(object)),
   since the runtime system may generate objects with lesser alignment
   rules if the processor allows them.
 */
#define MPID_ALIGNED_PTR_INT   1
#define MPID_ALIGNED_PTR_LONG  1
#define MPID_ALIGNED_PTR_VOIDP 1
/* ------------------------------------------------------------------------- */
/* end of code that should the following be moved into mpihandlemem.h ?*/
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
/* mpiparam.h*/
/* ------------------------------------------------------------------------- */

/* Parameter handling.  These functions have not been implemented yet.
   See src/util/param.[ch] */
typedef enum MPIU_Param_result_t { 
    MPIU_PARAM_FOUND = 0, 
    MPIU_PARAM_OK = 1, 
    MPIU_PARAM_ERROR = 2 
} MPIU_Param_result_t;
int MPIU_Param_init( int *, char *[], const char [] );
int MPIU_Param_bcast( void );
int MPIU_Param_register( const char [], const char [], const char [] );
int MPIU_Param_get_int( const char [], int, int * );
int MPIU_Param_get_string( const char [], const char *, char ** );
void MPIU_Param_finalize( void );
/* ------------------------------------------------------------------------- */
/* end of mpiparam.h*/
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
/* Info */
/*TInfoOverview.tex

  'MPI_Info' provides a way to create a list of '(key,value)' pairs
  where the 'key' and 'value' are both strings.  Because many routines, both
  in the MPI implementation and in related APIs such as the PMI process
  management interface, require 'MPI_Info' arguments, we define a simple 
  structure for each 'MPI_Info' element.  Elements are allocated by the 
  generic object allocator; the head element is always empty (no 'key'
  or 'value' is defined on the head element).  
  
  For simplicity, we have not abstracted the info data structures;
  routines that want to work with the linked list may do so directly.
  Because the 'MPI_Info' type is a handle and not a pointer, an MPIU
  (utility) routine is provided to handle the 
  deallocation of 'MPID_Info' elements.  See the implementation of
  'MPI_Info_create' for how an Info type is allocated.

  Thread Safety:

  The info interface itself is not thread-robust.  In particular, the routines
  'MPI_INFO_GET_NKEYS' and 'MPI_INFO_GET_NTHKEY' assume that no other 
  thread modifies the info key.  (If the info routines had the concept
  of a next value, they would not be thread safe.  As it stands, a user
  must be careful if several threads have access to the same info object.) 
  Further, 'MPI_INFO_DUP', while not 
  explicitly advising implementers to be careful of one thread modifying the
  'MPI_Info' structure while 'MPI_INFO_DUP' is copying it, requires that the
  operation take place in a thread-safe manner.
  There isn'' much that we can do about these cases.  There are other cases
  that must be handled.  In particular, multiple threads are allowed to 
  update the same info value.  Thus, all of the update routines must be thread
  safe; the simple implementation used in the MPICH implementation uses locks.
  Note that the 'MPI_Info_delete' call does not need a lock; the defintion of
  thread-safety means that any order of the calls functions correctly; since
  it invalid either to delete the same 'MPI_Info' twice or to modify an
  'MPI_Info' that has been deleted, only one thread at a time can call 
  'MPI_Info_free' on any particular 'MPI_Info' value.  

  T*/
/*S
  MPID_Info - Structure of an MPID info

  Notes:
  There is no reference count because 'MPI_Info' values, unlike other MPI 
  objects, may be changed after they are passed to a routine without 
  changing the routine''s behavior.  In other words, any routine that uses
  an 'MPI_Info' object must make a copy or otherwise act on any info value
  that it needs.

  A linked list is used because the typical 'MPI_Info' list will be short
  and a simple linked list is easy to implement and to maintain.  Similarly,
  a single structure rather than separate header and element structures are
  defined for simplicity.  No separate thread lock is provided because
  info routines are not performance critical; they use the 'common_lock' 
  in the 'MPIR_Process' structure when they need a thread lock.
  
  This particular form of linked list (in particular, with this particular
  choice of the first two members) is used because it allows us to use 
  the same routines to manage this list as are used to manage the 
  list of free objects (in the file 'src/util/mem/handlemem.c').  In 
  particular, if lock-free routines for updating a linked list are 
  provided, they can be used for managing the 'MPID_Info' structure as well.

  The MPI standard requires that keys can be no less that 32 characters and
  no more than 255 characters.  There is no mandated limit on the size 
  of values.

  Module:
  Info-DS
  S*/
typedef struct MPID_Info {
    int                handle;
    volatile int       ref_count;  /* FIXME: ref_count isn't needed by Info objects, but MPIU_Info_free does not work correctly
				      unless MPID_Info and MPIU_Handle_common have the next pointer in the same location */
    struct MPID_Info   *next;
    char               *key;
    char               *value;
} MPID_Info;
extern MPIU_Object_alloc_t MPID_Info_mem;
/* Preallocated info objects */
extern MPID_Info MPID_Info_direct[];
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
/* Error Handlers */
/*E
  MPID_Errhandler_fn - MPID Structure to hold an error handler function

  Notes:
  The MPI-1 Standard declared only the C version of this, implicitly 
  assuming that 'int' and 'MPI_Fint' were the same. 

  Module:
  ErrHand-DS
  
  Questions:
  What do we want to do about C++?  Do we want a hook for a routine that can
  be called to throw an exception in C++, particularly if we give C++ access
  to this structure?  Does the C++ handler need to be different (not part
  of the union)?

  What is the interface for the Fortran version of the error handler?  
  E*/
typedef union MPID_Errhandler_fn {
   void (*C_Comm_Handler_function) ( MPI_Comm *, int *, ... );
   void (*F77_Handler_function) ( MPI_Fint *, MPI_Fint *, ... );
   void (*C_Win_Handler_function) ( MPI_Win *, int *, ... );
   void (*C_File_Handler_function) ( MPI_File *, int *, ... );
} MPID_Errhandler_fn;

/*S
  MPID_Errhandler - Description of the error handler structure

  Notes:
  Device-specific information may indicate whether the error handler is active;
  this can help prevent infinite recursion in error handlers caused by 
  user-error without requiring the user to be as careful.  We might want to 
  make this part of the interface so that the 'MPI_xxx_call_errhandler' 
  routines would check.

  It is useful to have a way to indicate that the errhandler is no longer
  valid, to help catch the case where the user has freed the errhandler but
  is still using a copy of the 'MPI_Errhandler' value.  We may want to 
  define the 'id' value for deleted errhandlers.

  Module:
  ErrHand-DS
  S*/
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
extern MPID_Errhandler MPID_Errhandler_builtin[];
extern MPID_Errhandler MPID_Errhandler_direct[];
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
/* Keyvals and attributes */
/*TKyOverview.tex

  Keyvals are MPI objects that, unlike most MPI objects, are defined to be
  integers rather than a handle (e.g., 'MPI_Comm').  However, they really
  `are` MPI opaque objects and are handled by the MPICH implementation in
  the same way as all other MPI opaque objects.  The only difference is that
  there is no 'typedef int MPI_Keyval;' in 'mpi.h'.  In particular, keyvals
  are encoded (for direct and indirect references) in the same way that 
  other MPI opaque objects are

  Each keyval has a copy and a delete function associated with it.
  Unfortunately, these have a slightly different calling sequence for
  each language, particularly when the size of a pointer is 
  different from the size of a Fortran integer.  The unions 
  'MPID_Copy_function' and 'MPID_Delete_function' capture the differences
  in a single union type.

  Notes:
  One potential user error is to access an attribute in one language (say
  Fortran) that was created in another (say C).  We could add a check and
  generate an error message in this case; note that this would have to 
  be an option, because (particularly when accessing the attribute from C), 
  it may be what the user intended, and in any case, it is a valid operation.

  T*/
/*TAttrOverview.tex
 *
 * The MPI standard allows `attributes`, essentially an '(integer,pointer)'
 * pair, to be attached to communicators, windows, and datatypes.  
 * The integer is a `keyval`, which is allocated by a call (at the MPI level)
 * to 'MPI_Comm/Type/Win_create_keyval'.  The pointer is the value of 
 * the attribute.
 * Attributes are primarily intended for use by the user, for example, to save
 * information on a communicator, but can also be used to pass data to the
 * MPI implementation.  For example, an attribute may be used to pass 
 * Quality of Service information to an implementation to be used with 
 * communication on a particular communicator.  
 * To provide the most general access by the ADI to all attributes, the
 * ADI defines a collection of routines that are used by the implementation
 * of the MPI attribute routines (such as 'MPI_Comm_get_attr').
 * In addition, the MPI routines involving attributes will invoke the 
 * corresponding 'hook' functions (e.g., 'MPID_Dev_comm_attr_set_hook') 
 * should the device define them.
 *
 * Attributes on windows and datatypes are defined by MPI but not of 
 * interest (as yet) to the device.
 *
 * In addition, there are seven predefined attributes that the device must
 * supply to the implementation.  This is accomplished through 
 * data values that are part of the 'MPIR_Process' data block.
 *  The predefined keyvals on 'MPI_COMM_WORLD' are\:
 *.vb
 * Keyval                     Related Module
 * MPI_APPNUM                 Dynamic
 * MPI_HOST                   Core
 * MPI_IO                     Core
 * MPI_LASTUSEDCODE           Error
 * MPI_TAG_UB                 Communication
 * MPI_UNIVERSE_SIZE          Dynamic
 * MPI_WTIME_IS_GLOBAL        Timer
 *.ve
 * The values stored in the 'MPIR_Process' block are the actual values.  For 
 * example, the value of 'MPI_TAG_UB' is the integer value of the largest tag.
 * The
 * value of 'MPI_WTIME_IS_GLOBAL' is a '1' for true and '0' for false.  Likely
 * values for 'MPI_IO' and 'MPI_HOST' are 'MPI_ANY_SOURCE' and 'MPI_PROC_NULL'
 * respectively.
 *
 T*/

/* Because Comm, Datatype, and File handles are all ints, and because
   attributes are otherwise identical between the three types, we
   only store generic copy and delete functions.  This allows us to use
   common code for the attribute set, delete, and dup functions */
/*E
  MPID_Copy_function - MPID Structure to hold an attribute copy function

  Notes:
  The appropriate element of this union is selected by using the language
  field of the 'keyval'.

  Because 'MPI_Comm', 'MPI_Win', and 'MPI_Datatype' are all 'int's in 
  MPICH2, we use a single C copy function rather than have separate
  ones for the Communicator, Window, and Datatype attributes.

  There are no corresponding typedefs for the Fortran functions.  The 
  F77 function corresponds to the Fortran 77 binding used in MPI-1 and the
  F90 function corresponds to the Fortran 90 binding used in MPI-2.

  Module:
  Attribute-DS

  E*/
typedef union MPID_Copy_function {
  int  (*C_CopyFunction)( int, int, void *, void *, void *, int * );
  void (*F77_CopyFunction)  ( MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *, 
                              MPI_Fint *, MPI_Fint *, MPI_Fint * );
  void (*F90_CopyFunction)  ( MPI_Fint *, MPI_Fint *, MPI_Aint *, MPI_Aint *,
                              MPI_Aint *, MPI_Fint *, MPI_Fint * );
  /* The C++ function is the same as the C function */
} MPID_Copy_function;

/*E
  MPID_Delete_function - MPID Structure to hold an attribute delete function

  Notes:
  The appropriate element of this union is selected by using the language
  field of the 'keyval'.

  Because 'MPI_Comm', 'MPI_Win', and 'MPI_Datatype' are all 'int's in 
  MPICH2, we use a single C delete function rather than have separate
  ones for the Communicator, Window, and Datatype attributes.

  There are no corresponding typedefs for the Fortran functions.  The 
  F77 function corresponds to the Fortran 77 binding used in MPI-1 and the
  F90 function corresponds to the Fortran 90 binding used in MPI-2.

  Module:
  Attribute-DS

  E*/
typedef union MPID_Delete_function {
  int  (*C_DeleteFunction)  ( int, int, void *, void * );
  void (*F77_DeleteFunction)( MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *, 
                              MPI_Fint * );
  void (*F90_DeleteFunction)( MPI_Fint *, MPI_Fint *, MPI_Aint *, MPI_Aint *, 
                              MPI_Fint * );
} MPID_Delete_function;

/*S
  MPID_Keyval - Structure of an MPID keyval

  Module:
  Attribute-DS

  S*/
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
/*S
  MPID_Attribute - Structure of an MPID attribute

  Notes:
  Attributes don''t have 'ref_count's because they don''t have reference
  count semantics.  That is, there are no shallow copies or duplicates
  of an attibute.  An attribute is copied when the communicator that
  it is attached to is duplicated.  Subsequent operations, such as
  'MPI_Comm_attr_free', can change the attribute list for one of the
  communicators but not the other, making it impractical to keep the
  same list.  (We could defer making the copy until the list is changed,
  but even then, there would be no reference count on the individual
  attributes.)
 
  A pointer to the keyval, rather than the (integer) keyval itself is
  used since there is no need within the attribute structure to make
  it any harder to find the keyval structure.

  The attribute value is a 'void *'.  If 'sizeof(MPI_Fint)' > 'sizeof(void*)',
  then this must be changed (no such system has been encountered yet).
  For the Fortran 77 routines in the case where 'sizeof(MPI_Fint)' < 
  'sizeof(void*)', the high end of the 'void *' value is used.  That is,
  we cast it to 'MPI_Fint *' and use that value.
 
  Module:
  Attribute-DS

 S*/
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
/* ------------------------------------------------------------------------- */

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

/* Any changes in the MPID_Group structure must be made to the
   predefined value in MPID_Group_builtin for MPI_GROUP_EMPTY in 
   src/mpi/group/grouputil.c */
/*S
 MPID_Group - Description of the Group data structure

 The processes in the group of 'MPI_COMM_WORLD' have lpid values 0 to 'size'-1,
 where 'size' is the size of 'MPI_COMM_WORLD'.  Processes created by 
 'MPI_Comm_spawn' or 'MPI_Comm_spawn_multiple' or added by 'MPI_Comm_attach' 
 or  
 'MPI_Comm_connect'
 are numbered greater than 'size - 1' (on the calling process). See the 
 discussion of LocalPID values.

 Note that when dynamic process creation is used, the pids are `not` unique
 across the universe of connected MPI processes.  This is ok, as long as
 pids are interpreted `only` on the process that owns them.

 Only for MPI-1 are the lpid''s equal to the `global` pids.  The local pids
 can be thought of as a reference not to the remote process itself, but
 how the remote process can be reached from this process.  We may want to 
 have a structure 'MPID_Lpid_t' that contains information on the remote
 process, such as (for TCP) the hostname, ip address (it may be different if
 multiple interfaces are supported; we may even want plural ip addresses for
 stripping communication), and port (or ports).  For shared memory connected
 processes, it might have the address of a remote queue.  The lpid number 
 is an index into a table of 'MPID_Lpid_t'''s that contain this (device- and
 method-specific) information.

 Module:
 Group-DS

 S*/
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
/* ------------------------------------------------------------------------- */

typedef struct MPIDI_VCRT * MPID_VCRT;
typedef struct MPIDI_VC   * MPID_VCR;

/*E
  MPID_Comm_kind_t - Name the two types of communicators
  E*/
typedef enum MPID_Comm_kind_t { 
    MPID_INTRACOMM = 0, 
    MPID_INTERCOMM = 1 } MPID_Comm_kind_t;
/* Communicators */

/*S
  MPID_Comm - Description of the Communicator data structure

  Notes:
  Note that the size and rank duplicate data in the groups that
  make up this communicator.  These are used often enough that this
  optimization is valuable.  

  This definition provides only a 16-bit integer for context id''s .
  This should be sufficient for most applications.  However, extending
  this to a 32-bit (or longer) integer should be easy.

  The virtual connection table is an explicit member of this structure.
  This contains the information used to contact a particular process,
  indexed by the rank relative to this communicator.

  Groups are allocated lazily.  That is, the group pointers may be
  null, created only when needed by a routine such as 'MPI_Comm_group'.
  The local process ids needed to form the group are available within
  the virtual connection table.
  For intercommunicators, we may want to always have the groups.  If not, 
  we either need the 'local_group' or we need a virtual connection table
  corresponding to the 'local_group' (we may want this anyway to simplify
  the implementation of the intercommunicator collective routines).

  The pointer to the structure 'MPID_Collops' containing pointers to the 
  collective  
  routines allows an implementation to replace each routine on a 
  routine-by-routine basis.  By default, this pointer is null, as are the 
  pointers within the structure.  If either pointer is null, the implementation
  uses the generic provided implementation.  This choice, rather than
  initializing the table with pointers to all of the collective routines,
  is made to reduce the space used in the communicators and to eliminate the
  need to include the implementation of all collective routines in all MPI 
  executables, even if the routines are not used.

  The macro 'MPID_HAS_HETERO' may be defined by a device to indicate that
  the device supports MPI programs that must communicate between processes with
  different data representations (e.g., different sized integers or different
  byte orderings).  If the device does need to define this value, it should
  be defined in the file 'mpidpre.h'. 

  Module:
  Communicator-DS

  Question:
  For fault tolerance, do we want to have a standard field for communicator 
  health?  For example, ok, failure detected, all (live) members of failed 
  communicator have acked.
  S*/
typedef struct MPID_Comm { 
    int           handle;        /* value of MPI_Comm for this structure */
    volatile int  ref_count;
#if !defined(MPICH_SINGLE_THREADED)
    MPID_Thread_lock_t mutex;
#endif
    int16_t       context_id;    /* Assigned context id */
    int           remote_size;   /* Value of MPI_Comm_(remote)_size */
    int           rank;          /* Value of MPI_Comm_rank */
    MPID_VCRT     vcrt;          /* virtual connecton reference table */
    MPID_VCR *    vcr;           /* alias to the array of virtual connections
				    in vcrt */
    MPID_VCRT     local_vcrt;    /* local virtual connecton reference table */
    MPID_VCR *    local_vcr;     /* alias to the array of local virtual
				    connections in local vcrt */
    MPID_Attribute *attributes;    /* List of attributes */
    int           local_size;    /* Value of MPI_Comm_size for local group */
    MPID_Group   *local_group,   /* Groups in communicator. */
                 *remote_group;  /* The local and remote groups are the
                                    same for intra communicators */
    MPID_Comm_kind_t comm_kind;  /* MPID_INTRACOMM or MPID_INTERCOMM */
    char          name[MPI_MAX_OBJECT_NAME];  /* Required for MPI-2 */
    MPID_Errhandler *errhandler;  /* Pointer to the error handler structure */
    struct MPID_Comm    *local_comm; /* Defined only for intercomms, holds
				        an intracomm for the local group */
    int           is_low_group;   /* For intercomms only, this boolean is
				     set for all members of one of the 
				     two groups of processes and clear for 
				     the other.  It enables certain
				     intercommunicator collective operations
				     that wish to use half-duplex operations
				     to implement a full-duplex operation */
    struct MPID_Collops  *coll_fns; /* Pointer to a table of functions 
                                              implementing the collective 
                                              routines */
#ifdef MPID_HAS_HETERO
    int is_hetero;
#endif
  /* Other, device-specific information */
#ifdef MPID_DEV_COMM_DECL
    MPID_DEV_COMM_DECL
#endif
} MPID_Comm;
extern MPIU_Object_alloc_t MPID_Comm_mem;
void MPIR_Comm_add_ref(MPID_Comm *);
int MPIR_Comm_release(MPID_Comm *);

#define MPIR_Comm_add_ref(_comm) MPIU_Object_add_ref((_comm))
/* Preallocated comm objects */
#define MPID_COMM_N_BUILTIN 2
extern MPID_Comm MPID_Comm_builtin[MPID_COMM_N_BUILTIN];
extern MPID_Comm MPID_Comm_direct[];

/*
 * The order of the context offsets is important.  The collective routines
 * in the case of intercommunicator operations use offsets 2 and 3 for
 * the local intracommunicator; thus it is vital that the offsets used 
 * for communication between processes in the intercommunicator in a
 * collective operation (MPID_CONTEXT_INTER_COLL) be distinct from the 
 * offsets uses for communication on the local intracommunicator (2+
 * MPID_CONTEXT_INTRA_COLL)
 */
#define MPID_CONTEXT_INTRA_PT2PT 0
#define MPID_CONTEXT_INTRA_COLL  1
#define MPID_CONTEXT_INTRA_FILE  2
#define MPID_CONTEXT_INTRA_WIN   3
#define MPID_CONTEXT_INTER_PT2PT 0
#define MPID_CONTEXT_INTER_COLL  1
#define MPID_CONTEXT_INTER_COLLA 2
#define MPID_CONTEXT_INTER_COLLB 3
/* ------------------------------------------------------------------------- */

/* Requests */
/* This currently defines a single structure type for all requests.  
   Eventually, we may want a union type, as used in MPICH-1 */
/*E
  MPID_Request_kind - Kinds of MPI Requests

  Module:
  Request-DS

  E*/
typedef enum MPID_Request_kind_t {
    MPID_REQUEST_SEND, MPID_REQUEST_RECV, MPID_PREQUEST_SEND,
    MPID_PREQUEST_RECV, MPID_UREQUEST } MPID_Request_kind_t;

/* Typedefs for Fortran generalized requests */
typedef void (MPIR_Grequest_f77_cancel_function)(void *, int*, int *); 
typedef void (MPIR_Grequest_f77_free_function)(void *, int *); 
typedef void (MPIR_Grequest_f77_query_function)(void *, MPI_Status *, int *); 

/*S
  MPID_Request - Description of the Request data structure

  Module:
  Request-DS

  Notes:
  If it is necessary to remember the MPI datatype, this information is 
  saved within the device-specific fields provided by 'MPID_DEV_REQUEST_DECL'.

  Requests come in many flavors, as stored in the 'kind' field.  It is 
  expected that each kind of request will have its own structure type 
  (e.g., 'MPID_Request_send_t') that extends the 'MPID_Request'.
  
  S*/
typedef struct MPID_Request {
    int          handle;
    volatile int ref_count;
#ifndef MPICH_SINGLE_THREADED
    MPID_Thread_lock_t mutex;
#endif
#ifndef MPICH_SINGLE_THREADED
    /* initialized flag/lock used to by recv queue and recv code for 
       thread safety */
    MPID_Thread_lock_t initialized;
#endif
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
    MPI_Grequest_free_function   *free_fn;
    MPI_Grequest_query_function  *query_fn;
    void *grequest_extra_state;
    MPID_Lang_t                  greq_lang;         /* langage that defined
						       the generalize req */
    
    /* Other, device-specific information */
#ifdef MPID_DEV_REQUEST_DECL
    MPID_DEV_REQUEST_DECL
#endif
} MPID_Request;
extern MPIU_Object_alloc_t MPID_Request_mem;
/* Preallocated request objects */
extern MPID_Request MPID_Request_direct[];
/* ------------------------------------------------------------------------- */


/* ------------------------------------------------------------------------- */
/* end of mpirma.h (in src/mpi/rma?) */
/* ------------------------------------------------------------------------- */

/* Windows */
/*S
  MPID_Win - Description of the Window Object data structure.

  Module:
  Win-DS

  Notes:
  The following 3 keyvals are defined for attributes on all MPI 
  Window objects\:
.vb
 MPI_WIN_SIZE
 MPI_WIN_BASE
 MPI_WIN_DISP_UNIT
.ve
  These correspond to the values in 'length', 'start_address', and 
  'disp_unit'.

  The communicator in the window is the same communicator that the user
  provided to 'MPI_Win_create' (not a dup).  However, each intracommunicator
  has a special context id that may be used if MPI communication is used 
  by the implementation to implement the RMA operations.

  There is no separate window group; the group of the communicator should be
  used.

  Question:
  Should a 'MPID_Win' be defined after 'MPID_Segment' in case the device 
  wants to 
  store a queue of pending put/get operations, described with 'MPID_Segment'
  (or 'MPID_Request')s?

  S*/
typedef struct MPID_Win {
    int           handle;             /* value of MPI_Win for this structure */
    volatile int  ref_count;
    int fence_cnt;     /* 0 = no fence has been called; 
                          1 = fence has been called */ 
    MPID_Errhandler *errhandler;  /* Pointer to the error handler structure */
    void *base;
    MPI_Aint    size;        
    int          disp_unit;      /* Displacement unit of *local* window */
    MPID_Attribute *attributes;
    MPID_Group *start_group_ptr; /* group passed in MPI_Win_start */
    int start_assert;            /* assert passed to MPI_Win_start */
    MPI_Comm    comm;         /* communicator of window (dup) */
    volatile int my_counter;  /* completion counter for operations
                                 targeting this window */
    void **base_addrs;     /* array of base addresses of the windows of
                              all processes */
    int *disp_units;      /* array of displacement units of all windows */
    MPI_Win *all_win_handles;    /* array of handles to the window objects
                                          of all processes */
    MPIDI_RMA_ops *rma_ops_list; /* list of outstanding RMA requests */
    volatile int lock_granted;  /* flag to indicate whether lock has 
                                   been granted to this process (as source) for
                                   passive target rma */
    volatile int current_lock_type;   /* current lock type on this window (as target)
                              * (none, shared, exclusive) */
    volatile int shared_lock_ref_cnt;
    struct MPIDI_Win_lock_queue volatile *lock_queue;  /* list of unsatisfied locks */

    int *pt_rma_puts_accs;  /* array containing the no. of passive target
                               puts/accums issued from this process to other 
                               processes. */
    volatile int my_pt_rma_puts_accs;  /* no. of passive target puts/accums
                                          that this process has 
                                          completed as target */
 
#ifdef USE_THREADED_WINDOW_CODE
    /* These were causing compilation errors.  We need to figure out how to
       integrate threads into MPICH2 before including these fields. */
#ifdef HAVE_PTHREAD_H
    pthread_t wait_thread_id; /* id of thread handling MPI_Win_wait */
    pthread_t passive_target_thread_id; /* thread for passive target RMA */
#elif defined(HAVE_WINTHREADS)
    HANDLE wait_thread_id;
    HANDLE passive_target_thread_id;
#endif
#endif
    /* These are COPIES of the values so that addresses to them
       can be returned as attributes.  They are initialized by the
       MPI_Win_get_attr function */
    int  copyDispUnit;
    MPI_Aint copySize;
    
    char          name[MPI_MAX_OBJECT_NAME];  
  /* Other, device-specific information */
#ifdef MPID_DEV_WIN_DECL
    MPID_DEV_WIN_DECL
#endif
} MPID_Win;
extern MPIU_Object_alloc_t MPID_Win_mem;
/* Preallocated win objects */
extern MPID_Win MPID_Win_direct[];

/* ------------------------------------------------------------------------- */
/* also in mpirma.h ?*/
/* ------------------------------------------------------------------------- */
extern volatile int MPIDI_Passive_target_thread_exit_flag;
#define MPIDI_PASSIVE_TARGET_DONE_TAG  348297
#define MPIDI_PASSIVE_TARGET_RMA_TAG 563924

/*
 * Good Memory (may be required for passive target operations on MPI_Win)
 */

/*@
  MPID_Mem_alloc - Allocate memory suitable for passive target RMA operations

  Input Parameter:
+ size - Number of types to allocate.
- info - Info object

  Return value:
  Pointer to the allocated memory.  If the memory is not available, 
  returns null.

  Notes:
  This routine is used to implement 'MPI_Alloc_mem'.  It is for that reason
  that there is no communicator argument.  

  This memory may `only` be freed with 'MPID_Mem_free'.

  This is a `local`, not a collective operation.  It functions more like a
  good form of 'malloc' than collective shared-memory allocators such as
  the 'shmalloc' found on SGI systems.

  Implementations of this routine may wish to use 'MPID_Memory_register'.  
  However, this routine has slighly different requirements, so a separate
  entry point is provided.

  Question:
  Since this takes an info object, should there be an error routine in the 
  case that the info object contains an error?

  Module:
  Win
  @*/
void *MPID_Mem_alloc( size_t size, MPID_Info *info );

/*@
  MPID_Mem_free - Frees memory allocated with 'MPID_Mem_alloc'

  Input Parameter:
. ptr - Pointer to memory allocated by 'MPID_Mem_alloc'.

  Return value:
  'MPI_SUCCESS' if memory was successfully freed; an MPI error code otherwise.

  Notes:
  The return value is provided because it may not be easy to validate the
  value of 'ptr' without attempting to free the memory.

  Module:
  Win
  @*/
int MPID_Mem_free( void *ptr );

/*@
  MPID_Mem_was_alloced - Return true if this memory was allocated with 
  'MPID_Mem_alloc'

  Input Parameters:
+ ptr  - Address of memory
- size - Size of reqion in bytes.

  Return value:
  True if the memory was allocated with 'MPID_Mem_alloc', false otherwise.

  Notes:
  This routine may be needed by 'MPI_Win_create' to ensure that the memory 
  for passive target RMA operations was allocated with 'MPI_Mem_alloc'.
  This may be used, for example, for ensuring that memory used with
  passive target operations was allocated with 'MPID_Mem_alloc'.

  Module:
  Win
  @*/
int MPID_Mem_was_alloced( void *ptr );

/* ------------------------------------------------------------------------- */
/* end of also in mpirma.h ? */
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
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
typedef enum MPID_Op_kind { MPID_OP_MAX=1, MPID_OP_MIN=2, 
			    MPID_OP_SUM=3, MPID_OP_PROD=4, 
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
/* FIXME: Should there be "restrict" in the definitions above, e.g., 
   (*c_function)( const void restrict * , void restrict *, ... )? */

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
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
/* mpicoll.h (in src/mpi/coll? */
/* ------------------------------------------------------------------------- */

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
/* ------------------------------------------------------------------------- */
/* end of mpicoll.h (in src/mpi/coll? */
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
/* Files */
/* FIXME: Make this compatible with ROMIO */
/* The "USE_ROMIO_FILE" definition tells MPICH2 to use the MPI_File
   defined by ROMIO, rather than the standard object handles used for the
   other MPI objects.  This is temporary, until ROMIO is modified to 
   work with the new object handles */
#define USE_ROMIO_FILE
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
/* ------------------------------------------------------------------------- */

/* Time stamps */
/* Get the timer definitions.  The source file for this include is
   src/mpi/timer/mpichtimer.h.in */
#include "mpichtimer.h"

typedef struct MPID_Stateinfo_t {
    MPID_Time_t stamp;
    int count;
} MPID_Stateinfo_t;
#define MPICH_MAX_STATES 512

/* ------------------------------------------------------------------------- */
/* Thread types */
/* Temporary; this will include "mpichthread.h" eventually */

typedef struct MPICH_PerThread_t {
    int              nest_count;   /* For layered MPI implementation */
    int              op_errno;     /* For errors in predefined MPI_Ops */
#ifdef HAVE_TIMING
    MPID_Stateinfo_t timestamps[MPICH_MAX_STATES];  /* per thread state info */
#endif
#if defined(MPID_DEV_PERTHREAD_DECL)
    MPID_DEV_PERTHREAD_DECL
#endif    
} MPICH_PerThread_t;

#ifdef MPICH_SINGLE_THREADED
extern MPICH_PerThread_t MPIR_Thread;
#define MPID_Common_thread_lock()
#define MPID_Common_thread_unlock()

/*M
  MPID_Comm_thread_lock - Acquire a thread lock for a communicator

  Synopsis:
.vb
    void MPID_Comm_thread_lock( MPID_Comm *comm )
.ve

  Input Parameter:
. comm - Communicator to lock

  Notes:
  This routine acquires a lock among threads in the same MPI process that
  may use this communicator.  In all MPI thread modes except for
  'MPI_THREAD_MULTIPLE', this can be a no-op.  In an MPI implementation
  that does not provide 'MPI_THREAD_MULTIPLE', this may be a macro.

  It is invalid for a thread that has acquired the lock to attempt to 
  acquire it again.  The lock must be released by 'MPID_Comm_thread_unlock'.

  Note that there is also a common per-process lock ('common_lock').  
  That lock should be used instead of a lock on lock on 'MPI_COMM_WORLD' when
  a lock across all threads is required.

  A high-quality implementation may wish to provide fair access to the lock.

  In general, the MPICH implementation tries to avoid using locks because 
  they can cause problems such as livelock and deadlock, particularly when
  an error occurs.  However, the semantics of MPI collective routines make 
  it difficult to avoid using locks.  Further, good programming practice by
  MPI programmers should be to avoid having multiple threads using the
  same communicator.

  Module:
  Communicator

  See Also: 
  'MPID_Comm_thread_unlock'

  Questions:
  Do we also need versions of this for datatypes and window objects?  
  For example, communicators, datatypes, and window objects all have 
  attributes; do we need a thread lock for each type?  Should we instead have
  an MPI Object, on which some common operations, such as thread lock, 
  reference count, and name are implemented?  Note that there is a 
  common lock for operations that are infrequently performed and do not 
  require fine-grain locking.
  M*/
#define MPID_Comm_thread_lock(comm_ptr_)
/*M
  MPID_Comm_thread_unlock - Release a thread lock for a communicator

  Synopsis:
.vb
   void MPID_Comm_thread_unlock( MPID_Comm *comm )
.ve

  Input Parameter:
. comm - Communicator to unlock

  Module:
  Communicator

  See Also: 
  'MPID_Comm_thread_lock'
M*/
#define MPID_Comm_thread_unlock(comm_ptr_)

#define MPID_Request_construct(request_ptr_)
#define MPID_Request_destruct(request_ptr_)
#define MPID_Request_thread_lock(request_ptr_)
#define MPID_Request_thread_unlock(request_ptr_)
#define MPID_Request_initialized_clear(request_ptr_)
#define MPID_Request_initialized_set(request_ptr_)
#define MPID_Request_initialized_wait(request_ptr_)
/* The basic thread lock/unlock are defined in mpiimplthread.h */
/* #define MPID_Thread_lock( ptr ) */
/* #define MPID_Thread_unlock( ptr ) */
#else
#define MPID_Common_thread_lock() MPID_Thread_lock( &MPIR_Process.common_lock )
#define MPID_Common_thread_unlock() MPID_Thread_unlock( &MPIR_Process.common_lock )

#define MPID_Comm_thread_lock(comm_ptr_) MPID_Thread_lock(&(comm_ptr_)->mutex)
#define MPID_Comm_thread_unlock(comm_ptr_) MPID_Thread_unlock(&(comm_ptr_)->mutex)

#define MPID_Request_construct(request_ptr_)			\
{								\
    MPID_Thread_lock_init(&(request_ptr_)->mutex);		\
    MPID_Thread_lock_init(&(request_ptr_)->initialized);	\
}
#define MPID_Request_destruct(request_ptr_)			\
{								\
    MPID_Thread_lock_destroy(&(request_ptr_)->mutex);		\
    MPID_Thread_lock_destroy(&(request_ptr_)->initialized);	\
}
#define MPID_Request_thread_lock(request_ptr_) MPID_Thread_lock(&(request_ptr_)->mutex)
#define MPID_Request_thread_unlock(request_ptr_) MPID_Thread_unlock(&(request_ptr_)->mutex)
/* TODO: MT: these should be rewritten to use busy waiting and appropriate processor memory fences.  They and an necessary
   variables should probably be defined by the device rather than in the top level include file.  */
#define MPID_Request_initialized_clear(request_ptr_) MPID_Thread_lock(&(request_ptr_)->initialized)
#define MPID_Request_initialized_set(request_ptr_)  MPID_Thread_unlock(&(request_ptr_)->initialized)
#define MPID_Request_initialized_wait(request_ptr_)	\
{							\
    MPID_Thread_lock(&(request_ptr_)->initialized);	\
    MPID_Thread_unlock(&(request_ptr_)->initialized);	\
}    
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

struct MPID_Datatype;

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
    int (*attr_dup)( int, MPID_Attribute *, MPID_Attribute ** );
    int (*attr_free)( int, MPID_Attribute * );
    /* There is no win_attr_dup function because there can be no MPI_Win_dup
       function */
    /* Routine to get the messages corresponding to dynamically created
       error messages */
    const char *(*errcode_to_string)( int );
#ifdef HAVE_CXX_BINDING
    /* Routines to call C++ functions from the C implementation of the
       MPI reduction and attribute routines */
    void (*cxx_call_op_fn)( void *, void *, int, MPI_Datatype, 
			    MPI_User_function * );
    /* Attribute functions.  We use a single "call" function for Comm, Datatype,
       and File, since all are ints (and we can cast in the call) */
    int  (*cxx_call_delfn)( int, int, void *, void *, 
			    void (*)(void) );
    int  (*cxx_call_copyfn)( int, int, void *, void *, 
			    void (*)(void) );
    /* Error handling functions.  As for the attribute functions,
       we pass the integer file/comm/win, the address of the error code, 
       and the C function to call (itself a function defined by the
       C++ interface and exported to C).  The first argument is used
       to specify the kind (comm,file,win) */
    void  (*cxx_call_errfn) ( int, int *, int *, void (*)(void) );
#endif    
} MPICH_PerProcess_t;
extern MPICH_PerProcess_t MPIR_Process;

/* Record the level of thread support */
/* FIXME: Where is this used, and why isn't in the PerProcess structure? */
extern int MPID_THREAD_LEVEL;

/*D
  MPID_MAX_THREAD_LEVEL - Indicates the maximum level of thread
  support provided at compile time.
 
  Values:
  Any of the 'MPI_THREAD_xxx' values (these are preprocessor-time constants)

  Notes:
  The macro 'MPID_MAX_THREAD_LEVEL' defines the maximum level of
  thread support provided, and may be used at compile time to remove
  thread locks and other code needed only in a multithreaded environment.

  A typical use is 
.vb
  #if MPID_MAX_THREAD_LEVEL >= MPI_THREAD_MULTIPLE
     lock((r)->lock_ptr);
     (r)->ref_count++;
     unlock((r)->lock_ptr);
  #else
     (r)->ref_count ++;
  #fi
.ve

  Module:
  Environment-DS
  D*/
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
/* ------------------------------------------------------------------------- */

/* set up the timing macros */
#include "mpitimerimpl.h"

/* Definitions for error handling and reporting */
#include "mpierror.h"
#include "mpierrs.h"

/* ------------------------------------------------------------------------- */
/* FIXME: Merge these with the object refcount update routines (perhaps as
   part of a general "atomic update" file */
/*
 * Standardized general-purpose atomic update routines.  Some comments:
 * Setmax atomically implements *a_ptr = max(b,*a_ptr) .  This can
 * be implemented using compare-and-swap (form max, if new max is 
 * larger, compare-and-swap against old max.  if failure, restart).
 * Fetch_and_increment can be implemented in a similar way; for
 * example, in IA32, 
 * loop:
 *   mov eax, valptr
 *   mov oldvalptr, eax
 *   mov ebx, eax
 *   inc ebx
 *   lock: cmpxchg valptr, ebx
 *   jnz loop
 *
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
#if MPID_MAX_THREAD_LEVEL < MPI_THREAD_FUNNELED
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
/* ------------------------------------------------------------------------- */

/* FIXME: Move these to the communicator block; make sure that all 
   objects have such hooks */
#ifndef HAVE_DEV_COMM_HOOK
#define MPID_Dev_comm_create_hook( a )
#define MPID_Dev_comm_destroy_hook( a )
#endif

/* ------------------------------------------------------------------------- */
/* FIXME: What is the scope of these functions?  Can they be moved into
   src/mpi/pt2pt? */
/* ------------------------------------------------------------------------- */

#define MPIR_Status_set_empty(status_)			\
{							\
    if ((status_) != MPI_STATUS_IGNORE)			\
    {							\
	(status_)->MPI_SOURCE = MPI_ANY_SOURCE;		\
	(status_)->MPI_TAG = MPI_ANY_TAG;		\
	/* (status_)->MPI_ERROR = MPI_SUCCESS; */	\
	(status_)->count = 0;				\
	(status_)->cancelled = FALSE;			\
    }							\
}
/* See MPI 1.1, section 3.11, Null Processes */
#define MPIR_Status_set_procnull(status_)		\
{							\
    if ((status_) != MPI_STATUS_IGNORE)			\
    {							\
	(status_)->MPI_SOURCE = MPI_PROC_NULL;		\
	(status_)->MPI_TAG = MPI_ANY_TAG;		\
	/* (status_)->MPI_ERROR = MPI_SUCCESS; */	\
	(status_)->count = 0;				\
	(status_)->cancelled = FALSE;			\
    }							\
}

#define MPIR_Request_extract_status(request_ptr_, status_)								\
{															\
    if ((status_) != MPI_STATUS_IGNORE)											\
    {															\
	int error__;													\
															\
	/* According to the MPI 1.1 standard page 22 lines 9-12, the MPI_ERROR field may not be modified except by the	\
	   functions in section 3.7.5 which return MPI_ERR_IN_STATUSES (MPI_Wait{all,some} and MPI_Test{all,some}). */	\
	error__ = (status_)->MPI_ERROR;											\
	*(status_) = (request_ptr_)->status;										\
	(status_)->MPI_ERROR = error__;											\
    }															\
}
/* ------------------------------------------------------------------------- */

/* FIXME: The bindings should be divided into three groups:
   1. ADI3 routines.  These should have structure comment documentation, e.g., 
   the text from doc/adi3/adi3.c
   2. General utility routines.  These should have a short description
   3. Local utility routines, e.g., routines used within a single subdirectory.
   These should be moved into an include file in that subdirectory 
*/
/* Bindings for internal routines */
/*@ MPIR_Add_finalize - Add a routine to be called when MPI_Finalize is invoked

+ routine - Routine to call
. extra   - Void pointer to data to pass to the routine
- priority - Indicates the priority of this callback and controls the order
  in which callbacks are executed.  Use a priority of zero for most handlers;
  higher priorities will be executed first.
@*/
void MPIR_Add_finalize( int (*routine)( void * ), void *extra, int priority );

/* For no error checking, we could define MPIR_Nest_incr/decr as empty */
#ifdef MPICH_SINGLE_THREADED
#define MPIR_Nest_incr() MPIR_Thread.nest_count++
#define MPIR_Nest_decr() MPIR_Thread.nest_count--
#define MPIR_Nest_value() MPIR_Thread.nest_count
#else
void MPIR_Nest_incr( void );
void MPIR_Nest_decr( void );
int MPIR_Nest_value( void );
#endif
/*int MPIR_Comm_attr_dup(MPID_Comm *, MPID_Attribute **);
  int MPIR_Comm_attr_delete(MPID_Comm *, MPID_Attribute *);*/
int MPIR_Comm_copy( MPID_Comm *, int, MPID_Comm ** );
void MPIR_Keyval_set_fortran( int );
#ifdef HAVE_CXX_BINDING
extern void MPIR_Keyval_set_cxx( int, void (*)(void), void (*)(void) );
extern void MPIR_Op_set_cxx( MPI_Op, void (*)(void) );
#endif

#ifdef HAVE_FORTRAN_BINDING
    /* Initialize Fortran special names (MPI_BOTTOM and STATUS_IGNOREs) */
#if defined(F77_NAME_LOWER_USCORE) || defined(F77_NAME_LOWER_2USCORE)
extern void mpirinitf_(void);
#elif defined(F77_NAME_UPPER)
extern void MPIRINITF(void);
#else
extern void mpirinitf(void);
#endif
#endif

int MPIR_Group_create( int, MPID_Group ** );
int MPIR_Group_release(MPID_Group *group_ptr);

int MPIR_dup_fn ( MPI_Comm, int, void *, void *, void *, int * );
int MPIR_Request_complete(MPI_Request *, MPID_Request *, MPI_Status *, int *);
int MPIR_Request_get_error(MPID_Request *);

/* ADI Bindings */
/*@
  MPID_Init - Initialize the device

  Input Parameters:
+ argc_p - Pointer to the argument count
. argv_p - Pointer to the argument list
- requested - Requested level of thread support.  Values are the same as
  for the 'required' argument to 'MPI_Init_thread', except that we define
  an enum for these values.

  Output Parameters:
+ provided - Provided level of thread support.  May be less than the 
  requested level of support.
. has_args - Set to true if 'argc_p' and 'argv_p' contain the command
  line arguments.  See below.
- has_env  - Set to true if the environment of the process has been 
  set as the user expects.  See below.

  Return value:
  Returns '0' on success and an MPI error code on failure.  Failure can happen
  when, for example, the device is unable to start or contact the number of
  processes specified by the 'mpiexec' command.

  Notes:
  Null arguments for 'argc_p' and 'argv_p' `must` be valid (see MPI-2, section
  4.2)

  Multi-method devices should initialize each method within this call.
  They can use environment variables and/or command-line arguments
  to decide which methods to initialize (but note that they must not
  `depend` on using command-line arguments).

  This call also initializes all MPID data needed by the device.  This
  includes the 'MPID_Request's and any other data structures used by 
  the device.

  The arguments 'has_args' and 'has_env' indicate whether the process was
  started with command-line arguments or environment variables.  In some
  cases, only the root process is started with these values; in others, 
  the startup environment ensures that each process receives the 
  command-line arguments and environment variables that the user expects. 
  While the MPI standard makes no requirements that command line arguments or 
  environment variables are provided to all processes, most users expect a
  common environment.  These variables allow an MPI implementation (that is
  based on ADI-3) to provide both of these by making use of MPI communication
  after 'MPID_Init' is called but before 'MPI_Init' returns to the user, if
  the process management environment does not provide this service.


  This routine is used to implement both 'MPI_Init' and 'MPI_Init_thread'.

  Setting the environment requires a 'setenv' function.  Some
  systems may not have this.  In that case, the documentation must make 
  clear that the environment may not be propagated to the generated processes.

  Module:
  MPID_CORE

  Questions:

  The values for 'has_args' and 'has_env' are boolean.  
  They could be more specific.  For 
  example, the value could indicate the rank in 'MPI_COMM_WORLD' of a 
  process that has the values; the value 'MPI_ANY_SOURCE' (or a '-1') could
  indicate that the value is available on all processes (including this one).
  We may want this since otherwise the processes may need to determine whether
  any process needs the command line.  Another option would be to use positive 
  values in the same way that the 'color' argument is used in 'MPI_Comm_split';
  a negative value indicates the member of the processes with that color that 
  has the values of the command line arguments (or environment).  This allows
  for non-SPMD programs.

  Do we require that the startup environment (e.g., whatever 'mpiexec' is 
  using to start processes) is responsible for delivering
  the command line arguments and environment variables that the user expects?
  That is, if the user is running an SPMD program, and expects each process
  to get the same command line argument, who is responsible for this?  
  The 'has_args' and 'has_env' values are intended to allow the ADI to 
  handle this while taking advantage of any support that the process 
  manager framework may provide.

  Alternately, how do we find out from the process management environment
  whether it took care of the environment or the command line arguments?  
  Do we need a 'PMI_Env_query' function that can answer these questions
  dynamically (in case a different process manager is used through the same
  interface)?

  Can we fix the Fortran command-line arguments?  That is, can we arrange for
  'iargc' and 'getarg' (and the POSIX equivalents) to return the correct 
  values?  See, for example, the Absoft implementations of 'getarg'.  
  We could also contact PGI about the Portland Group compilers, and of 
  course the 'g77' source code is available.
  Does each process have the same values for the environment variables 
  when this routine returns?

  If we don''t require that all processes get the same argument list, 
  we need to find out if they did anyway so that 'MPI_Init_thread' can
  fixup the list for the user.  This argues for another return value that
  flags how much of the environment the 'MPID_Init' routine set up
  so that the 'MPI_Init_thread' call can provide the rest.  The reason
  for this is that, even though the MPI standard does not require it, 
  a user-friendly implementation should, in the SPMD mode, give each
  process the same environment and argument lists unless the user 
  explicitly directed otherwise.

  How does this interface to PMI?  Do we need to know anything?  Should
  this call have an info argument to support PMI?

  The following questions involve how environment variables and command
  line arguments are used to control the behavior of the implementation. 
  Many of these values must be determined at the time that 'MPID_Init' 
  is called.  These all should be considered in the context of the 
  parameter routines described in the MPICH2 Design Document.

  Are there recommended environment variable names?  For example, in ADI-2,
  there are many debugging options that are part of the common device.
  In MPI-2, we can''t require command line arguments, so any such options
  must also have environment variables.  E.g., 'MPICH_ADI_DEBUG' or
  'MPICH_ADI_DB'.

  Names that are explicitly prohibited?  For example, do we want to 
  reserve any names that 'MPI_Init_thread' (as opposed to 'MPID_Init')
  might use?  

  How does information on command-line arguments and environment variables
  recognized by the device get added to the documentation?

  What about control for other impact on the environment?  For example,
  what signals should the device catch (e.g., 'SIGFPE'? 'SIGTRAP'?)?  
  Which of these should be optional (e.g., ignore or leave signal alone) 
  or selectable (e.g., port to listen on)?  For example, catching 'SIGTRAP'
  causes problems for 'gdb', so we''d like to be able to leave 'SIGTRAP' 
  unchanged in some cases.

  Another environment variable should control whether fault-tolerance is 
  desired.  If fault-tolerance is selected, then some collective operations 
  will need to use different algorithms and most fatal errors detected by the 
  MPI implementation should abort only the affected process, not all processes.
  @*/
int MPID_Init( int *argc_p, char ***argv_p, int requested, 
	       int *provided, int *has_args, int *has_env );

/* was: 
 int MPID_Init( int *argc_p, char ***argv_p, 
	       int requested, int *provided,
	       MPID_Comm **parent_comm, int *has_args, int *has_env ); */

/*@
  MPID_Finalize - Perform the device-specific termination of an MPI job

  Return Value:
  'MPI_SUCCESS' or a valid MPI error code.  Normally, this routine will
  return 'MPI_SUCCESS'.  Only in extrordinary circumstances can this
  routine fail; for example, if some process stops responding during the
  finalize step.  In this case, 'MPID_Finalize' should return an MPI 
  error code indicating the reason that it failed.

  Notes:

  Module:
  MPID_CORE

  Questions:
  Need to check the MPI-2 requirements on 'MPI_Finalize' with respect to
  things like which process must remain after 'MPID_Finalize' is called.
  @*/
int MPID_Finalize(void);
/*@
  MPID_Abort - Abort at least the processes in the specified communicator.

  Input Parameters:
+ comm        - Communicator of processes to abort
. mpi_errno   - MPI error code containing the reason for the abort
- exit_code   - Exit code to return to the calling environment.  See notes.

  Return value:
  'MPI_SUCCESS' or an MPI error code.  Normally, this routine should not 
  return, since the calling process must be a member of the communicator.  
  However, under some circumstances, the 'MPID_Abort' might fail; in this 
  case, returning an error indication is appropriate.

  Notes:

  In a fault-tolerant MPI implementation, this operation should abort `only` 
  the processes in the specified communicator.  Any communicator that shares
  processes with the aborted communicator becomes invalid.  For more 
  details, see (paper not yet written on fault-tolerant MPI).

  In particular, if the communicator is 'MPI_COMM_SELF', only the calling 
  process should be aborted.

  The 'exit_code' is the exit code that this particular process will 
  attempt to provide to the 'mpiexec' or other program invocation 
  environment.  See 'mpiexec' for a discussion of how exit codes from 
  many processes may be combined.

  An external agent that is aborting processes can invoke this with either
  'MPI_COMM_WORLD' or 'MPI_COMM_SELF'.  For example, if the process manager
  wishes to abort a group of processes, it should cause 'MPID_Abort' to 
  be invoked with 'MPI_COMM_SELF' on each process in the group.

  Question:
  An alternative design is to provide an 'MPID_Group' instead of a
  communicator.  This would allow a process manager to ask the ADI 
  to kill an entire group of processes without needing a communicator.
  However, the implementation of 'MPID_Abort' will either do this by
  communicating with other processes or by requesting the process manager
  to kill the processes.  That brings up this question: should 
  'MPID_Abort' use 'PMI' to kill processes?  Should it be required to
  notify the process manager?  What about persistent resources (such 
  as SYSV segments or forked processes)?  

  This suggests that for any persistent resource, an exit handler be
  defined.  These would be executed by 'MPID_Abort' or 'MPID_Finalize'.  
  See the implementation of 'MPI_Finalize' for an example of exit callbacks.
  In addition, code that registered persistent resources could use persistent
  storage (i.e., a file) to record that information, allowing cleanup 
  utilities (such as 'mpiexec') to remove any resources left after the 
  process exits.

  'MPI_Finalize' requires that attributes on 'MPI_COMM_SELF' be deleted 
  before anything else happens; this allows libraries to attach end-of-job
  actions to 'MPI_Finalize'.  It is valuable to have a similar 
  capability on 'MPI_Abort', with the caveat that 'MPI_Abort' may not 
  guarantee that the run-on-abort routines were called.  This provides a
  consistent way for the MPICH implementation to handle freeing any 
  persistent resources.  However, such callbacks must be limited since
  communication may not be possible once 'MPI_Abort' is called.  Further,
  any callbacks must guarantee that they have finite termination.  
  
  One possible extension would be to allow `users` to add actions to be 
  run when 'MPI_Abort' is called, perhaps through a special attribute value
  applied to 'MPI_COMM_SELF'.  Note that is is incorrect to call the delete 
  functions for the normal attributes on 'MPI_COMM_SELF' because MPI
  only specifies that those are run on 'MPI_Finalize' (i.e., normal 
  termination). 

  Module:
  MPID_CORE
  @*/
int MPID_Abort( MPID_Comm *comm, int mpi_errno, int exit_code );

int MPID_Open_port(MPID_Info *, char *);
int MPID_Close_port(char *);
int MPID_Comm_accept(char *, MPID_Info *, int, MPID_Comm *, MPID_Comm **);
int MPID_Comm_connect(char *, MPID_Info *, int, MPID_Comm *, MPID_Comm **);
int MPID_Comm_disconnect(MPID_Comm *);
int MPID_Comm_spawn_multiple(int, char *[], char* *[], int [], MPID_Info* [],
                             int, MPID_Comm *, MPID_Comm **, int []);

/*@
  MPID_Send - MPID entry point for MPI_Send

  Notes:
  The only difference between this and 'MPI_Send' is that the basic
  error checks (e.g., valid communicator, datatype, dest, and tag)
  have been made, the MPI opaque objects have been replaced by
  MPID objects, a context id offset is provided in addition to the 
  communicator, and a request may be returned.  The context offset is 
  added to the context of the communicator
  to get the context it used by the message.
  A request is returned only if the ADI implementation was unable to 
  complete the send of the message.  In that case, the usual 'MPI_Wait'
  logic should be used to complete the request.  This approach is used to 
  allow a simple implementation of the ADI.  The ADI is free to always 
  complete the message and never return a request.

  Module:
  Communication

  @*/
int MPID_Send( const void *buf, int count, MPI_Datatype datatype,
	       int dest, int tag, MPID_Comm *comm, int context_offset,
	       MPID_Request **request );

/*@
  MPID_Rsend - MPID entry point for MPI_Rsend

  Notes:
  The only difference between this and 'MPI_Rsend' is that the basic
  error checks (e.g., valid communicator, datatype, dest, and tag)
  have been made, the MPI opaque objects have been replaced by
  MPID objects, a context id offset is provided in addition to the 
  communicator, and a request may be returned.  The context offset is 
  added to the context of the communicator
  to get the context it used by the message.
  A request is returned only if the ADI implementation was unable to 
  complete the send of the message.  In that case, the usual 'MPI_Wait'
  logic should be used to complete the request.  This approach is used to 
  allow a simple implementation of the ADI.  The ADI is free to always 
  complete the message and never return a request.

  Module:
  Communication

  @*/
int MPID_Rsend( const void *buf, int count, MPI_Datatype datatype,
		int dest, int tag, MPID_Comm *comm, int context_offset,
		MPID_Request **request );

/*@
  MPID_Ssend - MPID entry point for MPI_Ssend

  Notes:
  The only difference between this and 'MPI_Ssend' is that the basic
  error checks (e.g., valid communicator, datatype, dest, and tag)
  have been made, the MPI opaque objects have been replaced by
  MPID objects, a context id offset is provided in addition to the 
  communicator, and a request may be returned.  The context offset is 
  added to the context of the communicator
  to get the context it used by the message.
  A request is returned only if the ADI implementation was unable to 
  complete the send of the message.  In that case, the usual 'MPI_Wait'
  logic should be used to complete the request.  This approach is used to 
  allow a simple implementation of the ADI.  The ADI is free to always 
  complete the message and never return a request.

  Module:
  Communication

  @*/
int MPID_Ssend( const void *buf, int count, MPI_Datatype datatype,
		int dest, int tag, MPID_Comm *comm, int context_offset,
		MPID_Request **request );

/*@
  MPID_tBsend - Attempt a send and return if it would block

  Notes:
  This has the semantics of 'MPI_Bsend', except that it returns the internal
  error code 'MPID_WOULD_BLOCK' if the message can''t be sent immediately
  (t is for "try").  
 
  The reason that this interface is chosen over a query to check whether
  a message `can` be sent is that the query approach is not
  thread-safe.  Since the decision on whether a message can be sent
  without blocking depends (among other things) on the state of flow
  control managed by the device, this approach also gives the device
  the greatest freedom in implementing flow control.  In particular,
  if another MPI process can change the flow control parameters, then
  even in a single-threaded implementation, it would not be safe to
  return, for example, a message size that could be sent with 'MPI_Bsend'.

  This routine allows an MPI implementation to optimize 'MPI_Bsend'
  for the case when the message can be delivered without blocking the
  calling process.  An ADI implementation is free to have this routine
  always return 'MPID_WOULD_BLOCK', but is encouraged not to.

  To allow the MPI implementation to avoid trying this routine when it
  is not implemented by the ADI, the C preprocessor constant 'MPID_HAS_TBSEND'
  should be defined if this routine has a nontrivial implementation.

  This is an optional routine.  The MPI code for 'MPI_Bsend' will attempt
  to call this routine only if the device defines 'MPID_HAS_TBSEND'.

  Module:
  Communication
  @*/
int MPID_tBsend( const void *buf, int count, MPI_Datatype datatype,
		 int dest, int tag, MPID_Comm *comm, int context_offset );

/*@
  MPID_Isend - MPID entry point for MPI_Isend

  Notes:
  The only difference between this and 'MPI_Isend' is that the basic
  error checks (e.g., valid communicator, datatype, dest, and tag)
  have been made, the MPI opaque objects have been replaced by
  MPID objects, and a context id offset is provided in addition to the 
  communicator.  This offset is added to the context of the communicator
  to get the context it used by the message.

  Module:
  Communication

  @*/
int MPID_Isend( const void *buf, int count, MPI_Datatype datatype,
		int dest, int tag, MPID_Comm *comm, int context_offset,
		MPID_Request **request );

/*@
  MPID_Irsend - MPID entry point for MPI_Irsend

  Notes:
  The only difference between this and 'MPI_Irsend' is that the basic
  error checks (e.g., valid communicator, datatype, dest, and tag)
  have been made, the MPI opaque objects have been replaced by
  MPID objects, and a context id offset is provided in addition to the 
  communicator.  This offset is added to the context of the communicator
  to get the context it used by the message.

  Module:
  Communication

  @*/
int MPID_Irsend( const void *buf, int count, MPI_Datatype datatype,
		 int dest, int tag, MPID_Comm *comm, int context_offset,
		 MPID_Request **request );

/*@
  MPID_Issend - MPID entry point for MPI_Issend

  Notes:
  The only difference between this and 'MPI_Issend' is that the basic
  error checks (e.g., valid communicator, datatype, dest, and tag)
  have been made, the MPI opaque objects have been replaced by
  MPID objects, and a context id offset is provided in addition to the 
  communicator.  This offset is added to the context of the communicator
  to get the context it used by the message.

  Module:
  Communication

  @*/
int MPID_Issend( const void *buf, int count, MPI_Datatype datatype,
		 int dest, int tag, MPID_Comm *comm, int context_offset,
		 MPID_Request **request );

/*@
  MPID_Recv - MPID entry point for MPI_Recv

  Notes:
  The only difference between this and 'MPI_Recv' is that the basic
  error checks (e.g., valid communicator, datatype, source, and tag)
  have been made, the MPI opaque objects have been replaced by
  MPID objects, a context id offset is provided in addition to the 
  communicator, and a request may be returned.  The context offset is added 
  to the context of the communicator to get the context it used by the message.
  As in 'MPID_Send', the request is returned only if the operation did not
  complete.  Conversely, the status object is populated with valid information
  only if the operation completed.

  Module:
  Communication

  @*/
int MPID_Recv( void *buf, int count, MPI_Datatype datatype,
	       int source, int tag, MPID_Comm *comm, int context_offset,
	       MPI_Status *status, MPID_Request **request );


/*@
  MPID_Irecv - MPID entry point for MPI_Irecv

  Notes:
  The only difference between this and 'MPI_Irecv' is that the basic
  error checks (e.g., valid communicator, datatype, source, and tag)
  have been made, the MPI opaque objects have been replaced by
  MPID objects, and a context id offset is provided in addition to the 
  communicator.  This offset is added to the context of the communicator
  to get the context it used by the message.

  Module:
  Communication

  @*/
int MPID_Irecv( void *buf, int count, MPI_Datatype datatype,
		int source, int tag, MPID_Comm *comm, int context_offset,
		MPID_Request **request );

/*@
  MPID_Send_init - MPID entry point for MPI_Send_init

  Notes:
  The only difference between this and 'MPI_Send_init' is that the basic
  error checks (e.g., valid communicator, datatype, dest, and tag)
  have been made, the MPI opaque objects have been replaced by
  MPID objects, and a context id offset is provided in addition to the 
  communicator.  This offset is added to the context of the communicator
  to get the context it used by the message.

  Module:
  Communication

  @*/
int MPID_Send_init( const void *buf, int count, MPI_Datatype datatype,
		    int dest, int tag, MPID_Comm *comm, int context_offset,
		    MPID_Request **request );

int MPID_Bsend_init(const void *, int, MPI_Datatype, int, int, MPID_Comm *,
		   int, MPID_Request **);
/*@
  MPID_Rsend_init - MPID entry point for MPI_Rsend_init

  Notes:
  The only difference between this and 'MPI_Rsend_init' is that the basic
  error checks (e.g., valid communicator, datatype, dest, and tag)
  have been made, the MPI opaque objects have been replaced by
  MPID objects, and a context id offset is provided in addition to the 
  communicator.  This offset is added to the context of the communicator
  to get the context it used by the message.

  Module:
  Communication

  @*/
int MPID_Rsend_init( const void *buf, int count, MPI_Datatype datatype,
		     int dest, int tag, MPID_Comm *comm, int context_offset,
		     MPID_Request **request );
/*@
  MPID_Ssend_init - MPID entry point for MPI_Ssend_init

  Notes:
  The only difference between this and 'MPI_Ssend_init' is that the basic
  error checks (e.g., valid communicator, datatype, dest, and tag)
  have been made, the MPI opaque objects have been replaced by
  MPID objects, and a context id offset is provided in addition to the 
  communicator.  This offset is added to the context of the communicator
  to get the context it used by the message.

  Module:
  Communication

  @*/
int MPID_Ssend_init( const void *buf, int count, MPI_Datatype datatype,
		     int dest, int tag, MPID_Comm *comm, int context_offset,
		     MPID_Request **request );

/*@
  MPID_Recv_init - MPID entry point for MPI_Recv_init

  Notes:
  The only difference between this and 'MPI_Recv_init' is that the basic
  error checks (e.g., valid communicator, datatype, source, and tag)
  have been made, the MPI opaque objects have been replaced by
  MPID objects, and a context id offset is provided in addition to the 
  communicator.  This offset is added to the context of the communicator
  to get the context it used by the message.

  Module:
  Communication

  @*/
int MPID_Recv_init( void *buf, int count, MPI_Datatype datatype,
		    int source, int tag, MPID_Comm *comm, int context_offset,
		    MPID_Request **request );

/*@
  MPID_Startall - MPID entry point for MPI_Startall

  Notes:
  The only difference between this and 'MPI_Startall' is that the basic
  error checks (e.g., count) have been made, and the MPI opaque objects
  have been replaced by pointers to MPID objects.  

  Rationale:
  This allows the device to schedule communication involving multiple requests,
  whereas an implementation built on just 'MPID_Start' would force the
  ADI to initiate the communication in the order encountered.
  @*/
int MPID_Startall(int count, MPID_Request *requests[]);

/*@
   MPID_Probe - Block until a matching request is found and return information 
   about it

  Input Parameters:
+ source - rank to match (or 'MPI_ANY_SOURCE')
. tag - Tag to match (or 'MPI_ANY_TAG')
. comm - communicator to match.
- context_offset - context id offset of communicator to match

  Output Parameter:
. status - 'MPI_Status' set as defined by 'MPI_Probe'


  Return Value:
  Error code.
  
  Notes:
  Note that the values returned in 'status' will be valid for a subsequent
  MPI receive operation only if no other thread attempts to receive the same
  message.  
  (See the 
  discussion of probe in Section 8.7.2 Clarifications of the MPI-2 standard.)

  Providing the 'context_offset' is necessary at this level to support the 
  way in which the MPICH implementation uses context ids in the implementation
  of other operations.  The communicator is present to allow the device 
  to use message-queues attached to particular communicators or connections
  between processes.

  Module:
  Request

  @*/
int MPID_Probe(int, int, MPID_Comm *, int, MPI_Status *);
/*@
   MPID_Iprobe - Look for a matching request in the receive queue 
   but do not remove or return it

  Input Parameters:
+ source - rank to match (or 'MPI_ANY_SOURCE')
. tag - Tag to match (or 'MPI_ANY_TAG')
. comm - communicator to match.
- context_offset - context id offset of communicator to match

  Output Parameter:
+ flag - true if a matching request was found, false otherwise.
- status - 'MPI_Status' set as defined by 'MPI_Iprobe' (only valid when return 
  flag is true).

  Return Value:
  Error Code.

  Notes:
  Note that the values returned in 'status' will be valid for a subsequent
  MPI receive operation only if no other thread attempts to receive the same
  message.  
  (See the 
  discussion of probe in Section 8.7.2 (Clarifications) of the MPI-2 standard.)

  Providing the 'context_offset' is necessary at this level to support the 
  way in which the MPICH implementation uses context ids in the implementation
  of other operations.  The communicator is present to allow the device 
  to use message-queues attached to particular communicators or connections
  between processes.

  Devices that rely solely on polling to make progress should call
  MPID_Progress_poke() (or some equivalent function) if a matching request
  could not be found.  This insures that progress continues to be made even if
  the application is calling MPI_Iprobe() from within a loop not containing
  calls to any other MPI functions.
  
  Module:
  Request

  @*/
int MPID_Iprobe(int, int, MPID_Comm *, int, int *, MPI_Status *);

/*@
  MPID_Cancel_send - Cancel the indicated send request

  Input Parameter:
. request - Send request to cancel

  Return Value:
  MPI error code.
  
  Notes:
  Cancel is a tricky operation, particularly for sends.  Read the
  discussion in the MPI-1 and MPI-2 documents carefully.  This call
  only requests that the request be cancelled; a subsequent wait 
  or test must first succeed (i.e., the request completion counter must be
  zeroed).

  Module:
  Request

  @*/
int MPID_Cancel_send(MPID_Request *);
/*@
  MPID_Cancel_recv - Cancel the indicated recv request

  Input Parameter:
. request - Receive request to cancel

  Return Value:
  MPI error code.
  
  Notes:
  This cancels a pending receive request.  In many cases, this is implemented
  by simply removing the request from a pending receive request queue.  
  However, some ADI implementations may maintain these queues in special 
  places, such as within a NIC (Network Interface Card).
  This call only requests that the request be cancelled; a subsequent wait 
  or test must first succeed (i.e., the request completion counter must be
  zeroed).

  Module:
  Request

  @*/
int MPID_Cancel_recv(MPID_Request *);

int MPID_Win_create(void *, MPI_Aint, int, MPI_Info, MPID_Comm *,
                    MPID_Win **);
int MPID_Win_fence(int, MPID_Win *);
int MPID_Put(void *, int, MPI_Datatype, int, MPI_Aint, int,
            MPI_Datatype, MPID_Win *); 
int MPID_Get(void *, int, MPI_Datatype, int, MPI_Aint, int,
            MPI_Datatype, MPID_Win *);
int MPID_Accumulate(void *, int, MPI_Datatype, int, MPI_Aint, int, 
		   MPI_Datatype,  MPI_Op, MPID_Win *);
int MPID_Win_free(MPID_Win **); 
int MPID_Win_test(MPID_Win *win_ptr, int *flag);
int MPID_Win_wait(MPID_Win *win_ptr);
int MPID_Win_complete(MPID_Win *win_ptr);
int MPID_Win_post(MPID_Group *group_ptr, int assert, MPID_Win *win_ptr);
int MPID_Win_lock(int lock_type, int dest, int assert, MPID_Win *win_ptr);
int MPID_Win_unlock(int dest, MPID_Win *win_ptr);

/*@
  MPID_Progress_start - Begin a block of operations that check the completion
  counters in requests.

  Notes:
  This routine is used to inform the progress engine that a block of code will
  examine the completion counter of some 'MPID_Request' objects and then call
  one of 'MPID_Progress_end', 'MPID_Progress_wait', or 'MPID_Progress_test'.
  
  This routine is needed to properly implement blocking tests when 
  multithreaded progress engines are used.  In a single-threaded implementation
  of the ADI, this may be defined as an empty macro.

  Module:
  Communication
  @*/
void MPID_Progress_start(void);
/*@
   MPID_Progress_end - End a block of operations begun with MPID_Progress_start

   Notes: 
   This instructs the progress engine to end the block begun with 
   'MPID_Progress_start'.  The progress engine is not required to check for
   any pending communication.

   The purpose of this call is to release any locks initiated by 
   'MPID_Progess_start'.  It is typically used when checks of the 
   relevant request completion counters found a completed request.  In a single
   threaded ADI implementation, this may be defined as an empty macro.

  Module:
  Communication
   @*/
void MPID_Progress_end(void);
/*@
  MPID_Progress_test - Check for communication since 'MPID_Progress_start'

  Return value:
  An mpi error code.
  
  Notes:
  Like 'MPID_Progress_end' and 'MPID_Progress_wait', this completes the block
  begun with 'MPID_Progress_start'.  Unlike 'MPID_Progress_wait', it is a
  nonblocking call.  It returns the number of communication events, which
  is only indicates the maximum number of separate requests that were
  completed.  The only restriction is that if the completion status of any 
  request changed between 'MPID_Progress_start' and  'MPID_Progress_test',
  the return value must be at least one.

  This function used to return TRUE if one or more requests have completed, 
  FALSE otherwise.  This functionality was not used so we removed it.

  Module:
  Communication
  @*/
int MPID_Progress_test(void);
/*@ MPID_Progress_wait - Wait for some communication since 
    'MPID_Progress_start' 

    Return value:
    An mpi error code.

    Notes:
    This instructs the progress engine to wait until some communication event
    happens since 'MPID_Progress_start' was called.  This call blocks the 
    calling thread (only, not the process).  Before returning, it releases
    the block begun with 'MPID_Progress_start'.

  Module:
  Communication
 @*/
int MPID_Progress_wait(void);
/*@
  MPID_Progress_poke - Allow a progress engine to check for pending 
  communication

  Return value:
  An mpi error code.
  
  Notes:
  This routine provides a way to invoke the progress engine in a polling 
  implementation of the ADI.  This routine must be nonblocking.

  A multithreaded implementation is free to define this as an empty macro.

  Module:
  Communication
  @*/
int MPID_Progress_poke(void);

/*@
  MPID_Request_create - Create and return a bare request

  Return value:
  A pointer to a new request object.

  Notes:
  This routine is intended for use by 'MPI_Grequest_start' only.  Note that 
  once a request is created with this routine, any progress engine must assume 
  that an outside function can complete a request with 
  'MPID_Request_set_completed'.

  The request object returned by this routine should be initialized such that
  ref_count is one and handle contains a valid handle referring to the object.
  @*/
MPID_Request * MPID_Request_create(void);
void MPID_Request_set_completed(MPID_Request *);
/*@
  MPID_Request_release - Release a request 

  Input Parameter:
. request - request to release

  Notes:
  This routine is called to release a reference to request object.  If
  the reference count of the request object has reached zero, the object will
  be deallocated.

  Module:
  Request
@*/
void MPID_Request_release(MPID_Request *);

/*TTopoOverview.tex
 *
 * The MPI collective and topology routines can benefit from information 
 * about the topology of the underlying interconnect.  Unfortunately, there
 * is no best form for the representation (the MPI-1 Forum tried to define
 * such a representation, but was unable to).  One useful decomposition
 * that has been used in cluster enviroments is a hierarchical decomposition.
 *
 * The other obviously useful topology information would match the needs of 
 * 'MPI_Cart_create'.  However, it may be simpler to for the device to 
 * implement this routine directly.
 *
 * Other useful information could be the topology information that matches
 * the needs of the collective operation, such as spanning trees and rings.
 * These may be added to ADI3 later.
 *
 * Question: Should we define a cart create function?  Dims create?
 *
 * Usage:
 * This routine has nothing to do with the choice of communication method
 * that a implementation of the ADI may make.  It is intended only to
 * communicate information on the heirarchy of processes, if any, to 
 * the implementation of the collective communication routines.  This routine
 * may also be useful for the MPI Graph topology functions.
 *
 T*/

/*@
  MPID_Topo_cluster_info - Return information on the hierarchy of 
  interconnections

  Input Parameter:
. comm - Communicator to study.  May be 'NULL', in which case 'MPI_COMM_WORLD'
  is the effective communicator.

  Output Parameters:
+ levels - The number of levels in the hierarchy.  
  To simplify the use of this routine, the maximum value is 
  'MPID_TOPO_CLUSTER_MAX_LEVELS' (typically 8 or less).
. my_cluster - For each level, the id of the cluster that the calling process
  belongs to.
- my_rank - For each level, the rank of the calling process in its cluster

  Notes:
  This routine returns a description of the system in terms of nested 
  clusters of processes.  Levels are numbered from zero.  At each level,
  each process may belong to no more than cluster; if a process is in any
  cluster at level i, it must be in some cluster at level i-1.

  The communicator argument allows this routine to be used in the dynamic
  process case (i.e., with communicators that are created after 'MPI_Init' 
  and that involve processes that are not part of 'MPI_COMM_WORLD').

  For non-hierarchical systems, this routine simply returns a single 
  level containing all processes.

  Sample Outputs:
  For a single, switch-connected cluster or a uniform-memory-access (UMA)
  symmetric multiprocessor (SMP), the return values could be
.vb
    level       my_cluster         my_rank
    0           0                  rank in comm_world
.ve
  This is also a valid response for `any` device.

  For a switch-connected cluster of 2 processor SMPs
.vb
    level       my_cluster         my_rank
    0           0                  rank in comm_world
    1           0 to p/2           0 or 1
.ve
 where the value each process on the same SMP has the same value for
 'my_cluster[1]' and a different value for 'my_rank[1]'.

  For two SMPs connected by a network,
.vb
    level       my_cluster         my_rank
    0           0                  rank in comm_world
    1           0 or 1             0 to # on SMP
.ve

  An example with more than 2 levels is a collection of clusters, each with
  SMP nodes.  

  Limitations:
  This approach does not provide a representations for topologies that
  are not hierarchical.  For example, a mesh interconnect is a single-level
  cluster in this view.

  Module: 
  Topology
  @*/
int MPID_Topo_cluster_info( MPID_Comm *comm, 
			    int *levels, int my_cluster[], int my_rank[] );

/*@
  MPID_Get_processor_name - Return the name of the current processor

  Output Parameters:
+ name - A unique specifier for the actual (as opposed to virtual) node. This
  must be an array of size at least 'MPI_MAX_PROCESSOR_NAME'.
- resultlen - Length (in characters) of the name 

  Notes:
  The name returned should identify a particular piece of hardware; 
  the exact format is implementation defined.  This name may or may not
  be the same as might be returned by 'gethostname', 'uname', or 'sysinfo'.

  This routine is essentially an MPID version of 'MPI_Get_processor_name' .  
  It must be part of the device because not all environments support calls
  to return the processor name.
  @*/
int MPID_Get_processor_name( char *name, int *resultlen);

void MPID_Errhandler_free(MPID_Errhandler *errhan_ptr);

/*@
  MPID_VCRT_Create - Create a virtual connection reference table
  @*/
int MPID_VCRT_Create(int size, MPID_VCRT *vcrt_ptr);
/*@
  MPID_VCRT_Add_ref - Add a reference to a VCRT
  @*/
int MPID_VCRT_Add_ref(MPID_VCRT vcrt);
/*@
  MPID_VCRT_Release - Release a reference to a VCRT
  @*/
int MPID_VCRT_Release(MPID_VCRT vcrt);
/*@
  MPID_VCRT_Get_ptr - 
  @*/
int MPID_VCRT_Get_ptr(MPID_VCRT vcrt, MPID_VCR **vc_pptr);

/*@
  MPID_VCR_Dup - 
  @*/
int MPID_VCR_Dup(MPID_VCR orig_vcr, MPID_VCR * new_vcr);

int MPID_VCR_Release(MPID_VCR vcr);

/*@
   MPID_VCR_Get_lpid - Get the local process id that corresponds to a 
   virtual connection reference.

   Notes:
   The local process ids are described elsewhere.  Basically, they are
   a nonnegative number by which this process can refer to other processes 
   to which it is connected.  These are local process ids because different
   processes may use different ids to identify the same target process
  @*/
int MPID_VCR_Get_lpid(MPID_VCR vcr, int * lpid_ptr);

/* Debugger support */
#ifdef HAVE_DEBUGGER_SUPPORT
void MPIR_WaitForDebugger( void );
#endif

/* Include definitions from the device which require items defined by this file (mpiimpl.h). */
#include "mpidpost.h"

/* ------------------------------------------------------------------------- */
/* FIXME: Also for mpicoll.h, in src/mpi/coll?  */
/* ------------------------------------------------------------------------- */
/* thresholds to switch between long and short vector algorithms for
   collective operations */ 
/* FIXME: Should there be a way to (a) update/compute these at configure time
   and (b) provide runtime control?  Should these be MPIR_xxx_DEFAULT 
   instead? */
#define MPIR_BCAST_SHORT_MSG          12288
#define MPIR_BCAST_LONG_MSG           524288
#define MPIR_BCAST_MIN_PROCS          8
#define MPIR_ALLTOALL_SHORT_MSG       256
#define MPIR_ALLTOALL_MEDIUM_MSG      32768
#define MPIR_REDSCAT_COMMUTATIVE_LONG_MSG 524288
#define MPIR_REDSCAT_NONCOMMUTATIVE_SHORT_MSG 512
#define MPIR_ALLGATHER_SHORT_MSG      81920
#define MPIR_ALLGATHER_LONG_MSG       524288
#define MPIR_REDUCE_SHORT_MSG         2048
#define MPIR_ALLREDUCE_SHORT_MSG      2048
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
   MPID_CONTEXT_INTRA_COLL. */
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
int MPIC_Irecv(void *buf, int count, MPI_Datatype datatype, int
               source, int tag, MPI_Comm comm, MPI_Request *request);
int MPIC_Isend(void *buf, int count, MPI_Datatype datatype, int dest, int tag,
               MPI_Comm comm, MPI_Request *request);
int MPIC_Wait(MPID_Request * request_ptr);


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

int MPIR_MAXF_check_dtype  ( MPI_Datatype ) ;
int MPIR_MINF_check_dtype ( MPI_Datatype ) ;
int MPIR_SUM_check_dtype  ( MPI_Datatype ) ;
int MPIR_PROD_check_dtype  ( MPI_Datatype ) ;
int MPIR_LAND_check_dtype  ( MPI_Datatype ) ;
int MPIR_BAND_check_dtype  ( MPI_Datatype ) ;
int MPIR_LOR_check_dtype  ( MPI_Datatype ) ;
int MPIR_BOR_check_dtype  ( MPI_Datatype ) ;
int MPIR_LXOR_check_dtype ( MPI_Datatype ) ;
int MPIR_BXOR_check_dtype  ( MPI_Datatype ) ;
int MPIR_MAXLOC_check_dtype  ( MPI_Datatype ) ;
int MPIR_MINLOC_check_dtype  ( MPI_Datatype ) ;

#define MPIR_PREDEF_OP_COUNT 12
extern MPI_User_function *MPIR_Op_table[];

typedef int (MPIR_Op_check_dtype_fn) ( MPI_Datatype ); 
extern MPIR_Op_check_dtype_fn *MPIR_Op_check_dtype_table[];

#ifndef MPIR_MIN
#define MPIR_MIN(a,b) (((a)>(b))?(b):(a))
#endif
#ifndef MPIR_MAX
#define MPIR_MAX(a,b) (((b)>(a))?(b):(a))
#endif

int MPIR_Bcast_inter(void *buffer, int count, MPI_Datatype datatype, 
		     int root, MPID_Comm *comm_ptr);
int MPIR_Bcast (void *buffer, int count, MPI_Datatype datatype, int
                root, MPID_Comm *comm_ptr);
int MPIR_Gather (void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                 void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                 int root, MPID_Comm *comm_ptr);
int MPIR_Gatherv (void *sendbuf, int sendcnt, MPI_Datatype sendtype, 
                  void *recvbuf, int *recvcnts, int *displs,
                  MPI_Datatype recvtype, int root, MPID_Comm
                  *comm_ptr); 
int MPIR_Reduce_inter (void *sendbuf, void *recvbuf, int count, MPI_Datatype
                 datatype, MPI_Op op, int root, MPID_Comm *comm_ptr); 
int MPIR_Scatterv (void *sendbuf, int *sendcnts, int *displs,
                   MPI_Datatype sendtype, void *recvbuf, int recvcnt,
                   MPI_Datatype recvtype, int root, MPID_Comm
                   *comm_ptr );
int MPIR_Barrier( MPID_Comm *comm_ptr );

int MPIR_Setup_intercomm_localcomm( MPID_Comm * );

int MPIR_Comm_create( MPID_Comm *, MPID_Comm ** );

/* Collective functions cannot be called from multiple threads. These
   are stubs used in the collective communication calls to check for
   user error. Currently they are just being macroed out. */
#define MPIDU_ERR_CHECK_MULTIPLE_THREADS_ENTER(comm_ptr)
#define MPIDU_ERR_CHECK_MULTIPLE_THREADS_EXIT(comm_ptr)

#endif /* MPIIMPL_INCLUDED */
