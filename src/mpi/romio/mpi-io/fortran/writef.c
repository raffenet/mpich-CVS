/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpio.h"

#ifdef __MPIO_BUILD_PROFILING
#ifdef FORTRANCAPS
#define mpi_file_write_ PMPI_FILE_WRITE
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_write_ pmpi_file_write__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_write pmpi_file_write_
#endif
#define mpi_file_write_ pmpi_file_write
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_write_ pmpi_file_write
#endif
#define mpi_file_write_ pmpi_file_write_
#endif
#else
#ifdef FORTRANCAPS
#define mpi_file_write_ MPI_FILE_WRITE
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_write_ mpi_file_write__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_write mpi_file_write_
#endif
#define mpi_file_write_ mpi_file_write
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_write_ mpi_file_write
#endif
#endif
#endif

#ifdef __MPIHP
void mpi_file_write_(MPI_Fint *fh,void *buf,int *count,
                   MPI_Fint *datatype,MPI_Status *status, int *__ierr )
{
    MPI_File fh_c;
    MPI_Datatype datatype_c;
    
    fh_c = MPI_File_f2c(*fh);
    datatype_c = MPI_Type_f2c(*datatype);

    *__ierr = MPI_File_write(fh_c, buf,*count,datatype_c,status);
}
#else
void mpi_file_write_(MPI_Fint *fh,void *buf,int *count,
                   MPI_Datatype *datatype,MPI_Status *status, int *__ierr )
{
    MPI_File fh_c;
    
    fh_c = MPI_File_f2c(*fh);
    *__ierr = MPI_File_write(fh_c, buf,*count,*datatype,status);
}
#endif
