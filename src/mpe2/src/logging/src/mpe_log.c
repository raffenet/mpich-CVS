/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
/* mpe_log.c - the externally callable functions in MPE_Log

   New version to use CLOG - Bill Gropp and Rusty Lusk

*/

#include <stdio.h>

#include "mpe_logging_conf.h"
#include "clog.h"
#include "clog_merge.h"
#include "mpe_log.h"
#include "mpi.h"                /* Needed for PMPI routines */

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

    Notes:
    MPE_Init_log() & MPE_Finish_log() are NOT needed when liblmpe.a is linked
    because MPI_Init() would have called MPE_Init_log() already.
    liblmpe.a will be included in the final executable if it is linked with
    mpicc -mpe=log

    Returns:
    Always return MPE_Log_OK.

.seealso: MPE_Finish_log
@*/
int MPE_Init_log( void )
{

    if (!MPE_Log_hasBeenInit || MPE_Log_hasBeenClosed) {
        PMPI_Comm_rank( MPI_COMM_WORLD, &MPE_Log_procid ); /* get process ID */
        CLOG_Init();
        CLOG_LOGCOMM(INIT, -1, (int) MPI_COMM_WORLD);
        MPE_Log_hasBeenInit = 1;        /* set MPE_Log as being initialized */
        MPE_Log_hasBeenClosed = 0;
        MPE_Log_isLockedOut = 0;
        if ( MPE_Log_procid == 0 ) {
            CLOG_LOGCONST( CLOG_CONST_DEF, MPI_PROC_NULL, "MPI_PROC_NULL" );
            CLOG_LOGCONST( CLOG_CONST_DEF, MPI_ANY_SOURCE, "MPI_ANY_SOURCE" );
            CLOG_LOGCONST( CLOG_CONST_DEF, MPI_ANY_TAG, "MPI_ANY_TAG" );
        }
    }
    return MPE_Log_OK;
}

/*@
    MPE_Start_log - Begin logging of events
@*/
int MPE_Start_log( void )
{
    if (!MPE_Log_hasBeenInit)
        return MPE_Log_NOT_INITIALIZED;
    CLOG_status = 0;
    MPE_Log_isLockedOut = 0;
    return MPE_Log_OK;
}

/*@
    MPE_Stop_log - Stop logging events
@*/
int MPE_Stop_log( void )
{
    if (!MPE_Log_hasBeenInit)
        return MPE_Log_NOT_INITIALIZED;
    MPE_Log_isLockedOut = 1;
    CLOG_status = 1;
    return MPE_Log_OK;
}

/*@
  MPE_Initialized_logging - Indicate whether MPE_Init_log or MPE_Finish_log
  have been called.

  Returns:
  0 if MPE_Init_log has not been called, 1 if MPE_Init_log has been called
  but MPE_Finish_log has not been called, and 2 otherwise.
@*/
int MPE_Initialized_logging( void )
{
    return MPE_Log_hasBeenInit + MPE_Log_hasBeenClosed;
}

/*N MPE_LOG_BYTE_FORMAT
  Notes on storage format control support:
    The format control string is printf like, e.g. "Comment = %s".
    All the MPE %-token storage support is provided by SLOG-2.  That is
    whatever supported by SLOG-2 will be supported by MPE.  Currently,
    the following is supported.

    %s : variable length string, byte buffer size is length of string + 2.

    %h : 2-byte integer, printed as decimal integer, byte buffer size is 2.

    %d : 4-byte integer, printed as decimal integer, byte buffer size is 4.

    %l : 8-byte integer, printed as decimal integer, byte buffer size is 8.

    %x : 4-byte integer, printed as hexadecimal integer, byte buffer size is 4.

    %X : 8-byte integer, printed as hexadecimal integer, byte buffer size is 8.

    %e : 4-byte float, printed as decimal float, byte buffer size is 4.

    %E : 8-byte float, printed as decimal float, byte buffer size is 8.
.n
N*/

/*@
    MPE_Describe_info_state - Describe attributes of a state
                              with byte informational data.

    Input Parameters:
+ start_etype  - event number for the beginning of the state
. final_etype  - event number for the ending of the state
. name         - name of the state.
                 maximum length of the NULL-terminated string is,
                 sizeof(CLOG_DESC), 32.
. color        - color of the state.
                 maximum length of the NULL-terminated string is,
                 sizeof(CLOG_COLOR), 24.
- format       - printf style %-token format control string for the state.
                 maximum length of the NULL-terminated string is,
                 sizeof(CLOG_FORMAT), 40.  If format is NULL, it is
                 equivalent to calling MPE_Describe_state().  For fortran
                 interface, zero-length string, i.e. "", is considered NULL.

    Notes:
    Adds a state definition to the logfile.
    States are added to a log file by calling 'MPE_Log_event()'
    for the start and end event numbers.

.N MPE_LOG_BYTE_FORMAT

.seealso: MPE_Log_get_event_number
@*/
int MPE_Describe_info_state( int start_etype, int final_etype,
                             const char *name, const char *color,
                             const char *format )
{
    int stateID;

    if (!MPE_Log_hasBeenInit)
        return MPE_Log_NOT_INITIALIZED;

    stateID = CLOG_get_new_state();
    CLOG_LOGSTATE( stateID, start_etype, final_etype, color, name, format );

    return MPE_Log_OK;
}

/*@
    MPE_Describe_state - Describe the attributes of a state
                         without byte informational data.

    Input Parameters:
+ start_etype  - event number for the beginning of the state
. final_etype  - event number for the ending of the state
. name         - name of the state
                 maximum length of the NULL-terminated string is,
                 sizeof(CLOG_DESC), 32.
- color        - color of the state
                 maximum length of the NULL-terminated string is,
                 sizeof(CLOG_COLOR), 24.

    Notes:
    Adds a state definition to the logfile.
    States are added to a log file by calling 'MPE_Log_event'
    for the start and end event numbers.  The function is provided
    for backward compatibility purpose.  Users are urged to use
    'MPE_Describe_info_state' instead.

.seealso: MPE_Log_get_event_number, MPE_Describe_info_state
@*/
int MPE_Describe_state( int start_etype, int final_etype,
                        const char *name, const char *color )
{
    return MPE_Describe_info_state( start_etype, final_etype, name, color,
                                    NULL );
}


/*@
    MPE_Describe_info_event - Describe the attributes of an event
                              with byte informational data.
    
    Input Parameters:
+ event        - event number for the event.
. name         - name of the event.
                 maximum length of the NULL-terminated string is,
                 sizeof(CLOG_DESC), 32.
. color        - color of the event.
                 maximum length of the NULL-terminated string is,
                 sizeof(CLOG_COLOR), 24.
- format       - printf style %-token format control string for the event.
                 maximum length of the NULL-terminated string is,
                 sizeof(CLOG_FORMAT), 40.  If format is NULL, it is
                 equivalent to calling MPE_Describe_event().  For fortran
                 interface, zero-length string, i.e. "", is considered NULL.

    Notes:
    Adds a event definition to the logfile.

.N MPE_LOG_BYTE_FORMAT

.seealso: MPE_Log_get_event_number 
@*/
int MPE_Describe_info_event( int event, const char *name, const char *color,
                             const char *format )
{
    if (!MPE_Log_hasBeenInit)
        return MPE_Log_NOT_INITIALIZED;

    CLOG_LOGEVENT( event, color, name, format );

    return MPE_Log_OK;
}

/*@
   MPE_Describe_event - Describe the attributes of an event
                        without byte informational data.

    Input Parameters:
+ event        - event number for the event.
. name         - name of the event.
                 maximum length of the NULL-terminated string is,
                 sizeof(CLOG_DESC), 32.
- color        - color of the event.
                 maximum length of the NULL-terminated string is,
                 sizeof(CLOG_COLOR), 24.

    Notes:
    Adds a event definition to the logfile.  The function is provided
    for backward compatibility purpose.  Users are urged to use
    'MPE_Describe_info_event' instead.

.seealso: MPE_Log_get_event_number
@*/
int MPE_Describe_event( int event, const char *name, const char *color )
{
    return MPE_Describe_info_event( event, name, color, NULL );
}

/*@
  MPE_Log_get_event_number - Gets an unused event number

  Returns:
  A value that can be used in MPE_Describe_info_event() and
  MPE_Describe_info_state() which define an event or state not used before.  

  Notes: 
  This routine is provided to allow packages to ensure that they are 
  using unique event numbers.  It relies on all packages using this
  routine.
@*/
int MPE_Log_get_event_number( void )
{
    return CLOG_get_new_event();
}

/*@
    MPE_Log_send - Logs the send event of a message

    Input Parameters:
+ other_party   -  the rank of the other party, i.e. receive event's rank.
. tag           -  message tag ID.
- size          -  message size in byte.

@*/
int MPE_Log_send( int other_party, int tag, int size )
{
    if (other_party != MPI_PROC_NULL)
        CLOG_LOGMSG( CLOG_MSG_SEND, tag, other_party, 0, size );
    return MPE_Log_OK;
}

/*@
    MPE_Log_receive - log the receive event of a message

    Input Parameters:
+ other_party   -  the rank of the other party, i.e. send event's rank.
. tag           -  message tag ID.
- size          -  message size in byte.
@*/
int MPE_Log_receive( int other_party, int tag, int size )
{
    if (other_party != MPI_PROC_NULL)
        CLOG_LOGMSG( CLOG_MSG_RECV, tag, other_party, 0, size );
    return MPE_Log_OK;
}

/*@
    MPE_Log_pack - pack informational data into byte buffer to be stored
                   in a infomational event.  The routine will byteswap
                   data if it is invoked on a small endian machine.

    Ouput Parameters:
+ bytebuf    - output byte buffer which is of sizeof(MPE_LOG_BYTES),
               i.e. 32 bytes.  For C, bytebuf could be of type
               MPE_LOG_BYTES.  For Fortran, bytebuf could be of
               type 'character*32'
- position   - an offset measured from the beginning of the bytebuf.
               On input, data will be written to the offset position.
               On Output, position will be updated to reflect the next
               available position in the byte buffer.
             
   
    Input Parameters:
+ tokentype  - a character token type indicator, currently supported tokens are
               's', 'h', 'd', 'l', 'x', 'X', 'e' and 'E'.
. count      - the number of continuous storage units as indicated by tokentype.
- data       - pointer to the beginning of the storage units being copied.

.N MPE_LOG_BYTE_FORMAT

@*/
int MPE_Log_pack( MPE_LOG_BYTES bytebuf, int *position,
                  char tokentype, int count, const void *data )
{
    void *vptr;
    int   tot_sz;
    vptr = ( (char *) bytebuf + *position );
    /* 
     * Do the byte swapping here for now.
     * Moving byteswapping to clog2TOslog2 convertor should speed up logging.
     */
    switch (tokentype) {
        case 's':  /* STR */
            tot_sz = sizeof( short ) + count;
            if ( *position + tot_sz <= sizeof( MPE_LOG_BYTES ) ) {
                *((short *) vptr) = (short) count;
                CLOG_byteswap( vptr, sizeof( short ) , 1 );
                vptr = (void *)((char *)(vptr) + sizeof( short ));
                memcpy( vptr, data, count );
                *position += tot_sz;
                return MPE_Log_OK;
            }
            break;
        case 'h':  /* INT2 */
            tot_sz = count * 2;
            if ( *position + tot_sz <= sizeof( MPE_LOG_BYTES ) ) {
                memcpy( vptr, data, tot_sz );
                CLOG_byteswap( vptr, 2 , count );
                *position += tot_sz;
                return MPE_Log_OK;
            }
            break;
        case 'd': /* INT4 */
        case 'x': /* BYTE4 */
        case 'e': /* FLT4 */
            tot_sz = count * 4;
            if ( *position + tot_sz <= sizeof( MPE_LOG_BYTES ) ) {
                memcpy( vptr, data, tot_sz );
                CLOG_byteswap( vptr, 4, count );
                *position += tot_sz;
                return MPE_Log_OK;
            }
            break;
        case 'l': /* INT8 */
        case 'X': /* BYTE8 */
        case 'E': /* FLT8 */
            tot_sz = count * 8;
            if ( *position + tot_sz <= sizeof( MPE_LOG_BYTES ) ) {
                memcpy( vptr, data, tot_sz );
                CLOG_byteswap( vptr, 8, count );
                *position += tot_sz;
                return MPE_Log_OK;
            }
            break;
        default:
            fprintf( stderr, "MPE_Log_pack(): Unknown tokentype %c\n",
                             tokentype );
    }
    return MPE_Log_PACK_FAIL;
}

/*@
    MPE_Log_event - Logs an event

    Input Parameters:
+   event   - event number
.   data    - integer data value
              (not used, provided for backward compatibility purpose)
-   bytebuf - optional byte informational array.  In C, bytebuf should be
              set to NULL when no extra byte informational data.  In Fortran,
              an zero-length string, i.e. "", is equivalent to NULL in C.

    Returns:
    alway returns MPE_Log_OK
@*/
int MPE_Log_event( int event, int data, const char *bytebuf )
{
    if ( bytebuf ) 
        CLOG_LOGCARGO( event, bytebuf );
    else
        CLOG_LOGBARE( event );
    return MPE_Log_OK;
}

/*@
    MPE_Log_bare_event - Logs a bare event

    Input Parameters:
.   event   - event number

    Returns:
    alway returns MPE_Log_OK
@*/
int MPE_Log_bare_event( int event )
{
    CLOG_LOGBARE( event );
    return MPE_Log_OK;
}

/*@
    MPE_Log_info_event - Logs an infomational event

    Input Parameters:
+   event   - event number
-   bytebuf - byte informational array.  If no byte inforamtional array,
              use MPE_Log_bare_event() instead.

    Returns:
    alway returns MPE_Log_OK
@*/
int MPE_Log_info_event( int event, const char *bytebuf )
{
    CLOG_LOGCARGO( event, bytebuf );
    return MPE_Log_OK;
}

/*@
    MPE_Finish_log - Send log to master, who writes it out

    Notes:
    MPE_Finish_log() & MPE_Init_log() are NOT needed when liblmpe.a is linked
    because MPI_Finalize) would have called MPE_Finish_log() already.
    liblmpe.a will be included in the final executable if it is linked with
    mpicc -mpe=log

    This routine dumps a logfile in clog2 format.  It is collective over
    'MPI_COMM_WORLD'.

@*/
int MPE_Finish_log( char *filename )
{
/*
 * The environment variable MPE_LOG_FORMAT is NOT read
 */
    char *env_log_format;
    char *env_logfile_prefix;
    int shift, log_format, final_log_format;
    int *is_globalp, flag;

    if (MPE_Log_hasBeenClosed == 0) {
        CLOG_Finalize();

        PMPI_Attr_get( MPI_COMM_WORLD, MPI_WTIME_IS_GLOBAL,
                       &is_globalp, &flag );

        if (!flag || (is_globalp && !*is_globalp))
            shift = CMERGE_SHIFT;
        else
            shift = CMERGE_NOSHIFT;
        /*
            Ignore what MPI says if the clock is sync.,
            force synchronization of the clocks
        */
        /*  
            printf( "Forcing the synchronization of the clock\n" );
            shift = CMERGE_SHIFT;
        */

        log_format = CLOG_LOG;
        env_log_format = (char *) getenv( "MPE_LOG_FORMAT" );

        /*
        if ( env_log_format != NULL )
            printf( "MPE_LOG_FORMAT = %s\n", env_log_format );
        */
 
        if ( env_log_format != NULL ) {
            if (strcmp(env_log_format,"ALOG") == 0)
                log_format = ALOG_LOG;
            else if (strcmp(env_log_format,"SLOG") == 0)
                log_format = SLOG_LOG;
        }
             

        /* 
           We should do a compare across all processes to choose the format,
           in case the environment is not the same on all processes.  We use
           MPI_MAX since SLOG_LOG > ALOG_LOG > CLOG_LOG.
           Since log_format is initialized to CLOG, then CLOG will be 
           the default logfile format unless MPE_LOG_FORMAT is set.
        */
        PMPI_Allreduce( &log_format, &final_log_format, 1, MPI_INT,
                        MPI_MAX, MPI_COMM_WORLD );

        /*  printf( "final_log_format = %d\n", final_log_format );  */


        env_logfile_prefix = (char *) getenv( "MPE_LOGFILE_PREFIX" );
        if ( env_logfile_prefix != NULL )
            CLOG_mergelogs(shift, env_logfile_prefix, final_log_format); 
        else
            CLOG_mergelogs(shift, filename, final_log_format); 

        MPE_Log_hasBeenClosed = 1;
        MPE_Stop_log();
    }
    return MPE_Log_OK;
}
