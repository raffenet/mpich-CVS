/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpio.h"
#include "adio.h"


#if defined(__MPIO_BUILD_PROFILING) || defined(HAVE_WEAK_SYMBOLS)
#ifdef FORTRANCAPS
#define mpi_file_read_all_ PMPI_FILE_READ_ALL
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_read_all_ pmpi_file_read_all__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_read_all pmpi_file_read_all_
#endif
#define mpi_file_read_all_ pmpi_file_read_all
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_read_all_ pmpi_file_read_all
#endif
#define mpi_file_read_all_ pmpi_file_read_all_
#endif

#if defined(HAVE_WEAK_SYMBOLS)
#if defined(HAVE_PRAGMA_WEAK)
#if defined(FORTRANCAPS)
#pragma weak MPI_FILE_READ_ALL = PMPI_FILE_READ_ALL
#elif defined(FORTRANDOUBLEUNDERSCORE)
#pragma weak mpi_file_read_all__ = pmpi_file_read_all__
#elif !defined(FORTRANUNDERSCORE)
#pragma weak mpi_file_read_all = pmpi_file_read_all
#else
#pragma weak mpi_file_read_all_ = pmpi_file_read_all_
#endif

#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#if defined(FORTRANCAPS)
#pragma _HP_SECONDARY_DEF PMPI_FILE_READ_ALL = MPI_FILE_READ_ALL
#elif defined(FORTRANDOUBLEUNDERSCORE)
#pragma _HP_SECONDARY_DEF pmpi_file_read_all__ = mpi_file_read_all__
#elif !defined(FORTRANUNDERSCORE)
#pragma _HP_SECONDARY_DEF pmpi_file_read_all = mpi_file_read_all
#else
#pragma _HP_SECONDARY_DEF pmpi_file_read_all_ = mpi_file_read_all_
#endif

#elif defined(HAVE_PRAGMA_CRI_DUP)
#if defined(FORTRANCAPS)
#pragma _CRI duplicate MPI_FILE_READ_ALL as PMPI_FILE_READ_ALL
#elif defined(FORTRANDOUBLEUNDERSCORE)
#pragma _CRI duplicate mpi_file_read_all__ as pmpi_file_read_all__
#elif !defined(FORTRANUNDERSCORE)
#pragma _CRI duplicate mpi_file_read_all as pmpi_file_read_all
#else
#pragma _CRI duplicate mpi_file_read_all_ as pmpi_file_read_all_
#endif

/* end of weak pragmas */
#endif
/* Include mapping from MPI->PMPI */
#include "mpioprof.h"
#endif

#else

#ifdef FORTRANCAPS
#define mpi_file_read_all_ MPI_FILE_READ_ALL
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_read_all_ mpi_file_read_all__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_read_all mpi_file_read_all_
#endif
#define mpi_file_read_all_ mpi_file_read_all
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_read_all_ mpi_file_read_all
#endif
#endif
#endif

#if defined(__MPIHP) || defined(__MPILAM)
void mpi_file_read_all_(MPI_Fint *fh,void *buf,int *count,
                      MPI_Fint *datatype,MPI_Status *status, int *__ierr )
{
    MPI_File fh_c;
    MPI_Datatype datatype_c;
    
    fh_c = MPI_File_f2c(*fh);
    datatype_c = MPI_Type_f2c(*datatype);

    *__ierr = MPI_File_read_all(fh_c,buf,*count,datatype_c,status);
}
#else
void mpi_file_read_all_(MPI_Fint *fh,void *buf,int *count,
                      MPI_Datatype *datatype,MPI_Status *status, int *__ierr ){
    MPI_File fh_c;
    
    fh_c = MPI_File_f2c(*fh);
    *__ierr = MPI_File_read_all(fh_c,buf,*count,*datatype,status);
}
#endif
