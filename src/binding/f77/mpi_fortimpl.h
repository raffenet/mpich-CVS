/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef NO_FORTCONF_H
#include "mpi_fortconf.h"
#endif

#ifdef USE_FORT_STDCALL
#define FORT_CALL __stdcall
#elif defined (USE_FORT_CDECL)
#define FORT_CALL __cdecl
#else
#define FORT_CALL
#endif

#ifdef USE_FORT_MIXED_STR_LEN
#define FORT_MIXED_LEN_DECL   , MPI_Fint
#define FORT_END_LEN_DECL
#define FORT_MIXED_LEN(a)     , MPI_Fint a
#define FORT_END_LEN(a)
#else
#define FORT_MIXED_LEN_DECL
#define FORT_END_LEN_DECL     , MPI_Fint
#define FORT_MIXED_LEN(a)
#define FORT_END_LEN(a)       , MPI_Fint a
#endif

#ifdef HAVE_FORTRAN_API
# ifdef FORTRAN_EXPORTS
#  define FORTRAN_API __declspec(dllexport)
# else
#  define FORTRAN_API __declspec(dllimport)
# endif
#else
# define FORTRAN_API
#endif

#ifdef USE_WEAK_ATTRIBUTE
#define FUNC_ATTRIBUTES __attribute__ weak
#else
#define FUNC_ATTRIBUTES
#endif

/* mpi.h includes the definitions of MPI_Fint */
#include "mpi.h"

/* Utility functions */

/* Define the internal values needed for Fortran support */

/* Fortran logicals */

/* Fortran logical values */
#ifndef _CRAY
#if !defined(F77_RUNTIME_VALUES) && defined(F77_TRUE_VALUE_SET)
extern const MPI_Fint MPIR_F_TRUE, MPIR_F_FALSE;
#else
extern MPI_Fint MPIR_F_TRUE, MPIR_F_FALSE;
#endif
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

/* If Cray-style pointers are supported, we don't need to check for a 
   "special" address. */
#ifdef USE_POINTER_FOR_BOTTOM
#define MPIR_F_PTR(a) (a)
#else
/* MPIR_F_MPI_BOTTOM is the address of the Fortran MPI_BOTTOM value */
extern void *MPIR_F_MPI_BOTTOM;

/* MPIR_F_PTR checks for the Fortran MPI_BOTTOM and provides the value 
   MPI_BOTTOM if found 
   See src/pt2pt/addressf.c for why MPIR_F_PTR(a) is just (a)
*/
/*  #define MPIR_F_PTR(a) (((a)==(MPIR_F_MPI_BOTTOM))?MPI_BOTTOM:a) */
#define MPIR_F_PTR(a) (a)
#endif

/*  
 * These are hooks for Fortran characters.
 * MPID_FCHAR_T is the type of a Fortran character argument
 * MPID_FCHAR_LARG is the "other" argument that some Fortran compilers use
 * MPID_FCHAR_STR gives the pointer to the characters
 */
#ifdef MPID_CHARACTERS_ARE_CRAYPVP
typedef <whatever> MPID_FCHAR_T;
#define MPID_FCHAR_STR(a) (a)->characters   <or whatever>
#define MPID_FCHAR_LARG(d) 
#else
typedef char *MPID_FCHAR_T;
#define MPID_FCHAR_STR(a) a
#define MPID_FCHAR_LARG(d) ,d
#endif

#ifndef MPIR_FALLOC
#define MPIR_FALLOC(ptr,expr,a,b,c) \
    if (! (ptr = (expr))) { MPIR_ERROR(a,b,c); }
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

/* Temporary patch for the space routines.  Eventually, this should use
   (FIXME) *just* the memory definitions currently in mpiimpl.h */
#define MPIU_Malloc(a)    malloc((unsigned)(a))
#define MPIU_Calloc(a,b)  calloc((unsigned)(a),(unsigned)(b))
#define MPIU_Free(a)      free((void *)(a))

#ifndef MPIR_USE_LOCAL_ARRAY
#define MPIR_USE_LOCAL_ARRAY 32
#endif




