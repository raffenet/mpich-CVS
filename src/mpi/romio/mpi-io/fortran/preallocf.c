/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpio.h"

#ifdef __MPIO_BUILD_PROFILING
#ifdef FORTRANCAPS
#define mpi_file_preallocate_ PMPI_FILE_PREALLOCATE
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_preallocate_ pmpi_file_preallocate__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_preallocate pmpi_file_preallocate_
#endif
#define mpi_file_preallocate_ pmpi_file_preallocate
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_preallocate_ pmpi_file_preallocate
#endif
#define mpi_file_preallocate_ pmpi_file_preallocate_
#endif
#else
#ifdef FORTRANCAPS
#define mpi_file_preallocate_ MPI_FILE_PREALLOCATE
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_preallocate_ mpi_file_preallocate__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_preallocate mpi_file_preallocate_
#endif
#define mpi_file_preallocate_ mpi_file_preallocate
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_preallocate_ mpi_file_preallocate
#endif
#endif
#endif


#if defined(__cplusplus)
extern "C" {
#endif
void mpi_file_preallocate_(MPI_Fint *fh,MPI_Offset *size, int *__ierr )
{
    MPI_File fh_c;
    
    fh_c = MPI_File_f2c(*fh);
    *__ierr = MPI_File_preallocate(fh_c,*size);
}
#if defined(__cplusplus)
}
#endif
