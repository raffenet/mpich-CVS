/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Comm_disconnect

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
    static const char FCNAME[] = "MPID_Comm_disconnect";

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPID_COMM_DISCONNECT);

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPID_COMM_DISCONNECT);

    return MPI_SUCCESS;
}
