/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

/* contains definitions, declarations, and macros specific to the
   implementation of ADIO on PIOFS */

#ifndef AD_PIOFS_INCLUDE
#define AD_PIOFS_INCLUDE

#include <unistd.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <piofs/piofs_ioctl.h>
#include "adio.h"

void ADIOI_PIOFS_Open(ADIO_File fd, int *error_code);
void ADIOI_PIOFS_ReadContig(ADIO_File fd, void *buf, int count, 
                      MPI_Datatype datatype, int file_ptr_type,
                     ADIO_Offset offset, ADIO_Status *status, int
		     *error_code);
void ADIOI_PIOFS_WriteContig(ADIO_File fd, void *buf, int count, 
                      MPI_Datatype datatype, int file_ptr_type,
                      ADIO_Offset offset, ADIO_Status *status, int
		      *error_code);   
void ADIOI_PIOFS_IwriteContig(ADIO_File fd, void *buf, int count, 
                      MPI_Datatype datatype, int file_ptr_type,
                      ADIO_Offset offset, ADIO_Request *request, int
		      *error_code);   
void ADIOI_PIOFS_IreadContig(ADIO_File fd, void *buf, int count, 
                      MPI_Datatype datatype, int file_ptr_type,
                      ADIO_Offset offset, ADIO_Request *request, int
		      *error_code);   
int ADIOI_PIOFS_ReadDone(ADIO_Request *request, ADIO_Status *status, int
		       *error_code);
int ADIOI_PIOFS_WriteDone(ADIO_Request *request, ADIO_Status *status, int
		       *error_code);
void ADIOI_PIOFS_ReadComplete(ADIO_Request *request, ADIO_Status *status, int
		       *error_code); 
void ADIOI_PIOFS_WriteComplete(ADIO_Request *request, ADIO_Status *status,
			int *error_code); 
void ADIOI_PIOFS_Fcntl(ADIO_File fd, int flag, ADIO_Fcntl_t *fcntl_struct, int
		*error_code); 
void ADIOI_PIOFS_WriteStrided(ADIO_File fd, void *buf, int count,
		       MPI_Datatype datatype, int file_ptr_type,
		       ADIO_Offset offset, ADIO_Status *status, int
		       *error_code);
void ADIOI_PIOFS_ReadStrided(ADIO_File fd, void *buf, int count,
		       MPI_Datatype datatype, int file_ptr_type,
		       ADIO_Offset offset, ADIO_Status *status, int
		       *error_code);
void ADIOI_PIOFS_WriteStridedColl(ADIO_File fd, void *buf, int count,
		       MPI_Datatype datatype, int file_ptr_type,
		       ADIO_Offset offset, ADIO_Status *status, int
		       *error_code);
void ADIOI_PIOFS_ReadStridedColl(ADIO_File fd, void *buf, int count,
		       MPI_Datatype datatype, int file_ptr_type,
		       ADIO_Offset offset, ADIO_Status *status, int
		       *error_code);
void ADIOI_PIOFS_IreadStrided(ADIO_File fd, void *buf, int count,
		       MPI_Datatype datatype, int file_ptr_type,
		       ADIO_Offset offset, ADIO_Request *request, int
		       *error_code);
void ADIOI_PIOFS_IwriteStrided(ADIO_File fd, void *buf, int count,
		       MPI_Datatype datatype, int file_ptr_type,
		       ADIO_Offset offset, ADIO_Request *request, int
		       *error_code);
ADIO_Offset ADIOI_PIOFS_SeekIndividual(ADIO_File fd, ADIO_Offset offset, 
                       int whence, int *error_code);
void ADIOI_PIOFS_SetInfo(ADIO_File fd, MPI_Info users_info, int *error_code);

#endif
