/* -*- Mode: C; c-basic-offset:4 ; -*-
 * vim: ts=8 sts=4 sw=4 noexpandtab
 *   $Id$
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_pvfs2.h"

void ADIOI_PVFS2_Open(ADIO_File fd, int *error_code)
{
#if 0
    int ret;
    PVFS_credentials credentials;
    PVFS_sysresp_lookup resp_lookup;
    PVFS_sysresp_create resp_create;

    ADIOI_PVFS2_Init(error_code);

    /* lookup the file.  if we get an error, it might not exist. in that case,
     * create the file */

    ADIOI_PVFS2_makecredentials(&credentials);

    fd->fs_ptr = malloc(sizeof(PVFS_pinode_reference));

    ret = PVFS_sys_lookup(fs_id, fd->filename, credentials, &resp_lookup);
    if (ret < 0) {
	ret = PVFS_sys_create(fd->filename, /* parent */ /* attributes */ 
		resp_create);
	if (ret < 0) {
	    /* XXX: handle error */
	} else {
	    memcpy(fd->fs_ptr, resp_create.pinode_refn, 
		    sizeof(PVFS_pinode_reference));
	}
    } else {
	memcpy(fd->fs_ptr, resp_create.pinode_refn, 
		sizeof(PVFS_pinode_reference));
    }
#endif

    *error_code = MPI_SUCCESS;
}
