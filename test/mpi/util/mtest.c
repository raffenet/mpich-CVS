/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "mpitestconf.h"
#include "mpitest.h"
#include <stdio.h>
#include <stdlib.h>

static int dbgflag = 0;
static int wrank = -1;
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
    /* Check for debugging control */
    if (getenv( "MPITEST_DEBUG" )) {
	dbgflag = 1;
	MPI_Comm_rank( MPI_COMM_WORLD, &wrank );
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
static void *MTestTypeContigInit( MTestDatatype *mtype )
{
    MPI_Aint size;
    if (mtype->count > 0) {
	signed char *p;
	int  i, totsize;
	MPI_Type_extent( mtype->datatype, &size );
	totsize = size * mtype->count;
	mtype->buf = (void *) malloc( totsize );
	p = (signed char *)(mtype->buf);
	if (!p) {
	    /* Error - out of memory */
	    fprintf( stderr, "Out of memory in type buffer init\n" );
	    MPI_Abort( MPI_COMM_WORLD, 1 );
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
static void *MTestTypeContigFree( MTestDatatype *mtype )
{
    if (mtype->buf) {
	free( mtype->buf );
	mtype->buf = 0;
    }
    return 0;
}
static int MTestTypeContigCheckbuf( MTestDatatype *mtype )
{
    unsigned char *p;
    unsigned char expected;
    int  i, totsize, err = 0;
    MPI_Aint size;

    p = (unsigned char *)mtype->buf;
    if (p) {
	MPI_Type_extent( mtype->datatype, &size );
	totsize = size * mtype->count;
	for (i=0; i<totsize; i++) {
	    expected = (0xff ^ (i & 0xff));
	    if (p[i] != expected) {
		err++;
		if (mtype->printErrors && err < 10) {
		    printf( "Data expected = %x but got %x for %dth entry\n",
			    expected, p[i], i );
		}
	    }
	}
    }
    return err;
}

/*
 * 
 */

static void *MTestTypeVectorInit( MTestDatatype *mtype )
{
    MPI_Aint size;
    if (mtype->count > 0) {
	MPI_Type_extent( mtype->datatype, &size );
	mtype->buf = (void *) malloc( mtype->count * size );
    }
    else {
	mtype->buf = 0;
    }
    return mtype->buf;
}

static void *MTestTypeVectorFree( MTestDatatype *mtype )
{
    if (mtype->buf) {
	free( mtype->buf );
	mtype->buf = 0;
    }
    return 0;
}

/* 
   Create a range of datatypes with a given count elements.
   This uses a selection of types, rather than an exhaustive collection.
   It allocates both send and receive types so that they can have the same
   type signature (collection of basic types) but different type maps (layouts
   in memory) 
 */
int MTestGetDatatypes( MTestDatatype *sendtype, MTestDatatype *recvtype,
		       int count )
{
    sendtype->InitBuf	  = 0;
    sendtype->FreeBuf	  = 0;
    sendtype->datatype	  = 0;
    sendtype->isBasic	  = 0;
    sendtype->printErrors = 0;
    recvtype->InitBuf	  = 0;
    recvtype->FreeBuf	  = 0;
    recvtype->datatype	  = 0;
    recvtype->isBasic	  = 0;
    recvtype->printErrors = 0;

    /* Set the defaults for the message lengths */
    sendtype->count    = count;
    recvtype->count    = count;
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
	recvtype->count    *= sizeof(int);
	break;
    case 3:
	sendtype->datatype = MPI_FLOAT_INT;
	sendtype->isBasic  = 1;
	recvtype->datatype = MPI_FLOAT_INT;
	recvtype->isBasic  = 1;
	break;
    case 4:
	MPI_Type_dup( MPI_INT, &sendtype->datatype );
	MPI_Type_set_name( sendtype->datatype, "dup of MPI_INT" );
	MPI_Type_dup( MPI_INT, &recvtype->datatype );
	MPI_Type_set_name( recvtype->datatype, "dup of MPI_INT" );
	/* dup'ed types are already committed if the original type 
	   was committed (MPI-2, section 8.8) */
	break;
    case 5:
	/* vector send type and contiguous receive type */
	sendtype->stride = 3;
	MPI_Type_vector( recvtype->count, 1, sendtype->stride, MPI_INT, 
			 &sendtype->datatype );
        MPI_Type_commit( &sendtype->datatype );
	MPI_Type_set_name( sendtype->datatype, "int-vector" );
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

    if (dbgflag && datatype_index > 0) {
	int typesize;
	fprintf( stderr, "%d: sendtype is %s\n", wrank, MTestGetDatatypeName( sendtype ) );
	MPI_Type_size( sendtype->datatype, &typesize );
	fprintf( stderr, "%d: sendtype size = %d\n", wrank, typesize );
	fprintf( stderr, "%d: recvtype is %s\n", wrank, MTestGetDatatypeName( recvtype ) );
	MPI_Type_size( recvtype->datatype, &typesize );
	fprintf( stderr, "%d: recvtype size = %d\n", wrank, typesize );
	fflush( stderr );
	
    }
    return datatype_index;
}

void MTestResetDatatypes( void )
{
    datatype_index = 0;
}

void MTestFreeDatatype( MTestDatatype *mtype )
{
    if (mtype->FreeBuf) {
	(mtype->FreeBuf)( mtype );
    }
}

int MTestCheckRecv( MPI_Status *status, MTestDatatype *recvtype )
{
    int count;
    int errs = 0;

    if (status) {
	MPI_Get_count( status, recvtype->datatype, &count );
	
	/* Check count against expected count */
	if (count != recvtype->count) {
	    errs ++;
	}
    }

    /* Check received data */
    if (!errs && recvtype->CheckBuf( recvtype )) {
	errs++;
    }
    return errs;
}

const char *MTestGetDatatypeName( MTestDatatype *dtype )
{
    static char name[MPI_MAX_OBJECT_NAME];
    int rlen;

    MPI_Type_get_name( dtype->datatype, name, &rlen );
    return (const char *)name;
}
/* ----------------------------------------------------------------------- */

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

int MTestGetIntracommGeneral( MPI_Comm *comm, int min_size, int allowSmaller )
{
    int size, rank;
    int done=0;
    int isBasic = 0;

    /* The while loop allows us to skip communicators that are too small.
       MPI_COMM_NULL is always considered large enough */
    while (!done) {
	switch (intraCommIdx) {
	case 0:
	    *comm = MPI_COMM_WORLD;
	    isBasic = 1;
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
	    isBasic = 1;
	    intraCommName = "MPI_COMM_SELF";
	    break;

	case 5:
	case 6:
	case 7:
	case 8:
	{
	    int newsize;
	    MPI_Comm_size( MPI_COMM_WORLD, &size );
	    newsize = size - (intraCommIdx - 4);
	    
	    if (allowSmaller && newsize >= min_size) {
		MPI_Comm_rank( MPI_COMM_WORLD, &rank );
		MPI_Comm_split( MPI_COMM_WORLD, rank < newsize, rank, comm );
		if (rank >= newsize) {
		    MPI_Comm_free( comm );
		    *comm = MPI_COMM_NULL;
		}
	    }
	    else {
		/* Act like default */
		*comm = MPI_COMM_NULL;
		isBasic = 1;
		intraCommName = "MPI_COMM_NULL";
		intraCommIdx = -1;
	    }
	}
	break;
	    
	    /* Other ideas: dup of self, cart comm, graph comm */
	default:
	    *comm = MPI_COMM_NULL;
	    isBasic = 1;
	    intraCommName = "MPI_COMM_NULL";
	    intraCommIdx = -1;
	    break;
	}

	if (*comm != MPI_COMM_NULL) {
	    MPI_Comm_size( *comm, &size );
	    if (size >= min_size) 
		done = 1;
	    else {
		/* Try again */
		if (!isBasic) MPI_Comm_free( comm );
		intraCommIdx++;
	    }
	}
	else
	    done = 1;
    }

    intraCommIdx++;
    return intraCommIdx;
}

int MTestGetIntracomm( MPI_Comm *comm, int min_size ) 
{
    return MTestGetIntracommGeneral( comm, min_size, 0 );
}

const char *MTestGetIntracommName( void )
{
    return intraCommName;
}

/* 
 * Return an intercomm; set isLeftGroup to 1 if the calling process is 
 * a member of the "left" group.
 */
int MTestGetIntercomm( MPI_Comm *comm, int *isLeftGroup, int min_size )
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
	    /* Split comm world in half */
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
		*isLeftGroup = rank < size/2;
		MPI_Intercomm_create( mcomm, 0, MPI_COMM_WORLD, rleader, 12345,
				      comm );
		MPI_Comm_free( &mcomm );
		interCommName = "Intercomm by splitting MPI_COMM_WORLD";
	    }
	    else 
		*comm = MPI_COMM_NULL;
	    break;
	case 1:
	    /* Split comm world in to 1 and the rest */
	    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
	    MPI_Comm_size( MPI_COMM_WORLD, &size );
	    if (size > 1) {
		MPI_Comm_split( MPI_COMM_WORLD, rank == 0, rank, 
				&mcomm );
		if (rank == 0) {
		    rleader = 1;
		}
		else if (rank == 1) {
		    rleader = 0;
		}
		else {
		    /* Remote leader is signficant only for the processes
		       designated local leaders */
		    rleader = -1;
		}
		*isLeftGroup = rank == 0;
		MPI_Intercomm_create( mcomm, 0, MPI_COMM_WORLD, rleader, 12346,
				      comm );
		MPI_Comm_free( &mcomm );
		interCommName = "Intercomm by splitting MPI_COMM_WORLD into 1, rest";
	    }
	    else 
		*comm = MPI_COMM_NULL;
	    break;

	case 2:
	    /* Split comm world in to 2 and the rest */
	    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
	    MPI_Comm_size( MPI_COMM_WORLD, &size );
	    if (size > 3) {
		MPI_Comm_split( MPI_COMM_WORLD, rank < 2, rank, 
				&mcomm );
		if (rank == 0) {
		    rleader = 2;
		}
		else if (rank == 2) {
		    rleader = 0;
		}
		else {
		    /* Remote leader is signficant only for the processes
		       designated local leaders */
		    rleader = -1;
		}
		*isLeftGroup = rank < 2;
		MPI_Intercomm_create( mcomm, 0, MPI_COMM_WORLD, rleader, 12347,
				      comm );
		MPI_Comm_free( &mcomm );
		interCommName = "Intercomm by splitting MPI_COMM_WORLD into 2, rest";
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
	int isLeft;
	idx = MTestGetIntercomm( comm, &isLeft, min_size );
	if (idx == 0) {
	    getinter = 0;
	}
    }

    return idx;
}

void MTestFreeComm( MPI_Comm *comm )
{
    if (*comm != MPI_COMM_WORLD &&
	*comm != MPI_COMM_SELF &&
	*comm != MPI_COMM_NULL) {
	MPI_Comm_free( comm );
    }
}

/* ------------------------------------------------------------------------ */
void MTestPrintError( int errcode )
{
    int errclass, slen;
    char string[MPI_MAX_ERROR_STRING];
    
    MPI_Error_class( errcode, &errclass );
    MPI_Error_string( errcode, string, &slen );
    printf( "Error class %d (%s)\n", errclass, string );
}
void MTestPrintErrorMsg( const char msg[], int errcode )
{
    int errclass, slen;
    char string[MPI_MAX_ERROR_STRING];
    
    MPI_Error_class( errcode, &errclass );
    MPI_Error_string( errcode, string, &slen );
    printf( "%s: Error class %d (%s)\n", msg, errclass, string ); 
    fflush( stdout );
}

#ifdef HAVE_MPI_WIN_CREATE
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
	winName = "active-window";
	break;
    case 1:
	/* Passive target window */
	MPI_Alloc_mem( 1024, MPI_INFO_NULL, &pasbuf );
	/* FIXME: storage leak */
	MPI_Win_create( pasbuf, 1024, 1, MPI_INFO_NULL, MPI_COMM_WORLD, 
			win );
	winName = "passive-window";
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
	winName = "active-all-different-win";
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
#endif
