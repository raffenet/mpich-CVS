/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpio.h"

#ifdef __MPIO_BUILD_PROFILING
#ifdef FORTRANCAPS
#define mpi_type_create_subarray_ PMPI_TYPE_CREATE_SUBARRAY
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_type_create_subarray_ pmpi_type_create_subarray__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_type_create_subarray pmpi_type_create_subarray_
#endif
#define mpi_type_create_subarray_ pmpi_type_create_subarray
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_type_create_subarray_ pmpi_type_create_subarray
#endif
#define mpi_type_create_subarray_ pmpi_type_create_subarray_
#endif
#else
#ifdef FORTRANCAPS
#define mpi_type_create_subarray_ MPI_TYPE_CREATE_SUBARRAY
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_type_create_subarray_ mpi_type_create_subarray__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_type_create_subarray mpi_type_create_subarray_
#endif
#define mpi_type_create_subarray_ mpi_type_create_subarray
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_type_create_subarray_ mpi_type_create_subarray
#endif
#endif
#endif

#ifdef __MPIHP
void mpi_type_create_subarray_(int *ndims,int *array_of_sizes,
                             int *array_of_subsizes,int *array_of_starts,
                             int *order,MPI_Fint *oldtype,
                             MPI_Fint *newtype, int *__ierr )
{
    MPI_Datatype oldtype_c, newtype_c;

    oldtype_c = MPI_Type_f2c(*oldtype);

    *__ierr = MPI_Type_create_subarray(*ndims,array_of_sizes,array_of_subsizes,array_of_starts,*order,oldtype_c,&newtype_c);
    *newtype = MPI_Type_c2f(newtype_c);
}

#else

void mpi_type_create_subarray_(int *ndims,int *array_of_sizes,
                             int *array_of_subsizes,int *array_of_starts,
                             int *order,MPI_Datatype *oldtype,
                             MPI_Datatype *newtype, int *__ierr ){
*__ierr = MPI_Type_create_subarray(*ndims,array_of_sizes,array_of_subsizes,array_of_starts,*order,*oldtype,newtype);
}
#endif
