/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpio.h"

#ifdef __MPIO_BUILD_PROFILING
#ifdef FORTRANCAPS
#define mpi_info_get_nkeys_ PMPI_INFO_GET_NKEYS
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_info_get_nkeys_ pmpi_info_get_nkeys__
#elif !defined(FORTRANUNDERSCORE)
#define mpi_info_get_nkeys_ pmpi_info_get_nkeys
#else
#define mpi_info_get_nkeys_ pmpi_info_get_nkeys_
#endif
#else
#ifdef FORTRANCAPS
#define mpi_info_get_nkeys_ MPI_INFO_GET_NKEYS
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_info_get_nkeys_ mpi_info_get_nkeys__
#elif !defined(FORTRANUNDERSCORE)
#define mpi_info_get_nkeys_ mpi_info_get_nkeys
#endif
#endif

void mpi_info_get_nkeys_(MPI_Fint *info, int *nkeys, int *__ierr )
{
    MPI_Info info_c;
    
    info_c = MPI_Info_f2c(*info);
    *__ierr = MPI_Info_get_nkeys(info_c, nkeys);
}
