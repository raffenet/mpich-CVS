/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpio.h"

#ifdef __MPIO_BUILD_PROFILING
#ifdef FORTRANCAPS
#define mpi_file_write_ordered_ PMPI_FILE_WRITE_ORDERED
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_write_ordered_ pmpi_file_write_ordered__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_write_ordered pmpi_file_write_ordered_
#endif
#define mpi_file_write_ordered_ pmpi_file_write_ordered
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_write_ordered_ pmpi_file_write_ordered
#endif
#define mpi_file_write_ordered_ pmpi_file_write_ordered_
#endif
#else
#ifdef FORTRANCAPS
#define mpi_file_write_ordered_ MPI_FILE_WRITE_ORDERED
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_write_ordered_ mpi_file_write_ordered__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_write_ordered mpi_file_write_ordered_
#endif
#define mpi_file_write_ordered_ mpi_file_write_ordered
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_write_ordered_ mpi_file_write_ordered
#endif
#endif
#endif

#ifdef __MPIHP
void mpi_file_write_ordered_(MPI_Fint *fh,void *buf,int *count,
                       MPI_Fint *datatype,MPI_Status *status, int *__ierr ){
    MPI_File fh_c;
    MPI_Datatype datatype_c;
    
    fh_c = MPI_File_f2c(*fh);
    datatype_c = MPI_Type_f2c(*datatype);

    *__ierr = MPI_File_write_ordered(fh_c,buf,*count,datatype_c,status);
}
#else
void mpi_file_write_ordered_(MPI_Fint *fh,void *buf,int *count,
                       MPI_Datatype *datatype,MPI_Status *status, int *__ierr ){
    MPI_File fh_c;
    
    fh_c = MPI_File_f2c(*fh);
    *__ierr = MPI_File_write_ordered(fh_c,buf,*count,*datatype,status);
}
#endif
