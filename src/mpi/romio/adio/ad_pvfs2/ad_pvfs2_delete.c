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
    int ret;

    ADIOI_PVFS2_Init(error_code);
    if (*error_code != MPI_SUCCESS) 
    {
	/* XXX: handle errors */
	return;
    }

    /* in most cases we'll store the credentials in the fs struct, but we don't
     * have one of those in Delete  */
    ADIOI_PVFS2_makecredentials(&credentials);

    /* XXX: I don't like that i have to arbitrarily pick the first fs_id, but
     * them's the breaks */
    ret = PVFS_sys_getparent(ADIOI_PVFS2_fs_id_list[0], filename,
	    credentials, &resp_getparent);

    ret = PVFS_sys_remove(resp_getparent.basename, 
	    resp_getparent.parent_refn, credentials);
    if (ret < 0) {
	/* XXX: better error handling */
	fprintf(stderr, "remove of %s failed with error %s\n", 
		filename, strerror(ret));
	*error_code = MPI_UNDEFINED;
	return;
    }
    *error_code = MPI_SUCCESS;
}

/* 
 * vim: ts=8 sts=4 sw=4 noexpandtab 
 */
