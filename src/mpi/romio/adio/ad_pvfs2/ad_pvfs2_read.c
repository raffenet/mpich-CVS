/* -*- Mode: C; c-basic-offset:4 ; -*- 
 *   vim: ts=8 sts=4 sw=4 noexpandtab
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "adio.h"
#include "adio_extern.h"
#include "ad_pvfs2.h"
#include "ad_pvfs2_io.h"
#include "ad_pvfs2_common.h"

/* Function prototypes */
int ADIOI_PVFS2_ReadStridedListIO(ADIO_File fd, void *buf, int count,
				  MPI_Datatype datatype, int file_ptr_type,
				  ADIO_Offset offset, ADIO_Status *status, 
				  int *error_code);
int ADIOI_PVFS2_ReadStridedDtypeIO(ADIO_File fd, void *buf, int count,
				   MPI_Datatype datatype, int file_ptr_type,
				   ADIO_Offset offset, ADIO_Status *status, 
				   int *error_code);

void ADIOI_PVFS2_ReadContig(ADIO_File fd, void *buf, int count, 
			    MPI_Datatype datatype, int file_ptr_type,
			    ADIO_Offset offset, ADIO_Status *status,
			    int *error_code)
{
    int ret = -1;
    ret = ADIOI_PVFS2_Contig(fd, buf, count,
			     datatype, file_ptr_type,
			     offset, status,
			     error_code, READ);
    return;
}


int ADIOI_PVFS2_ReadStridedListIO(ADIO_File fd, void *buf, int count,
				  MPI_Datatype datatype, int file_ptr_type,
				  ADIO_Offset offset, ADIO_Status *status,
				  int *error_code)
{
    return ADIOI_PVFS2_StridedListIO(fd, buf, count,
				     datatype, file_ptr_type,
				     offset, status,
				     error_code, READ);
}

int ADIOI_PVFS2_ReadStridedDtypeIO(ADIO_File fd, void *buf, int count,
				   MPI_Datatype datatype, int file_ptr_type,
				   ADIO_Offset offset, ADIO_Status *status, 
				   int *error_code)
{
    return ADIOI_PVFS2_StridedDtypeIO(fd, buf, count,
				      datatype, file_ptr_type,
				      offset, status, error_code,
				      READ);
}

void ADIOI_PVFS2_ReadStrided(ADIO_File fd, void *buf, int count,
			     MPI_Datatype datatype, int file_ptr_type,
			     ADIO_Offset offset, ADIO_Status *status, 
			     int *error_code)
{
    int ret = -1;
    if ( fd->hints->fs_hints.pvfs2.posix_read == ADIOI_HINT_ENABLE) {
	ADIOI_GEN_ReadStrided(fd, buf, count,
			      datatype, file_ptr_type,
			      offset, status, error_code);
	return;
    }
    if ( fd->hints->fs_hints.pvfs2.dtype_read == ADIOI_HINT_ENABLE) {
	ret = ADIOI_PVFS2_ReadStridedDtypeIO(fd, buf, count, 
					     datatype, file_ptr_type,
					     offset, status, error_code);
	
	/* Fall back to list I/O if datatype I/O didn't work */
	if (ret != 0)
	{
	    fprintf(stderr, 
		    "Falling back to list I/O since datatype I/O failed\n");
	    ret = ADIOI_PVFS2_ReadStridedListIO(fd, buf, count, 
						datatype, file_ptr_type,
						offset, status, error_code);
	}
	return;
    }
    /* Use list I/O in the base case */
    ret = ADIOI_PVFS2_ReadStridedListIO(fd, buf, count, 
					datatype, file_ptr_type,
					offset, status, error_code);
    return;
}

/*
 * vim: ts=8 sts=4 sw=4 noexpandtab 
 */
