/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

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
    return mm_close_port(port_name);
}
