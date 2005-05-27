/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$    
 *
 *   Copyright (C) 2001 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_ln.h"
#include "adioi.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif


void ADIOI_LN_ReadContig(ADIO_File fd, void *buf, int count, 
			 MPI_Datatype datatype, int file_ptr_type,
			 ADIO_Offset offset, ADIO_Status *status, int
			 *error_code)
{
   int err = -1, datatype_size, len;
   static char myname[] = "ADIOI_LN_READCONTIG";

    MPI_Type_size(datatype, &datatype_size);
    len = datatype_size * count;

    if (file_ptr_type == ADIO_INDIVIDUAL) {
	offset = fd->fp_ind;
    }

    if (fd->fp_sys_posn != offset) {
	err = ADIOI_LNIO_Lseek(fd, offset, SEEK_SET);
	/* --BEGIN ERROR HANDLING-- */
	if (err == -1) {
	    *error_code = MPIO_Err_create_code(MPI_SUCCESS,
					       MPIR_ERR_RECOVERABLE,
					       myname, __LINE__,
					       MPI_ERR_IO, "**io",
					       "**io %s", strerror(errno));
	    fd->fp_sys_posn = -1;
	    return;
	}
	/* --END ERROR HANDLING-- */
    }

    err = ADIOI_LNIO_Read(fd, buf, len);

    /* --BEGIN ERROR HANDLING-- */
    if (err == -1) {
	*error_code = MPIO_Err_create_code(MPI_SUCCESS,
					   MPIR_ERR_RECOVERABLE,
					   myname, __LINE__,
					   MPI_ERR_IO, "**io",
					   "**io %s", strerror(errno));
	fd->fp_sys_posn = -1;
	return;
    }
    /* --END ERROR HANDLING-- */
    
    fd->fp_sys_posn = offset + err;
    
    if (file_ptr_type == ADIO_INDIVIDUAL) {
	fd->fp_ind += err; 
    }


#ifdef HAVE_STATUS_SET_BYTES
    if (err != -1) MPIR_Status_set_bytes(status, datatype, err);
#endif

    *error_code = MPI_SUCCESS;
}


void ADIOI_LN_ReadStridedColl(ADIO_File fd, void *buf, int count,
			      MPI_Datatype datatype, int file_ptr_type,
			      ADIO_Offset offset, ADIO_Status *status, 
			      int *error_code)
{
    struct lnio_handle_t *handle = (struct lnio_handle_t *)fd->fs_ptr;
    
    /* sync exnodes first - there could have been an independent write */
    if (handle->sync_at_collective_io) ADIOI_LNIO_Flush(fd);
    
    ADIOI_GEN_ReadStridedColl(fd, buf, count, datatype, file_ptr_type,
			      offset, status, error_code); 
}

