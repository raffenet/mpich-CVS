/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpio.h"

#ifdef __MPIO_BUILD_PROFILING
#ifdef FORTRANCAPS
#define mpi_file_seek_ PMPI_FILE_SEEK
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_seek_ pmpi_file_seek__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_seek pmpi_file_seek_
#endif
#define mpi_file_seek_ pmpi_file_seek
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_seek_ pmpi_file_seek
#endif
#define mpi_file_seek_ pmpi_file_seek_
#endif
#else
#ifdef FORTRANCAPS
#define mpi_file_seek_ MPI_FILE_SEEK
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_seek_ mpi_file_seek__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_seek mpi_file_seek_
#endif
#define mpi_file_seek_ mpi_file_seek
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_seek_ mpi_file_seek
#endif
#endif
#endif

void mpi_file_seek_(MPI_Fint *fh,MPI_Offset *offset,int *whence, int *__ierr )
{
    MPI_File fh_c;
    
    fh_c = MPI_File_f2c(*fh);
    *__ierr = MPI_File_seek(fh_c,*offset,*whence);
}
