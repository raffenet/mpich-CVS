/* clog.c */
#include "mpe_logging_conf.h"
#include <fcntl.h>
#if defined( STDC_HEADERS ) || defined( HAVE_STDLIB_H )
#include <stdlib.h>
#endif
#if defined( STDC_HEADERS ) || defined( HAVE_STRING_H )
#include <string.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "clogimpl.h"
#include "mpi.h"
#ifdef HAVE_WINDOWS_H
#include <io.h>
#include <stdlib.h>
#include <windows.h>
#endif

void CLOG_nodebuffer2disk( void );

#define     CLOG_name_len 256

char        CLOG_outdir[CLOG_DIR_LEN];
int         CLOG_status = 2;                /* initialized to CLOG not init,
                                               but logging on */
int         CLOG_Comm;                      /* Default communicator */
void       *CLOG_ptr;                       /* pointer into buffer,
                                               where next rec goes */
void       *CLOG_block_end;                 /* pointer to end of buffer */
CLOG_BLOCK *CLOG_first, *CLOG_currbuff;     /* blocks of buffer */
int         CLOG_intsperdouble;
int         CLOG_charsperdouble;
int         CLOG_nextevent = CLOG_MAXEVENT;
int         CLOG_nextstate = CLOG_MAXSTATE;
char        CLOG_execname[CLOG_name_len];   /* name used for naming logfile
                                               (executable) */
char        CLOG_tmpfilename[CLOG_name_len] = "";
                                            /* local logfile name (abhi)*/ 
int         CLOG_tempFD = -5;             /* temp log file descriptor (abhi) */
                                          /* buffers for clog-merge (abhi) */ 
double     *CLOG_out_buffer;
double     *CLOG_left_buffer;
double     *CLOG_right_buffer;
int         CLOG_num_blocks = 0;
static int  my_rank;                 /* process id of this process required for
                                        error messages (abhi)*/

/*@
    CLOG_Init - Initialize for CLOG logging
@*/
void CLOG_Init( void )
{
    PMPI_Comm_rank(MPI_COMM_WORLD, &my_rank); /* (abhi) */
    CLOG_Comm      = 0;                /* until we pass communicator in to CLOG_Init*/
    CLOG_timeinit();                /* initialize timer */
    CLOG_init_buffers();        /*(abhi)*/
    CLOG_status          &= 0x01;        /* set initialized  */
    CLOG_intsperdouble  = sizeof(double) / sizeof(int);
    CLOG_charsperdouble = sizeof(double) / sizeof(char);
    CLOG_init_tmpfilename();    /*(abhi)*/
}

/*@
    CLOG_init_buffers - initialize necessary buffers for clog logging.
@*/
void CLOG_init_buffers( void )
{
    CLOG_left_buffer   = (double *) MALLOC ( CLOG_BLOCK_SIZE );
    CLOG_right_buffer  = (double *) MALLOC ( CLOG_BLOCK_SIZE );
    CLOG_out_buffer    = (double *) MALLOC ( CLOG_BLOCK_SIZE );
    CLOG_newbuff(&CLOG_first);        /* get first buffer */
    if (    (!CLOG_left_buffer) || (!CLOG_right_buffer)
         || (!CLOG_out_buffer) || (!CLOG_first) ) {
        fprintf(stderr, __FILE__":CLOG_init_buffers() - \n"
                        "\t""Unable to allocate memory for logging "
                        "at process %d\n", my_rank);
        fflush(stderr);
        PMPI_Abort(MPI_COMM_WORLD, 1);
    }
}


/*@
    CLOG_Finalize - Finalize  CLOG logging
@*/
void CLOG_Finalize( void )
{
    CLOG_LOGENDLOG();
}

/*@
    CLOG_newbuff - get and initialize new block of buffer

Input Parameter:

. bufptr - pointer to be filled in with address of new block

@*/
void CLOG_newbuff( CLOG_BLOCK **bufptr )
{
    if ( CLOG_num_blocks == MAX_CLOG_BLOCKS )
        CLOG_nodebuffer2disk();
    else if( CLOG_tempFD == -5 ) {
        *bufptr = (CLOG_BLOCK *) MALLOC ( sizeof( CLOG_BLOCK ) );
        if ( *bufptr == NULL )
            CLOG_nodebuffer2disk();
        else {
            (*bufptr)->next = NULL;
            CLOG_currbuff = *bufptr;
        }
    }
    else {
        /* bufptr = (*bufptr)->next; */
        if ( *bufptr == NULL ) 
            CLOG_nodebuffer2disk();
        else
            CLOG_currbuff = *bufptr;
    }        
    CLOG_num_blocks++;
    /* CLOG_currbuff = *bufptr; */
    CLOG_ptr = CLOG_currbuff->data;
    CLOG_block_end = (void *) ((char *) CLOG_ptr + CLOG_BLOCK_SIZE);
}

/*@
    CLOG_nodebuffer2disk - dump buffers into temporary log file.
@*/
void CLOG_nodebuffer2disk( void )
{
    CLOG_BLOCK   *buffer_parser;
    int           ierr;

    if ( CLOG_tempFD == -5 ) {
        CLOG_tempFD = OPEN(CLOG_tmpfilename, O_RDWR|O_CREAT|O_TRUNC, 0600);
        if ( CLOG_tempFD == -1 ) {
            fprintf( stderr, __FILE__":CLOG_nodebuffer2disk() - \n"
                             "\t""Unable to open temporary log file %s.\n"
                             "\t""Check if the directory where the logfile "
                             "resides exists\n"
                             "\t""and the corresponding file system is "
                             "NOT full\n."
                             "If not so, set environmental variable TMPDIR to "
                             "a bigger filesystem.", CLOG_tmpfilename );
            fflush( stderr );
            PMPI_Abort( MPI_COMM_WORLD, 1 );
        }
    }

    buffer_parser = CLOG_first;
    while ( buffer_parser && (CLOG_num_blocks--) ) {
        ierr = write( CLOG_tempFD, buffer_parser, sizeof( CLOG_BLOCK ) );
        if ( ierr != sizeof( CLOG_BLOCK ) ) {
            fprintf( stderr, __FILE__":CLOG_nodebuffer2disk() - \n"
                             "\t""Unable to write temporary log file %s.\n"
                             "\t""Check if the filesystem where the logfile "
                             "resides is full\n."
                             "If so, set environmental variable TMPDIR to "
                             "a bigger filesystem.", CLOG_tmpfilename );
            fflush( stderr );
            PMPI_Abort( MPI_COMM_WORLD, 1 );
        }
        buffer_parser = buffer_parser->next;
    }
    CLOG_currbuff = CLOG_first;
    CLOG_num_blocks = 0;
}

void CLOG_init_tmpfilename( void )
{
    char   *env_tmpdir = NULL;
    char    tmpdirname_ref[ CLOG_name_len ] = "";
    char    tmpdirname[ CLOG_name_len ] = "";
    char    tmpfilename[ CLOG_name_len ] = "";

    int     ierr;

    env_tmpdir = ( char * ) getenv( "TMPDIR" );

    /*  Set tmpdirname_ref to TMPDIR at Master if available  */
    if ( my_rank == 0 ) {
        if ( env_tmpdir != NULL )
            strcat( tmpdirname_ref, env_tmpdir );
        else
#ifdef HAVE_WINDOWS_H
            if (GetTempPath(CLOG_name_len, tmpdirname_ref) == 0)
                strcat( tmpdirname_ref, "\\");
#else
            strcat( tmpdirname_ref, "/tmp" );
#endif
    }

    /*  Let everyone in MPI_COMM_WORLD know  */
    ierr = PMPI_Bcast( tmpdirname_ref, CLOG_name_len, MPI_CHAR,
                       0, MPI_COMM_WORLD );
    if ( ierr != MPI_SUCCESS ) {
        fprintf( stderr, __FILE__"CLOG_init_tmpfilename() - \n"
                         "\t""PMPI_Bcast() fails\n" );
        fflush( stderr );
        PMPI_Abort( MPI_COMM_WORLD, 1 );
    }

    /*  Use TMPDIR if set in children processes  */
    if ( env_tmpdir != NULL )
        strcpy( tmpdirname, env_tmpdir );
    else
        strcpy( tmpdirname, tmpdirname_ref );

    if ( strlen( tmpdirname ) <= 0 ) {
        fprintf( stderr, __FILE__"CLOG_init_tmpfilename() - \n"
                         "\t""strlen(tmpdirname) = %d\n",
                         (int)strlen( tmpdirname ) );
        fflush( stderr );
        PMPI_Abort( MPI_COMM_WORLD, 1 );
    }

    /*  Set the local tmp filename then CLOG_tmpfilename */
    strcpy( CLOG_tmpfilename, tmpdirname );
    sprintf( tmpfilename, "/clog_taskID=%04d_XXXXXX", my_rank );
    strcat( CLOG_tmpfilename, tmpfilename );

    /*  Make the filename unique ( optional ) */
#ifdef HAVE_MKSTEMP
    mkstemp( CLOG_tmpfilename );
#else
    mktemp( CLOG_tmpfilename );
#endif
}
    
    
    

/************* to become macros once debugged ***********************/

void CLOG_put_hdr( int type )
{
    if (((char *) CLOG_ptr + CLOG_MAX_REC_LEN) >= (char *) CLOG_block_end) {
        CLOG_LOGENDBLOCK();
        CLOG_newbuff(&CLOG_currbuff->next);
    }
    ((CLOG_HEADER *) CLOG_ptr)->timestamp = CLOG_timestamp(); 
    ((CLOG_HEADER *) CLOG_ptr)->rectype   = type;
                                /* int length will be filled in later */
                                /* int procID will be filled in later */
    CLOG_ptr = ((CLOG_HEADER *) CLOG_ptr)->rest; /* point past header */
}

void CLOG_LOGSTATE( int stateID, int startetype, int finaletype,
                    const char *color, const char *name, const char *format )
{
    if (CLOG_OK) {
        CLOG_put_hdr(CLOG_STATEDEF);
        ((CLOG_STATE *) CLOG_ptr)-> stateID    = stateID;
        ((CLOG_STATE *) CLOG_ptr)-> startetype = startetype;
        ((CLOG_STATE *) CLOG_ptr)-> finaletype = finaletype;

        if (color) {
            strncpy(((CLOG_STATE *)CLOG_ptr)->color, color,
                    sizeof(CLOG_COLOR));
            ((CLOG_STATE *)CLOG_ptr)->color[sizeof(CLOG_COLOR)-1] = '\0';
        }
        else
            * ((char *) ((CLOG_STATE *)CLOG_ptr)->color) = '\0';

        if (name) {
            strncpy(((CLOG_STATE *)CLOG_ptr)->name, name,
                    sizeof(CLOG_DESC));
            ((CLOG_STATE *)CLOG_ptr)->name[sizeof(CLOG_DESC)-1] = '\0';
        }
        else
            * ((char *) ((CLOG_STATE *)CLOG_ptr)->name) = '\0';

        if (format) {
            strncpy(((CLOG_STATE *)CLOG_ptr)->format, format,
                    sizeof(CLOG_FORMAT));
            ((CLOG_STATE *)CLOG_ptr)->format[sizeof(CLOG_FORMAT)-1] = '\0';
        }
        else
            * ((char *) ((CLOG_STATE *)CLOG_ptr)->format) = '\0';

        CLOG_ptr                      = ((CLOG_STATE *) CLOG_ptr)->end;
    }                                                                 
    else if (CLOG_ERROR)                                              
        CLOG_not_init;                                                  
}

void CLOG_LOGEVENT( int etype,
                    const char *color, const char *name, const char *format )
{
    if (CLOG_OK) {
        CLOG_put_hdr(CLOG_EVENTDEF);
        ((CLOG_EVENT *) CLOG_ptr)->etype   = etype;

        if (color) {
            strncpy(((CLOG_EVENT *)CLOG_ptr)->color, color,
                    sizeof(CLOG_COLOR));
            ((CLOG_EVENT *)CLOG_ptr)->color[sizeof(CLOG_COLOR)-1] = '\0';
        }

        if (name) {
            strncpy(((CLOG_EVENT *)CLOG_ptr)->name, name,
                    sizeof(CLOG_DESC) - 1);
            ((CLOG_EVENT *)CLOG_ptr)->name[sizeof(CLOG_DESC)-1] = '\0';
        }
        else
            * ((char *) ((CLOG_EVENT *)CLOG_ptr)->name) = '\0';

        if (format) {
            strncpy(((CLOG_EVENT *)CLOG_ptr)->format, format,
                    sizeof(CLOG_FORMAT));
            ((CLOG_EVENT *)CLOG_ptr)->format[sizeof(CLOG_FORMAT)-1] = '\0';
        }
        else
            * ((char *) ((CLOG_EVENT *)CLOG_ptr)->format) = '\0';

        CLOG_ptr                  = ((CLOG_EVENT *) CLOG_ptr)->end;
    }                                                                 
    else if (CLOG_ERROR)                                              
        CLOG_not_init;                                                  
}

void CLOG_LOGCONST( int etype, int value, const char *name )
{
    if (CLOG_OK) {
        CLOG_put_hdr(CLOG_CONSTDEF);
        ((CLOG_CONST *) CLOG_ptr)-> etype   = etype;
        ((CLOG_CONST *) CLOG_ptr)-> value   = value;
        if (name) {
            strncpy(((CLOG_CONST *)CLOG_ptr)->name, name,
                    sizeof(CLOG_DESC));
            ((CLOG_CONST *)CLOG_ptr)->name[sizeof(CLOG_DESC)-1] = '\0';
        }
        else
            * ((char *) ((CLOG_CONST *)CLOG_ptr)->name) = '\0';
        CLOG_ptr                         = ((CLOG_CONST *) CLOG_ptr)->end;
    }                                                                 
    else if (CLOG_ERROR)                                              
        CLOG_not_init;                                                  
}

void CLOG_LOGBARE( int etype )
{
    if (CLOG_OK) {
        CLOG_put_hdr(CLOG_BAREEVENT);
        ((CLOG_BARE *) CLOG_ptr)-> etype   = etype;
        CLOG_ptr    = ((CLOG_BARE *) CLOG_ptr)->end;
    }
    else if (CLOG_ERROR)
        CLOG_not_init;
}

void CLOG_LOGCARGO( int etype, const char *bytes)
{
    if (CLOG_OK) {
        CLOG_put_hdr(CLOG_CARGOEVENT);
        ((CLOG_CARGO *) CLOG_ptr)-> etype   = etype;

        if (bytes)
            memcpy(((CLOG_CARGO *)CLOG_ptr)->bytes, bytes,
                   sizeof(CLOG_BYTES));
        CLOG_ptr    = ((CLOG_CARGO *) CLOG_ptr)->end;
    }
    else if (CLOG_ERROR)
        CLOG_not_init;
}

void CLOG_LOGMSG( int etype, int tag, int partner, int comm, int size )
{
    if (CLOG_OK) {                                                    
        CLOG_put_hdr(CLOG_MSGEVENT);
        ((CLOG_MSG *) CLOG_ptr)-> etype   = etype;
        ((CLOG_MSG *) CLOG_ptr)-> tag     = tag;
        ((CLOG_MSG *) CLOG_ptr)-> partner = partner;
        ((CLOG_MSG *) CLOG_ptr)-> comm    = comm;
        ((CLOG_MSG *) CLOG_ptr)-> size    = size;
        CLOG_ptr                          = ((CLOG_MSG *) CLOG_ptr)->end;
    }                                                                 
    else if (CLOG_ERROR)                                              
        CLOG_not_init;
}

void CLOG_LOGCOLL( int etype, int root, int size, int comm )
{
    if (CLOG_OK) {                                                    
        CLOG_put_hdr(CLOG_COLLEVENT);
        ((CLOG_COLL *) CLOG_ptr)-> etype   = etype;
        ((CLOG_COLL *) CLOG_ptr)-> root    = root;
        ((CLOG_COLL *) CLOG_ptr)-> comm    = comm;
        ((CLOG_COLL *) CLOG_ptr)-> size    = size;
        CLOG_ptr                           = ((CLOG_COLL *) CLOG_ptr)->end;
    }                                                                 
    else if (CLOG_ERROR)                                              
        CLOG_not_init;                                                  
}

void CLOG_LOGCOMM( int etype, int parent, int newcomm )
{
    if (CLOG_OK) {
        CLOG_put_hdr(CLOG_COMMEVENT);
        ((CLOG_COMM *) CLOG_ptr)-> etype   = etype;
        ((CLOG_COMM *) CLOG_ptr)-> parent  = parent;
        ((CLOG_COMM *) CLOG_ptr)-> newcomm = newcomm;
        CLOG_ptr                           = ((CLOG_COMM *) CLOG_ptr)->end;
    }                                                                 
    else if (CLOG_ERROR)                                              
        CLOG_not_init;                                                  
}

void CLOG_LOGSRCLOC( int srcloc, int lineno, const char *filename )
{
    if (CLOG_OK) {
        CLOG_put_hdr(CLOG_SRCLOC);
        ((CLOG_SRC *) CLOG_ptr)->srcloc        = srcloc; 
        ((CLOG_SRC *) CLOG_ptr)->lineno        = lineno; 
        strncpy(((CLOG_SRC *)CLOG_ptr)->filename, filename,
                sizeof(CLOG_FILE));
        ((CLOG_SRC *)CLOG_ptr)->filename[sizeof(CLOG_FILE) - 1] = '\0';
        CLOG_ptr                        = ((CLOG_SRC *) CLOG_ptr)->end;
    }
    else if (CLOG_ERROR)
        CLOG_not_init;
}

void CLOG_LOGTIMESHIFT( double shift )
{
    if (CLOG_OK) {
        CLOG_put_hdr(CLOG_SHIFT);
        ((CLOG_TSHIFT *) CLOG_ptr)->timeshift = shift; 
        CLOG_ptr                = ((CLOG_TSHIFT *) CLOG_ptr)->end;
    }
    else if (CLOG_ERROR)
        CLOG_not_init;
}

void CLOG_LOGENDBLOCK( void )
{
    /* assumes there is room for this record */
    if (CLOG_OK) {
        ((CLOG_HEADER *) CLOG_ptr)->timestamp = CLOG_timestamp(); 
        ((CLOG_HEADER *) CLOG_ptr)->rectype   = CLOG_ENDBLOCK;
        CLOG_ptr = ((CLOG_HEADER *) CLOG_ptr)->rest; 
    }
    else
        if (CLOG_ERROR)                                              
            CLOG_not_init;                                                  
}
    
void CLOG_LOGENDLOG( void )
{
    /* assumes there is room for this record */
    if (CLOG_OK) {
        ((CLOG_HEADER *) CLOG_ptr)->timestamp = CLOG_timestamp(); 
        ((CLOG_HEADER *) CLOG_ptr)->rectype   = CLOG_ENDLOG;
        CLOG_ptr = ((CLOG_HEADER *) CLOG_ptr)->rest; 
    }
    else
        if (CLOG_ERROR)                                              
            CLOG_not_init;                                                  
}
       
/*@
    CLOG_get_new_event - obtain unused event id
@*/
int CLOG_get_new_event( void )
{
    return CLOG_nextevent++;
}

/*@
    CLOG_get_new_state - obtain unused state id
@*/
int CLOG_get_new_state( void )
{
    return CLOG_nextstate++;
}
