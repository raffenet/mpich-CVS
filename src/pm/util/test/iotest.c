/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "pmutil.h"

int debug = 1;
/*
 * This test checks some features of the IOHandlers by using a single process
 * and using file input and output instead of reading/writing from a pipe 
 * connected to a separate process.  This simplifies debugging though
 * it does not fully test the routines.
 */
int main( int argc, char *argv[] )
{
    IOExitReason reason;
    ProcessTable ptable;
    int src1, src2, rc;

    /* Fake up a process table */
    ptable.nActive    = 2;
    ptable.nProcesses = 2;
    ptable.table = (ProcessState *)malloc( 2 * sizeof(ProcessState) );
    ptable.timeout_seconds = 0;
    ptable.table[0].nIos = 1;
    ptable.table[1].nIos = 1;
    
    /* Open the fds for reading and writing from test files */
    src1 = open( "test1", O_RDONLY );
    src2 = open( "test2", O_RDONLY );
    if (src1 < 0 || src2 < 0) {
	fprintf( stderr, "Could not open test files!" );
	return 1;
    }
#define NEW
#ifdef NEW
    /* Initialize fd's 1 and 2 for output */
    IOManyToOneNotifySetup( 1 );
    IOManyToOneNotifySetup( 2 );
    /* Initialize fds src1 and src2 as input for writing to 
       fds 1 and 2 respectively */
    rc = IOSetupOutHandler( &ptable.table[0].ios[0], src1, 1, "1>" );
    if (rc) {
	fprintf( stderr, "Error initializing src1\n" );
    }
    rc = IOSetupOutHandler( &ptable.table[1].ios[0], src2, 2, "2>" );
    if (rc) {
	fprintf( stderr, "Error initializing src2\n" );
    }
#else
    IOSetupOutSimple( &ptable.table[0].ios[0], src1, 1, "1>" );
    IOSetupOutSimple( &ptable.table[1].ios[0], src2, 2, "2>" );
#endif
    /* IOSetupInHandler( ... ); */
    
    /* Run the IO loop until the files are empty */
    while (IOHandleLoop( &ptable, &reason )) ;

    /* Done! The output must be checked manually */
    return 0;
}
