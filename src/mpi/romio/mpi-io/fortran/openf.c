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
#define mpi_file_open_ PMPI_FILE_OPEN
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_open_ pmpi_file_open__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_open pmpi_file_open_
#endif
#define mpi_file_open_ pmpi_file_open
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_open_ pmpi_file_open
#endif
#define mpi_file_open_ pmpi_file_open_
#endif
#else
#ifdef FORTRANCAPS
#define mpi_file_open_ MPI_FILE_OPEN
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_open_ mpi_file_open__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_open mpi_file_open_
#endif
#define mpi_file_open_ mpi_file_open
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_open_ mpi_file_open
#endif
#endif
#endif

#ifdef __MPIHP
void mpi_file_open_(MPI_Fint *comm,char *filename,int *amode,
                  MPI_Fint *info, MPI_Fint *fh, int *__ierr, int str_len )
{
    char *newfname;
    MPI_File fh_c;
    int real_len, i;
    MPI_Comm comm_c;
    MPI_Info info_c;

    comm_c = MPI_Comm_f2c(*comm);
    info_c = MPI_Info_f2c(*info);

    /* strip trailing blanks */
    if (filename <= (char *) 0) {
        printf("MPI_File_open: filename is an invalid address\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    for (i=str_len-1; i>=0; i--) if (filename[i] != ' ') break;
    if (i < 0) {
	printf("MPI_File_open: filename is a blank string\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    real_len = i + 1;

    newfname = (char *) ADIOI_Malloc((real_len+1)*sizeof(char));
    strncpy(newfname, filename, real_len);
    newfname[real_len] = '\0';

    *__ierr = MPI_File_open(comm_c, newfname, *amode, info_c, &fh_c);

    *fh = MPI_File_c2f(fh_c);
    ADIOI_Free(newfname);
}

#else

void mpi_file_open_(MPI_Comm *comm,char *filename,int *amode,
                  MPI_Fint *info, MPI_Fint *fh, int *__ierr, int str_len )
{
    char *newfname;
    MPI_File fh_c;
    int real_len, i;
    MPI_Info info_c;

    info_c = MPI_Info_f2c(*info);

    /* strip trailing blanks */
    if (filename <= (char *) 0) {
        printf("MPI_File_open: filename is an invalid address\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    for (i=str_len-1; i>=0; i--) if (filename[i] != ' ') break;
    if (i < 0) {
	printf("MPI_File_open: filename is a blank string\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    real_len = i + 1;

    newfname = (char *) ADIOI_Malloc((real_len+1)*sizeof(char));
    strncpy(newfname, filename, real_len);
    newfname[real_len] = '\0';

    *__ierr = MPI_File_open(*comm, newfname, *amode, info_c, &fh_c);

    *fh = MPI_File_c2f(fh_c);
    ADIOI_Free(newfname);
}
#endif
