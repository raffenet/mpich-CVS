/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *
 *   Copyright (C) 2007 UChicago/Argonne LLC
 *   See COPYRIGHT notice in top-level directory.
 */

#include "adio.h"

/* 
 * Scalable open:  for file systems capable of having one process
 * create/open a file and broadcast the result to everyone else. 
 * - Does not need one process to create the file
 * - Does not need special handling for CREATE|EXCL
 */
void ADIOI_SCALABLE_OpenColl(ADIO_File fd, int rank, 
		int access_mode, int *error_code)
{    
    int orig_amode_wronly, orig_amode_excl;
    int max_error_code;
    static char myname[] = "ADIOI_SCALABLE_OpenColl";

    /* if we are doing deferred open, non-aggregators should return now */
    if (fd->hints->deferred_open ) {
        if (fd->agg_comm == MPI_COMM_NULL) {
            *error_code = MPI_SUCCESS;
            goto fn_exit;
        }
    } 
    
    /* For writing with data sieving, a read-modify-write is needed. If 
       the file is opened for write_only, the read will fail. Therefore,
       if write_only, open the file as read_write, but record it as
       write_only in fd, so that get_amode returns the right answer. */

    orig_amode_wronly = access_mode;
    if (access_mode & ADIO_WRONLY) {
	access_mode = access_mode ^ ADIO_WRONLY;
	access_mode = access_mode | ADIO_RDWR;
    }
    fd->access_mode = access_mode;

    (*(fd->fns->ADIOI_xxx_Open))(fd, error_code);

    /* if error, may be it was due to the change in amode above. 
       therefore, reopen with access mode provided by the user.*/ 
    fd->access_mode = orig_amode_wronly;  
    if (*error_code != MPI_SUCCESS) 
        (*(fd->fns->ADIOI_xxx_Open))(fd, error_code);

    /* for deferred open: this process has opened the file (because if we are
     * not an aggregaor and we are doing deferred open, we returned earlier)*/
    fd->is_open = 1;

 fn_exit:
    MPI_Allreduce(error_code, &max_error_code, 1, MPI_INT, MPI_MAX, fd->comm);
    if (max_error_code != MPI_SUCCESS) {

        /* If the file was successfully opened, close it */
        if (*error_code == MPI_SUCCESS) {
        
            /* in the deferred open case, only those who have actually
               opened the file should close it */
            if (fd->hints->deferred_open)  {
                if (fd->agg_comm != MPI_COMM_NULL) {
                    (*(fd->fns->ADIOI_xxx_Close))(fd, error_code);
                }
            }
            else {
                (*(fd->fns->ADIOI_xxx_Close))(fd, error_code);
            }
        }
	if (fd->filename) ADIOI_Free(fd->filename);
	if (fd->hints->ranklist) ADIOI_Free(fd->hints->ranklist);
	if (fd->hints->cb_config_list) ADIOI_Free(fd->hints->cb_config_list);
	if (fd->hints) ADIOI_Free(fd->hints);
	if (fd->info != MPI_INFO_NULL) MPI_Info_free(&(fd->info));
	ADIOI_Free(fd);
        fd = ADIO_FILE_NULL;
	if (*error_code == MPI_SUCCESS)
	{
	    *error_code = MPIO_Err_create_code(MPI_SUCCESS,
					       MPIR_ERR_RECOVERABLE, myname,
					       __LINE__, MPI_ERR_IO,
					       "**oremote_fail", 0);
	}
    }
}

/* 
 * vim: ts=8 sts=4 sw=4 noexpandtab 
 */