/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
/* mpe_log.c - the externally callable functions in MPE_Log

   New version to use CLOG - Bill Gropp and Rusty Lusk

*/

#include <stdio.h>

#include "mpeconf.h"
#include "clog.h"
#include "clog_merge.h"
#include "mpi.h"		/* Needed for MPI routines */
#include "mpe_log.h"

#ifdef HAVE_STDLIB_H
/* Needed for getenv */
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

/* we want to use the PMPI routines instead of the MPI routines for all of 
   the logging calls internal to the mpe_log package */
#ifdef USE_PMPI
#define MPI_BUILD_PROFILING
#include "mpiprof.h"
#endif

/* temporarily borrowed from mpe_log_genproc.h */ /* try to replace by CLOG */
int    MPE_Log_hasBeenInit = 0;
int    MPE_Log_hasBeenClosed = 0;
int    MPE_Log_clockIsRunning = 0;
int    MPE_Log_isLockedOut = 0;
int    MPE_Log_AdjustedTimes = 0;
#define MPE_HAS_PROCID
int    MPE_Log_procid;
/* end of borrowing */

/*@
    MPE_Init_log - Initialize for logging

    Notes:
    Initializes the MPE logging package.  This must be called before any of
    the other MPE logging routines.  It is collective over 'MPI_COMM_WORLD'

.seealso: MPE_Finish_log
@*/
int MPE_Init_log()
{
    return MPE_Log_OK;
}

/*@
    MPE_Start_log - Begin logging of events
@*/
int MPE_Start_log()
{
  return MPE_Log_OK;
}

/*@
    MPE_Stop_log - Stop logging events
@*/
int MPE_Stop_log()
{
  return MPE_Log_OK;
}

/*@
  MPE_Initialized_logging - Indicate whether MPE_Init_log or MPE_Finish_log
  have been called.

  Returns:
  0 if MPE_Init_log has not been called, 1 if MPE_Init_log has been called
  but MPE_Finish_log has not been called, and 2 otherwise.
@*/
int MPE_Initialized_logging ()
{
    return MPE_Log_hasBeenInit + MPE_Log_hasBeenClosed;
}

/*@
    MPE_Describe_state - Create log record describing a state

    Input Parameters:
. start - event number for the start of the state
. end   - event number for the end of the state
. name  - Name of the state
. color - color to display the state in

    Notes:
    Adds string containing a state def to the logfile.  The format of the
    definition is (in ALOG)
.vb
    (LOG_STATE_DEF) 0 sevent eevent 0 0 "color" "name"
.ve
    States are added to a log file by calling 'MPE_Log_event' for the start and
    end event numbers.

.seealso: MPE_Log_get_event_number
@*/
int MPE_Describe_state( start, end, name, color )
int start, end;
char *name, *color;
{
    return MPE_Log_OK;
}

/*@
    MPE_Describe_event - Create log record describing an event type
    
    Input Parameters:
+   event - Event number
-   name  - String describing the event. 

.seealso: MPE_Log_get_event_number 
@*/
int MPE_Describe_event( event, name )
int event;
char *name;
{
    return MPE_Log_OK;
}

/*@
  MPE_Log_get_event_number - Gets an unused event number

  Returns:
  A value that can be provided to MPE_Describe_event or MPE_Describe_state
  which will define an event or state not used before.  

  Notes: 
  This routine is provided to allow packages to ensure that they are 
  using unique event numbers.  It relies on all packages using this
  routine.
@*/
int MPE_Log_get_event_number( )

{
    return CLOG_UNDEF;
}

/*@
    MPE_Log_send - Logs the sending of a message
@*/
int MPE_Log_send( otherParty, tag, size )
int otherParty, tag, size;
{
    return MPE_Log_OK;
}

/*@
    MPE_Log_receive - log the sending of a message
@*/
int MPE_Log_receive( otherParty, tag, size )
int otherParty, tag, size;
{
    return MPE_Log_OK;
}

/*@
    MPE_Log_event - Logs an event

    Input Parameters:
+   event - Event number
.   data  - Integer data value
-   string - Optional string describing event
@*/
int MPE_Log_event(event,data,string)
int event, data;
char *string;
{
    return MPE_Log_OK;
}

/*@
    MPE_Finish_log - Send log to master, who writes it out

    Notes:
    This routine dumps a logfile in alog or clog format.  It is collective over
    'MPI_COMM_WORLD'.  The default is alog format.  To generate clog output,
    set the environment variable MPE_LOG_FORMAT to CLOG.

@*/
int MPE_Finish_log( filename )
char *filename;
{
    return MPE_Log_OK;
}
