/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "mpe_logging_conf.h"

#if defined( HAVE_SYS_TYPES_H)
#include <sys/types.h>
#endif
#if defined( STDC_HEADERS ) || defined( HAVE_STDLIB_H )
/* Needed for getenv */
#include <stdlib.h>
#endif
#if defined( STDC_HEADERS ) || defined( HAVE_STRING_H )
#include <string.h>
#endif
#if defined( STDC_HEADERS ) || defined( HAVE_STDIO_H )
#include <stdio.h>
#endif

#include "mpe_log.h"
#if !defined( CLOG_NOMPI )
#include "mpi.h"                /* Needed for PMPI routines */
#endif

#include "clog.h"
#include "clog_common.h"
#include "clog_util.h"
#include "clog_timer.h"
#include "clog_sync.h"

/* we want to use the PMPI routines instead of the MPI routines for all of 
   the logging calls internal to the mpe_log package */
#ifdef USE_PMPI
#define MPI_BUILD_PROFILING
#include "mpiprof.h"
#endif

/* Global variables for MPE logging */
CLOG_Stream_t  *clog_stream            = NULL;
CLOG_Buffer_t  *clog_buffer            = NULL;
int             MPE_Log_hasBeenInit    = 0;
int             MPE_Log_hasBeenClosed  = 0;


/*@
    MPE_Init_log - Initialize the logging of events.

    Notes:
    Initializes the MPE logging package.  This must be called before any of
    the other MPE logging routines.  It is collective over 'MPI_COMM_WORLD'.

    Notes:
    MPE_Init_log() & MPE_Finish_log() are NOT needed when liblmpe.a is linked
    because MPI_Init() would have called MPE_Init_log() already.
    liblmpe.a will be included in the final executable if it is linked with
    mpicc -mpe=log

    Returns:
    Always return MPE_LOG_OK.

.seealso: MPE_Finish_log
@*/
int MPE_Init_log( void )
{
    if (!MPE_Log_hasBeenInit || MPE_Log_hasBeenClosed) {
        clog_stream  = CLOG_Open();
        CLOG_Local_init( clog_stream, NULL );
        clog_buffer  = clog_stream->buffer;
#if !defined( CLOG_NOMPI )
        CLOG_Buffer_save_commevt( clog_buffer, CLOG_COMM_INIT,
                                  CLOG_COMM_NULL, (int) MPI_COMM_WORLD );
        if ( clog_buffer->local_mpi_rank == 0 ) {
            CLOG_Buffer_save_constdef( clog_buffer, CLOG_EVT_CONST,
                                       MPI_PROC_NULL, "MPI_PROC_NULL" );
            CLOG_Buffer_save_constdef( clog_buffer, CLOG_EVT_CONST,
                                       MPI_ANY_SOURCE, "MPI_ANY_SOURCE" );
            CLOG_Buffer_save_constdef( clog_buffer, CLOG_EVT_CONST,
                                       MPI_ANY_TAG, "MPI_ANY_TAG" );
        }
#endif
        MPE_Log_hasBeenInit = 1;        /* set MPE_Log as being initialized */
        MPE_Log_hasBeenClosed = 0;
    }
    MPE_Start_log();
    return MPE_LOG_OK;
}

/*@
    MPE_Start_log - Start the logging of events.
@*/
int MPE_Start_log( void )
{
    if (!MPE_Log_hasBeenInit)
        return MPE_LOG_NOT_INITIALIZED;
    clog_buffer->status = CLOG_INIT_AND_ON;
    return MPE_LOG_OK;
}

/*@
    MPE_Stop_log - Stop the logging of events
@*/
int MPE_Stop_log( void )
{
    if (!MPE_Log_hasBeenInit)
        return MPE_LOG_NOT_INITIALIZED;
    clog_buffer->status = CLOG_INIT_AND_OFF;
    return MPE_LOG_OK;
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
                 equivalent to calling MPE_Describe_state().  The fortran
                 interface of this routine considers the zero-length string,
                 "", and single-blank string, " ", as NULL.

    Notes:
    Adds a state definition to the logfile.
    States are added to a logfile by calling 'MPE_Log_event()'
    for the start and end event numbers.

.N MPE_LOG_BYTE_FORMAT

.see also: MPE_Log_get_event_number
@*/
int MPE_Describe_info_state( int start_etype, int final_etype,
                             const char *name, const char *color,
                             const char *format )
{
    int stateID;

    if (!MPE_Log_hasBeenInit)
        return MPE_LOG_NOT_INITIALIZED;

    stateID = CLOG_Get_user_stateID( clog_stream );
    CLOG_Buffer_save_statedef( clog_buffer, stateID,
                               start_etype, final_etype,
                               color, name, format );

    return MPE_LOG_OK;
}

/*
    This is an MPE internal function to describe MPI states.
    It is not meant for user application.
    stateID should be fetched from CLOG_Get_known_stateID()
    i.e, MPE_Log_get_known_stateID()
*/
int MPE_Describe_known_state( int stateID, int start_etype, int final_etype,
                              const char *name, const char *color,
                              const char *format )
{
    if (!MPE_Log_hasBeenInit)
        return MPE_LOG_NOT_INITIALIZED;

    if ( CLOG_Check_known_stateID( clog_stream, stateID ) != CLOG_BOOL_TRUE ) {
        fprintf( stderr, __FILE__":MPE_Describe_known_state() - \n"
                         "\t""The input stateID, %d, for state %s "
                         "is out of known range [%d..%d].\n"
                         "\t""Use user-space stateID instead.\n",
                         stateID, name, CLOG_KNOWN_STATEID_START,
                         CLOG_USER_STATEID_START-1 );
        fflush( stderr );
        stateID = CLOG_Get_user_stateID( clog_stream );
    }

    CLOG_Buffer_save_statedef( clog_buffer, stateID,
                               start_etype, final_etype,
                               color, name, format );

    return MPE_LOG_OK;

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
                 equivalent to calling MPE_Describe_event(). The fortran
                 interface of this routine considers the zero-length string,
                 "", and single-blank string, " ", as NULL.

    Notes:
    Adds a event definition to the logfile.

.N MPE_LOG_BYTE_FORMAT

.seealso: MPE_Log_get_event_number 
@*/
int MPE_Describe_info_event( int event, const char *name, const char *color,
                             const char *format )
{
    if (!MPE_Log_hasBeenInit)
        return MPE_LOG_NOT_INITIALIZED;

    CLOG_Buffer_save_eventdef( clog_buffer, event, color, name, format );

    return MPE_LOG_OK;
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
    return CLOG_Get_user_eventID( clog_stream );
}

/*
    This is an MPE internal function in defining MPE_state[] evtID components.
    It is not meant for user application.
*/
int MPE_Log_get_known_eventID( void )
{
    return CLOG_Get_known_eventID( clog_stream );
}

/*
    This is an MPE internal function in defining MPE_state[] stateID component.
    It is not meant for user application.
*/
int MPE_Log_get_known_stateID( void )
{
    return CLOG_Get_known_stateID( clog_stream );
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
#if !defined( CLOG_NOMPI )
    if (other_party != MPI_PROC_NULL)
#endif
        CLOG_Buffer_save_msgevt( clog_buffer, CLOG_EVT_SENDMSG,
                                 tag, other_party, 0, size );
    return MPE_LOG_OK;
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
#if !defined( CLOG_NOMPI )
    if (other_party != MPI_PROC_NULL)
#endif
        CLOG_Buffer_save_msgevt( clog_buffer, CLOG_EVT_RECVMSG,
                                 tag, other_party, 0, size );
    return MPE_LOG_OK;
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
#if !defined( WORDS_BIGENDIAN )
                CLOG_Util_swap_bytes( vptr, sizeof( short ) , 1 );
#endif
                vptr = (void *)( (char *) vptr + sizeof( short ) );
                memcpy( vptr, data, count );
                *position += tot_sz;
                return MPE_LOG_OK;
            }
            break;
        case 'h':  /* INT2 */
            tot_sz = count * 2;
            if ( *position + tot_sz <= sizeof( MPE_LOG_BYTES ) ) {
                memcpy( vptr, data, tot_sz );
#if !defined( WORDS_BIGENDIAN ) 
                CLOG_Util_swap_bytes( vptr, 2 , count );
#endif
                *position += tot_sz;
                return MPE_LOG_OK;
            }
            break;
        case 'd': /* INT4 */
        case 'x': /* BYTE4 */
        case 'e': /* FLT4 */
            tot_sz = count * 4;
            if ( *position + tot_sz <= sizeof( MPE_LOG_BYTES ) ) {
                memcpy( vptr, data, tot_sz );
#if !defined( WORDS_BIGENDIAN )
                CLOG_Util_swap_bytes( vptr, 4, count );
#endif
                *position += tot_sz;
                return MPE_LOG_OK;
            }
            break;
        case 'l': /* INT8 */
        case 'X': /* BYTE8 */
        case 'E': /* FLT8 */
            tot_sz = count * 8;
            if ( *position + tot_sz <= sizeof( MPE_LOG_BYTES ) ) {
                memcpy( vptr, data, tot_sz );
#if !defined( WORDS_BIGENDIAN )
                CLOG_Util_swap_bytes( vptr, 8, count );
#endif
                *position += tot_sz;
                return MPE_LOG_OK;
            }
            break;
        default:
            fprintf( stderr, "MPE_Log_pack(): Unknown tokentype %c\n",
                             tokentype );
    }
    return MPE_LOG_PACK_FAIL;
}

/*@
    MPE_Log_event - Logs an event

    Input Parameters:
+   event   - event number
.   data    - integer data value
              (not used, provided for backward compatibility purpose)
-   bytebuf - optional byte informational array.  In C, bytebuf should be
              set to NULL when no extra byte informational data.  In Fortran,
              an zero-length string "", or a single blank string " ",
              is equivalent to NULL in C.

    Returns:
    alway returns MPE_LOG_OK
@*/
int MPE_Log_event( int event, int data, const char *bytebuf )
{
    if ( bytebuf ) 
        CLOG_Buffer_save_cargoevt( clog_buffer, event, bytebuf );
    else
        CLOG_Buffer_save_bareevt( clog_buffer, event );
    return MPE_LOG_OK;
}

/*@
    MPE_Log_bare_event - Logs a bare event

    Input Parameters:
.   event   - event number

    Returns:
    alway returns MPE_LOG_OK
@*/
int MPE_Log_bare_event( int event )
{
    CLOG_Buffer_save_bareevt( clog_buffer, event );
    return MPE_LOG_OK;
}

/*@
    MPE_Log_info_event - Logs an infomational event

    Input Parameters:
+   event   - event number
-   bytebuf - byte informational array.  If no byte inforamtional array,
              use MPE_Log_bare_event() instead.

    Returns:
    alway returns MPE_LOG_OK
@*/
int MPE_Log_info_event( int event, const char *bytebuf )
{
    CLOG_Buffer_save_cargoevt( clog_buffer, event, bytebuf );
    return MPE_LOG_OK;
}

/*@
    MPE_Log_sync_clocks - synchronize or recalibrate all MPI clocks to
                          minimize the effect of time drift.  It is like a 
                          longer version of MPI_Comm_barrier( MPI_COMM_WORLD );

    Returns:
    alway returns MPE_LOG_OK
@*/
int MPE_Log_sync_clocks( void )
{
    CLOG_Sync_t  *clog_syncer;
    CLOG_Time_t   local_timediff;

    clog_syncer = clog_stream->syncer;
    if ( clog_syncer->is_ok_to_sync == CLOG_BOOL_TRUE ) {
        local_timediff = CLOG_Sync_update_timediffs( clog_syncer );
        CLOG_Buffer_set_timeshift( clog_buffer, local_timediff,
                                   CLOG_BOOL_TRUE );
    }
    return MPE_LOG_OK;
}


/*@
    MPE_Finish_log - Send log to master, who writes it out

    Notes:
    MPE_Finish_log() & MPE_Init_log() are NOT needed when liblmpe.a is linked
    because MPI_Finalize) would have called MPE_Finish_log() already.
    liblmpe.a will be included in the final executable if it is linked with
    mpicc -mpe=log.

    This routine dumps the logfile in clog2 format.
    It is collective over 'MPI_COMM_WORLD'.

@*/
int MPE_Finish_log( char *filename )
{
/*
   The environment variable MPE_LOG_FORMAT is NOT read
*/
    char         *env_logfile_prefix;

    if ( MPE_Log_hasBeenClosed == 0 ) {
        CLOG_Local_finalize( clog_stream );
        /*
           Call MPE_Stop_log() before CLOG_Close() which nullifies clog_stream
        */
        MPE_Stop_log();

        env_logfile_prefix = (char *) getenv( "MPE_LOGFILE_PREFIX" );
        if ( env_logfile_prefix != NULL )
            CLOG_Converge_init( clog_stream, env_logfile_prefix );
        else
            CLOG_Converge_init( clog_stream, filename );
        /*
           Update the "filename" with the real output filename
           Assuming the filename[]'s length = merger->out_filename[]'s length.
        */
        strcpy( filename, clog_stream->merger->out_filename );
        CLOG_Converge_sort( clog_stream );
        CLOG_Converge_finalize( clog_stream );

        /*
           Finalize the CLOG_Stream_t and nullify clog_buffer so calling
           other MPE routines after MPE_Finish_log will cause seg. fault.
        */
        CLOG_Close( &clog_stream );
        clog_buffer = NULL;

        MPE_Log_hasBeenClosed = 1;
    }
    return MPE_LOG_OK;
}
