/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "viaimpl.h"

VIA_PerProcess VIA_Process;

/*@
   via_init - initialize via method

   Notes:
@*/
int via_init( void )
{
    return MPI_SUCCESS;
}

/*@
   via_finalize - finalize via method

   Notes:
@*/
int via_finalize( void )
{
    return MPI_SUCCESS;
}
