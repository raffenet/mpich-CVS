/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "adio.h"
#include "mpio.h"


#if defined(MPIO_BUILD_PROFILING) || defined(HAVE_WEAK_SYMBOLS)
#ifdef FORTRANCAPS
#define mpi_file_read_shared_ PMPI_FILE_READ_SHARED
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_read_shared_ pmpi_file_read_shared__
#elif !defined(FORTRANUNDERSCORE)
#if defined(HPUX) || defined(SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_read_shared pmpi_file_read_shared_
#endif
#define mpi_file_read_shared_ pmpi_file_read_shared
#else
#if defined(HPUX) || defined(SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_read_shared_ pmpi_file_read_shared
#endif
#define mpi_file_read_shared_ pmpi_file_read_shared_
#endif

#if defined(HAVE_WEAK_SYMBOLS)
#if defined(HAVE_PRAGMA_WEAK)
#if defined(FORTRANCAPS)
#pragma weak MPI_FILE_READ_SHARED = PMPI_FILE_READ_SHARED
#elif defined(FORTRANDOUBLEUNDERSCORE)
#pragma weak mpi_file_read_shared__ = pmpi_file_read_shared__
#elif !defined(FORTRANUNDERSCORE)
#pragma weak mpi_file_read_shared = pmpi_file_read_shared
#else
#pragma weak mpi_file_read_shared_ = pmpi_file_read_shared_
#endif

#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#if defined(FORTRANCAPS)
#pragma _HP_SECONDARY_DEF PMPI_FILE_READ_SHARED MPI_FILE_READ_SHARED
#elif defined(FORTRANDOUBLEUNDERSCORE)
#pragma _HP_SECONDARY_DEF pmpi_file_read_shared__ mpi_file_read_shared__
#elif !defined(FORTRANUNDERSCORE)
#pragma _HP_SECONDARY_DEF pmpi_file_read_shared mpi_file_read_shared
#else
#pragma _HP_SECONDARY_DEF pmpi_file_read_shared_ mpi_file_read_shared_
#endif

#elif defined(HAVE_PRAGMA_CRI_DUP)
#if defined(FORTRANCAPS)
#pragma _CRI duplicate MPI_FILE_READ_SHARED as PMPI_FILE_READ_SHARED
#elif defined(FORTRANDOUBLEUNDERSCORE)
#pragma _CRI duplicate mpi_file_read_shared__ as pmpi_file_read_shared__
#elif !defined(FORTRANUNDERSCORE)
#pragma _CRI duplicate mpi_file_read_shared as pmpi_file_read_shared
#else
#pragma _CRI duplicate mpi_file_read_shared_ as pmpi_file_read_shared_
#endif

/* end of weak pragmas */
#endif
/* Include mapping from MPI->PMPI */
#include "mpioprof.h"
#endif

#else

#ifdef FORTRANCAPS
#define mpi_file_read_shared_ MPI_FILE_READ_SHARED
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_read_shared_ mpi_file_read_shared__
#elif !defined(FORTRANUNDERSCORE)
#if defined(HPUX) || defined(SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_read_shared mpi_file_read_shared_
#endif
#define mpi_file_read_shared_ mpi_file_read_shared
#else
#if defined(HPUX) || defined(SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_read_shared_ mpi_file_read_shared
#endif
#endif
#endif

#if defined(MPIHP) || defined(MPILAM)
/* Prototype to keep compiler happy */
void mpi_file_read_shared_(MPI_Fint *fh,void *buf,int *count,
			   MPI_Fint *datatype,MPI_Status *status, int *ierr );
void mpi_file_read_shared_(MPI_Fint *fh,void *buf,int *count,
                  MPI_Fint *datatype,MPI_Status *status, int *ierr )
{
    MPI_File fh_c;
    MPI_Datatype datatype_c;
    
    fh_c = MPI_File_f2c(*fh);
    datatype_c = MPI_Type_f2c(*datatype);

    *ierr = MPI_File_read_shared(fh_c,buf,*count,datatype_c,status);
}
#else
/* Prototype to keep compiler happy */
void FORT_CALL mpi_file_read_shared_(MPI_Fint *fh,void *buf,int *count,
			   MPI_Datatype *datatype,MPI_Status *status, int *ierr ); 
void FORT_CALL mpi_file_read_shared_(MPI_Fint *fh,void *buf,int *count,
                  MPI_Datatype *datatype,MPI_Status *status, int *ierr )
{
    MPI_File fh_c;
    
    fh_c = MPI_File_f2c(*fh);
    *ierr = MPI_File_read_shared(fh_c,buf,*count,*datatype,status);
}
#endif
