/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_ntfs.h"

void ADIOI_NTFS_Open(ADIO_File fd, int *error_code)
{
    int err;
    int cmode, amode, attrib;
    static char myname[] = "ADIOI_NTFS_Open";

    amode = 0;
    cmode = OPEN_EXISTING;
    /*attrib = FILE_ATTRIBUTE_NORMAL;*/
    attrib = FILE_FLAG_OVERLAPPED;

    if (fd->access_mode & ADIO_CREATE)
    {
	cmode = OPEN_ALWAYS;
    }
    if (fd->access_mode & ADIO_EXCL)
    {
	cmode = CREATE_NEW;
    }

    if (fd->access_mode & ADIO_RDONLY)
    {
	amode = GENERIC_READ;
    }
    if (fd->access_mode & ADIO_WRONLY)
    {
	amode = GENERIC_WRITE;
    }
    if (fd->access_mode & ADIO_RDWR)
    {
	amode = GENERIC_READ | GENERIC_WRITE;
    }

    if (fd->access_mode & ADIO_DELETE_ON_CLOSE)
    {
	attrib = attrib | FILE_FLAG_DELETE_ON_CLOSE;
    }
    if (fd->access_mode & ADIO_SEQUENTIAL)
    {
	attrib = attrib | FILE_FLAG_SEQUENTIAL_SCAN;
    }
    else
    {
	attrib = attrib | FILE_FLAG_RANDOM_ACCESS;
    }

    fd->fd_sys = CreateFile(fd->filename, 
	amode,
	FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 
	NULL, 
	cmode, 
	attrib, 
	NULL);
        fd->fd_direct = -1;

    if ((fd->fd_sys != INVALID_HANDLE_VALUE) && (fd->access_mode & ADIO_APPEND))
    {
	fd->fp_ind = fd->fp_sys_posn = SetFilePointer(fd->fd_sys, 0, NULL, FILE_END);
    }

    /* --BEGIN ERROR HANDLING-- */
    if (fd->fd_sys == INVALID_HANDLE_VALUE)
    {
	err = GetLastError();
	*error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
					   myname, __LINE__, MPI_ERR_IO,
					   "**io",
					   "**io %s", ADIOI_NTFS_Strerror(err));
	return;
    }
    /* --END ERROR HANDLING-- */
    *error_code = MPI_SUCCESS;
}
