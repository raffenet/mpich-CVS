/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Close_port

/*@
   MPID_Close_port - close port

   Arguments:
.  char *port_name - port name

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPID_Close_port(char *port_name)
{
    static const char FCNAME[] = "MPID_Close_port";

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPID_CLOSE_PORT);

    MM_Close_port(port_name);

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPID_CLOSE_PORT);
    return MPI_SUCCESS;
}
