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
/*@ MPIU_Strncpy - Copy a string with a maximum length
  
    Input Parameters:
+   instr - String to copy
-   maxlen - Maximum total length of 'outstr'

    Output Parameter:
.   outstr - String to copy into

    Notes:
    This routine is the routine that you wish 'strncpy' was.  In copying 
    'instr' to 'outstr', it stops when either the end of 'outstr' (the 
    null character) is seen or the maximum length 'maxlen' is reached.
    Unlike 'strncpy', it does not add enough nulls to 'outstr' after 
    copying 'instr' in order to move precisely 'maxlen' characters.  
    Thus, this routine may be used anywhere 'strcpy' is used, without any
    performance cost related to large values of 'maxlen'.

  Module:
  Utility
  @*/
int MPIU_Strncpy( char *outstr, const char *instr, size_t maxlen );

/*@ MPIU_Strnapp - Append to a string with a maximum length

    Input Parameters:
+   instr - String to copy
-   maxlen - Maximum total length of 'outstr'

    Output Parameter:
.   outstr - String to copy into

    Notes:
    This routine is similar to 'strncat' except that the 'maxlen' argument
    is the maximum total length of 'outstr', rather than the maximum 
    number of characters to move from 'instr'.  Thus, this routine is
    easier to use when the declared size of 'instr' is known.

  Module:
  Utility
  @*/
int MPIU_Strnapp( char *, const char *, size_t );

/*@ 
  MPIU_Strdup - Duplicate a string

  Synopsis:
.vb
    char *MPIU_Strdup( const char *str )
.ve

  Input Parameter:
. str - null-terminated string to duplicate

  Return value:
  A pointer to a copy of the string, including the terminating null.  A
  null pointer is returned on error, such as out-of-memory.

  Notes:
  Like 'MPIU_Malloc' and 'MPIU_Free', this will often be implemented as a 
  macro but may use 'MPIU_trstrdup' to provide a tracing version.

  Module:
  Utility
  @*/
char *MPIU_Strdup( const char * );



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

/*@ MPIU_Str_get_string_arg - Extract an option from a string with a maximum length
  
    Input Parameters:
+   str - Source string
.   key - key
-   maxlen - Maximum total length of 'val'

    Output Parameter:
.   val - output string

    Return value:
    MPIU_STR_SUCCESS, MPIU_STR_NOMEM, MPIU_STR_FAIL

    Notes:
    This routine searches for a "key = value" entry in a string

  Module:
  Utility
  @*/
int MPIU_Str_get_string_arg(const char *str, const char *key, char *val, int maxlen);

/*@ MPIU_Str_get_binary_arg - Extract an option from a string with a maximum length
  
    Input Parameters:
+   str - Source string
.   key - key
-   maxlen - Maximum total length of 'buffer'

    Output Parameter:
.   buffer - output buffer

    Return value:
    MPIU_STR_SUCCESS, MPIU_STR_NOMEM, MPIU_STR_FAIL

    Notes:
    This routine searches for a "key = value" entry in a string and decodes the value
    back to binary data.  The data must have been encoded with MPIU_Str_add_binary_arg.

  Module:
  Utility
  @*/
int MPIU_Str_get_binary_arg(const char *str, const char *key, char *buffer, int maxlen);

/*@ MPIU_Str_get_int_arg - Extract an option from a string
  
    Input Parameters:
+   str - Source string
-   key - key

    Output Parameter:
.   val_ptr - pointer to the output integer

    Return value:
    MPIU_STR_SUCCESS, MPIU_STR_NOMEM, MPIU_STR_FAIL

    Notes:
    This routine searches for a "key = value" entry in a string and decodes the value
    back to an int.

  Module:
  Utility
  @*/
int MPIU_Str_get_int_arg(const char *str, const char *key, int *val_ptr);

/*@ MPIU_Str_add_string_arg - Add an option to a string with a maximum length
  
    Input Parameters:
+   str_ptr - Pointer to the destination string
.   maxlen_ptr - Pointer to the maximum total length of '*str_ptr'
.   key - key
-   val - input string

    Output Parameter:
+   str_ptr - The string pointer is updated to the next available location in the string
-   maxlen_ptr - maxlen is reduced by the number of characters written

    Return value:
    MPIU_STR_SUCCESS, MPIU_STR_NOMEM, MPIU_STR_FAIL

    Notes:
    This routine adds a string option to a string in the form "key = value".

  Module:
  Utility
  @*/
int MPIU_Str_add_string_arg(char **str_ptr, int *maxlen_ptr, const char *key, const char *val);

/*@ MPIU_Str_add_binary_arg - Add an option to a string with a maximum length
  
    Input Parameters:
+   str_ptr - Pointer to the destination string
.   maxlen_ptr - Pointer to the maximum total length of '*str_ptr'
.   key - key
.   val - input data
-   length - length of the input data

    Output Parameter:
+   str_ptr - The string pointer is updated to the next available location in the string
-   maxlen_ptr - maxlen is reduced by the number of characters written

    Return value:
    MPIU_STR_SUCCESS, MPIU_STR_NOMEM, MPIU_STR_FAIL

    Notes:
    This routine encodes binary data into a string option in the form "key = encoded_value".

  Module:
  Utility
  @*/
int MPIU_Str_add_binary_arg(char **str_ptr, int *maxlen_ptr, const char *key, const char *buffer, int length);

/*@ MPIU_Str_add_int_arg - Add an option to a string with a maximum length
  
    Input Parameters:
+   str_ptr - Pointer to the destination string
.   maxlen_ptr - Pointer to the maximum total length of '*str_ptr'
.   key - key
-   val - input integer

    Output Parameter:
+   str_ptr - The string pointer is updated to the next available location in the string
-   maxlen_ptr - maxlen is reduced by the number of characters written

    Return value:
    MPIU_STR_SUCCESS, MPIU_STR_NOMEM, MPIU_STR_FAIL

    Notes:
    This routine adds an integer option to a string in the form "key = value".

  Module:
  Utility
  @*/
int MPIU_Str_add_int_arg(char **str_ptr, int *maxlen_ptr, const char *key, int val);

/*@ MPIU_Str_hide_string_arg - Over-write the value of an string option with * characters
  
    Input Parameters:
+   str - input string
-   key - key

    Output Parameter:
.   str - The string data is modified if the key is found in the string

    Return value:
    MPIU_STR_SUCCESS, MPIU_STR_FAIL

    Notes:
    This routine covers an option in a string converting "key = value" to "key = *****"

  Module:
  Utility
  @*/
MPIU_BOOL MPIU_Str_hide_string_arg(char *str, const char *key);

/*@ MPIU_Str_add_string - Add a string to a string
  
    Input Parameters:
+   str_ptr - pointer to the destination string
.   maxlen_ptr - pointer to the maximum length of '*str_ptr'
-   val - string to add

    Output Parameter:
+   str_ptr - The string pointer is updated to the next available location in the string
-   maxlen_ptr - maxlen is decremented by the amount str_ptr is incremented

    Return value:
    MPIU_STR_SUCCESS, MPIU_STR_NOMEM, MPIU_STR_FAIL

    Notes:
    This routine adds a string to a string in such a way that MPIU_Str_get_string can
    retreive the same string back.  It takes into account spaces and quote characters.
    The string pointer is updated to the start of the next string in the string and maxlen
    is updated accordingly.

  Module:
  Utility
  @*/
int MPIU_Str_add_string(char **str_ptr, int *maxlen_ptr, const char *val);

/*@ MPIU_Str_get_string - Get the next string from a string
  
    Input Parameters:
+   str_ptr - pointer to the destination string
-   maxlen_ptr - pointer to the maximum length of '*str_ptr'

    Output Parameter:
+   str_ptr - location of the next string
-   val - location to store the string

    Return value:
    MPIU_STR_SUCCESS, MPIU_STR_NOMEM, MPIU_STR_FAIL

    Return Value:
    The return value is 0 for success, -1 for insufficient buffer space, and 1 for failure.

    Notes:
    This routine gets a string that was previously added by MPIU_Str_add_string.
    It takes into account spaces and quote characters. The string pointer is updated to the
    start of the next string in the string.

  Module:
  Utility
  @*/
int MPIU_Str_get_string(char **str_ptr, char *val, int maxlen);

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
   src/util/mem/trmem.c package, and are no longer used */
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
#endif

/* ------------------------------------------------------------------------- */
/* end of mpimem.h */
/* ------------------------------------------------------------------------- */
#endif
