/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpio.h"
#include "adio.h"

#ifdef __MPIO_BUILD_PROFILING
#ifdef FORTRANCAPS
#define mpi_file_delete_ PMPI_FILE_DELETE
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_delete_ pmpi_file_delete__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_delete pmpi_file_delete_
#endif
#define mpi_file_delete_ pmpi_file_delete
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_delete_ pmpi_file_delete
#endif
#define mpi_file_delete_ pmpi_file_delete_
#endif
#else
#ifdef FORTRANCAPS
#define mpi_file_delete_ MPI_FILE_DELETE
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_delete_ mpi_file_delete__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_delete mpi_file_delete_
#endif
#define mpi_file_delete_ mpi_file_delete
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_delete_ mpi_file_delete
#endif
#endif
#endif

void mpi_file_delete_(char *filename, MPI_Fint *info, int *__ierr, int str_len)
{
    char *newfname;
    int real_len, i;
    MPI_Info info_c;

    info_c = MPI_Info_f2c(*info);

    /* strip trailing blanks */
    if (filename <= (char *) 0) {
        printf("MPI_File_delete: filename is an invalid address\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    for (i=str_len-1; i>=0; i--) if (filename[i] != ' ') break;
    if (i < 0) {
        printf("MPI_File_delete: filename is a blank string\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    real_len = i + 1;

    newfname = (char *) ADIOI_Malloc((real_len+1)*sizeof(char));
    strncpy(newfname, filename, real_len);
    newfname[real_len] = '\0';

    *__ierr = MPI_File_delete(newfname, info_c);

    ADIOI_Free(newfname);
}
