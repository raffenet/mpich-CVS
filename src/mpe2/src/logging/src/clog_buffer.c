/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "mpe_logging_conf.h"

#if defined( STDC_HEADERS ) || defined( HAVE_STDIO_H )
#include <stdio.h>
#endif
#if defined( STDC_HEADERS ) || defined( HAVE_STRING_H )
#include <string.h>
#endif
#if defined( STDC_HEADERS ) || defined( HAVE_STDLIB_H )
#include <stdlib.h>
#endif
#if defined( HAVE_FCNTL_H )
#include <fcntl.h>
#endif
#if defined( HAVE_UNISTD_H )
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_IO_H
#include <io.h>
#endif

#include "clog_common.h"
#include "clog_preamble.h"
#include "clog_buffer.h"
#include "clog_util.h"
#include "clog_record.h"

#if !defined( CLOG_NOMPI )
#include "mpi.h"
#endif

#define CLOG_NULL_FILE       -5

CLOG_Buffer_t* CLOG_Buffer_create( void )
{
    CLOG_Buffer_t *buffer;

    buffer = (CLOG_Buffer_t *) MALLOC( sizeof(CLOG_Buffer_t) );
    if ( buffer == NULL ) {
        fprintf( stderr, __FILE__":CLOG_Buffer_create() - MALLOC() fails.\n" );
        fflush( stderr );
        return NULL;
    }

    buffer->preamble = CLOG_Preamble_create();
    if ( buffer->preamble == NULL ) {
        fprintf( stderr, __FILE__":CLOG_Buffer_create() - \n"
                         "\t""CLOG_Preamble_create() returns NULL.\n" );
        fflush( stderr );
        return NULL;
    }

    buffer->head_block       = NULL;
    buffer->curr_block       = NULL;

    buffer->block_size       = 0;
    buffer->num_blocks       = 0;
    buffer->num_used_blocks  = 0;

    buffer->local_fd         = CLOG_NULL_FILE;
    strcpy( buffer->local_filename, "" );
    buffer->timeshift_fptr   = 0;
    buffer->delete_localfile = CLOG_BOOL_TRUE;
    buffer->status           = CLOG_UNINIT;

    return buffer;
}

void CLOG_Buffer_free( CLOG_Buffer_t **buffer_handle )
{
    CLOG_Buffer_t *buffer;
    CLOG_Block_t  *block;

    buffer = *buffer_handle;
    block = buffer->head_block;
    while ( block != NULL ) {
        buffer->head_block = block->next;
        CLOG_Block_free( &block );
        buffer->num_blocks--;
        block = buffer->head_block;
    }
    buffer->block_size        = 0;
    buffer->num_blocks        = 0;
    buffer->num_used_blocks   = 0;
    CLOG_Preamble_free( &(buffer->preamble) );
    if ( buffer != NULL )
        FREE( buffer );
    *buffer_handle = NULL;
}

void CLOG_Buffer_env_init( CLOG_Buffer_t *buffer )
{
    char          *env_delete_localfile;
#if !defined( CLOG_NOMPI )
    int            ierr;
#endif

    env_delete_localfile = (char *) getenv( "MPE_DELETE_LOCALFILE" );
    if ( env_delete_localfile != NULL ) {
        if (    strcmp( env_delete_localfile, "false" ) == 0
             || strcmp( env_delete_localfile, "FALSE" ) == 0
             || strcmp( env_delete_localfile, "no" ) == 0
             || strcmp( env_delete_localfile, "NO" ) == 0 )
            buffer->delete_localfile = CLOG_BOOL_FALSE;
    }

#if !defined( CLOG_NOMPI )
    /*  Let everyone in MPI_COMM_WORLD know what root has */
    ierr = PMPI_Bcast( &(buffer->delete_localfile), 1, MPI_INT,
                       0, MPI_COMM_WORLD );
    if ( ierr != MPI_SUCCESS ) {
        fprintf( stderr, __FILE__":CLOG_Buffer_env_init() - \n"
                         "\t""PMPI_Bcast() fails\n" );
        fflush( stderr );
        PMPI_Abort( MPI_COMM_WORLD, 1 );
    }
#endif
}

/*
    If local_tmpfile_name is NULL, the local temporary filename will be chosen
    randomly
*/
void CLOG_Buffer_init( CLOG_Buffer_t *buffer, const char *local_tmpfile_name )
{
    CLOG_Block_t  *block;
    int            num_buffered_blocks;

    /* Initialize CLOG_Preamble_t from the environmental variables */
    CLOG_Preamble_env_init( buffer->preamble );

    /* Initialize core CLOG_Buffer_t's from the environmental variables */
    CLOG_Buffer_env_init( buffer );

    /* Initialize CLOG_Buffer_t's blocks[] based on CLOG_Preamble_t */
    buffer->block_size   = buffer->preamble->block_size;
    num_buffered_blocks  = buffer->preamble->num_buffered_blocks;

    block                = CLOG_Block_create( buffer->block_size );
    buffer->head_block   = block;
    for ( buffer->num_blocks = 1;
          buffer->num_blocks < num_buffered_blocks;
          buffer->num_blocks++ ) {
        block->next = CLOG_Block_create( buffer->block_size );
        block       = block->next;
    }
    buffer->curr_block       = buffer->head_block;
    buffer->num_used_blocks  = 1;

#if !defined( CLOG_NOMPI )
    PMPI_Comm_rank( MPI_COMM_WORLD, &(buffer->local_mpi_rank) );
    PMPI_Comm_size( MPI_COMM_WORLD, &(buffer->num_mpi_procs) );
#else
    buffer->local_mpi_rank = 0;
    buffer->num_mpi_procs  = 1;
#endif

    if ( local_tmpfile_name != NULL )
        strcpy( buffer->local_filename, local_tmpfile_name );
    if ( strlen( buffer->local_filename ) == 0 ) {
        CLOG_Util_set_tmpfilename( buffer->local_filename );
        if ( strlen( buffer->local_filename ) == 0 ) {
            fprintf( stderr, __FILE__":CLOG_Buffer_init() - \n"
                             "\t""CLOG_Util_set_tmpfilename() fails.\n" );
            fflush( stderr );
            CLOG_Util_abort( 1 );
        }
    }
    buffer->status          = CLOG_INIT_AND_ON;
}

/*
    CLOG_Buffer_localIO_init4write - Open CLOG_Buffer_t.local_fd
    and initialize it with the CLOG_Preamble_t.
    (assuming the local_filename is known ).
*/
void CLOG_Buffer_localIO_init4write( CLOG_Buffer_t *buffer )
{
    buffer->local_fd = OPEN( buffer->local_filename,
                             O_RDWR|O_CREAT|O_TRUNC, 0600 );
    if ( buffer->local_fd == -1 ) {
        fprintf( stderr, __FILE__":CLOG_Buffer_localIO_init4write() - \n"
                         "\t""Fail to open the temporary logfile %s.\n"
                         "\t""Check if the directory where the logfile "
                         "resides exists\n"
                         "\t""and the corresponding file system is "
                         "NOT full.\n"
                         "If not so, set environmental variable TMPDIR to "
                         "a bigger filesystem.\n",
                         buffer->local_filename );
        fflush( stderr );
        CLOG_Util_abort( 1 );
    }
    /* Save the CLOG_Preamble_t at the beginning of the file */
    CLOG_Preamble_write( buffer->preamble, CLOG_BOOL_FALSE, buffer->local_fd );
}

void CLOG_Buffer_localIO_write( CLOG_Buffer_t *buffer )
{
    CLOG_Block_t  *block;
    int            ierr;

    /*  Write the filled block in the buffer to the disk  */
    for ( block = buffer->head_block;
          block != NULL && buffer->num_used_blocks > 0;
          block = block->next ) {
        ierr = write( buffer->local_fd, block->data->head, buffer->block_size );
        if ( ierr != buffer->block_size ) {
            fprintf( stderr, __FILE__":CLOG_Buffer_localIO_write() - \n"
                             "\t""Fail to write to the temporary logfile %s.\n"
                             "\t""Check if the filesystem where the logfile "
                             "resides is full.\n"
                             "If so, set environmental variable TMPDIR to "
                             "a bigger filesystem.\n",
                             buffer->local_filename );
            fflush( stderr );
            CLOG_Util_abort( 1 );
        }
        buffer->num_used_blocks--;
    }

    /*  Reinit the CLOG_Buffer_t for future processing  */
    buffer->curr_block       = buffer->head_block;
    buffer->num_used_blocks  = 1;
}

void CLOG_Buffer_advance_block( CLOG_Buffer_t *buffer )
{
    if ( buffer->curr_block->next != NULL ) {
        buffer->curr_block = buffer->curr_block->next;
        buffer->num_used_blocks++;
    }
    else {
        if ( buffer->local_fd == CLOG_NULL_FILE )
            CLOG_Buffer_localIO_init4write( buffer );
        CLOG_Buffer_localIO_write( buffer );
    }
    CLOG_Block_reset( buffer->curr_block );
}

void CLOG_Buffer_localIO_flush( CLOG_Buffer_t *buffer )
{
    if ( buffer->local_fd != CLOG_NULL_FILE ) {
        CLOG_Buffer_localIO_write( buffer );
    }
}

off_t CLOG_Buffer_localIO_ftell( CLOG_Buffer_t *buffer )
{
    CLOG_BlockData_t     *blkdata;
    off_t                 fptr;

    /* Determine the current offset within curr_block */
    blkdata  = buffer->curr_block->data;
    fptr     = blkdata->ptr - blkdata->head;

    /* Determine the space taken by the number of filled blocks */
    fptr    += ( buffer->num_used_blocks - 1 ) * buffer->block_size;

    if ( buffer->local_fd != CLOG_NULL_FILE )
        fptr += lseek( buffer->local_fd, 0, SEEK_CUR );
    else
        fptr += CLOG_PREAMBLE_SIZE;
    return fptr;
}

/* Allocate a default CLOG_Rec_Timeshift record in the memory buffer.  */
void CLOG_Buffer_init_timeshift( CLOG_Buffer_t *buffer )
{
    /*
       The default CLOG_Rec_Timeshift recird needs to be saved before
       CLOG_Buffer_t.timeshift_fptr is updated to account for the possibility
       that CLOG_Buffer_t.curr_block may not have enough room for a
       CLOG_Rec_Timeshift record.  After saving the record, the location
       of the CLOG_Rec_Timeshift is unchangable.
    */
    CLOG_Buffer_save_timeshift( buffer, 0.0 );
    buffer->timeshift_fptr = CLOG_Buffer_localIO_ftell( buffer )
                           - CLOG_Rec_size( CLOG_REC_TIMESHIFT );
}

void CLOG_Buffer_set_timeshift( CLOG_Buffer_t *buffer,
                                CLOG_Time_t    new_timediff,
                                int            init_next_timeshift )
{
    CLOG_Rec_Header_t    *hdr, hdr_rec;
    CLOG_Rec_Timeshift_t *tshift, tshift_rec;
    CLOG_Block_t         *block;
    CLOG_BlockData_t     *blkdata;
    off_t                 last_timeshift_fptr, curr_fptr, fptr;
    int                   reclen_hdr, reclen_tshift;
    int                   block_ptr, ierr = 0;

    /* Save the CLOG_Buffer_t.timeshift_fptr for later update */
    last_timeshift_fptr    = buffer->timeshift_fptr;

    if ( init_next_timeshift == CLOG_BOOL_TRUE )
        CLOG_Buffer_init_timeshift( buffer );

    /* Save the current file pointer */
    if ( buffer->local_fd != CLOG_NULL_FILE )
        curr_fptr = lseek( buffer->local_fd, 0, SEEK_CUR );
    else
        curr_fptr = CLOG_PREAMBLE_SIZE;
    /* Check if last_timeshift_ptr is in the memory buffer or on the disk */
    if (  last_timeshift_fptr < curr_fptr ) {
        if (buffer->local_fd  == CLOG_NULL_FILE ) {
            fprintf( stderr, __FILE__":CLOG_Buffer_set_timeshift() - \n"
                             "\t""buffer->local_fd == NULL_FILE detected.\n" );
            fflush( stderr );
            return;
        }
        /* Locat the last CLOG_Rec_Timeshift_t in the file */
        lseek( buffer->local_fd, last_timeshift_fptr, SEEK_SET );
        reclen_hdr     = CLOG_RECLEN_HEADER;
        reclen_tshift  = CLOG_RECLEN_TIMESHIFT;
        /* Set CLOG_Rec_Header_t & CLOG_Rec_Timeshift_t to local tmp storages */
        hdr            = &hdr_rec;   
        tshift         = &tshift_rec;
        /* Fetch the CLOG_Rec_Header of the CLOG_Rec_Timeshift from the disk */
        ierr = read( buffer->local_fd, hdr, reclen_hdr );
        if ( ierr != reclen_hdr ) {
            fprintf( stderr, __FILE__":CLOG_Buffer_set_timeshift() - \n"
                             "\t""read(CLOG_Rec_Header) fails w/ err=%d.\n",
                             ierr );
            fflush( stderr );
            return;
        }
        if ( hdr->rectype != CLOG_REC_TIMESHIFT ) {
            fprintf( stderr, __FILE__":CLOG_Buffer_set_timeshift() - \n"
                             "\t""1st disk record is not CLOG_Rec_Timeshift.\n"
                             );
            fflush( stderr );
            return;
        }
        fptr = lseek( buffer->local_fd, 0, SEEK_CUR );
        ierr = read( buffer->local_fd, tshift, reclen_tshift );
        if ( ierr != reclen_tshift ) {
            fprintf( stderr, __FILE__":CLOG_Buffer_set_timeshift() - \n"
                             "\t""read(CLOG_Rec_Timeshift) fails w/ err=%d.\n",
                             ierr );
            fflush( stderr );
            return;
        }
        /* Update the CLOG_Rec_Timeshift_t with the new timeshift value */
        tshift->timeshift = new_timediff;
        /* Seek back in file of where CLOG_Rec_Timeshift_t was retrieved */
        lseek( buffer->local_fd, fptr, SEEK_SET );
        ierr = write( buffer->local_fd, tshift, reclen_tshift );
        if ( ierr != reclen_tshift ) {
            fprintf( stderr, __FILE__":CLOG_Buffer_set_timeshift() - \n"
                             "\t""write(CLOG_Rec_Timeshift) fails w/ err=%d.\n",
                             ierr );
            fflush( stderr );
            return;
        }
        /* Restore the current file pointer */
        lseek( buffer->local_fd, curr_fptr, SEEK_SET );
    }
    else { /* if (  last_timeshift_fptr >= curr_fptr ) */
        /* Locate the CLOG_Block_t that contains the last CLOG_Rec_Timeshift */
        block       = buffer->head_block;
        block_ptr   = last_timeshift_fptr - curr_fptr;
        while ( block_ptr >= buffer->block_size ) {
            block_ptr -= buffer->block_size;
            if ( block == NULL ) {
                fprintf( stderr, __FILE__":CLOG_Buffer_set_timeshift() - \n"
                                 "\t""End of memory buffer encountered!\n" );
                fflush( stderr );
                return;
            }
            block      = block->next;
        }

        blkdata     = block->data;
        hdr         = (CLOG_Rec_Header_t *) ( blkdata->head + block_ptr );
        if ( hdr->rectype != CLOG_REC_TIMESHIFT ) {
            fprintf( stderr, __FILE__":CLOG_Buffer_set_timeshift() -\n"
                             "\t""No CLOG_Rec_Timeshift at the expected "
                             "record location %d!\n", block_ptr );
            fflush( stderr );
            return;
        }
        tshift            = (CLOG_Rec_Timeshift_t *) hdr->rest;
        /* Update the CLOG_Rec_Timeshift_t with the new timeshift value */
        tshift->timeshift = new_timediff;
    }
}

/*
   Reset CLOG_Buffer_t's localIO to read mode
   If the local_fd has been written, update local buffer from the
   beginning of the file.  If the local_fd has not been used,
   reposition the current block of the buffer to the head block.
*/
void CLOG_Buffer_localIO_reinit4read( CLOG_Buffer_t *buffer )
{
    if ( buffer->local_fd != CLOG_NULL_FILE ) {
        lseek( buffer->local_fd, (off_t) CLOG_PREAMBLE_SIZE, SEEK_SET );
        CLOG_Buffer_localIO_read( buffer );
    }
    else
        buffer->curr_block = buffer->head_block;
}

void CLOG_Buffer_localIO_read( CLOG_Buffer_t *buffer )
{
    CLOG_Block_t  *block;
    int            ierr;

    /* If CLOG_Buffer_t.local_fd isn't initialized, nothing to read from disk */
    if ( buffer->local_fd == CLOG_NULL_FILE )
        return;

    /*
        CLOG_Buffer_t.num_used_blocks is used to indicate if only less than
        CLOG_Buffer_t.num_blocks is stored on the disk
    */
    buffer->num_used_blocks  = 0;
    ierr                     = buffer->block_size;
    for ( block = buffer->head_block; block != NULL; block = block->next ) {
        ierr = read( buffer->local_fd, block->data->head, buffer->block_size );
        if ( ierr > 0 ) {
            if ( ierr != buffer->block_size ) {
                fprintf( stderr, __FILE__":CLOG_Buffer_localIO_read() - \n"
                                 "\t""read(%s) cannot fetch the whole block "
                                 "in one scoop.\n", buffer->local_filename );
                fflush( stderr );
                CLOG_Util_abort( 1 );
            }
        }
        else { /* if ierr <= 0 */ 
            if ( ierr < 0 ) {
                fprintf( stderr, __FILE__":CLOG_Buffer_localIO_read() - \n"
                                 "\t""read(%s) fails with an error=%d.\n",
                                 buffer->local_filename, ierr );
                fflush( stderr );
                CLOG_Util_abort( 1 );
            }
            else /* if ierr == 0 */
                break;
        }
        buffer->num_used_blocks++;
    }

    buffer->curr_block       = buffer->head_block;
}

/*
    CLOG_Buffer_localIO_finalize -
    Close CLOG_Buffer_t.local_fd (assuming the local_fd is > 0 ).
    Also, delete the CLOG_Buffer_t.local_filename (assuming file already opened)
*/
void CLOG_Buffer_localIO_finalize( CLOG_Buffer_t *buffer )
{
    if ( buffer->local_fd != CLOG_NULL_FILE ) {
        close( buffer->local_fd );
        buffer->local_fd = CLOG_NULL_FILE;
        if (    buffer->delete_localfile == CLOG_BOOL_TRUE
             && strlen( buffer->local_filename ) != 0 )
            unlink( buffer->local_filename );
    }
}




void CLOG_Buffer_save_endlog( CLOG_Buffer_t *buffer )
{
    CLOG_BlockData_t   *blkdata;
    CLOG_Rec_Header_t  *hdr;

    if ( buffer->status == CLOG_INIT_AND_ON ) {
        blkdata          = buffer->curr_block->data;
        hdr              = (CLOG_Rec_Header_t *) blkdata->ptr;
        hdr->timestamp   = CLOG_Timer_get();
        hdr->rectype     = CLOG_REC_ENDLOG;
        hdr->procID      = buffer->local_mpi_rank;
        blkdata->ptr     = hdr->rest;  /* points to next available header */
    }
    else if ( buffer->status == CLOG_UNINIT ) {
        fprintf( stderr, __FILE__":CLOG_Buffer_save_endlog() - \n"
                         "\t""CLOG is used before being initialized.\n" );
        fflush( stderr );
        CLOG_Util_abort( 1 );
    }
}

void CLOG_Buffer_save_endblock( CLOG_Buffer_t *buffer )
{
    CLOG_BlockData_t   *blkdata;
    CLOG_Rec_Header_t  *hdr;

    if ( buffer->status == CLOG_INIT_AND_ON ) {
        blkdata          = buffer->curr_block->data;
        hdr              = (CLOG_Rec_Header_t *) blkdata->ptr;
        hdr->timestamp   = CLOG_Timer_get();
        hdr->rectype     = CLOG_REC_ENDBLOCK;
        hdr->procID      = buffer->local_mpi_rank;
        blkdata->ptr     = hdr->rest;  /* points to next available header */
    }
    else if ( buffer->status == CLOG_UNINIT ) {
        fprintf( stderr, __FILE__":CLOG_Buffer_save_endblock() - \n"
                         "\t""CLOG is used before being initialized.\n" );
        fflush( stderr );
        CLOG_Util_abort( 1 );
    }
}

void CLOG_Buffer_save_header( CLOG_Buffer_t *buffer, int rectype )
{
    CLOG_BlockData_t   *blkdata;
    CLOG_Rec_Header_t  *hdr;

    blkdata          = buffer->curr_block->data;
    if ( blkdata->ptr + CLOG_RECLEN_MAX >= blkdata->tail ) {
        CLOG_Buffer_save_endblock( buffer );
        CLOG_Buffer_advance_block( buffer );
        blkdata      = buffer->curr_block->data;
    }
    hdr              = (CLOG_Rec_Header_t *) blkdata->ptr;
    hdr->timestamp   = CLOG_Timer_get();
    hdr->rectype     = rectype;
    hdr->procID      = buffer->local_mpi_rank;
    blkdata->ptr     = hdr->rest;  /* advance to next available space */
}

void CLOG_Buffer_save_statedef( CLOG_Buffer_t *buffer,
                                int stateID, int startetype, int finaletype,
                                const char *color, const char *name,
                                const char *format )
{
    CLOG_BlockData_t     *blkdata;
    CLOG_Rec_StateDef_t  *statedef;

    if ( buffer->status == CLOG_INIT_AND_ON ) {
        CLOG_Buffer_save_header( buffer, CLOG_REC_STATEDEF );
        blkdata               = buffer->curr_block->data;
        statedef              = (CLOG_Rec_StateDef_t *) blkdata->ptr;
        statedef->stateID     = stateID;
        statedef->startetype  = startetype;
        statedef->finaletype  = finaletype;

        if ( color ) {
            strncpy( statedef->color, color, sizeof(CLOG_Str_Color_t) );
            statedef->color[ sizeof(CLOG_Str_Color_t) - 1 ] = '\0';
        }
        else
            * ( (char *) statedef->color ) = '\0';

        if ( name ) {
            strncpy( statedef->name, name, sizeof(CLOG_Str_Desc_t) );
            statedef->name[ sizeof(CLOG_Str_Desc_t) - 1 ] = '\0';
        }
        else
            * ( (char *) statedef->name ) = '\0';

        if ( format ) {
            strncpy( statedef->format, format, sizeof(CLOG_Str_Format_t) );
            statedef->format[ sizeof(CLOG_Str_Format_t) - 1 ] = '\0';
        }
        else
            * ( (char *) statedef->format ) = '\0';

        blkdata->ptr          = statedef->end;
    }
    else if ( buffer->status == CLOG_UNINIT ) {
        fprintf( stderr, __FILE__":CLOG_Buffer_save_statedef() - \n"
                         "\t""CLOG is used before being initialized.\n" );
        fflush( stderr );
        CLOG_Util_abort( 1 );
    }
}

void CLOG_Buffer_save_eventdef( CLOG_Buffer_t *buffer, int etype,
                                const char *color, const char *name,
                                const char *format )
{
    CLOG_BlockData_t     *blkdata;
    CLOG_Rec_EventDef_t  *eventdef;

    if ( buffer->status == CLOG_INIT_AND_ON ) {
        CLOG_Buffer_save_header( buffer, CLOG_REC_EVENTDEF );
        blkdata               = buffer->curr_block->data;
        eventdef              = (CLOG_Rec_EventDef_t *) blkdata->ptr;
        eventdef->etype       = etype;

        if ( color ) {
            strncpy( eventdef->color, color, sizeof(CLOG_Str_Color_t) );
            eventdef->color[ sizeof(CLOG_Str_Color_t) - 1 ] = '\0';
        }
        else
            * ( (char *) eventdef->color ) = '\0';

        if ( name ) {
            strncpy( eventdef->name, name, sizeof(CLOG_Str_Desc_t) );
            eventdef->name[ sizeof(CLOG_Str_Desc_t) - 1 ] = '\0';
        }
        else
            * ( (char *) eventdef->name ) = '\0';

        if ( format ) {
            strncpy( eventdef->format, format, sizeof(CLOG_Str_Format_t) );
            eventdef->format[ sizeof(CLOG_Str_Format_t) - 1 ] = '\0';
        }
        else
            * ( (char *) eventdef->format ) = '\0';

        blkdata->ptr          = eventdef->end;
    }
    else if ( buffer->status == CLOG_UNINIT ) {
        fprintf( stderr, __FILE__":CLOG_Buffer_save_eventdef() - \n"
                         "\t""CLOG is used before being initialized.\n" );
        fflush( stderr );
        CLOG_Util_abort( 1 );
    }
}

void CLOG_Buffer_save_constdef( CLOG_Buffer_t *buffer,
                                int etype, int value, const char *name )
{
    CLOG_BlockData_t     *blkdata;
    CLOG_Rec_ConstDef_t  *constdef;

    if ( buffer->status == CLOG_INIT_AND_ON ) {
        CLOG_Buffer_save_header( buffer, CLOG_REC_CONSTDEF );
        blkdata               = buffer->curr_block->data;
        constdef              = (CLOG_Rec_ConstDef_t *) blkdata->ptr;
        constdef->etype       = etype;
        constdef->value       = value;

        if ( name ) {
            strncpy( constdef->name, name, sizeof(CLOG_Str_Desc_t) );
            constdef->name[ sizeof(CLOG_Str_Desc_t) - 1 ] = '\0';
        }
        else
            * ( (char *) constdef->name ) = '\0';

        blkdata->ptr          = constdef->end;
    }
    else if ( buffer->status == CLOG_UNINIT ) {
        fprintf( stderr, __FILE__":CLOG_Buffer_save_constdef() - \n"
                         "\t""CLOG is used before being initialized.\n" );
        fflush( stderr );
        CLOG_Util_abort( 1 );
    }
}

void CLOG_Buffer_save_bareevt( CLOG_Buffer_t *buffer, int etype )
{
    CLOG_BlockData_t     *blkdata;
    CLOG_Rec_BareEvt_t   *bareevt;

    if ( buffer->status == CLOG_INIT_AND_ON ) {
        CLOG_Buffer_save_header( buffer, CLOG_REC_BAREEVT );
        blkdata               = buffer->curr_block->data;
        bareevt               = (CLOG_Rec_BareEvt_t *) blkdata->ptr;
        bareevt->etype        = etype;
        blkdata->ptr          = bareevt->end;
    }
    else if ( buffer->status == CLOG_UNINIT ) {
        fprintf( stderr, __FILE__":CLOG_Buffer_save_bareevt() - \n"
                         "\t""CLOG is used before being initialized.\n" );
        fflush( stderr );
        CLOG_Util_abort( 1 );
    }
}

void CLOG_Buffer_save_cargoevt( CLOG_Buffer_t *buffer,
                                int etype, const char *bytes )
{
    CLOG_BlockData_t     *blkdata;
    CLOG_Rec_CargoEvt_t  *cargoevt;

    if ( buffer->status == CLOG_INIT_AND_ON ) {
        CLOG_Buffer_save_header( buffer, CLOG_REC_CARGOEVT );
        blkdata               = buffer->curr_block->data;
        cargoevt              = (CLOG_Rec_CargoEvt_t *) blkdata->ptr;
        cargoevt->etype       = etype;

        if ( bytes )
            memcpy( cargoevt->bytes, bytes, sizeof(CLOG_Str_Bytes_t) );

        blkdata->ptr          = cargoevt->end;
    }
    else if ( buffer->status == CLOG_UNINIT ) {
        fprintf( stderr, __FILE__":CLOG_Buffer_save_cargoevt() - \n"
                         "\t""CLOG is used before being initialized.\n" );
        fflush( stderr );
        CLOG_Util_abort( 1 );
    }
}

void CLOG_Buffer_save_msgevt( CLOG_Buffer_t *buffer,
                              int etype, int tag, int partner,
                              int comm, int size )
{
    CLOG_BlockData_t     *blkdata;
    CLOG_Rec_MsgEvt_t    *msgevt;

    if ( buffer->status == CLOG_INIT_AND_ON ) {
        CLOG_Buffer_save_header( buffer, CLOG_REC_MSGEVT );
        blkdata               = buffer->curr_block->data;
        msgevt                = (CLOG_Rec_MsgEvt_t *) blkdata->ptr;
        msgevt->etype         = etype;
        msgevt->tag           = tag;
        msgevt->partner       = partner;
        msgevt->comm          = comm;
        msgevt->size          = size;
        blkdata->ptr          = msgevt->end;
    }
    else if ( buffer->status == CLOG_UNINIT ) {
        fprintf( stderr, __FILE__":CLOG_Buffer_save_msgevt() - \n"
                         "\t""CLOG is used before being initialized.\n" );
        fflush( stderr );
        CLOG_Util_abort( 1 );
    }
}

void CLOG_Buffer_save_collevt( CLOG_Buffer_t *buffer,
                               int etype, int root, int size, int comm )
{
    CLOG_BlockData_t     *blkdata;
    CLOG_Rec_CollEvt_t   *collevt;

    if ( buffer->status == CLOG_INIT_AND_ON ) {
        CLOG_Buffer_save_header( buffer, CLOG_REC_COLLEVT );
        blkdata               = buffer->curr_block->data;
        collevt               = (CLOG_Rec_CollEvt_t *) blkdata->ptr;
        collevt->etype        = etype;
        collevt->root         = root;
        collevt->comm         = comm;
        collevt->size         = size;
        blkdata->ptr          = collevt->end;
    }
    else if ( buffer->status == CLOG_UNINIT ) {
        fprintf( stderr, __FILE__":CLOG_Buffer_save_collevt() - \n"
                         "\t""CLOG is used before being initialized.\n" );
        fflush( stderr );
        CLOG_Util_abort( 1 );
    }
}

void CLOG_Buffer_save_commevt( CLOG_Buffer_t *buffer,
                               int etype, int parent, int newcomm )
{
    CLOG_BlockData_t     *blkdata;
    CLOG_Rec_CommEvt_t   *commevt;

    if ( buffer->status == CLOG_INIT_AND_ON ) {
        CLOG_Buffer_save_header( buffer, CLOG_REC_COMMEVT );
        blkdata               = buffer->curr_block->data;
        commevt               = (CLOG_Rec_CommEvt_t *) blkdata->ptr;
        commevt->etype        = etype;
        commevt->parent       = parent;
        commevt->newcomm      = newcomm;
        blkdata->ptr          = commevt->end;
    }
    else if ( buffer->status == CLOG_UNINIT ) {
        fprintf( stderr, __FILE__":CLOG_Buffer_save_commevt() - \n"
                         "\t""CLOG is used before being initialized.\n" );
        fflush( stderr );
        CLOG_Util_abort( 1 );
    }
}

void CLOG_Buffer_save_srcloc( CLOG_Buffer_t *buffer,
                              int srcloc, int lineno, const char *filename )
{
    CLOG_BlockData_t     *blkdata;
    CLOG_Rec_Srcloc_t    *srcevt;

    if ( buffer->status == CLOG_INIT_AND_ON ) {
        CLOG_Buffer_save_header( buffer, CLOG_REC_SRCLOC );
        blkdata               = buffer->curr_block->data;
        srcevt                = (CLOG_Rec_Srcloc_t *) blkdata->ptr;
        srcevt->srcloc        = srcloc;
        srcevt->lineno        = lineno;

        strncpy( srcevt->filename, filename, sizeof(CLOG_Str_File_t) );
        srcevt->filename[ sizeof(CLOG_Str_File_t) - 1 ] = '\0';

        blkdata->ptr          = srcevt->end;
    }
    else if ( buffer->status == CLOG_UNINIT ) {
        fprintf( stderr, __FILE__":CLOG_Buffer_save_srcloc() - \n"
                         "\t""CLOG is used before being initialized.\n" );
        fflush( stderr );
        CLOG_Util_abort( 1 );
    }
}

void CLOG_Buffer_save_timeshift( CLOG_Buffer_t *buffer,
                                 CLOG_Time_t    timeshift )
{
    CLOG_BlockData_t     *blkdata;
    CLOG_Rec_Timeshift_t *tshift;

    if ( buffer->status == CLOG_INIT_AND_ON ) {
        CLOG_Buffer_save_header( buffer, CLOG_REC_TIMESHIFT );
        blkdata               = buffer->curr_block->data;
        tshift                = (CLOG_Rec_Timeshift_t *) blkdata->ptr;
        tshift->timeshift     = timeshift;
        blkdata->ptr          = tshift->end;
    }
    else if ( buffer->status == CLOG_UNINIT ) {
        fprintf( stderr, __FILE__":CLOG_Buffer_save_timeshift() - \n"
                         "\t""CLOG is used before being initialized.\n" );
        fflush( stderr );
        CLOG_Util_abort( 1 );
    }
}
