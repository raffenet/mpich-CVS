/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef PMIMPL_H_INCLUDED
#define PMIMPL_H_INCLUDED

/* These definitions are used only within the implementation of the
   pm utility routines */

/* The following definitions are used to mark the beginning and ending
   of each routine.  This makes it easy to add debugging a profiling
   information.  These are similar to the macros used within the MPI
   implementation, but are indiependent so that they can work outside
   of an MPI implementation.  They also do not need to be as low-impact,
   since none of the pm routines are on a performance-critical path. */

#if 1
/* This is the debugging branch */
/*extern int PMUTIL_depth = 0;*/
#define PMUTIL_BEGIN(name) {printf( "starting %s\n", name ); fflush(stdout);}
#define PMUTIL_END(name)   {printf( "ending %s\n", name ); fflush(stdout);}
#define PMUTIL_CALL_BEGIN(from,name) {printf( "Calling %s from %s\n", name, from ); fflush(stdout);}
#define PMUTIL_CALL_END(from,name) {printf( "Returning from %s called from %s\n", name, from ); fflush(stdout);}
#else
/* This is the production branch */
#define PMUTIL_BEGIN(name)
#define PMUTIL_END(name)
#define PMUTIL_CALL_BEGIN(from,name)
#define PMUTIL_CALL_END(from,name)
#endif

#endif
