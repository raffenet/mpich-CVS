/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#if !defined( _CLOG )
#define _CLOG

#define CLOG_VERSION          "CLOG-02.10"

#include "clog_buffer.h"
#include "clog_sync.h"
#include "clog_merger.h"
#include "clog_record.h"

/* keep this larger than predefined event ids; it is for users */
#define CLOG_USER_EVENTID_START 500

/* keep this larger than predefined state ids; it is for users */
#define CLOG_USER_STATEID_START 200

#define CLOG_COMM_NULL          -1

typedef struct {
    CLOG_Buffer_t     *buffer;
    CLOG_Sync_t       *syncer;
    CLOG_Merger_t     *merger;
    int                next_eventID;
    int                next_stateID;
} CLOG_Stream_t;

CLOG_Stream_t *CLOG_Open( void );

void CLOG_Close( CLOG_Stream_t *stream );

void CLOG_Local_init( CLOG_Stream_t *stream, const char *local_tmpfile_name );

void CLOG_Local_finalize( CLOG_Stream_t *stream );

int  CLOG_Get_new_eventID( CLOG_Stream_t *stream );

int  CLOG_Get_new_stateID( CLOG_Stream_t *stream );

void CLOG_Converge_init(       CLOG_Stream_t *stream,
                         const char          *merged_file_prefix );

void CLOG_Converge_finalize( CLOG_Stream_t *stream );

void CLOG_Converge_sort( CLOG_Stream_t *stream );

#endif  /* of _CLOG */
