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
#define mpi_file_get_view_ PMPI_FILE_GET_VIEW
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_get_view_ pmpi_file_get_view__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_get_view pmpi_file_get_view_
#endif
#define mpi_file_get_view_ pmpi_file_get_view
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_get_view_ pmpi_file_get_view
#endif
#define mpi_file_get_view_ pmpi_file_get_view_
#endif
#else
#ifdef FORTRANCAPS
#define mpi_file_get_view_ MPI_FILE_GET_VIEW
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_get_view_ mpi_file_get_view__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_get_view mpi_file_get_view_
#endif
#define mpi_file_get_view_ mpi_file_get_view
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_get_view_ mpi_file_get_view
#endif
#endif
#endif


#ifdef __MPIHP
void mpi_file_get_view_(MPI_Fint *fh,MPI_Offset *disp,MPI_Fint *etype,
   MPI_Fint *filetype,char *datarep, int *__ierr, int str_len )
{
    MPI_File fh_c;
    MPI_Datatype etype_c, filetype_c;
    int i, tmpreplen;
    char *tmprep;

    if (datarep <= (char *) 0) {
        printf("MPI_File_get_view: datarep is an invalid address\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    tmprep = (char *) ADIOI_Malloc((MPI_MAX_DATAREP_STRING+1) * sizeof(char));
    fh_c = MPI_File_f2c(*fh);
    *__ierr = MPI_File_get_view(fh_c, disp, &etype_c, &filetype_c, tmprep);

    tmpreplen = strlen(tmprep);
    if (tmpreplen <= str_len) {
        strncpy(datarep, tmprep, tmpreplen);

        /* blank pad the remaining space */
        for (i=tmpreplen; i<str_len; i++) datarep[i] = ' ';
    }
    else {
        /* not enough space */
        strncpy(datarep, tmprep, str_len);
        /* this should be flagged as an error. */
        *__ierr = MPI_ERR_UNKNOWN;
    }
    
    *etype = MPI_Type_c2f(etype_c);
    *filetype = MPI_Type_c2f(filetype_c);
    ADIOI_Free(tmprep);
}

#else

void mpi_file_get_view_(MPI_Fint *fh,MPI_Offset *disp,MPI_Datatype *etype,
   MPI_Datatype *filetype,char *datarep, int *__ierr, int str_len )
{
    MPI_File fh_c;
    int i, tmpreplen;
    char *tmprep;

/* Initialize the string to all blanks */
    if (datarep <= (char *) 0) {
        printf("MPI_File_get_view: datarep is an invalid address\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    
    tmprep = (char *) ADIOI_Malloc((MPI_MAX_DATAREP_STRING+1) * sizeof(char));
    fh_c = MPI_File_f2c(*fh);
    *__ierr = MPI_File_get_view(fh_c, disp, etype, filetype, tmprep);

    tmpreplen = strlen(tmprep);
    if (tmpreplen <= str_len) {
        strncpy(datarep, tmprep, tmpreplen);

        /* blank pad the remaining space */
        for (i=tmpreplen; i<str_len; i++) datarep[i] = ' ';
    }
    else {
        /* not enough space */
        strncpy(datarep, tmprep, str_len);
        /* this should be flagged as an error. */
        *__ierr = MPI_ERR_UNKNOWN;
    }

    ADIOI_Free(tmprep);
}
#endif
