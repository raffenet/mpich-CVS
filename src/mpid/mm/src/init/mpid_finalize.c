/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "pmi.h"

/*@
   MPID_Finalize - Terminates mm device

   Notes:

.N fortran

.N Errors
.N MPI_SUCCESS
.N ... others
@*/
int MPID_Finalize( void )
{
    mm_car_finalize();
    mm_vc_finalize();

    /****pmi****/ printf("calling PMI_Barrier\n");fflush(stdout);
    PMI_Barrier();
    /****pmi****/ printf("calling PMI_Finalize\n");fflush(stdout);
    PMI_Finalize();

    return MPI_SUCCESS;
}
