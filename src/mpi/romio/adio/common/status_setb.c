/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpi.h"

#ifdef MPICH

void MPID_Status_set_bytes(MPI_Status *status, int nbytes);
int MPIR_Status_set_bytes(MPI_Status *status, MPI_Datatype datatype, 
			  int nbytes);

int MPIR_Status_set_bytes(MPI_Status *status, MPI_Datatype datatype, 
			  int nbytes)
{
#ifdef ROMIO_NTFS
    status->count = nbytes;
    return MPI_SUCCESS;
#else
    MPID_Status_set_bytes(status, nbytes);
#endif
    return MPI_SUCCESS;
}

#endif
