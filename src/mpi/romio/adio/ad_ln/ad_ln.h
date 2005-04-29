/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$    
 *
 *   Copyright (C) 2001 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#ifndef AD_LN_INCLUDE
#define AD_LN_INCLUDE

#include <unistd.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <fcntl.h>
#include "adio.h"
#include "ad_ln_lnio.h"

void ADIOI_LN_Open(ADIO_File fd, int *error_code);
void ADIOI_LN_Close(ADIO_File fd, int *error_code);
void ADIOI_LN_ReadContig(ADIO_File fd, void *buf, int count, 
			     MPI_Datatype datatype, int file_ptr_type,
			     ADIO_Offset offset, ADIO_Status *status, int
			     *error_code);
void ADIOI_LN_WriteContig(ADIO_File fd, void *buf, int count, 
			      MPI_Datatype datatype, int file_ptr_type,
			      ADIO_Offset offset, ADIO_Status *status, int
			      *error_code);   
void ADIOI_LN_IwriteContig(ADIO_File fd, void *buf, int count, 
			       MPI_Datatype datatype, int file_ptr_type,
			       ADIO_Offset offset, ADIO_Request *request, int
			       *error_code);   
void ADIOI_LN_IreadContig(ADIO_File fd, void *buf, int count, 
			      MPI_Datatype datatype, int file_ptr_type,
			      ADIO_Offset offset, ADIO_Request *request, int
			      *error_code);   
int ADIOI_LN_ReadDone(ADIO_Request *request, ADIO_Status *status, int
			  *error_code);
int ADIOI_LN_WriteDone(ADIO_Request *request, ADIO_Status *status, int
			   *error_code);
void ADIOI_LN_ReadComplete(ADIO_Request *request, ADIO_Status *status, int
			       *error_code); 
void ADIOI_LN_WriteComplete(ADIO_Request *request, ADIO_Status *status,
				int *error_code); 
void ADIOI_LN_Fcntl(ADIO_File fd, int flag, ADIO_Fcntl_t *fcntl_struct, 
			int *error_code); 
void ADIOI_LN_WriteStrided(ADIO_File fd, void *buf, int count,
			       MPI_Datatype datatype, int file_ptr_type,
			       ADIO_Offset offset, ADIO_Status *status,
			       int *error_code);
void ADIOI_LN_ReadStrided(ADIO_File fd, void *buf, int count,
			      MPI_Datatype datatype, int file_ptr_type,
			      ADIO_Offset offset, ADIO_Status *status, int
			      *error_code);
void ADIOI_LN_WriteStridedColl(ADIO_File fd, void *buf, int count,
				   MPI_Datatype datatype, int file_ptr_type,
				   ADIO_Offset offset, ADIO_Status *status, int
				   *error_code);
void ADIOI_LN_ReadStridedColl(ADIO_File fd, void *buf, int count,
				  MPI_Datatype datatype, int file_ptr_type,
				  ADIO_Offset offset, ADIO_Status *status, int
				  *error_code);
void ADIOI_LN_IreadStrided(ADIO_File fd, void *buf, int count,
			       MPI_Datatype datatype, int file_ptr_type,
			       ADIO_Offset offset, ADIO_Request *request, int
			       *error_code);
void ADIOI_LN_IwriteStrided(ADIO_File fd, void *buf, int count,
				MPI_Datatype datatype, int file_ptr_type,
				ADIO_Offset offset, ADIO_Request *request, int
				*error_code);
void ADIOI_LN_Flush(ADIO_File fd, int *error_code);
void ADIOI_LN_Resize(ADIO_File fd, ADIO_Offset size, int *error_code);
ADIO_Offset ADIOI_LN_SeekIndividual(ADIO_File fd, ADIO_Offset offset, 
					int whence, int *error_code);
void ADIOI_LN_SetInfo(ADIO_File fd, MPI_Info users_info, int *error_code);
void ADIOI_LN_Get_shared_fp(ADIO_File fd, int size, 
				ADIO_Offset *shared_fp, 
				int *error_code);
void ADIOI_LN_Set_shared_fp(ADIO_File fd, ADIO_Offset offset, 
				int *error_code);
void ADIOI_LN_Delete(char *filename, int *error_code);

#endif
