/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   MPID_Comm_connect - connect

   Input Arguments:
+  char *port_name - port name
.  MPI_Info info - info
.  int root - root
-  MPI_Comm comm - communicator

   Output Arguments:
.  MPID_Comm *newcomm - new communicator

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
.N ... others
@*/
int MPID_Comm_connect(char *port_name, MPID_Info *info_ptr, int root, MPID_Comm *comm_ptr, MPID_Comm **newcomm)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_COMM_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_COMM_CONNECT);
    assert(FALSE);
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_COMM_CONNECT);
    return MPI_SUCCESS;
}
