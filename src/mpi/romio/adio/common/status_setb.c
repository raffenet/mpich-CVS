/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#ifdef MPICH

#include "mpi.h"

void MPID_Status_set_bytes(MPI_Status *status, int nbytes);

int MPIR_Status_set_bytes(MPI_Status *status, MPI_Datatype datatype, 
			  int nbytes)
{
    MPID_Status_set_bytes(status, nbytes);
#if 0
    /* MPID_Status_set_bytes() should do this in the default case */
    status->count = nbytes;
    return MPI_SUCCESS;
#endif
}

#endif
