/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2004 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "pmutil.h"

/*
 * This test program checks the annotation routines.  The output must 
 * be manually exanmined.  The input data is chosen to test:
 * a) incomplete lines (i.e., lines without a terminating newline)
 * b) long lines 
 * c) full buffers
 * d) empty buffers
 */

int main( int argc, char *argv[] )
{
    /* Initialize the annotation buffer */

    /* Place some data within it */

    /* Call the annotation routine */

    /* Call the output routine that understands how to unpack the annotated 
       data */

    return 0;
}
