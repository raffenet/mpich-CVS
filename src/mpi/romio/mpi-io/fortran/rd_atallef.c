/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpio.h"

#ifdef __MPIO_BUILD_PROFILING
#ifdef FORTRANCAPS
#define mpi_file_read_at_all_end_ PMPI_FILE_READ_AT_ALL_END
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_read_at_all_end_ pmpi_file_read_at_all_end__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_read_at_all_end pmpi_file_read_at_all_end_
#endif
#define mpi_file_read_at_all_end_ pmpi_file_read_at_all_end
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_read_at_all_end_ pmpi_file_read_at_all_end
#endif
#define mpi_file_read_at_all_end_ pmpi_file_read_at_all_end_
#endif
#else
#ifdef FORTRANCAPS
#define mpi_file_read_at_all_end_ MPI_FILE_READ_AT_ALL_END
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_read_at_all_end_ mpi_file_read_at_all_end__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_read_at_all_end mpi_file_read_at_all_end_
#endif
#define mpi_file_read_at_all_end_ mpi_file_read_at_all_end
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_read_at_all_end_ mpi_file_read_at_all_end
#endif
#endif
#endif

void mpi_file_read_at_all_end_(MPI_Fint *fh,void *buf,MPI_Status *status, int *__ierr )
{
    MPI_File fh_c;
    
    fh_c = MPI_File_f2c(*fh);
    *__ierr = MPI_File_read_at_all_end(fh_c,buf,status);
}

