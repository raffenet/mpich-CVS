/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIMEM_H_INCLUDED
#define MPIMEM_H_INCLUDED
/* ------------------------------------------------------------------------- */
/* mpimem.h */
/* ------------------------------------------------------------------------- */
/* Memory allocation */
/* style: allow:malloc:2 sig:0 */
/* style: allow:free:2 sig:0 */
/* style: allow:strdup:2 sig:0 */
/* style: allow:calloc:2 sig:0 */
/* style: define:__strdup:1 sig:0 */
/* style: define:strdup:1 sig:0 */
/* style: allow:fprintf:5 sig:0 */   /* For handle debugging ONLY */
/* style: allow:snprintf:1 sig:0 */

/*D
  Memory - Memory Management Routines

  Rules for memory management:

  MPICH explicity prohibits the appearence of 'malloc', 'free', 
  'calloc', 'realloc', or 'strdup' in any code implementing a device or 
  MPI call (of course, users may use any of these calls in their code).  
  Instead, you must use 'MPIU_Malloc' etc.; if these are defined
  as 'malloc', that is allowed, but an explicit use of 'malloc' instead of
  'MPIU_Malloc' in the source code is not allowed.  This restriction is
  made to simplify the use of portable tools to test for memory leaks, 
  overwrites, and other consistency checks.

  Most memory should be allocated at the time that 'MPID_Init' is 
  called and released with 'MPID_Finalize' is called.  If at all possible,
  no other MPID routine should fail because memory could not be allocated
  (for example, because the user has allocated large arrays after 'MPI_Init').
  
  The implementation of the MPI routines will strive to avoid memory allocation
  as well; however, operations such as 'MPI_Type_index' that create a new
  data type that reflects data that must be copied from an array of arbitrary
  size will have to allocate memory (and can fail; note that there is an
  MPI error class for out-of-memory).

  Question:
  Do we want to have an aligned allocation routine?  E.g., one that
  aligns memory on a cache-line.
  D*/

/* Define the string copy and duplication functions */
/* Safer string routines */
int MPIU_Strncpy( char *outstr, const char *instr, size_t maxlen );

int MPIU_Strnapp( char *, const char *, size_t );
char *MPIU_Strdup( const char * );

/* ---------------------------------------------------------------------- */

#define MPIU_STR_SUCCESS    0
#define MPIU_STR_FAIL      -1
#define MPIU_STR_NOMEM      1

#define MPIU_TRUE  1
#define MPIU_FALSE 0

typedef int MPIU_BOOL;

#ifdef USE_HUMAN_READABLE_TOKENS

#define MPIU_STR_QUOTE_CHAR     '\"'
#define MPIU_STR_QUOTE_STR      "\""
#define MPIU_STR_DELIM_CHAR     '='
#define MPIU_STR_DELIM_STR      "="
#define MPIU_STR_ESCAPE_CHAR    '\\'
#define MPIU_STR_HIDE_CHAR      '*'
#define MPIU_STR_SEPAR_CHAR     ' '
#define MPIU_STR_SEPAR_STR      " "

#else

#define MPIU_STR_QUOTE_CHAR     '\"'
#define MPIU_STR_QUOTE_STR      "\""
#define MPIU_STR_DELIM_CHAR     '#'
#define MPIU_STR_DELIM_STR      "#"
#define MPIU_STR_ESCAPE_CHAR    '\\'
#define MPIU_STR_HIDE_CHAR      '*'
#define MPIU_STR_SEPAR_CHAR     '$'
#define MPIU_STR_SEPAR_STR      "$"

#endif

int MPIU_Str_get_string_arg(const char *str, const char *key, char *val, 
			    int maxlen);
int MPIU_Str_get_binary_arg(const char *str, const char *key, char *buffer, 
			    int maxlen);
int MPIU_Str_get_int_arg(const char *str, const char *key, int *val_ptr);
int MPIU_Str_add_string_arg(char **str_ptr, int *maxlen_ptr, const char *key, 
			    const char *val);
int MPIU_Str_add_binary_arg(char **str_ptr, int *maxlen_ptr, const char *key, 
			    const char *buffer, int length);
int MPIU_Str_add_int_arg(char **str_ptr, int *maxlen_ptr, const char *key, 
			 int val);
MPIU_BOOL MPIU_Str_hide_string_arg(char *str, const char *key);
int MPIU_Str_add_string(char **str_ptr, int *maxlen_ptr, const char *val);
int MPIU_Str_get_string(char **str_ptr, char *val, int maxlen);

/* ------------------------------------------------------------------------- */

#if 0
/*  
   These definitions were made for historical reasons; 
   earlier versions of MPICH made use of a tracing memory 
   package.  The current developers prefer to use other approaches.
*/
#define MPIU_Malloc(a)    malloc((unsigned)(a))
#define MPIU_Calloc(a,b)  calloc((unsigned)(a),(unsigned)(b))
#define MPIU_Free(a)      free((void *)(a))
#ifdef HAVE_STRDUP
/* Watch for the case where strdup is defined as a macro by a header include */
# if defined(NEEDS_STRDUP_DECL) && !defined(strdup)
extern char *strdup( const char * );
# endif
#define MPIU_Strdup(a)    strdup(a)
#else
/* Don't define MPIU_Strdup, provide it in safestr.c */
#endif /* HAVE_STRDUP */

#endif /* 0 */

#ifdef USE_MEMORY_TRACING
/*M
  MPIU_Malloc - Allocate memory

  Synopsis:
.vb
  void *MPIU_Malloc( size_t len )
.ve

  Input Parameter:
. len - Length of memory to allocate in bytes

  Return Value:
  Pointer to allocated memory, or null if memory could not be allocated.

  Notes:
  This routine will often be implemented as the simple macro
.vb
  #define MPIU_Malloc(n) malloc(n)
.ve
  However, it can also be defined as 
.vb
  #define MPIU_Malloc(n) MPIU_trmalloc(n,__FILE__,__LINE__)
.ve
  where 'MPIU_trmalloc' is a tracing version of 'malloc' that is included with 
  MPICH.

  Module:
  Utility
  M*/

#define MPIU_Malloc(a)    MPIU_trmalloc((unsigned)(a),__LINE__,__FILE__)
/*M
  MPIU_Calloc - Allocate memory that is initialized to zero.

  Synopsis:
.vb
    void *MPIU_Calloc( size_t nelm, size_t elsize )
.ve

  Input Parameters:
+ nelm - Number of elements to allocate
- elsize - Size of each element.

  Notes:
  Like 'MPIU_Malloc' and 'MPIU_Free', this will often be implemented as a 
  macro but may use 'MPIU_trcalloc' to provide a tracing version.

  Module:
  Utility
  M*/
#define MPIU_Calloc(a,b)  \
    MPIU_trcalloc((unsigned)(a),(unsigned)(b),__LINE__,__FILE__)

/*M
  MPIU_Free - Free memory

  Synopsis:
.vb
   void MPIU_Free( void *ptr )
.ve

  Input Parameter:
. ptr - Pointer to memory to be freed.  This memory must have been allocated
  with 'MPIU_Malloc'.

  Notes:
  This routine will often be implemented as the simple macro
.vb
  #define MPIU_Free(n) free(n)
.ve
  However, it can also be defined as 
.vb
  #define MPIU_Free(n) MPIU_trfree(n,__FILE__,__LINE__)
.ve
  where 'MPIU_trfree' is a tracing version of 'free' that is included with 
  MPICH.

  Module:
  Utility
  M*/
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

/* FIXME: Note that some of these prototypes are for old functions in the 
   src/util/mem/trmem.c package, and are no longer used.  Also, 
   it may be preferable to use trmem.h instead of these definitions */
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

#else
/* No memory tracing; just use native functions */
#define MPIU_Malloc(a)    malloc((unsigned)(a))
#define MPIU_Calloc(a,b)  calloc((unsigned)(a),(unsigned)(b))
#define MPIU_Free(a)      free((void *)(a))
#ifdef HAVE_STRDUP
/* Watch for the case where strdup is defined as a macro by a header include */
# if defined(NEEDS_STRDUP_DECL) && !defined(strdup)
extern char *strdup( const char * );
# endif
#define MPIU_Strdup(a)    strdup(a)
#else
/* Don't define MPIU_Strdup, provide it in safestr.c */
#endif /* HAVE_STRDUP */
#endif /* USE_MEMORY_TRACING */



#if 1
/* Memory allocation macros. See document. */

/* You can redefine this to indicate whether memory allocation errors
   are fatal */
#define MPID_CHKMEM_ISFATAL 1

/* Memory used and freed within the current scopy (alloca if feasible) */
#ifdef HAVE_ALLOCA
#define MPID_CHKLMEM_DECL(_n)
#define MPID_CHKLMEM_FREEALL
#define MPID_CHKLMEM_MALLOC_ORJUMP(_pointer,_type,_nbytes,_rc,_name,_label) \
{_pointer = (_type)MPID_Malloc(_nbytes); \
if (!(_pointer)) { \
    _rc = MPIR_Err_create_code( MPI_SUCCESS, MPID_CHKMEM_ISFATAL,  \
          FCNAME, __LINE__, \
          MPI_ERR_OTHER, "**nomem2", "**nomem2 %d %s", \
         _nbytes, _name ); \
    goto _label; \
}
#else
#define MPID_CHKLMEM_DECL(_n) \
 void *(_mpid_chklmem_stk[_n]);\
 int _mpid_chklmem_stk_sp=0;
#define MPID_CHKLMEM_MALLOC_ORJUMP(_pointer,_type,_nbytes,_rc,_name,_label) \
{_pointer = (_type)MPID_Malloc(_nbytes); \
if (_pointer) { \
    _mpid_chklmem_stk[_mpid_chklmem_stk_sp++] = _pointer;\
} else {\
    _rc = MPIR_Err_create_code( MPI_SUCCESS, MPID_CHKMEM_ISFATAL, \
          FCNAME, __LINE__, \
          MPI_ERR_OTHER, "**nomem2", "**nomem2 %d %s", \
         _nbytes, _name ); \
    goto _label; \
}
#define MPID_CHKLMEM_FREEALL \
    { while (_mpid_chklmem_stk_sp > 0) {\
       MPID_Free( _mpid_chklmem_stk[--_mpid_chklmem_stk_sp] ); } }
#endif /* HAVE_ALLOCA */
#define MPID_CHKLMEM_MALLOC(_pointer,_type,_nbytes,_rc,_name) \
    MPID_CHKLMEM_MALLOC_ORJUMP(_pointer,_type,_nbytes,_rc,_name,fn_fail)

/* Persistent memory that we may want to recover if something goes wrong */
#define MPID_CHKPMEM_DECL(_n) \
 void *(_mpid_chkpmem_stk[_n]);\
 int _mpid_chkpmem_stk_sp=0;
#define MPID_CHKPMEM_MALLOC_ORJUMP(_pointer,_type,_nbytes,_rc,_name,_label) \
{_pointer = (_type)MPID_Malloc(_nbytes); \
if (_pointer) { \
    _mpid_chkpmem_stk[_mpid_chkpmem_stk_sp++] = _pointer;\
} else {\
    _rc = MPIR_Err_create_code( MPI_SUCCESS, 1, FCNAME, __LINE__, \
          MPI_ERR_OTHER, "**nomem2", "**nomem2 %d %s", \
         _nbytes, _name ); \
    goto _label; \
}
#define MPID_CHKPMEM_REAP \
    { while (_mpid_chkpmem_stk_sp > 0) {\
       MPID_Free( _mpid_chkpmem_stk[--_mpid_chkpmem_stk_sp] ); } }
#define MPID_CHKPMEM_COMMIT \
    _mpid_chkpmem_stk_sp = 0
#define MPID_CHKPMEM_MALLOC(_pointer,_type,_nbytes,_rc,_name) \
    MPID_CHKPMEM_MALLOC_ORJUMP(_pointer,_type,_nbytes,_rc,_name,fn_fail)

#endif 

#if 0
/* Memory allocation stack.
   These are used to allocate multiple chunks of memory (with MPIU_Malloc)
   and ensuring that they are all freed.  This simplifies error handling
   for routines that may need to allocate multiple temporaries, and works
   on systems without alloca (allocate off of the routine's stack) */
#define MAX_MEM_STACK 16
typedef struct MPIU_Mem_stack { int n_alloc; void *ptrs[MAX_MEM_STACK]; } MPIU_Mem_stack;
#define MALLOC_STK(n,a) {a=MPIU_Malloc(n);\
               if (memstack.n_alloc >= MAX_MEM_STACK) abort(implerror);\
               memstack.ptrs[memstack.n_alloc++] = a;}
#define FREE_STK     {int i; for (i=memstack.n_alloc-1;i>=0;i--) {\
               MPIU_Free(memstack.ptrs[i]);}}
#define MALLOC_STK_INIT memstack.n_alloc = 0
#define MALLOC_STK_DECL MPIU_Mem_stack memstack
#endif

/* Utilities: Safe string copy and sprintf */
int MPIU_Strncpy( char *, const char *, size_t );

/* Provide a fallback snprintf for systems that do not have one */
/* Define attribute as empty if it has no definition */
#ifndef ATTRIBUTE
#define ATTRIBUTE(a)
#endif
#ifdef HAVE_SNPRINTF
#define MPIU_Snprintf snprintf
/* Sometimes systems don't provide prototypes for snprintf */
#ifdef NEEDS_SNPRINTF_DECL
extern int snprintf( char *, size_t, const char *, ... ) ATTRIBUTE((format(printf,3,4)));
#endif
#else
int MPIU_Snprintf( char *str, size_t size, const char *format, ... ) 
     ATTRIBUTE((format(printf,3,4)));
#endif /* HAVE_SNPRINTF */

/* ------------------------------------------------------------------------- */
/* end of mpimem.h */
/* ------------------------------------------------------------------------- */
#endif
