/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_pvfs2.h"
#include "ad_pvfs2_common.h"

void ADIOI_PVFS2_Resize(ADIO_File fd, ADIO_Offset size, int *error_code)
{
    int ret;
    ADIOI_PVFS2_fs *pvfs_fs;

    pvfs_fs = (ADIOI_PVFS2_fs*)fd->fs_ptr;

    ret = PVFS_sys_truncate(pvfs_fs->pinode_refn, size, pvfs_fs->credentials);
    if (ret < 0 ) {
	ADIOI_PVFS2_pvfs_error_convert(ret, error_code);
    } else {
	*error_code = MPI_SUCCESS;
    }
}

/*
 * vim: ts=8 sts=4 sw=4 noexpandtab 
 */
