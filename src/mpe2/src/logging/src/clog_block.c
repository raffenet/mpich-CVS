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

#include "clog_common.h"
#include "clog_block.h"
#include "clog_record.h"
/*
    Input parameter, block_size, is asummed to be divisible by sizoef(double)
*/
CLOG_BlockData_t* CLOG_BlockData_create( unsigned int block_size )
{
    CLOG_BlockData_t* data;
    int               data_unit_size;

    if ( block_size <= 0 )
        return NULL;

    data = (CLOG_BlockData_t *) MALLOC( sizeof( CLOG_BlockData_t) );
    if ( data == NULL ) {
        fprintf( stderr, __FILE__":CLOG_BlockData_create() - "
                         "MALLOC() fails for CLOG_BlockData_t!\n" );
        fflush( stderr );
        return NULL;
    }

    data->head = (CLOG_DataUnit_t *) MALLOC( block_size );
    if ( data->head == NULL ) {
        fprintf( stderr, __FILE__":CLOG_BlockData_create() - "
                         "MALLOC(%d) fails!\n", block_size );
        fflush( stderr );
        return NULL;
    }

    data_unit_size = block_size / sizeof(CLOG_DataUnit_t);
    data->tail = data->head + data_unit_size;
    data->ptr  = data->head;

    return data;
}

void CLOG_BlockData_free( CLOG_BlockData_t **data_handle )
{
    CLOG_BlockData_t *data;
    data = *data_handle;
    if ( data != NULL ) {
        if ( data->head != NULL )
            FREE( data->head );
        FREE( data );
    }
    *data_handle = NULL;
}

void CLOG_BlockData_reset( CLOG_BlockData_t *data )
{
    if ( data != NULL )
        data->ptr  = data->head;
}

/*
    local_proc_timediff is modifiable
*/
void CLOG_BlockData_patch( CLOG_BlockData_t *data,
                           CLOG_Time_t      *local_proc_timediff )
{
    CLOG_Rec_Header_t     *hdr;
    CLOG_Rec_Timeshift_t  *tshift;
    int                    rectype;

    /* Assume at least 1 CLOG_Rec_Header_t like record in CLOG_BlockData_t.  */
    /*
       Assume CLOG_REC_TIMESHIFT can be only in the start of the
       CLOG_BlockData_t.  If the assumption needs to relaxed such that
       CLOG_REC_TIMESHIFT can be anywhere in the block, then the detection
       of CLOG_REC_TIMESHIFT (the if statement) can be moved inside
       the do-while loop.
    */
    hdr      = (CLOG_Rec_Header_t *) data->head;
    do {
        rectype        = hdr->rectype;
        if ( rectype == CLOG_REC_TIMESHIFT ) {
            tshift                = (CLOG_Rec_Timeshift_t *) hdr->rest;
            *local_proc_timediff  = tshift->timeshift;
            tshift->timeshift    *= -1;
        }
        hdr->timestamp += *local_proc_timediff;
        hdr            = (CLOG_Rec_Header_t *)
                         ( (CLOG_DataUnit_t *) hdr + CLOG_Rec_size( rectype ) );
    } while ( rectype != CLOG_REC_ENDLOG && rectype != CLOG_REC_ENDBLOCK );
}

/*
   Assume readable(understandable) byte ordering before byteswapping
*/
void CLOG_BlockData_swap_bytes_last( CLOG_BlockData_t *data )
{
    CLOG_Rec_Header_t  *hdr;
    int                 rectype;

    /* Assume at least 1 CLOG_Rec_Header_t like record in CLOG_BlockData_t.  */
    hdr      = (CLOG_Rec_Header_t *) data->head;
    do {
        rectype  = hdr->rectype;
        CLOG_Rec_swap_bytes_last( hdr );
        hdr      = (CLOG_Rec_Header_t *)
                   ( (CLOG_DataUnit_t *) hdr + CLOG_Rec_size( rectype ) );
    } while ( rectype != CLOG_REC_ENDLOG && rectype != CLOG_REC_ENDBLOCK );
}

/*
   Assume non-readable byte ordering before byteswapping
*/
void CLOG_BlockData_swap_bytes_first( CLOG_BlockData_t *data )
{
    CLOG_Rec_Header_t  *hdr;
    int                 rectype;

    /* Assume at least 1 CLOG_Rec_Header_t like record in CLOG_BlockData_t.  */
    hdr      = (CLOG_Rec_Header_t *) data->head;
    do {
        CLOG_Rec_swap_bytes_first( hdr );
        rectype  = hdr->rectype;
        hdr      = (CLOG_Rec_Header_t *)
                   ( (CLOG_DataUnit_t *) hdr + CLOG_Rec_size( rectype ) );
    } while ( rectype != CLOG_REC_ENDLOG && rectype != CLOG_REC_ENDBLOCK );
}

/*
   Assume CLOG_BlockData_t is understandable, no byteswapping is needed.
*/
void CLOG_BlockData_print( CLOG_BlockData_t *data, FILE *stream )
{
    CLOG_Rec_Header_t     *hdr;
    int                    rectype;

    /* Assume at least 1 CLOG_Rec_Header_t like record in CLOG_BlockData_t.  */
    hdr      = (CLOG_Rec_Header_t *) data->head;
    do {
        CLOG_Rec_print( hdr, stream );
        rectype  = hdr->rectype;
        hdr      = (CLOG_Rec_Header_t *)
                   ( (CLOG_DataUnit_t *) hdr + CLOG_Rec_size( rectype ) );
    } while ( rectype != CLOG_REC_ENDLOG && rectype != CLOG_REC_ENDBLOCK );
}




/*
    Input parameter, block_size, is asummed to be divisible by sizoef(double)
*/
CLOG_Block_t* CLOG_Block_create( unsigned int block_size )
{
    CLOG_Block_t *blk;

    if ( block_size <= 0 )
        return NULL;

    blk = (CLOG_Block_t *) MALLOC( sizeof( CLOG_Block_t ) );
    if ( blk == NULL ) {
        fprintf( stderr, __FILE__":CLOG_Block_create() - "
                         "MALLOC() fails for CLOG_Block_t!\n" );
        fflush( stderr );
        return NULL;
    }

    blk->data = CLOG_BlockData_create( block_size );
    if ( blk->data == NULL ) {
        fprintf( stderr, __FILE__":CLOG_Block_create() - "
                         "CLOG_BlockData_create(%d) fails!",
                         block_size );
        fflush( stderr );
        return NULL;
    }
    blk->next      = NULL;

    return blk;
}

void CLOG_Block_free( CLOG_Block_t **blk_handle )
{
    CLOG_Block_t *blk;
    blk = *blk_handle;
    if ( blk != NULL ) {
        CLOG_BlockData_free( &(blk->data) );
        FREE( blk );
    }
    *blk_handle = NULL;
}

void CLOG_Block_reset( CLOG_Block_t *block )
{
    if ( block != NULL )
        CLOG_BlockData_reset( block->data );
}
