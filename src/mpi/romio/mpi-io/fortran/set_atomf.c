/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpio.h"

#ifdef __MPIO_BUILD_PROFILING
#ifdef FORTRANCAPS
#define mpi_file_set_atomicity_ PMPI_FILE_SET_ATOMICITY
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_set_atomicity_ pmpi_file_set_atomicity__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_set_atomicity pmpi_file_set_atomicity_
#endif
#define mpi_file_set_atomicity_ pmpi_file_set_atomicity
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_set_atomicity_ pmpi_file_set_atomicity
#endif
#define mpi_file_set_atomicity_ pmpi_file_set_atomicity_
#endif
#else
#ifdef FORTRANCAPS
#define mpi_file_set_atomicity_ MPI_FILE_SET_ATOMICITY
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_set_atomicity_ mpi_file_set_atomicity__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_set_atomicity mpi_file_set_atomicity_
#endif
#define mpi_file_set_atomicity_ mpi_file_set_atomicity
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_set_atomicity_ mpi_file_set_atomicity
#endif
#endif
#endif

#if defined(__cplusplus)
extern "C" {
#endif
void mpi_file_set_atomicity_(MPI_Fint *fh,int *flag, int *__ierr )
{
    MPI_File fh_c;
    
    fh_c = MPI_File_f2c(*fh);
    *__ierr = MPI_File_set_atomicity(fh_c,*flag);
}
#if defined(__cplusplus)
}
#endif
