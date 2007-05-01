/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *
 *   Copyright (C) 2001 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_testfs.h"
#include "adioi.h"

/* ADIOI_TESTFS_IreadContig()
 *
 * Implemented by immediately calling ReadContig()
 */
void ADIOI_TESTFS_IreadContig(ADIO_File fd, void *buf, int count, 
			      MPI_Datatype datatype, int file_ptr_type,
			      ADIO_Offset offset, ADIO_Request *request, int
			      *error_code)
{
    ADIO_Status status;
    int myrank, nprocs, typesize, len;

    *error_code = MPI_SUCCESS;

    MPI_Comm_size(fd->comm, &nprocs);
    MPI_Comm_rank(fd->comm, &myrank);
    FPRINTF(stdout, "[%d/%d] ADIOI_TESTFS_IreadContig called on %s\n", 
	    myrank, nprocs, fd->filename);
    FPRINTF(stdout, "[%d/%d]    calling ADIOI_TESTFS_ReadContig\n", 
	    myrank, nprocs);

    len = count * typesize;
    ADIOI_TESTFS_ReadContig(fd, buf, len, MPI_BYTE, file_ptr_type, 
			    offset, &status, error_code);

#ifdef HAVE_STATUS_SET_BYTES
    if (*error_code == MPI_SUCCESS) {
	MPI_Get_elements(&status, MPI_BYTE, &len);
	/* what do to with len? */
    }
#endif
}

void ADIOI_TESTFS_IreadStrided(ADIO_File fd, void *buf, int count,
			       MPI_Datatype datatype, int file_ptr_type,
			       ADIO_Offset offset, ADIO_Request *request, int
			       *error_code)
{
    ADIO_Status status;
    int myrank, nprocs;
#ifdef HAVE_STATUS_SET_BYTES
    int typesize;
#endif

    MPI_Comm_size(fd->comm, &nprocs);
    MPI_Comm_rank(fd->comm, &myrank);
    FPRINTF(stdout, "[%d/%d] ADIOI_TESTFS_IreadStrided called on %s\n", 
	    myrank, nprocs, fd->filename);
    FPRINTF(stdout, "[%d/%d]    calling ADIOI_TESTFS_ReadStrided\n", 
	    myrank, nprocs);

    ADIOI_TESTFS_ReadStrided(fd, buf, count, datatype, file_ptr_type, 
			     offset, &status, error_code);

#ifdef HAVE_STATUS_SET_BYTES
    if (*error_code == MPI_SUCCESS) {
	MPI_Type_size(datatype, &typesize);
	/* what to do with count * typesize */
    }
#endif
}

