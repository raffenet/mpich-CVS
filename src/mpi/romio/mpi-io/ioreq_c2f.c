/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"
#include "adio_extern.h"

/*@
    MPIO_Request_c2f - Translates a C I/O-request handle to a 
                       Fortran I/O-request handle

Input Parameters:
. fh - C I/O-request handle (integer)

Return Value:
  Fortran I/O-request handle (handle)
@*/
MPI_Fint MPIO_Request_c2f(MPIO_Request request)
{
    int i;

#ifndef __INT_LT_POINTER
    return (MPI_Fint) request;
#else
    if ((request <= (MPIO_Request) 0) || (request->cookie != ADIOI_REQ_COOKIE))
	return (MPI_Fint) 0;
    if (!ADIOI_Reqtable) {
	ADIOI_Reqtable_max = 1024;
	ADIOI_Reqtable = (MPIO_Request *)
	    ADIOI_Malloc(ADIOI_Reqtable_max*sizeof(MPIO_Request)); 
        ADIOI_Reqtable_ptr = 0;  /* 0 can't be used though, because 
                                  MPIO_REQUEST_NULL=0 */
	for (i=0; i<ADIOI_Reqtable_max; i++) ADIOI_Reqtable[i] = MPIO_REQUEST_NULL;
    }
    if (ADIOI_Reqtable_ptr == ADIOI_Reqtable_max-1) {
	ADIOI_Reqtable = (MPIO_Request *) ADIOI_Realloc(ADIOI_Reqtable, 
                           (ADIOI_Reqtable_max+1024)*sizeof(MPIO_Request));
	for (i=ADIOI_Reqtable_max; i<ADIOI_Reqtable_max+1024; i++) 
	    ADIOI_Reqtable[i] = MPIO_REQUEST_NULL;
	ADIOI_Reqtable_max += 1024;
    }
    ADIOI_Reqtable_ptr++;
    ADIOI_Reqtable[ADIOI_Reqtable_ptr] = request;
    return (MPI_Fint) ADIOI_Reqtable_ptr;
#endif
}
