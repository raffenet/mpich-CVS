/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   MPID_Comm_disconnect - disconnect

   Arguments:
.  MPID_Comm *comm_ptr - communicator

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPID_Comm_disconnect(MPID_Comm *comm_ptr)
{
    return MPI_SUCCESS;
}
