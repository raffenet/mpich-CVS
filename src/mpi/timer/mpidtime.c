
#include "timerconf.h"
#include "mpichtimer.h"

#if MPICH_TIMER_KIND == USE_GETHRTIME 
void MPID_Wtime( MPID_Time_t *timeval )
{
    *timeval = gethrtime();
}
void MPID_Wtime_diff( MPID_Time_t *t1, MPID_Time_t *t2, double *diff )
{
    *diff = 1.0e-9 * (double)( t2 - t1 );
}
void MPID_Wtime_todouble( MPID_Time_t *t, double *val )
{
    *val = 1.0e-9 * t;
}
#elif MPICH_TIMER_KIND == USE_CLOCK_GETTIME
void MPID_Wtime( MPID_Time_t *timeval )
{
    /* POSIX timer (14.2.1, page 311) */
    clock_gettime( CLOCK_REALTIME, timeval );
}
void MPID_Wtime_diff( MPID_Time_t *t1, MPID_Time_t *t2, double *diff )
{
    *diff = ((double) (t2->tv_sec - t1->tv_sec) + 
		1.0e-9 * (double) (t2->tv_nsec - t1->tv_nsec) );
}
void MPID_Wtime_todouble( MPID_Time_t *t, double *val )
{
}
#elif MPICH_TIMER_KIND == USE_GETTIMEOFDAY
void MPID_Wtime( MPID_Time_t *tval )
{
    struct timezone tzp;

    gettimeofday(tval,&tzp);
}
void MPID_Wtime_diff( MPID_Time_t *t1, MPID_Time_t *t2, double *diff )
{
    *diff = ((double) (t2->tv_sec - t1->tv_sec) + 
		.000001 * (double) (t2->tv_usec - t1->tv_usec) );
}
void MPID_Wtime_todouble( MPID_Time_t *t, double *val )
{
    *diff = (double) t->tv_sec + .000001 * (double) t->tv_usec;
}
#elif MPICH_TIMER_KIND == USE_LINUX86_CYCLE
/* Time stamps created by a macro */
void MPID_Wtime_diff( MPID_Time_t *t1, MPID_Time_t *t2, double *diff )
{
    *diff = (double)( *t2 - *t1 );
}
void MPID_Wtime_todouble( MPID_Time_t *t, double *val )
{
    /* This returns the number of cycles as the "time".  This isn't correct
       for implementing MPI_Wtime, but it does allow us to insert cycle
       counters into test programs */
    *val = (double)*t;
}
#elif MPICH_TIMER_KIND == USE_LINUXALPHA_CYCLE
/* Code from LinuxJournal #42 (Oct-97), p50; 
   thanks to Dave Covey dnc@gi.alaska.edu
   Untested
 */
    unsigned long cc
    asm volatile( "rpcc %0" : "=r"(cc) : : "memory" );
    /* Convert to time.  Scale cc by 1024 incase it would overflow a double;
       consider using long double as well */
void MPID_Wtime_diff( MPID_Time_t *t1, MPID_Time_t *t2, double *diff )
    *diff = 1024.0 * ((double)(cc/1024) / (double)CLOCK_FREQ_HZ);
void MPID_Wtime_todouble( MPID_Time_t *t, double *val )
{
}
#endif


