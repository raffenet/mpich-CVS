/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "mpe_logging_conf.h"

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

void CLOG_Close( CLOG_Stream_t *stream )
{
    CLOG_Buffer_localIO_finalize( stream->buffer );
    CLOG_Buffer_free( &(stream->buffer) );
}

/*
   CLOG_Local_init() has to be called before any of CLOG_Buffer_save_xxxx()
   is invoked.  It is usually called right after CLOG_Stream_t is created. 
*/
void CLOG_Local_init( CLOG_Stream_t *stream, const char *local_tmpfile_name )
{
    stream->next_eventID = CLOG_USER_EVENTID_START;
    stream->next_stateID = CLOG_USER_STATEID_START;

    CLOG_Rec_sizes_init();
    CLOG_Buffer_init( stream->buffer, local_tmpfile_name );

    /* Initialize the synchronizer */
    stream->syncer = CLOG_Sync_create( stream->buffer->num_mpi_procs,
                                       stream->buffer->local_mpi_rank );
    CLOG_Sync_init( stream->syncer );

    /* CLOG_Timer_start() HAS TO BE CALLED before any CLOG_Buffer_save_xxx() */
    CLOG_Timer_start();
    /*
       Save a default CLOG_REC_TIMESHIFT as the very first record
       of the local clog file, so that its value can be used to
       adjust events happened afterward.
    */
    CLOG_Buffer_init_timeshift( stream->buffer );
}

void CLOG_Local_finalize( CLOG_Stream_t *stream )
{
    CLOG_Time_t  local_timediff;

    if ( stream->syncer->local_mpi_rank == 0 ) {
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

int  CLOG_Get_new_eventID( CLOG_Stream_t *stream )
{
    return (stream->next_eventID)++;
}

int  CLOG_Get_new_stateID( CLOG_Stream_t *stream )
{
    return (stream->next_stateID)++;
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
