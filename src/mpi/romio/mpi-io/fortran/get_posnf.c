/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpio.h"

#ifdef __MPIO_BUILD_PROFILING
#ifdef FORTRANCAPS
#define mpi_file_get_position_ PMPI_FILE_GET_POSITION
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_get_position_ pmpi_file_get_position__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_get_position pmpi_file_get_position_
#endif
#define mpi_file_get_position_ pmpi_file_get_position
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_get_position_ pmpi_file_get_position
#endif
#define mpi_file_get_position_ pmpi_file_get_position_
#endif
#else
#ifdef FORTRANCAPS
#define mpi_file_get_position_ MPI_FILE_GET_POSITION
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_get_position_ mpi_file_get_position__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_get_position mpi_file_get_position_
#endif
#define mpi_file_get_position_ mpi_file_get_position
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_get_position_ mpi_file_get_position
#endif
#endif
#endif

void mpi_file_get_position_(MPI_Fint *fh, MPI_Offset *offset, int *__ierr )
{
    MPI_File fh_c;
    
    fh_c = MPI_File_f2c(*fh);
    *__ierr = MPI_File_get_position(fh_c, offset);
}
