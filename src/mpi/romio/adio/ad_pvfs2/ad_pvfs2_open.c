/* -*- Mode: C; c-basic-offset:4 ; -*-
 * vim: ts=8 sts=4 sw=4 noexpandtab
 *   $Id$
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_pvfs2.h"
#include "ad_pvfs2_common.h"

void ADIOI_PVFS2_Open(ADIO_File fd, int *error_code)
{
    int ret, rank;
    ADIOI_PVFS2_fs *pvfs2_fs;
    PVFS_object_attr attribs;
    MPI_Datatype pinode_type;

    PVFS_sysresp_lookup resp_lookup;
    PVFS_sysresp_create resp_create;
    PVFS_sysresp_getparent resp_getparent;

    MPI_Comm_rank(fd->comm, &rank);

    MPI_Type_contiguous(sizeof(PVFS_pinode_reference), MPI_BYTE, &pinode_type);
    MPI_Type_commit(&pinode_type);

    ADIOI_PVFS2_Init(error_code);

    pvfs2_fs = (ADIOI_PVFS2_fs *)ADIOI_Malloc(sizeof(ADIOI_PVFS2_fs));

    /* lookup the file.  if we get an error, it might not exist. in that case,
     * create the file */

    ADIOI_PVFS2_makeattribs(&attribs);
    ADIOI_PVFS2_makecredentials(&(pvfs2_fs->credentials));

    /* we only have to do this on one node. we'll pick rank 0 for now, but this
     * has to change for aggregation */
    if (rank == 0 ) {

	ret = PVFS_sys_lookup(ADIOI_PVFS2_fs_id_list[0], fd->filename, 
		pvfs2_fs->credentials, &resp_lookup);
	if (ret < 0 && (fd->access_mode & MPI_MODE_CREATE) ) {
	    ret = PVFS_sys_getparent(ADIOI_PVFS2_fs_id_list[0], fd->filename,
		    pvfs2_fs->credentials, &resp_getparent);
	    if (ret < 0) {
		fprintf(stderr, "pvfs_sys_getparent returns with %d\n", ret);
		goto error_getparent;
	    }

	    ret = PVFS_sys_create(resp_getparent.basename, 
		    resp_getparent.parent_refn, attribs, 
		    pvfs2_fs->credentials, &resp_create);
	    if (ret < 0) {
		fprintf(stderr, "pvfs_sys_create returns with %d\n", ret);
		goto error_create;
	    } 
	    memcpy(&(pvfs2_fs->pinode), &(resp_create.pinode_refn), 
		    sizeof(PVFS_pinode_reference));
	} else {
	    memcpy(&(pvfs2_fs->pinode), &(resp_lookup.pinode_refn), 
		    sizeof(PVFS_pinode_reference));
	}
	/* define a pvfs_pinode_reference datatype and broadcast the pinode
	 * reference: need to learn how to create a 64-bit datatype */
	MPI_Bcast(&(pvfs2_fs->pinode), 1, pinode_type, 0, fd->comm);
    }
    /* recieve the broadcast */
    MPI_Bcast(&(pvfs2_fs->pinode), 1, pinode_type, 0, fd->comm);

    MPI_Type_free(&pinode_type);

    fd->fs_ptr = pvfs2_fs;
    *error_code = MPI_SUCCESS;
    return;

error_getparent:
error_create:
    MPI_Type_free(&pinode_type);
    ADIOI_Free(pvfs2_fs);
    *error_code = MPI_UNDEFINED;
    return;
}
