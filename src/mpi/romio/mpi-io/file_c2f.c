/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"
#include "adio_extern.h"

/*@
    MPI_File_c2f - Translates a C file handle to a Fortran file handle

Input Parameters:
. fh - C file handle (integer)

Return Value:
. Fortran file handle (handle)
@*/
MPI_Fint MPI_File_c2f(MPI_File fh)
{
    int i;

#ifndef __INT_LT_POINTER
    return (MPI_Fint) fh;
#else
    if ((fh <= (MPI_File) 0) || (fh->cookie != ADIOI_FILE_COOKIE))
	return (MPI_Fint) 0;
    if (!ADIOI_Ftable) {
	ADIOI_Ftable_max = 1024;
	ADIOI_Ftable = (MPI_File *)
	    ADIOI_Malloc(ADIOI_Ftable_max*sizeof(MPI_File)); 
        ADIOI_Ftable_ptr = 0;  /* 0 can't be used though, because 
                                  MPI_FILE_NULL=0 */
	for (i=0; i<ADIOI_Ftable_max; i++) ADIOI_Ftable[i] = MPI_FILE_NULL;
    }
    if (ADIOI_Ftable_ptr == ADIOI_Ftable_max-1) {
	ADIOI_Ftable = (MPI_File *) ADIOI_Realloc(ADIOI_Ftable, 
                           (ADIOI_Ftable_max+1024)*sizeof(MPI_File));
	for (i=ADIOI_Ftable_max; i<ADIOI_Ftable_max+1024; i++) 
	    ADIOI_Ftable[i] = MPI_FILE_NULL;
	ADIOI_Ftable_max += 1024;
    }
    ADIOI_Ftable_ptr++;
    ADIOI_Ftable[ADIOI_Ftable_ptr] = fh;
    return (MPI_Fint) ADIOI_Ftable_ptr;
#endif
}
