/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_pvfs2.h"
#include "ad_pvfs2_common.h"

/* we want to be a bit clever here:  at scale, every client sending a flush
 * will stress the server unnecessarily.  One process should wait for everyone
 * to catch up, do the sync, then broadcast the result.
 */
void ADIOI_PVFS2_Flush(ADIO_File fd, int *error_code)
{
    int ret, dummy1, dummy2;
    ADIOI_PVFS2_fs *pvfs_fs;

    *error_code = MPI_SUCCESS;

    pvfs_fs = (ADIOI_PVFS2_fs*)fd->fs_ptr;

    /* the cheapest way we know to let one process know everyone is here */
    MPI_Gather(&dummy1, 1, MPI_INT, &dummy2, 1, MPI_INT, 0, fd->comm);

    /* io_worker computed in ADIO_Open */
    if (fd->io_worker) {
	ret = PVFS_sys_flush(pvfs_fs->pinode_refn, pvfs_fs->credentials);
	MPI_Bcast(&ret, 1, MPI_INT, 0, fd->comm);
    } else {
	MPI_Bcast(&ret, 1, MPI_INT, 0, fd->comm);
    }
    if (ret < 0)
	ADIOI_PVFS2_pvfs_error_convert(ret, error_code);
}

/* 
 * vim: ts=8 sts=4 sw=4 noexpandtab 
 */
