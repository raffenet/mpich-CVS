/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   MPID_Open_port - short description

   Input Arguments:
+  MPI_Info info - info
-  char *port_name - port name

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPID_Open_port(MPID_Info *info_ptr, char *port_name)
{
    mm_open_port(info_ptr, port_name);

    return MPI_SUCCESS;
}
