/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"
#include "adio_extern.h"

/*@
    MPI_Info_f2c - Translates a Fortran info handle to a C info handle

Input Parameters:
. info - Fortran info handle (integer)

Return Value:
  C info handle (handle)
@*/
MPI_Info MPI_Info_f2c(MPI_Fint info)
{

#ifndef __INT_LT_POINTER
    return (MPI_Info) info;
#else
    if (!info) return MPI_INFO_NULL;
    if ((info < 0) || (info > MPIR_Infotable_ptr)) {
	printf("MPI_Info_f2c: Invalid info handle\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }
    return MPIR_Infotable[info];
#endif
}
