/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpio.h"

#ifdef __MPIO_BUILD_PROFILING
#ifdef FORTRANCAPS
#define mpi_file_get_amode_ PMPI_FILE_GET_AMODE
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_get_amode_ pmpi_file_get_amode__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_get_amode pmpi_file_get_amode_
#endif
#define mpi_file_get_amode_ pmpi_file_get_amode
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_get_amode_ pmpi_file_get_amode
#endif
#define mpi_file_get_amode_ pmpi_file_get_amode_
#endif
#else
#ifdef FORTRANCAPS
#define mpi_file_get_amode_ MPI_FILE_GET_AMODE
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_get_amode_ mpi_file_get_amode__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_get_amode mpi_file_get_amode_
#endif
#define mpi_file_get_amode_ mpi_file_get_amode
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_get_amode_ mpi_file_get_amode
#endif
#endif
#endif

void mpi_file_get_amode_(MPI_Fint *fh,int *amode, int *__ierr )
{
    MPI_File fh_c;
    
    fh_c = MPI_File_f2c(*fh);
    *__ierr = MPI_File_get_amode(fh_c, amode);
}
