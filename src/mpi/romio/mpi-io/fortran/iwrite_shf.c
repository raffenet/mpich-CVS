/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpio.h"

#ifdef __MPIO_BUILD_PROFILING
#ifdef FORTRANCAPS
#define mpi_file_iwrite_shared_ PMPI_FILE_IWRITE_SHARED
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_iwrite_shared_ pmpi_file_iwrite_shared__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_iwrite_shared pmpi_file_iwrite_shared_
#endif
#define mpi_file_iwrite_shared_ pmpi_file_iwrite_shared
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_iwrite_shared_ pmpi_file_iwrite_shared
#endif
#define mpi_file_iwrite_shared_ pmpi_file_iwrite_shared_
#endif
#else
#ifdef FORTRANCAPS
#define mpi_file_iwrite_shared_ MPI_FILE_IWRITE_SHARED
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_iwrite_shared_ mpi_file_iwrite_shared__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_iwrite_shared mpi_file_iwrite_shared_
#endif
#define mpi_file_iwrite_shared_ mpi_file_iwrite_shared
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_iwrite_shared_ mpi_file_iwrite_shared
#endif
#endif
#endif

#ifdef __MPIHP
void mpi_file_iwrite_shared_(MPI_Fint *fh,void *buf,int *count,
                    MPI_Fint *datatype,MPI_Fint *request, int *__ierr )
{
    MPI_File fh_c;
    MPIO_Request req_c;
    MPI_Datatype datatype_c;
    
    fh_c = MPI_File_f2c(*fh);
    datatype_c = MPI_Type_f2c(*datatype);

    *__ierr = MPI_File_iwrite_shared(fh_c,buf,*count,datatype_c,&req_c);
    *request = MPIO_Request_c2f(req_c);
}
#else
void mpi_file_iwrite_shared_(MPI_Fint *fh,void *buf,int *count,
                    MPI_Datatype *datatype,MPI_Fint *request, int *__ierr )
{
    MPI_File fh_c;
    MPIO_Request req_c;
    
    fh_c = MPI_File_f2c(*fh);
    *__ierr = MPI_File_iwrite_shared(fh_c,buf,*count,*datatype,&req_c);
    *request = MPIO_Request_c2f(req_c);
}
#endif
