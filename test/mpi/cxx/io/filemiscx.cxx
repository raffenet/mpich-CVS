/* -*- Mode: C++; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "mpitestconf.h"
#ifdef HAVE_IOSTREAM
// Not all C++ compilers have iostream instead of iostream.h
#include <iostream>
#ifdef HAVE_NAMESPACE_STD
// Those that do need the std namespace; otherwise, a bare "cout"
// is likely to fail to compile
using namespace std;
#endif
#else
#include <iostream.h>
#endif
#include <string.h>
#include <stdio.h>
#include "mpitestcxx.h"

/* tests various miscellaneous functions. */
/* This is a C++ version of romio/test/misc.c */
#define VERBOSE 0

int main(int argc, char **argv)
{
    int buf[1024], amode, mynod, len, i;
    bool flag;
    int errs = 0, toterrs;
    MPI::File fh;
    MPI::Status status;
    MPI::Datatype newtype;
    MPI::Offset disp, offset;
    MPI::Group group;
    MPI::Datatype etype, filetype;
    char datarep[25], *filename;

    MPI::Init();

    mynod = MPI::COMM_WORLD.Get_rank();

    /* process 0 takes the file name as a command-line argument and 
       broadcasts it to other processes */
    if (!mynod) {
	i = 1;
	while ((i < argc) && strcmp("-fname", *argv)) {
	    i++;
	    argv++;
	}
	if (i >= argc) {
	    cout << "\n*#  Usage: misc -fname filename\n\n";
	    MPI::COMM_WORLD.Abort( 1 );
	}
	argv++;
	len = strlen(*argv);
	filename = new char [len+1];
	strcpy(filename, *argv);
	MPI::COMM_WORLD.Bcast(&len, 1, MPI::INT, 0 );
	MPI::COMM_WORLD.Bcast(filename, len+1, MPI::CHAR, 0 );
    }
    else {
	MPI::COMM_WORLD.Bcast(&len, 1, MPI::INT, 0 );
	filename = new char [len+1];
	MPI::COMM_WORLD.Bcast(filename, len+1, MPI::CHAR, 0 );
    }


    fh = MPI::File::Open(MPI::COMM_WORLD, filename, 
			 MPI::MODE_CREATE | MPI::MODE_RDWR, MPI::INFO_NULL );

    fh.Write( buf, 1024, MPI::INT );

    fh.Sync();

    amode = fh.Get_amode();
#if VERBOSE
    if (!mynod) cout << "testing File::_get_amode\n";
#endif
    if (amode != (MPI::MODE_CREATE | MPI::MODE_RDWR)) {
	errs++;
	cout << "amode is " << amode << ",  should be " << 
		(int)(MPI::MODE_CREATE | MPI::MODE_RDWR) << "\n";
    }

    flag = fh.Get_atomicity();
    if (flag) {
	errs++;
	cout << "atomicity is " << flag << ", should be false\n";
    }
#if VERBOSE
    if (!mynod) cout << "setting atomic mode\n";
#endif
    fh.Set_atomicity( true );
    flag = fh.Get_atomicity();
    if (!flag) {
	errs;
	cout << "atomicity is " << flag << ", should be true\n";
    }
    fh.Set_atomicity( false );
#if VERBOSE
    if (!mynod) cout << "reverting back to nonatomic mode\n";
#endif

    newtype = MPI::INT.Create_vector( 10, 10, 20 );
    newtype.Commit();

    fh.Set_view( 1000, MPI::INT, newtype, "native", MPI::INFO_NULL);
#if VERBOSE
    if (!mynod) cout << "testing File::_get_view\n";
#endif
    fh.Get_view( disp, etype, filetype, datarep );
    if ((disp != 1000) || strcmp(datarep, "native")) {
	errs++;
	cout << "disp = " << disp << " datarep = " << datarep << 
", should be 1000, native\n\n";
    }
#if VERBOSE
    if (!mynod) cout << "testing File::_get_byte_offset\n");
#endif
    disp = fh.Get_byte_offset( 10 );
    if (disp != (1000+20*sizeof(int))) {
	errs++;
	cout << "byte offset = " << disp << ", should be " <<
	    (int) (1000+20*sizeof(int)) << "\n";
    }

    group = fh.Get_group();

#if VERBOSE
    if (!mynod) cout << "testing File::_set_size\n";
#endif
    fh.Set_size( 1000+15*sizeof(int) );
    MPI::COMM_WORLD.Barrier();
    fh.Sync();
    disp = fh.Get_size();
    if (disp != 1000+15*sizeof(int)) {
	errs++;
	cout << "file size = " << disp << ", should be " <<
	    (int) (1000+15*sizeof(int)) << "\n";
    }

#if VERBOSE
    if (!mynod) cout << "seeking to eof and testing File::_get_position\n";
#endif
    fh.Seek( 0, MPI_SEEK_END );
    disp = fh.Get_position();
    if (disp != 10) {
	errs++;
	cout << "file pointer posn = " << disp << ", should be 10\n\n";
    }

#if VERBOSE
    if (!mynod) cout << "testing File::_get_byte_offset\n";
#endif
    offset = fh.Get_byte_offset( disp );
    if (offset != (1000+20*sizeof(int))) {
	errs++;
	cout << "byte offset = " << offset << ", should be " << 
	    (int) (1000+20*sizeof(int)) << "\n";
    }
    MPI::COMM_WORLD.Barrier();

#if VERBOSE
    if (!mynod) cout << "testing File::_seek with MPI_SEEK_CUR\n";
#endif
    fh.Seek( -10, MPI_SEEK_CUR );
    disp = fh.Get_position();
    offset = fh.Get_byte_offset( disp );
    if (offset != 1000) {
	errs++;
	cout << "file pointer posn in bytes = " << offset << 
	    ", should be 1000\n\n";
    }

#if VERBOSE
    if (!mynod) cout << "preallocating disk space up to 8192 bytes\n";
#endif
    fh.Preallocate( 8192 );

#if VERBOSE
    if (!mynod) cout << "closing the file and deleting it\n";
#endif
    fh.Close();
    
    MPI::COMM_WORLD.Barrier();
    if (!mynod) MPI::File::Delete(filename, MPI::INFO_NULL);

    MPI::COMM_WORLD.Allreduce( &errs, &toterrs, 1, MPI::INT, MPI::SUM );
    if (mynod == 0) {
	if( toterrs > 0) {
	    cout << "Found " << toterrs << "\n";
	}
	else {
	    cout << " No Errors\n";
	}
    }
    newtype.Free();
    filetype.Free();
    group.Free();

    delete [] filename;
    MPI::Finalize(); 
    return 0;
}
