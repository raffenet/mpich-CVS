/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Finalize

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
    static const char FCNAME[] = "MPID_Finalize";

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPID_FINALIZE);

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPID_FINALIZE);
    return MPI_SUCCESS;
}
