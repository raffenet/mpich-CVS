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
    /*MPID_Timer_finalize();*/ /* called in MPI_Finalize */

    mm_car_finalize();
    mm_vc_finalize();

    /*dbg_printf("+PMI_Barrier");*/
    PMI_Barrier();
    /*dbg_printf("-\n+PMI_Finalize");*/
    PMI_Finalize();
    /*dbg_printf("-\n");*/

    bsocket_finalize();

    return MPI_SUCCESS;
}
