/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_pvfs2.h"
#include "ad_pvfs2_common.h"

/* we want to be a bit clever here:  at scale, if every client sends a
 * flush request, it will stress the PVFS2 servers with redundant
 * PVFS_sys_flush requests.  Instead, one process should wait for
 * everyone to catch up, do the sync, then broadcast the result.  We can
 * get away with this thanks to PVFS2's stateless design 
 */

void ADIOI_PVFS2_Flush(ADIO_File fd, int *error_code) 
{ 
    int ret, rank, dummy=0, dummy_in=0; 
    ADIOI_PVFS2_fs *pvfs_fs;

    *error_code = MPI_SUCCESS;

    pvfs_fs = (ADIOI_PVFS2_fs*)fd->fs_ptr;

    MPI_Comm_rank(fd->comm, &rank);


    /* unlike ADIOI_PVFS2_Resize, MPI_File_sync() does not perform any
     * syncronization */
    MPI_Reduce(&dummy_in, &dummy, 1, MPI_INT, MPI_SUM, 
	    fd->hints->ranklist[0], fd->comm);

    /* io_worker computed in ADIO_Open */
    if (rank == fd->hints->ranklist[0]) {
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
