/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpio.h"

#ifdef __MPIO_BUILD_PROFILING
#ifdef FORTRANCAPS
#define mpi_file_set_size_ PMPI_FILE_SET_SIZE
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_set_size_ pmpi_file_set_size__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_set_size pmpi_file_set_size_
#endif
#define mpi_file_set_size_ pmpi_file_set_size
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_set_size_ pmpi_file_set_size
#endif
#define mpi_file_set_size_ pmpi_file_set_size_
#endif
#else
#ifdef FORTRANCAPS
#define mpi_file_set_size_ MPI_FILE_SET_SIZE
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_set_size_ mpi_file_set_size__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_set_size mpi_file_set_size_
#endif
#define mpi_file_set_size_ mpi_file_set_size
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_set_size_ mpi_file_set_size
#endif
#endif
#endif

#if defined(__cplusplus)
extern "C" {
#endif
void mpi_file_set_size_(MPI_Fint *fh,MPI_Offset *size, int *__ierr )
{
    MPI_File fh_c;
    
    fh_c = MPI_File_f2c(*fh);
    *__ierr = MPI_File_set_size(fh_c,*size);
}
#if defined(__cplusplus)
}
#endif
