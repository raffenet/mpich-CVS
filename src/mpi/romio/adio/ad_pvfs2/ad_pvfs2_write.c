/* -*- Mode: C; c-basic-offset:4 ; -*- 
 *  vim: ts=8 sts=4 sw=4 noexpandtab
 *
 *   $Id$
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_pvfs2.h"
#include "adio_extern.h"

void ADIOI_PVFS2_WriteContig(ADIO_File fd, void *buf, int count, 
                     MPI_Datatype datatype, int file_ptr_type,
		     ADIO_Offset offset, ADIO_Status *status, int *error_code)
{
	*error_code = MPI_SUCCESS;
}



void ADIOI_PVFS2_WriteStrided(ADIO_File fd, void *buf, int count,
                       MPI_Datatype datatype, int file_ptr_type,
                       ADIO_Offset offset, ADIO_Status *status, int
                       *error_code)
{
	*error_code = MPI_SUCCESS;
}
