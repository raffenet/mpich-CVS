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
// Those that do often need the std namespace; otherwise, a bare "cout"
// is likely to fail to compile
using namespace std;
#endif
#else
#include <iostream.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include "mpitestcxx.h"

int main( int argc, char *argv[] )
{
    int errs = 0;
    char port_name[MPI::MAX_PORT_NAME], port_name_out[MPI::MAX_PORT_NAME];
    char serv_name[256];
    int merr, mclass;
    const char *errmsg;
    int msglen;
    int rank;

    MTest_Init( );

    rank = MPI::COMM_WORLD.Get_rank();

    /* Note that according to the MPI standard, port_name must
       have been created by MPI_Open_port.  For current testing
       purposes, we'll use a fake name.  This test should eventually use
       a valid name from Open_port */

    strcpy( port_name, "otherhost:122" );
    strcpy( serv_name, "MyTest" );

    if (rank == 0) {
	try {
	    MPI::Publish_name( serv_name, MPI::INFO_NULL, port_name );
	} catch (MPI::Exception e) {
	    errs++;
	    errmsg = e.Get_error_string();
	    cout << "Error in Publish_name " << errmsg << "\n";
	}

	MPI::COMM_WORLD.Barrier();
	MPI::COMM_WORLD.Barrier();
	
	try {
	    MPI::Unpublish_name( serv_name, MPI::INFO_NULL, port_name );
	} catch (MPI::Exception e) {
	    errs++;
	    errmsg = e.Get_error_string();
	    cout << "Error in Unpublish name " << errmsg << "\n";
	}
    }
    else {
	MPI::COMM_WORLD.Barrier();

	merr = MPI::SUCCESS;
	try {
	    MPI::Lookup_name( serv_name, MPI::INFO_NULL, port_name_out );
	} catch ( MPI::Exception e ) {
	    errs++;
	    merr   = e.Get_error_code();
	    errmsg = e.Get_error_string();
	    cout << "Error in Lookup name " << errmsg << "\n";
	}
	if (merr == MPI::SUCCESS) {
	    if (strcmp( port_name, port_name_out )) {
		errs++;
		cout << "Lookup name returned the wrong value (" << 
		    port_name_out << ")\n";
	    }
	}

	MPI::COMM_WORLD.Barrier();
    }


    MPI::COMM_WORLD.Barrier();
    
    merr = MPI::SUCCESS;
    try {
	MPI::Lookup_name( serv_name, MPI::INFO_NULL, port_name_out );
    } catch (MPI::Exception e) {
	merr = e.Get_error_code();
    }
    if (!merr) {
	errs++;
	cout << "Lookup name returned name after it was unpublished\n";
    }
    else {
	/* Must be class MPI::ERR_NAME */
	mclass = MPI::Get_error_class( merr );
	if (mclass != MPI::ERR_NAME) {
	    char lerrmsg[MPI::MAX_ERROR_STRING];
	    errs++;
	    MPI::Get_error_string( merr, lerrmsg, msglen );
	    cout << "Lookup name returned the wrong error class (" << mclass <<
		"), msg " << errmsg << "\n";
	}
    }

    MTest_Finalize( errs );
    MPI::Finalize();
    return 0;
  
}
