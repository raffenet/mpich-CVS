/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpio.h"

#ifdef __MPIO_BUILD_PROFILING
#ifdef FORTRANCAPS
#define mpi_info_create_ PMPI_INFO_CREATE
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_info_create_ pmpi_info_create__
#elif !defined(FORTRANUNDERSCORE)
#define mpi_info_create_ pmpi_info_create
#else
#define mpi_info_create_ pmpi_info_create_
#endif
#else
#ifdef FORTRANCAPS
#define mpi_info_create_ MPI_INFO_CREATE
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_info_create_ mpi_info_create__
#elif !defined(FORTRANUNDERSCORE)
#define mpi_info_create_ mpi_info_create
#endif
#endif

void mpi_info_create_(MPI_Fint *info, int *__ierr )
{
    MPI_Info info_c;

    *__ierr = MPI_Info_create(&info_c);
    *info = MPI_Info_c2f(info_c);
}
