/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$    
 *
 *   Copyright (C) 2003 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_pvfs2.h"
#include "adio.h"

#include "ad_pvfs2_common.h"

void ADIOI_PVFS2_Delete(char *filename, int *error_code)
{
    PVFS_credentials credentials;
    PVFS_sysresp_getparent resp_getparent;
    int ret, i, mnt_index;
    char pvfs_path[PVFS_NAME_MAX] = {0};

    ADIOI_PVFS2_Init(error_code);
    if (*error_code != MPI_SUCCESS) 
    {
	ADIOI_PVFS2_pvfs_error_convert(0, error_code);
	return;
    }

    /* in most cases we'll store the credentials in the fs struct, but we don't
     * have one of those in Delete  */
    ADIOI_PVFS2_makecredentials(&credentials);

    /* given the filename, figure out which pvfs filesystem it is on */
    for (i=0; i<ADIOI_PVFS2_mntlist.ptab_count; i++) {
	ret = PVFS_util_remove_dir_prefix(filename, 
		ADIOI_PVFS2_mntlist.ptab_array[i].mnt_dir, 
		pvfs_path, PVFS_NAME_MAX);
	if (ret == 0) {
	    mnt_index = i;
	    break;
	}
    }
    if (mnt_index == -1) {
	fprintf(stderr, "Error: could not find filesystem for %s in pvfstab",
		filename);
	/* TODO: pick a good error for this */
	ret = -1;
	goto resolve_error;
    }

    ret = PVFS_sys_getparent(ADIOI_PVFS2_fs_id_list[mnt_index], pvfs_path,
	    credentials, &resp_getparent);

    ret = PVFS_sys_remove(resp_getparent.basename, 
	    resp_getparent.parent_refn, credentials);
    if (ret < 0) {
	/* XXX: better error handling */
	ADIOI_PVFS2_pvfs_error_convert(ret, error_code);
	return;
    }
    *error_code = MPI_SUCCESS;
    return;

resolve_error:
    ADIOI_PVFS2_pvfs_error_convert(ret, error_code);
}

/* 
 * vim: ts=8 sts=4 sw=4 noexpandtab 
 */
