/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPICHTIMER_H
#define MPICHTIMER_H
/*
 * This include file provide the definitions that are necessary to use the
 * timer calls, including the definition of the time stamp type and 
 * any inlined timer calls.
 *
 * The include file timerconf.h (created by autoheader from configure.in)
 * is needed only to build the function versions of the timers.
 */
/* Include the appropriate files */
#define USE_GETHRTIME 1
#define USE_CLOCK_GETTIME 2
#define USE_GETTIMEOFDAY 3
#define USE_LINUX86_COUNTER 4
#define USE_LINUXALPHA_COUNTER 5
#define USE_QUERYPERFORMANCECOUNTER 6
#define USE_WIN86_CYCLE 7
#define MPICH_TIMER_KIND USE_WIN86_CYCLE

#if MPICH_TIMER_KIND == USE_GETHRTIME 
#include <sys/time.h>
#elif MPICH_TIMER_KIND == USE_CLOCK_GETTIME
#include <time.h>
#elif MPICH_TIMER_KIND == USE_GETTIMEOFDAY
#include <sys/types.h>
#include <sys/time.h>
#elif MPICH_TIMER_KIND == USE_LINUX86_CYCLE
#elif MPICH_TIMER_KIND == USE_LINUXALPHA_CYCLE
#elif MPICH_TIMER_KIND == USE_QUERYPERFORMANCECOUNTER
#include <winsock2.h>
#include <windows.h>
#elif MPICH_TIMER_KIND == USE_WIN86_CYCLE
#include <winsock2.h>
#include <windows.h>
#endif

/* Define a time stamp */
/*typedef LARGE_INTEGER MPID_Time_t;*/
typedef unsigned __int64 MPID_Time_t;

/* 
 * Prototypes.  These are defined here so that inlined timer calls can
 * use them, as well as any profiling and timing code that is built into
 * MPICH
 */
void MPID_Wtime( MPID_Time_t * );
void MPID_Wtime_diff( MPID_Time_t *, MPID_Time_t *, double * );
void MPID_Wtime_acc( MPID_Time_t *, MPID_Time_t *, MPID_Time_t * );
void MPID_Wtime_todouble( MPID_Time_t *, double * );
double MPID_Wtick( void );
void MPID_Wtime_init(void);

/* Inlined timers.  Note that any definition of one of the functions
   prototyped above in terms of a macro will simply cause the compiler
   to use the macro instead of the function definition.

   Currently, all except the Windows performance counter timers
   define MPID_Wtime_init as null; by default, the value of
   MPI_WTIME_IS_GLOBAL is false.
 */

/* MPIDM_Wtime_todouble() is a hack to get a macro version
   of the todouble function.  The logging library should
   save the native MPID_Timer_t structure to disk and
   use the todouble function in the post-processsing step
   to convert the values to doubles.
   */

#if MPICH_TIMER_KIND == USE_GETHRTIME 
#define MPID_Wtime_init()
#define MPIDM_Wtime_todouble MPID_Wtime_todouble

#elif MPICH_TIMER_KIND == USE_CLOCK_GETTIME
#define MPID_Wtime_init()
#define MPIDM_Wtime_todouble MPID_Wtime_todouble

#elif MPICH_TIMER_KIND == USE_GETTIMEOFDAY
#define MPID_Wtime_init()
#define MPIDM_Wtime_todouble MPID_Wtime_todouble

#elif MPICH_TIMER_KIND == USE_LINUX86_CYCLE
/* This is a new version of the cycle counter, based on the read time stamp
   (rdtsc) instruction */
/* There may be a better way to read the two 32 bit values into a single
   long long, but this should work */
#define MPID_Wtime(var_ptr) \
{ \
    __asm__ __volatile__  ( "cpuid ; rdtsc ; mov %%edx,%1 ; mov %%eax,%0" \
                            : "=m" (*((char *) (var_ptr))), \
                              "=m" (*(((char *) (var_ptr))+4)) \
                            :: "eax", "ebx", "ecx", "edx" ); \
}
extern double g_timer_frequency;
#define MPIDM_Wtime_todouble(t, d) *d = (double)*t / g_timer_frequency
#if 0
#define MPIDM_Wtime_todouble(t, d) *d = (double)*t
#endif

#elif MPICH_TIMER_KIND == USE_LINUXALPHA_CYCLE
#define MPID_Wtime_init()

#elif MPICH_TIMER_KIND == USE_QUERYPERFORMANCECOUNTER
#define MPID_Wtime(var) QueryPerformanceCounter(var)
extern double g_timer_frequency;
#define MPIDM_Wtime_todouble( var, t ) *t = (double)var->QuadPart / g_timer_frequency /* convert to seconds */
/*#define MPIDM_Wtime_todouble( var, t ) *t = (double)var->QuadPart */ /* don't convert to seconds */

#elif MPICH_TIMER_KIND == USE_WIN86_CYCLE
#define MPID_Wtime(var_ptr) \
{ \
    register int *f1 = (int*)var_ptr; \
    __asm cpuid \
    __asm rdtsc \
    __asm mov ecx, f1 \
    __asm mov [ecx], eax \
    __asm mov [ecx + TYPE int], edx \
}
extern double g_timer_frequency;
#define MPIDM_Wtime_todouble(t, d) *d = (double)(__int64)*t / g_timer_frequency
#define MPIDM_Wtime_diff(t1,t2,diff) *diff = (double)((__int64)( *t2 - *t1 )) / g_timer_frequency

#endif

#endif
