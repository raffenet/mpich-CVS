/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#if !defined(MPITYPEDEFS_H_INCLUDED)
#define MPITYPEDEFS_H_INCLUDED

#include "mpichconf.h"

/* ------------------------------------------------------------------------- */
/* mpitypedefs.h */
/* ------------------------------------------------------------------------- */
/* Basic typedefs */
#ifdef HAVE_SYS_BITYPES_H
#include <sys/bitypes.h>
#endif

#ifndef HAVE_INT16_T 
#ifdef MPIU_INT16_T
typedef MPIU_INT16_T int16_t;
#else
#error 'Configure did not find a 16-bit integer type'
#endif
#endif

#ifndef HAVE_INT32_T
#ifdef MPIU_INT32_T
typedef MPIU_INT32_T int32_t;
#else
#error 'Configure did not find a 32-bit integer type'
#endif
#endif

#ifndef HAVE_INT64_T
#ifdef MPIU_INT64_T
typedef MPIU_INT64_T int64_t;
#else
/* Don't define a 64 bit integer type if we didn't find one, but 
   allow the code to compile as long as we don't need that type */
#endif
#endif

#ifdef HAVE_WINDOWS_H
#include <winsock2.h>
#include <windows.h>
#else
#ifndef BOOL
#define BOOL int
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#endif

#define MPIDU_MAX(a,b)    (((a) > (b)) ? (a) : (b))
#define MPIDU_MIN(a,b)    (((a) < (b)) ? (a) : (b))

#include "mpiiov.h"

typedef MPIU_SIZE_T MPIU_Size_t;

/* FIXME: These need a definition.  What are they for? */
/* These don't work.
   I don't think there is a legal way in C to convert a pointer to an integer
   without generating compiler warnings or errors.
 */
#ifdef HAVE_WINDOWS_H
#define POINTER_TO_AINT(a)   ( ( MPI_Aint )( 0xffffffff & (__int64) ( a ) ) )
#define POINTER_TO_OFFSET(a) ( ( MPI_Offset ) ( (__int64) ( a ) ) )
#else
#define POINTER_TO_AINT(a)   ( ( MPI_Aint )( a ) )
#define POINTER_TO_OFFSET(a) ( ( MPI_Offset ) ( a ) )
#endif


/* ------------------------------------------------------------------------- */
/* end of mpitypedefs.h */
/* ------------------------------------------------------------------------- */

#endif /* !defined(MPITYPEDEFS_H_INCLUDED) */
