/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#if !defined( _CLOG_MERGER )
#define _CLOG_MERGER

#include "clog_block.h"
#include "clog_buffer.h"
#include "clog_record.h"
#include "clog_common.h"

#define CLOG_MERGE_LOGBUFTYPE  777

#define CLOG_MAXTIME           100000000.0       /* later than all times */

typedef struct {
   CLOG_BlockData_t   *left_blk;
   CLOG_BlockData_t   *right_blk;
   CLOG_BlockData_t   *sorted_blk;
   unsigned int        block_size;
   int                 num_active_blks;
   int                 num_mpi_procs;
   int                 local_mpi_rank;
   int                 left_mpi_rank;
   int                 right_mpi_rank;
   int                 parent_mpi_rank;
   int                 is_big_endian;
   char                out_filename[ CLOG_PATH_STRLEN ];
   int                 out_fd;
} CLOG_Merger_t;

CLOG_Merger_t* CLOG_Merger_create( unsigned int  block_size );

void CLOG_Merger_free( CLOG_Merger_t **merger_handle );

void CLOG_Merger_set_neighbor_ranks( CLOG_Merger_t *merger );

void CLOG_Merger_init(       CLOG_Merger_t    *merger,
                       const CLOG_Preamble_t  *preamble,
                       const char             *merged_file_prefix );

void CLOG_Merger_finalize( CLOG_Merger_t *merger );

void CLOG_Merger_flush( CLOG_Merger_t *merger );

void CLOG_Merger_save_rec( CLOG_Merger_t *merger, CLOG_Rec_Header_t *hdr );

int CLOG_Merger_reserved_block_size( unsigned int rectype );

void CLOG_Merger_refill_sideblock( CLOG_BlockData_t  *blockdata,
                                   int block_mpi_rank, int block_size );

void CLOG_Merger_refill_localblock( CLOG_BlockData_t *blockdata,
                                    CLOG_Buffer_t    *buffer,
                                    CLOG_Time_t      *timediff_handle );

CLOG_Rec_Header_t *
CLOG_Merger_next_sideblock_hdr( CLOG_BlockData_t   *blockdata,
                                CLOG_Rec_Header_t  *hdr,
                                CLOG_Merger_t      *merger,
                                int                 block_mpi_rank,
                                int                 block_size );

CLOG_Rec_Header_t *
CLOG_Merger_next_localblock_hdr( CLOG_BlockData_t   *blockdata,
                                 CLOG_Rec_Header_t  *hdr,
                                 CLOG_Merger_t      *merger,
                                 CLOG_Buffer_t      *buffer,
                                 CLOG_Time_t        *timediff_handle );

void CLOG_Merger_sort( CLOG_Merger_t *merger, CLOG_Buffer_t *buffer );

void CLOG_Merger_last_flush( CLOG_Merger_t *merger );

#endif
