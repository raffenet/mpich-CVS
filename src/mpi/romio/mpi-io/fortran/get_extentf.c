/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpio.h"

#ifdef __MPIO_BUILD_PROFILING
#ifdef FORTRANCAPS
#define mpi_file_get_type_extent_ PMPI_FILE_GET_TYPE_EXTENT
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_get_type_extent_ pmpi_file_get_type_extent__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_get_type_extent pmpi_file_get_type_extent_
#endif
#define mpi_file_get_type_extent_ pmpi_file_get_type_extent
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_get_type_extent_ pmpi_file_get_type_extent
#endif
#define mpi_file_get_type_extent_ pmpi_file_get_type_extent_
#endif
#else
#ifdef FORTRANCAPS
#define mpi_file_get_type_extent_ MPI_FILE_GET_TYPE_EXTENT
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_get_type_extent_ mpi_file_get_type_extent__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_get_type_extent mpi_file_get_type_extent_
#endif
#define mpi_file_get_type_extent_ mpi_file_get_type_extent
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_get_type_extent_ mpi_file_get_type_extent
#endif
#endif
#endif

#ifdef __MPIHP
void mpi_file_get_type_extent_(MPI_Fint *fh,MPI_Fint *datatype,
                             MPI_Fint *extent, int *__ierr )
{
    MPI_File fh_c;
    MPI_Datatype datatype_c;
    MPI_Aint extent_c;
    
    fh_c = MPI_File_f2c(*fh);
    datatype_c = MPI_Type_f2c(*datatype);

    *__ierr = MPI_File_get_type_extent(fh_c,datatype_c, &extent_c);
    *extent = (MPI_Fint) extent_c;
}

#else

void mpi_file_get_type_extent_(MPI_Fint *fh,MPI_Datatype *datatype,
                             MPI_Fint *extent, int *__ierr )
{
    MPI_File fh_c;
    MPI_Aint extent_c;
    
    fh_c = MPI_File_f2c(*fh);
    *__ierr = MPI_File_get_type_extent(fh_c,*datatype, &extent_c);
    *extent = (MPI_Fint) extent_c;
}
#endif
