/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "mpe_logging_conf.h"

#if defined( HAVE_SYS_TYPES_H)
#include <sys/types.h>
#endif
#if defined( STDC_HEADERS ) || defined( HAVE_STDIO_H )
#include <stdio.h>
#endif
#if defined( STDC_HEADERS ) || defined( HAVE_STDLIB_H )
#include <stdlib.h>
#endif

#include "clog.h"
#include "clog_record.h"
#include "clog_preamble.h"
#include "clog_timer.h"
#include "clog_sync.h"
#include "clog_merger.h"
#include "clog_commset.h"

CLOG_Stream_t *CLOG_Open( void )
{
    CLOG_Stream_t  *stream;
     
    stream = (CLOG_Stream_t *) MALLOC( sizeof( CLOG_Stream_t ) );
    if ( stream == NULL ) {
        fprintf( stderr, __FILE__":CLOG_Open() - MALLOC() fails.\n" );
        fflush( stderr );
        return NULL;
    }

    stream->buffer = CLOG_Buffer_create();
    if ( stream->buffer == NULL ) {
        fprintf( stderr, __FILE__":CLOG_Open() - \n"
                         "\t""CLOG_Buffer_create() returns NULL.\n" );
        fflush( stderr );
        return NULL;
    }

    stream->syncer = NULL;
    stream->merger = NULL;

    return stream;
}

void CLOG_Close( CLOG_Stream_t **stream_handle )
{
    CLOG_Stream_t *stream;

    stream = *stream_handle;
    if ( stream != NULL ) {
        CLOG_Buffer_localIO_finalize( stream->buffer );
        CLOG_Buffer_free( &(stream->buffer) );
        FREE( stream );
    }
    *stream_handle = NULL;
}

/*
   CLOG_Local_init() has to be called before any of CLOG_Buffer_save_xxxx()
   is invoked.  It is usually called right after CLOG_Stream_t is created. 
*/
void CLOG_Local_init( CLOG_Stream_t *stream, const char *local_tmpfile_name )
{
    const CLOG_CommIDs_t *commIDs;
          CLOG_Buffer_t  *buffer;

    stream->known_eventID  = CLOG_KNOWN_EVENTID_START;
    stream->known_stateID  = CLOG_KNOWN_STATEID_START;
    stream->user_eventID   = CLOG_USER_EVENTID_START;
    stream->user_stateID   = CLOG_USER_STATEID_START;

    CLOG_Rec_sizes_init();
    buffer  = stream->buffer;
    CLOG_Buffer_init( buffer, local_tmpfile_name );

    /* Initialize the synchronizer */
    stream->syncer = CLOG_Sync_create( buffer->world_size, buffer->world_rank );
    CLOG_Sync_init( stream->syncer );

    /* CLOG_Timer_start() HAS TO BE CALLED before any CLOG_Buffer_save_xxx() */
    CLOG_Timer_start();
    /*
       Save a default CLOG_REC_TIMESHIFT as the very first record
       of the local clog file, so that its value can be used to
       adjust events happened afterward.
    */
    CLOG_Buffer_init_timeshift( buffer );

    /* Adding the CLOG_Buffer_write2disk's state definition */
    if (    buffer->world_rank    == 0 
         && buffer->log_overhead  == CLOG_BOOL_TRUE ) {
        commIDs  = CLOG_CommSet_get_IDs( buffer->commset, MPI_COMM_WORLD );
        CLOG_Buffer_save_statedef( buffer, commIDs, 0,
                                   CLOG_STATEID_BUFFERWRITE,
                                   CLOG_EVT_BUFFERWRITE_START,
                                   CLOG_EVT_BUFFERWRITE_FINAL,
                                   "maroon", "CLOG_Buffer_write2disk",
                                   NULL );
    }
}

void CLOG_Local_finalize( CLOG_Stream_t *stream )
{
    CLOG_Time_t  local_timediff;

    if ( stream->syncer->world_rank == 0 ) {
        if ( stream->syncer->is_ok_to_sync == CLOG_BOOL_TRUE )
            printf( "Enabling the synchronization of the clocks...\n" );
        else
            printf( "Disabling the synchronization of the clocks...\n" );
    }
    if ( stream->syncer->is_ok_to_sync == CLOG_BOOL_TRUE ) {
        local_timediff = CLOG_Sync_update_timediffs( stream->syncer );
        CLOG_Buffer_set_timeshift( stream->buffer, local_timediff,
                                   CLOG_BOOL_FALSE );
    }
    CLOG_Sync_free( &(stream->syncer) );

    CLOG_Buffer_save_endlog( stream->buffer );
    CLOG_Buffer_localIO_flush( stream->buffer );
}

int  CLOG_Get_user_eventID( CLOG_Stream_t *stream )
{
    return (stream->user_eventID)++;
}

int  CLOG_Get_user_stateID( CLOG_Stream_t *stream )
{
    return (stream->user_stateID)++;
}

int  CLOG_Get_known_eventID( CLOG_Stream_t *stream )
{
    if ( stream->known_eventID < CLOG_USER_EVENTID_START )
        return (stream->known_eventID)++;
    else {
        fprintf( stderr, __FILE__":CLOG_Get_known_eventID() - \n"
                         "\t""CLOG internal eventID have been used up, "
                         "use CLOG user eventID %d.\n", stream->user_eventID );
        fflush( stderr );
        return (stream->user_eventID)++;
    }
}

int  CLOG_Get_known_stateID( CLOG_Stream_t *stream )
{
    if ( stream->known_stateID < CLOG_USER_STATEID_START )
        return (stream->known_stateID)++;
    else {
        fprintf( stderr, __FILE__":CLOG_Get_known_stateID() - \n"
                         "\t""CLOG internal stateID have been used up, "
                         "use CLOG user stateID %d.\n", stream->user_stateID );
        fflush( stderr );
        return (stream->user_stateID)++;
    }
}

int  CLOG_Check_known_stateID( CLOG_Stream_t *stream, int stateID )
{
    if (    stateID >= CLOG_KNOWN_STATEID_START
         && stateID <  CLOG_USER_STATEID_START )
        return CLOG_BOOL_TRUE;
    else
        return CLOG_BOOL_FALSE;
}

void CLOG_Converge_init(       CLOG_Stream_t *stream,
                         const char          *merged_file_prefix )
{
    /* stream->buffer->block_size == stream->buffer->preamble->block_size */
    stream->merger = CLOG_Merger_create( stream->buffer->block_size );
    CLOG_Merger_init( stream->merger, stream->buffer->preamble,
                      merged_file_prefix );
}

void CLOG_Converge_finalize( CLOG_Stream_t *stream )
{
    CLOG_Merger_finalize( stream->merger );
    CLOG_Merger_free( &(stream->merger) );
}

void CLOG_Converge_sort( CLOG_Stream_t *stream )
{
    CLOG_Merger_sort( stream->merger, stream->buffer );
    CLOG_Merger_last_flush( stream->merger );
}
