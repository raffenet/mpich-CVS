/* -*- Mode: C++; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "mpitestconf.h"
#ifdef HAVE_IOSTREAM
// Not all C++ compilers have iostream instead of iostream.h
#include <iostream>
#ifdef HAVE_NAMESPACE_STD
// Those that do often need the std namespace; otherwise, a bare "cout"
// is likely to fail to compile
using namespace std;
#endif
#else
#include <iostream.h>
#endif
#include "mpitestcxx.h"
#include <stdlib.h>

static int dbgflag = 0;
static int wrank = -1;
/* 
 * Initialize and Finalize MTest
 */
void MTest_Init( void )
{
    bool flag;

    flag = MPI::Is_initialized( );
    if (!flag) {
	MPI::Init( );
    }
    /* Check for debugging control */
    if (getenv( "MPITEST_DEBUG" )) {
	dbgflag = 1;
	wrank = MPI::COMM_WORLD.Get_rank();
    }
}

void MTest_Finalize( int errs )
{
    int rank, toterrs;

    rank = MPI::COMM_WORLD.Get_rank();

    MPI::COMM_WORLD.Allreduce( &errs, &toterrs, 1, MPI::INT, MPI::SUM );
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
    MPI::Aint size, lb;
    if (mtype->count > 0) {
	signed char *p;
	int  i, totsize;
	mtype->datatype.Get_extent( lb, size );
	totsize = size * mtype->count;
	mtype->buf = (void *) malloc( totsize );
	p = (signed char *)(mtype->buf);
	if (!p) {
	    /* Error - out of memory */
	    fprintf( stderr, "Out of memory in type buffer init\n" );
	    MPI::COMM_WORLD.Abort(1);
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
    signed char *p;
    int  i, totsize, err = 0;
    MPI::Aint size, lb;

    p = (signed char *)mtype->buf;
    if (p) {
	mtype->datatype.Get_extent(lb,size);
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

static void *MTestTypeVectorInit( MTestDatatype *mtype )
{
    MPI::Aint size, lb;
    if (mtype->count > 0) {
	mtype->datatype.Get_extent(lb,size);
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
    sendtype->InitBuf  = 0;
    sendtype->FreeBuf  = 0;
    sendtype->datatype = 0;
    sendtype->isBasic  = 0;
    recvtype->InitBuf  = 0;
    recvtype->FreeBuf  = 0;
    recvtype->datatype = 0;
    recvtype->isBasic  = 0;

    /* Set the defaults for the message lengths */
    sendtype->count    = count;
    recvtype->count    = count;
    /* Use datatype_index to choose a datatype to use.  If at the end of the
       list, return 0 */
    switch (datatype_index) {
    case 0:
	sendtype->datatype = MPI::INT;
	sendtype->isBasic  = 1;
	recvtype->datatype = MPI::INT;
	recvtype->isBasic  = 1;
	break;
    case 1:
	sendtype->datatype = MPI::DOUBLE;
	sendtype->isBasic  = 1;
	recvtype->datatype = MPI::DOUBLE;
	recvtype->isBasic  = 1;
	break;
    case 2:
	sendtype->datatype = MPI::INT;
	sendtype->isBasic  = 1;
	recvtype->datatype = MPI::BYTE;
	recvtype->isBasic  = 1;
	recvtype->count    *= sizeof(int);
	break;
    case 3:
	sendtype->datatype = MPI::FLOAT_INT;
	sendtype->isBasic  = 1;
	recvtype->datatype = MPI::FLOAT_INT;
	recvtype->isBasic  = 1;
	break;
    case 4:
	sendtype->datatype = MPI::INT.Dup();
	sendtype->datatype.Set_name( "dup of MPI::INT" );
	recvtype->datatype = MPI::INT.Dup();
	recvtype->datatype.Set_name( "dup of MPI::INT" );
	/* dup'ed types are already committed if the original type 
	   was committed (MPI-2, section 8.8) */
	break;
    case 5:
	/* vector send type and contiguous receive type */
	sendtype->stride = 3;
	sendtype->datatype = MPI::INT.Create_vector( recvtype->count, 1, sendtype->stride );
        sendtype->datatype.Commit();
	sendtype->datatype.Set_name( "int-vector" );
	sendtype->count    = 1;
	recvtype->datatype = MPI::INT;
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
	typesize = sendtype->datatype.Get_size();
	fprintf( stderr, "%d: sendtype size = %d\n", wrank, typesize );
	fprintf( stderr, "%d: recvtype is %s\n", wrank, MTestGetDatatypeName( recvtype ) );
	typesize = recvtype->datatype.Get_size();
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

int MTestCheckRecv( MPI::Status &status, MTestDatatype *recvtype )
{
    int count;
    int errs = 0;

    if (status) {
	count = status.Get_count( recvtype->datatype );
	
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

    dtype->datatype.Get_name( name, rlen );
    return (const char *)name;
}
/* ----------------------------------------------------------------------- */

/* 
 * Create communicators.  Use separate routines for inter and intra
 * communicators (there is a routine to give both)
 * Note that the routines may return MPI::COMM_NULL, so code should test for
 * that return value as well.
 * 
 */
static int interCommIdx = 0;
static int intraCommIdx = 0;
static const char *intraCommName = 0;
static const char *interCommName = 0;

int MTestGetIntracommGeneral( MPI::Intracomm &comm, int min_size, 
			      bool allowSmaller )
{
    int size, rank;
    bool done=false;
    bool isBasic = false;

    /* The while loop allows us to skip communicators that are too small.
       MPI::COMM_NULL is always considered large enough */
    while (!done) {
	switch (intraCommIdx) {
	case 0:
	    comm = MPI::COMM_WORLD;
	    isBasic = true;
	    intraCommName = "MPI::COMM_WORLD";
	    break;
	case 1:
	    /* dup of world */
	    comm = MPI::COMM_WORLD.Dup();
	    intraCommName = "Dup of MPI::COMM_WORLD";
	    break;
	case 2:
	    /* reverse ranks */
	    size = MPI::COMM_WORLD.Get_size();
	    rank = MPI::COMM_WORLD.Get_rank();
	    comm = MPI::COMM_WORLD.Split( 0, size-rank );
	    intraCommName = "Rank reverse of MPI::COMM_WORLD";
	    break;
	case 3:
	    /* subset of world, with reversed ranks */
	    size = MPI::COMM_WORLD.Get_size();
	    rank = MPI::COMM_WORLD.Get_rank();
	    comm = MPI::COMM_WORLD.Split( (rank < size/2), size-rank );
	    intraCommName = "Rank reverse of half of MPI::COMM_WORLD";
	    break;
	case 4:
	    comm = MPI::COMM_SELF;
	    isBasic = true;
	    intraCommName = "MPI::COMM_SELF";
	    break;

	case 5:
	case 6:
	case 7:
	case 8:
	{
	    int newsize;
	    size = MPI::COMM_WORLD.Get_size();
	    newsize = size - (intraCommIdx - 4);
	    
	    if (allowSmaller && newsize >= min_size) {
		rank = MPI::COMM_WORLD.Get_rank();
		*comm = MPI::COMM_WORLD.Split( rank < newsize, rank );
		if (rank >= newsize) {
		    comm.Free();
		    *comm = MPI::COMM_NULL;
		}
	    }
	    else {
		/* Act like default */
		comm = MPI::COMM_NULL;
		isBasic = true;
		intraCommName = "MPI::COMM_NULL";
		intraCommIdx = -1;
	    }
	}
	break;
	    
	    /* Other ideas: dup of self, cart comm, graph comm */
	default:
	    comm = MPI::COMM_NULL;
	    isBasic = true;
	    intraCommName = "MPI::COMM_NULL";
	    intraCommIdx = -1;
	    break;
	}

	if (comm != MPI::COMM_NULL) {
	    size = comm.Get_size();
	    if (size >= min_size) 
		done = true;
	    else {
		/* Try again */
		if (!isBasic) comm.Free();
		intraCommIdx++;
	    }
	}
	else
	    done = true;
    }

    intraCommIdx++;
    return intraCommIdx;
}

const char *MTestGetIntracommName( void )
{
    return intraCommName;
}

/* 
 * Return an intercomm; set isLeftGroup to 1 if the calling process is 
 * a member of the "left" group.
 */
int MTestGetIntercomm( MPI::Intercomm &comm, int &isLeftGroup, int min_size )
{
    int size, rank, remsize;
    bool done=false;
    MPI::Intracomm mcomm;
    int rleader;

    /* The while loop allows us to skip communicators that are too small.
       MPI::COMM_NULL is always considered large enough.  The size is
       the sum of the sizes of the local and remote groups */
    while (!done) {
	switch (interCommIdx) {
	case 0:
	    /* Split comm world in half */
	    rank = MPI::COMM_WORLD.Get_rank();
	    size = MPI::COMM_WORLD.Get_size();
	    if (size > 1) {
		mcomm = MPI::COMM_WORLD.Split( (rank < size/2), rank );
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
		isLeftGroup = rank < size/2;
		comm = mcomm.Create_intercomm( 0, MPI::COMM_WORLD, rleader, 12345 );
		mcomm.Free();
		interCommName = "Intercomm by splitting MPI::COMM_WORLD";
	    }
	    else 
		comm = MPI::COMM_NULL;
	    break;
	case 1:
	    /* Split comm world in to 1 and the rest */
	    rank = MPI::COMM_WORLD.Get_rank();
	    size = MPI::COMM_WORLD.Get_size();
	    if (size > 1) {
		mcomm = MPI::COMM_WORLD.Split( rank == 0, rank );
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
		isLeftGroup = rank == 0;
		comm = mcomm.Create_intercomm( 0, MPI::COMM_WORLD, rleader, 12346 );
		mcomm.Free();
		interCommName = "Intercomm by splitting MPI::COMM_WORLD into 1, rest";
	    }
	    else 
		comm = MPI::COMM_NULL;
	    break;

	case 2:
	    /* Split comm world in to 2 and the rest */
	    rank = MPI::COMM_WORLD.Get_rank();
	    size = MPI::COMM_WORLD.Get_size();
	    if (size > 3) {
		mcomm = MPI::COMM_WORLD.Split( rank < 2, rank );
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
		isLeftGroup = rank < 2;
		comm = mcomm.Create_intercomm( 0, MPI::COMM_WORLD, rleader, 12347 );
		mcomm.Free();
		interCommName = "Intercomm by splitting MPI::COMM_WORLD into 2, rest";
	    }
	    else 
		comm = MPI::COMM_NULL;
	    break;
	    
	default:
	    comm = MPI::COMM_NULL;
	    interCommName = "MPI::COMM_NULL";
	    interCommIdx = -1;
	    break;
	}
	if (comm != MPI::COMM_NULL) {
	    size = comm.Get_size();
	    remsize = comm.Get_remote_size();
	    if (size + remsize >= min_size) done = true;
	}
	else
	    done = true;
    }

    interCommIdx++;
    return interCommIdx;
}
const char *MTestGetIntercommName( void )
{
    return interCommName;
}

int MTestGetComm( MPI::Comm *comm, int min_size )
{
    int idx;
    static int getinter = 0;
    MPI::Intracomm rcomm;

    if (!getinter) {
	idx = MTestGetIntracomm( rcomm, min_size );
	if (idx == 0) {
	    getinter = 1;
	}
	else {
	    *comm = rcomm;
	}
    }
    if (getinter) {
	MPI::Intercomm icomm;
	int isLeft;
	idx = MTestGetIntercomm( icomm, isLeft, min_size );
	if (idx == 0) {
	    getinter = 0;
	}
	*comm = icomm;
    }

    return idx;
}

/* ------------------------------------------------------------------------ */
void MTestPrintError( int errcode )
{
    int errclass, slen;
    char string[MPI_MAX_ERROR_STRING];
    
    errclass = MPI::Get_error_class( errcode );
    MPI::Get_error_string( errcode, string, slen );
    printf( "Error class %d (%s)\n", errclass, string );
}


/* 
 * Get an intracommunicator with at least min_size members.
 */
int MTestGetIntracomm( MPI::Intracomm &comm, int min_size ) 
{
    return MTestGetIntracommGeneral( comm, min_size, false );
}

void MTestFreeComm( MPI::Comm &comm )
{
    if (comm != MPI::COMM_WORLD &&
	comm != MPI::COMM_SELF &&
	comm != MPI::COMM_NULL) {
	comm.Free();
    }
}

#ifdef HAVE_MPI_WIN_CREATE
/*
 * Create MPI Windows
 */
static int win_index = 0;
static const char *winName;
int MTestGetWin( MPI::Win &win, bool mustBePassive )
{
    static char actbuf[1024];
    static char *pasbuf;
    char *buf;
    int n, rank;

    switch (win_index) {
    case 0:
	/* Active target window */
	win = MPI::Win::Create( actbuf, 1024, 1, MPI::INFO_NULL, MPI::COMM_WORLD );
	winName = "active-window";
	break;
    case 1:
	/* Passive target window */
	pasbuf = (char *)MPI::Alloc_mem( 1024, MPI::INFO_NULL );
	/* FIXME: storage leak */
	win = MPI::Win::Create( pasbuf, 1024, 1, MPI::INFO_NULL, MPI::COMM_WORLD );
	winName = "passive-window";
	break;
    case 2:
	/* Active target; all windows different sizes */
	rank = MPI::COMM_WORLD.Get_rank();
	n = rank * 64;
	/* FIXME: storage leak */
	if (n) 
	    buf = (char *)malloc( n );
	else
	    buf = 0;
	win = MPI::Win::Create( buf, n, 1, MPI::INFO_NULL, MPI::COMM_WORLD );
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
