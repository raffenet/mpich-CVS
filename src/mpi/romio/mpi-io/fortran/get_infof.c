/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpio.h"

#ifdef __MPIO_BUILD_PROFILING
#ifdef FORTRANCAPS
#define mpi_file_get_info_ PMPI_FILE_GET_INFO
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_get_info_ pmpi_file_get_info__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_get_info pmpi_file_get_info_
#endif
#define mpi_file_get_info_ pmpi_file_get_info
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_get_info_ pmpi_file_get_info
#endif
#define mpi_file_get_info_ pmpi_file_get_info_
#endif
#else
#ifdef FORTRANCAPS
#define mpi_file_get_info_ MPI_FILE_GET_INFO
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_get_info_ mpi_file_get_info__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_get_info mpi_file_get_info_
#endif
#define mpi_file_get_info_ mpi_file_get_info
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_get_info_ mpi_file_get_info
#endif
#endif
#endif

void mpi_file_get_info_(MPI_Fint *fh, MPI_Fint *info_used, int *__ierr )
{
    MPI_File fh_c;
    MPI_Info info_used_c;
    
    fh_c = MPI_File_f2c(*fh);

    *__ierr = MPI_File_get_info(fh_c, &info_used_c);
    *info_used = MPI_Info_c2f(info_used_c);
}
