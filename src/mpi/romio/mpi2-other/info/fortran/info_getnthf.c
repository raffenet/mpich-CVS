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
#define mpi_info_get_nthkey_ PMPI_INFO_GET_NTHKEY
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_info_get_nthkey_ pmpi_info_get_nthkey__
#elif !defined(FORTRANUNDERSCORE)
#define mpi_info_get_nthkey_ pmpi_info_get_nthkey
#else
#define mpi_info_get_nthkey_ pmpi_info_get_nthkey_
#endif
#else
#ifdef FORTRANCAPS
#define mpi_info_get_nthkey_ MPI_INFO_GET_NTHKEY
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_info_get_nthkey_ mpi_info_get_nthkey__
#elif !defined(FORTRANUNDERSCORE)
#define mpi_info_get_nthkey_ mpi_info_get_nthkey
#endif
#endif

void mpi_info_get_nthkey_(MPI_Fint *info, int *n, char *key, int *__ierr,
                          int keylen)
{
    MPI_Info info_c;
    int i, tmpkeylen;
    char *tmpkey;

    if (key <= (char *) 0) {
        printf("MPI_Info_get_nthkey: key is an invalid address\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    tmpkey = (char *) ADIOI_Malloc((MPI_MAX_INFO_KEY+1) * sizeof(char));
    info_c = MPI_Info_f2c(*info);
    *__ierr = MPI_Info_get_nthkey(info_c, *n, tmpkey);

    tmpkeylen = strlen(tmpkey);

    if (tmpkeylen <= keylen) {
	strncpy(key, tmpkey, tmpkeylen);

	/* blank pad the remaining space */
	for (i=tmpkeylen; i<keylen; i++) key[i] = ' ';
    }
    else {
	/* not enough space */
	strncpy(key, tmpkey, keylen);
	/* this should be flagged as an error. */
	*__ierr = MPI_ERR_UNKNOWN;
    }

    ADIOI_Free(tmpkey);
}

