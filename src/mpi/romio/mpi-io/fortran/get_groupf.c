/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpio.h"

#ifdef __MPIO_BUILD_PROFILING
#ifdef FORTRANCAPS
#define mpi_file_get_group_ PMPI_FILE_GET_GROUP
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_get_group_ pmpi_file_get_group__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_get_group pmpi_file_get_group_
#endif
#define mpi_file_get_group_ pmpi_file_get_group
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_get_group_ pmpi_file_get_group
#endif
#define mpi_file_get_group_ pmpi_file_get_group_
#endif
#else
#ifdef FORTRANCAPS
#define mpi_file_get_group_ MPI_FILE_GET_GROUP
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_get_group_ mpi_file_get_group__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_get_group mpi_file_get_group_
#endif
#define mpi_file_get_group_ mpi_file_get_group
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_get_group_ mpi_file_get_group
#endif
#endif
#endif

#ifdef __MPIHP
void mpi_file_get_group_(MPI_Fint *fh,MPI_Fint *group, int *__ierr )
{
    MPI_File fh_c;
    MPI_Group group_c;

    fh_c = MPI_File_f2c(*fh);
    *__ierr = MPI_File_get_group(fh_c, &group_c);
    *group = MPI_Group_c2f(group_c);
}
#else
void mpi_file_get_group_(MPI_Fint *fh,MPI_Group *group, int *__ierr )
{
    MPI_File fh_c;
    
    fh_c = MPI_File_f2c(*fh);
    *__ierr = MPI_File_get_group(fh_c, group);
}
#endif
