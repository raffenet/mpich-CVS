/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#ifndef AD_PVFS_INCLUDE
#define AD_PVFS_INCLUDE

#ifndef ROMIOCONF_H_INCLUDED
#include "romioconf.h"
#define ROMIOCONF_H_INCLUDED
#endif
#ifdef ROMIO_PVFS_NEEDS_INT64_DEFINITION
typedef long long int int64_t;
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <fcntl.h>
#ifdef HAVE_PVFS_H
#include <pvfs.h>
#endif
#include "adio.h"

void ADIOI_PVFS_Open(ADIO_File fd, int *error_code);
void ADIOI_PVFS_Close(ADIO_File fd, int *error_code);
void ADIOI_PVFS_ReadContig(ADIO_File fd, void *buf, int count, 
                      MPI_Datatype datatype, int file_ptr_type,
                     ADIO_Offset offset, ADIO_Status *status, int
		     *error_code);
void ADIOI_PVFS_WriteContig(ADIO_File fd, void *buf, int count, 
                      MPI_Datatype datatype, int file_ptr_type,
                      ADIO_Offset offset, ADIO_Status *status, int
		      *error_code);   
int ADIOI_PVFS_ReadDone(ADIO_Request *request, ADIO_Status *status, int
		       *error_code);
int ADIOI_PVFS_WriteDone(ADIO_Request *request, ADIO_Status *status, int
		       *error_code);
void ADIOI_PVFS_ReadComplete(ADIO_Request *request, ADIO_Status *status, int
		       *error_code); 
void ADIOI_PVFS_WriteComplete(ADIO_Request *request, ADIO_Status *status,
			int *error_code); 
void ADIOI_PVFS_Fcntl(ADIO_File fd, int flag, ADIO_Fcntl_t *fcntl_struct, int
		*error_code); 
void ADIOI_PVFS_WriteStrided(ADIO_File fd, void *buf, int count,
		       MPI_Datatype datatype, int file_ptr_type,
		       ADIO_Offset offset, ADIO_Status *status, int
		       *error_code);
void ADIOI_PVFS_ReadStrided(ADIO_File fd, void *buf, int count,
		       MPI_Datatype datatype, int file_ptr_type,
		       ADIO_Offset offset, ADIO_Status *status, int
		       *error_code);
void ADIOI_PVFS_Flush(ADIO_File fd, int *error_code);
void ADIOI_PVFS_Delete(char *filename, int *error_code);
void ADIOI_PVFS_Resize(ADIO_File fd, ADIO_Offset size, int *error_code);
ADIO_Offset ADIOI_PVFS_SeekIndividual(ADIO_File fd, ADIO_Offset offset, 
                       int whence, int *error_code);
void ADIOI_PVFS_SetInfo(ADIO_File fd, MPI_Info users_info, int *error_code);


#endif
