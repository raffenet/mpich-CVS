/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpio.h"

#ifdef __MPIO_BUILD_PROFILING
#ifdef FORTRANCAPS
#define mpi_file_close_ PMPI_FILE_CLOSE
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_close_ pmpi_file_close__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_close pmpi_file_close_
#endif
#define mpi_file_close_ pmpi_file_close
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_close_ pmpi_file_close
#endif
#define mpi_file_close_ pmpi_file_close_
#endif
#else
#ifdef FORTRANCAPS
#define mpi_file_close_ MPI_FILE_CLOSE
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_close_ mpi_file_close__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_close mpi_file_close_
#endif
#define mpi_file_close_ mpi_file_close
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_close_ mpi_file_close
#endif
#endif
#endif

void mpi_file_close_(MPI_Fint *fh, int *__ierr )
{
    MPI_File fh_c;

    fh_c = MPI_File_f2c(*fh);
    *__ierr = MPI_File_close(&fh_c);
    *fh = MPI_File_c2f(fh_c);
}
