/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"
#include "adio_extern.h"

/*@
    MPIO_Request_f2c - Translates a Fortran I/O-request handle to 
                       a C I/O-request handle

Input Parameters:
. request - Fortran I/O-request handle (integer)

Return Value:
  C I/O-request handle (handle)
@*/
MPIO_Request MPIO_Request_f2c(MPI_Fint request)
{

#ifndef __INT_LT_POINTER
    return (MPIO_Request) request;
#else
    if (!request) return MPIO_REQUEST_NULL;
    if ((request < 0) || (request > ADIOI_Reqtable_ptr)) {
	printf("MPIO_Request_f2c: Invalid request\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }
    return ADIOI_Reqtable[request];
#endif
}
