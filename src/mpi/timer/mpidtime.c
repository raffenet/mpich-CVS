
#include "timerconf.h"
#include "mpichtimer.h"
#include "mpiimpl.h"

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
void MPID_Wtime_acc( MPID_Time_t *t1,MPID_Time_t *t2, MPID_Time_t *t3 )
{
    *t3 += (*t2 - *t1);
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
    *val = ((double) t->tv_sec + 1.0e-9 * (double) t->tv_nsec );
}
void MPID_Wtime_acc( MPID_Time_t *t1,MPID_Time_t *t2, MPID_Time_t *t3 )
{
  int nsec, sec;

  nsec = t1->tv_nsec + t2->tv_nsec;
  sec  = t1->tv_sec + t2->tv_sec;
  if (nsec > 1.0e9) {
    nsec -= 1.0e9;
    sec++;
  }
  t3->sec = sec;
  t3->nsec = nsec;
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
    *val = (double) t->tv_sec + .000001 * (double) t->tv_usec;
}
void MPID_Wtime_acc( MPID_Time_t *t1,MPID_Time_t *t2, MPID_Time_t *t3 )
{
  int usec, sec;

  usec = t2->tv_usec - t1->tv_usec;
  sec  = t2->tv_sec - t1->tv_sec;
  t3->tv_usec += usec;
  t3->tv_sec += sec;
  if (t3->tv_usec > 1.0e6) {
    t3->tv_usec -= 1.0e6;
    t3->tv_sec++;
  }
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
void MPID_Wtime_acc( MPID_Time_t *t1,MPID_Time_t *t2, MPID_Time_t *t3 )
{
  *t3 += (*t2 - *t1);
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
void MPID_Wtime_acc( MPID_Time_t *t1,MPID_Time_t *t2, MPID_Time_t *t3 )
{
}
#elif MPICH_TIMER_KIND == USE_QUERYPERFORMANCECOUNTER
static double timer_frequency=0.0;  /* High performance counter frequency */
void MPID_Wtime_init(void)
{
    LARGE_INTEGER n;
    QueryPerformanceFrequency(&n);
    timer_frequency = (double)n.QuadPart;
}
double MPID_Wtick(void)
{
    return timer_frequency;
}
void MPID_Wtime( MPID_Time_t *timeval )
{
    QueryPerformanceCounter(timeval);
}
void MPID_Wtime_todouble( MPID_Time_t *t, double *val )
{
    *val = (double)t->QuadPart;
}
void MPID_Wtime_diff( MPID_Time_t *t1, MPID_Time_t *t2, double *diff )
{
    LARGE_INTEGER n;
    n.QuadPart = t2->QuadPart - t1->QuadPart;
    *diff = (double)n.QuadPart / timer_frequency;
}
void MPID_Wtime_acc( MPID_Time_t *t1,MPID_Time_t *t2, MPID_Time_t *t3 )
{
  /* ??? */
}
#endif


