/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpio.h"

#ifdef __MPIO_BUILD_PROFILING
#ifdef FORTRANCAPS
#define mpi_file_get_byte_offset_ PMPI_FILE_GET_BYTE_OFFSET
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_get_byte_offset_ pmpi_file_get_byte_offset__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_get_byte_offset pmpi_file_get_byte_offset_
#endif
#define mpi_file_get_byte_offset_ pmpi_file_get_byte_offset
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_get_byte_offset_ pmpi_file_get_byte_offset
#endif
#define mpi_file_get_byte_offset_ pmpi_file_get_byte_offset_
#endif
#else
#ifdef FORTRANCAPS
#define mpi_file_get_byte_offset_ MPI_FILE_GET_BYTE_OFFSET
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_get_byte_offset_ mpi_file_get_byte_offset__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_get_byte_offset mpi_file_get_byte_offset_
#endif
#define mpi_file_get_byte_offset_ mpi_file_get_byte_offset
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_get_byte_offset_ mpi_file_get_byte_offset
#endif
#endif
#endif

void mpi_file_get_byte_offset_(MPI_Fint *fh,MPI_Offset *offset,MPI_Offset *disp, int *__ierr )
{
    MPI_File fh_c;
    
    fh_c = MPI_File_f2c(*fh);
    *__ierr = MPI_File_get_byte_offset(fh_c,*offset,disp);
}
