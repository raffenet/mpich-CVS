/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpio.h"

#ifdef __MPIO_BUILD_PROFILING
#ifdef FORTRANCAPS
#define mpi_file_set_info_ PMPI_FILE_SET_INFO
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_set_info_ pmpi_file_set_info__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_set_info pmpi_file_set_info_
#endif
#define mpi_file_set_info_ pmpi_file_set_info
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_set_info_ pmpi_file_set_info
#endif
#define mpi_file_set_info_ pmpi_file_set_info_
#endif
#else
#ifdef FORTRANCAPS
#define mpi_file_set_info_ MPI_FILE_SET_INFO
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_set_info_ mpi_file_set_info__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_set_info mpi_file_set_info_
#endif
#define mpi_file_set_info_ mpi_file_set_info
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_set_info_ mpi_file_set_info
#endif
#endif
#endif

void mpi_file_set_info_(MPI_Fint *fh, MPI_Fint *info, int *__ierr )
{
    MPI_File fh_c;
    MPI_Info info_c;
    
    fh_c = MPI_File_f2c(*fh);
    info_c = MPI_Info_f2c(*info);

    *__ierr = MPI_File_set_info(fh_c, info_c);
}
