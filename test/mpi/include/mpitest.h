/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MTEST_INCLUDED
#define MTEST_INCLUDED
/*
 * Init and finalize test 
 */
void MTest_Init( int *, char *** );
void MTest_Finalize( int );
void MTestPrintError( int );

/*
 * This structure contains the information used to test datatypes
 */
typedef struct _MTest_Datatype {
    MPI_Datatype datatype;
    void *buf;              /* buffer to use in communication */
    int  count;             /* count to use for this datatype */
    int  isBasic;           /* true if the type is predefined */
    /* The following is optional data that is used by some of
       the derived datatypes */
    int  stride, blksize, *index;
    void *(*InitBuf)( struct _MTest_Datatype *, int );
    void *(*FreeBuf)( struct _MTest_Datatype * );
    int   (*CheckBuf)( struct _MTest_Datatype * );
} MTest_Datatype;

int MTestCheckRecv( MPI_Status *, MTest_Datatype * );
int MTestGetDatatypes( MTest_Datatype *, MTest_Datatype * );
void MTestResetDatatypes( void );
void MTestFreeDatatype( MTest_Datatype * );

int MTestGetIntracomm( MPI_Comm *, int );
int MTestGetIntercomm( MPI_Comm *, int *, int );
int MTestGetComm( MPI_Comm *, int );
const char *MTestGetIntracommName( void );
const char *MTestGetIntercommName( void );

/*#ifdef HAVE_MPI_WIN_CREATE*/
int MTestGetWin( MPI_Win *, int );
const char *MTestGetWinName( void );
/*#endif*/
#endif
