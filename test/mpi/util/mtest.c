/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "mpitest.h"
#include <stdio.h>
#include <stdlib.h>

/* 
 * Initialize and Finalize MTest
 */
void MTest_Init( int *argc, char ***argv )
{
    int flag;

    MPI_Initialized( &flag );
    if (!flag) {
	MPI_Init( argc, argv );
    }
}

void MTest_Finalize( int errs )
{
    int rank, toterrs;

    MPI_Comm_rank( MPI_COMM_WORLD, &rank );

    MPI_Allreduce( &errs, &toterrs, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD );
    if (rank == 0) {
	if (toterrs) {
	    printf( " Found %d errors\n", toterrs );
	}
	else {
	    printf( " No Errors\n" );
	}
	fflush( stdout );
    }
}

/*
 * Datatypes
 *
 * Eventually, this could read a description of a file.  For now, we hard 
 * code the choices
 *
 */
static int datatype_index = 0;

/* 
 * Setup contiguous buffers of n copies of a datatype.
 */
static void *MTestTypeContigInit( MTest_Datatype *mtype, int n )
{
    MPI_Aint size;
    if (n > 0) {
	signed char *p;
	int  i, totsize;
	MPI_Type_extent( mtype->datatype, &size );
	totsize = size * n;
	mtype->buf = (void *) malloc( totsize );
	p = (signed char *)(mtype->buf);
	if (!p) {
	    /* Error - out of memory */
	    
	}
	for (i=0; i<totsize; i++) {
	    p[i] = 0xff ^ (i & 0xff);
	}
    }
    else {
	mtype->buf = 0;
    }
    return mtype->buf;
}
static void *MTestTypeContigFree( MTest_Datatype *mtype )
{
    if (mtype->buf) {
	free( mtype->buf );
	mtype->buf = 0;
    }
    return 0;
}
static int MTestTypeContigCheckbuf( MTest_Datatype *mtype )
{
    signed char *p;
    int  i, totsize, err = 0;
    MPI_Aint size;

    p = mtype->buf;
    if (p) {
	MPI_Type_extent( mtype->datatype, &size );
	totsize = size * mtype->count;
	for (i=0; i<totsize; i++) {
	    if (p[i] != (0xff ^ (i & 0xff)))
		err++;
	}
    }
    return err;
}

/*
 * 
 */

static void *MTestTypeVectorInit( MTest_Datatype *mtype, int n )
{
    int size;
    if (n > 0) {
	MPI_Type_extent( mtype->datatype, &size );
	mtype->buf = (void *) malloc( n * size );
    }
    else {
	mtype->buf = 0;
    }
    return mtype->buf;
}

static void *MTestTypeVectorFree( MTest_Datatype *mtype )
{
    if (mtype->buf) {
	free( mtype->buf );
	mtype->buf = 0;
    }
    return 0;
}

int MTestGetDatatypes( MTest_Datatype *sendtype, MTest_Datatype *recvtype )
{
    sendtype->InitBuf  = 0;
    sendtype->FreeBuf  = 0;
    sendtype->datatype = 0;
    sendtype->isBasic  = 0;
    recvtype->InitBuf  = 0;
    recvtype->FreeBuf  = 0;
    recvtype->datatype = 0;
    recvtype->isBasic  = 0;

    /* Use datatype_index to choose a datatype to use.  If at the end of the
       list, return 0 */
    switch (datatype_index) {
    case 0:
	sendtype->datatype = MPI_INT;
	sendtype->isBasic  = 1;
	recvtype->datatype = MPI_INT;
	recvtype->isBasic  = 1;
	break;
    case 1:
	sendtype->datatype = MPI_DOUBLE;
	sendtype->isBasic  = 1;
	recvtype->datatype = MPI_DOUBLE;
	recvtype->isBasic  = 1;
	break;
    case 2:
	sendtype->datatype = MPI_INT;
	sendtype->isBasic  = 1;
	recvtype->datatype = MPI_BYTE;
	recvtype->isBasic  = 1;
	break;
    case 3:
	sendtype->datatype = MPI_FLOAT_INT;
	sendtype->isBasic  = 1;
	recvtype->datatype = MPI_FLOAT_INT;
	recvtype->isBasic  = 1;
	break;
    case 4:
	MPI_Type_dup( MPI_INT, &sendtype->datatype );
	MPI_Type_dup( MPI_INT, &recvtype->datatype );
	break;
    case 5:
	/* vector send type and contiguous receive type */
	sendtype->stride = 3;
	MPI_Type_vector( recvtype->count, 1, sendtype->stride, MPI_INT, 
			 &sendtype->datatype );
	sendtype->count    = 1;
	recvtype->datatype = MPI_INT;
	recvtype->isBasic  = 1;
	sendtype->InitBuf  = MTestTypeVectorInit;
	recvtype->InitBuf  = MTestTypeContigInit;
	sendtype->FreeBuf  = MTestTypeVectorFree;
	recvtype->FreeBuf  = MTestTypeContigFree;
	break;
    default:
	datatype_index = -1;
    }

    if (!sendtype->InitBuf) {
	sendtype->InitBuf  = MTestTypeContigInit;
	recvtype->InitBuf  = MTestTypeContigInit;
	sendtype->FreeBuf  = MTestTypeContigFree;
	recvtype->FreeBuf  = MTestTypeContigFree;
	sendtype->CheckBuf = MTestTypeContigCheckbuf;
	recvtype->CheckBuf = MTestTypeContigCheckbuf;
    }
    datatype_index++;
    return datatype_index;
}

void MTestResetDatatypes( void )
{
    datatype_index = 0;
}

void MTestFreeDatatype( MTest_Datatype *mtype )
{
    if (mtype->FreeBuf) {
	(mtype->FreeBuf)( mtype );
    }
}

int MTestCheckRecv( MPI_Status *status, MTest_Datatype *recvtype )
{
    int count;
    int errs = 0;

    MPI_Get_count( status, recvtype->datatype, &count );

    /* Check count against expected count */
    if (count != recvtype->count) {
	errs ++;
    }

    /* Check received data */
    if (!errs && recvtype->CheckBuf( recvtype )) {
	errs++;
    }
    return errs;
}

/* 
 * Create communicators.  Use separate routines for inter and intra
 * communicators (there is a routine to give both)
 * Note that the routines may return MPI_COMM_NULL, so code should test for
 * that return value as well.
 * 
 */
static int interCommIdx = 0;
static int intraCommIdx = 0;
static const char *intraCommName = 0;
static const char *interCommName = 0;

int MTestGetIntracomm( MPI_Comm *comm, int min_size )
{
    int size, rank;
    int done=0;

    /* The while loop allows us to skip communicators that are too small.
       MPI_COMM_NULL is always considered large enough */
    while (!done) {
	switch (intraCommIdx) {
	case 0:
	    *comm = MPI_COMM_WORLD;
	    intraCommName = "MPI_COMM_WORLD";
	    break;
	case 1:
	    /* dup of world */
	    MPI_Comm_dup(MPI_COMM_WORLD, comm );
	    intraCommName = "Dup of MPI_COMM_WORLD";
	    break;
	case 2:
	    /* reverse ranks */
	    MPI_Comm_size( MPI_COMM_WORLD, &size );
	    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
	    MPI_Comm_split( MPI_COMM_WORLD, 0, size-rank, comm );
	    intraCommName = "Rank reverse of MPI_COMM_WORLD";
	    break;
	case 3:
	    /* subset of world, with reversed ranks */
	    MPI_Comm_size( MPI_COMM_WORLD, &size );
	    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
	    MPI_Comm_split( MPI_COMM_WORLD, (rank < size/2), size-rank, comm );
	    intraCommName = "Rank reverse of half of MPI_COMM_WORLD";
	    break;
	case 4:
	    *comm = MPI_COMM_SELF;
	    intraCommName = "MPI_COMM_SELF";
	    break;

	    /* Other ideas: dup of self, cart comm, graph comm */
	default:
	    *comm = MPI_COMM_NULL;
	    intraCommName = "MPI_COMM_NULL";
	    intraCommIdx = -1;
	    break;
	}
	if (*comm != MPI_COMM_NULL) {
	    MPI_Comm_size( *comm, &size );
	    if (size >= min_size) done = 1;
	}
	else
	    done = 1;
    }

    intraCommIdx++;
    return intraCommIdx;
}

const char *MTestGetIntracommName( void )
{
    return intraCommName;
}

int MTestGetIntercomm( MPI_Comm *comm, int min_size )
{
    int size, rank, remsize;
    int done=0;
    MPI_Comm mcomm;
    int rleader;

    /* The while loop allows us to skip communicators that are too small.
       MPI_COMM_NULL is always considered large enough.  The size is
       the sum of the sizes of the local and remote groups */
    while (!done) {
	switch (interCommIdx) {
	case 0:
	    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
	    MPI_Comm_size( MPI_COMM_WORLD, &size );
	    if (size > 1) {
		MPI_Comm_split( MPI_COMM_WORLD, (rank < size/2), rank, 
				&mcomm );
		if (rank == 0) {
		    rleader = size/2;
		}
		else if (rank == size/2) {
		    rleader = 0;
		}
		else {
		    /* Remote leader is signficant only for the processes
		       designated local leaders */
		    rleader = -1;
		}
		MPI_Intercomm_create( mcomm, 0, MPI_COMM_WORLD, rleader, 12345,
				      comm );
		MPI_Comm_free( &mcomm );
		interCommName = "Intercomm by splitting MPI_COMM_WORLD";
	    }
	    else 
		*comm = MPI_COMM_NULL;
	    break;
	default:
	    *comm = MPI_COMM_NULL;
	    interCommName = "MPI_COMM_NULL";
	    interCommIdx = -1;
	    break;
	}
	if (*comm != MPI_COMM_NULL) {
	    MPI_Comm_size( *comm, &size );
	    MPI_Comm_remote_size( *comm, &remsize );
	    if (size + remsize >= min_size) done = 1;
	}
	else
	    done = 1;
    }

    interCommIdx++;
    return interCommIdx;
}
const char *MTestGetIntercommName( void )
{
    return interCommName;
}

int MTestGetComm( MPI_Comm *comm, int min_size )
{
    int idx;
    static int getinter = 0;

    if (!getinter) {
	idx = MTestGetIntracomm( comm, min_size );
	if (idx == 0) {
	    getinter = 1;
	}
    }
    if (getinter) {
	idx = MTestGetIntercomm( comm, min_size );
	if (idx == 0) {
	    getinter = 0;
	}
    }

    return idx;
}

void MTestPrintError( int errcode )
{
    int errclass, slen;
    char string[MPI_MAX_ERROR_STRING];
    
    MPI_Error_class( errcode, &errclass );
    MPI_Error_string( errcode, string, &slen );
    printf( "Error class %d (%s)\n", errclass, string );
}

/*#ifdef HAVE_MPI_WIN_CREATE*/
/*
 * Create MPI Windows
 */
static int win_index = 0;
static const char *winName;
int MTestGetWin( MPI_Win *win, int mustBePassive )
{
    static char actbuf[1024];
    static char *pasbuf;
    char *buf;
    int n, rank;

    switch (win_index) {
    case 0:
	/* Active target window */
	MPI_Win_create( actbuf, 1024, 1, MPI_INFO_NULL, MPI_COMM_WORLD, 
			win );
	break;
    case 1:
	/* Passive target window */
	MPI_Alloc_mem( 1024, MPI_INFO_NULL, &pasbuf );
	/* FIXME: storage leak */
	MPI_Win_create( pasbuf, 1024, 1, MPI_INFO_NULL, MPI_COMM_WORLD, 
			win );
	break;
    case 2:
	/* Active target; all windows different sizes */
	MPI_Comm_rank( MPI_COMM_WORLD, &rank );
	n = rank * 64;
	/* FIXME: storage leak */
	if (n) 
	    buf = (char *)malloc( n );
	else
	    buf = 0;
	MPI_Win_create( buf, n, 1, MPI_INFO_NULL, MPI_COMM_WORLD, 
			win );
	break;
    default:
	win_index = -1;
    }
    win_index++;
    return win_index;
}
const char *MTestGetWinName( void )
{
    return winName;
}
/* #endif */
