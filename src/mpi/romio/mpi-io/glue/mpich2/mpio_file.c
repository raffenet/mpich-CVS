/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$    
 *
 *   Copyright (C) 2004 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"

/* Hooks for allocation of MPI_File handles.
 *
 * Three functions are used in ROMIO for allocation/deallocation
 * of MPI_File structures:
 * - MPIO_File_create(size)
 * - MPIO_File_resolve(mpi_fh)
 * - MPIO_File_free(mpi_fh)
 *
 */

MPI_File MPIO_File_create(int size)
{
    return (MPI_File) ADIOI_Malloc(size);
}

ADIO_File MPIO_File_resolve(MPI_File mpi_fh)
{
    return mpi_fh;
}

void MPIO_File_free(MPI_File *mpi_fh)
{
    ADIOI_Free(*mpi_fh);
    *mpi_fh = MPI_FILE_NULL;
}
