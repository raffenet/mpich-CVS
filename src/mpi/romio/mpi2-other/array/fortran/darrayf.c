/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpio.h"

#ifdef __MPIO_BUILD_PROFILING
#ifdef FORTRANCAPS
#define mpi_type_create_darray_ PMPI_TYPE_CREATE_DARRAY
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_type_create_darray_ pmpi_type_create_darray__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_type_create_darray pmpi_type_create_darray_
#endif
#define mpi_type_create_darray_ pmpi_type_create_darray
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_type_create_darray_ pmpi_type_create_darray
#endif
#define mpi_type_create_darray_ pmpi_type_create_darray_
#endif
#else
#ifdef FORTRANCAPS
#define mpi_type_create_darray_ MPI_TYPE_CREATE_DARRAY
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_type_create_darray_ mpi_type_create_darray__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_type_create_darray mpi_type_create_darray_
#endif
#define mpi_type_create_darray_ mpi_type_create_darray
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_type_create_darray_ mpi_type_create_darray
#endif
#endif
#endif

#ifdef __MPIHP
void mpi_type_create_darray_(int *size,int *rank,int *ndims,
                           int *array_of_gsizes,int *array_of_distribs,
                           int *array_of_dargs,int *array_of_psizes,
                           int *order, MPI_Fint *oldtype,
                           MPI_Fint *newtype, int *__ierr )
{
    MPI_Datatype oldtype_c, newtype_c;

    oldtype_c = MPI_Type_f2c(*oldtype);

    *__ierr = MPI_Type_create_darray(*size,*rank,*ndims,array_of_gsizes,array_of_distribs,array_of_dargs,array_of_psizes,*order,oldtype_c,&newtype_c);

    *newtype = MPI_Type_c2f(newtype_c);
}

#else

void mpi_type_create_darray_(int *size,int *rank,int *ndims,
                         int *array_of_gsizes,int *array_of_distribs,
                           int *array_of_dargs,int *array_of_psizes,
                           int *order,MPI_Datatype *oldtype,
                           MPI_Datatype *newtype, int *__ierr )
{
    *__ierr = MPI_Type_create_darray(*size,*rank,*ndims,array_of_gsizes,array_of_distribs,array_of_dargs,array_of_psizes,*order,*oldtype,newtype);
}
#endif
