/* -*- Mode: C; c-basic-offset:4 ; -*-
 * vim: ts=8 sts=4 sw=4 noexpandtab
 *   $Id$
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_pvfs2.h"
#include "ad_pvfs2_common.h"

struct open_status {
    int error;
    PVFS_pinode_reference pinode_refn;
};
    

/* if MPI_File_open was called with MPI_MODE_CREATE|MPI_MODE_EXCL, then we have
 * a little problem: our usua open-and-broadcast test will not work because
 * only one person (the first aggregator) will perform the open w/ CREATE|EXCL
 */
void ADIOI_PVFS2_Open(ADIO_File fd, int *error_code)
{
    int ret, rank;
    ADIOI_PVFS2_fs *pvfs2_fs;
    PVFS_object_attr attribs;

    PVFS_sysresp_lookup resp_lookup;
    PVFS_sysresp_create resp_create;
    PVFS_sysresp_getparent resp_getparent;

    /* since one process is doing the open, that means one process is also
     * doing the error checking.  define a struct for both the pinode and the
     * error code to broadcast to all the processors */

    struct open_status o_status;
    MPI_Datatype open_status_type;
    MPI_Datatype types[2] = {MPI_INT, MPI_BYTE};
    int lens[2] = {1, sizeof(PVFS_pinode_reference)};
    MPI_Aint offsets[2];
    
    pvfs2_fs = (ADIOI_PVFS2_fs *)ADIOI_Malloc(sizeof(ADIOI_PVFS2_fs));
    if (pvfs2_fs == NULL) {
	/* graceful way to handle out of memory? */
	ADIOI_PVFS2_pvfs_error_convert(0, error_code);
	return;
    }

    MPI_Comm_rank(fd->comm, &rank);

    MPI_Address(&o_status.error, &offsets[0]);
    MPI_Address(&o_status.pinode_refn, &offsets[1]);

    MPI_Type_struct(2, lens, offsets, types, &open_status_type);
    MPI_Type_commit(&open_status_type);

    ADIOI_PVFS2_Init(error_code);
    if (*error_code != MPI_SUCCESS)
    {
	/* XXX: handle errors */
	return;
    }

    ADIOI_PVFS2_makeattribs(&attribs);
    ADIOI_PVFS2_makecredentials(&(pvfs2_fs->credentials));

    /* lookup the file.  if we get an error, it might not exist. in that case,
     * create the file if we have MPI_MODE_CREATE */

    /* we only have to do this on one node. we'll broadcast the handle to
     * everyone else */
    if (rank == fd->hints->ranklist[0]) {
	ret = PVFS_sys_lookup(ADIOI_PVFS2_fs_id_list[0], fd->filename, 
		pvfs2_fs->credentials, &resp_lookup);
	if ( (ret < 0) ) {
	    if (fd->access_mode & MPI_MODE_CREATE)  {
		ret = PVFS_sys_getparent(ADIOI_PVFS2_fs_id_list[0], 
			fd->filename, pvfs2_fs->credentials, &resp_getparent);
		if (ret < 0) {
		    fprintf(stderr, "pvfs_sys_getparent returns with %d\n", ret);
		    o_status.error = ret;
		    goto error_getparent;
		} 
		ret = PVFS_sys_create(resp_getparent.basename, 
		    resp_getparent.parent_refn, attribs, 
		    pvfs2_fs->credentials, &resp_create); 
		if (ret < 0) {
		    fprintf(stderr, "pvfs_sys_create returns with %d\n", ret);
		    o_status.error = ret;
		    goto error_create;
		}
		o_status.pinode_refn = resp_create.pinode_refn;
	    } else {
		fprintf(stderr, "cannot create file without MPI_MODE_CREATE\n");
		o_status.error = ret;
		goto error_modes;
	    }
	} else {
	    o_status.pinode_refn = resp_lookup.pinode_refn;
	}
	o_status.error = ret;
    }
    /* fall through into the error codes: if things went well, we'll have a
     * real pinode reference */
    /* NOTE: if MPI_MODE_EXCL was set, ADIO_Open will call
     * ADIOI_PVFS2_Open from just one processor.  Since ADIO_Open will call
     * ADIOI_PVFS2_Open again (but w/o EXCL), we can bail out right here and
     * return early */
    if ( (fd->access_mode & MPI_MODE_EXCL)  ) {
	*error_code = MPI_SUCCESS;
	MPI_Type_free(&open_status_type);
	fd->fs_ptr = pvfs2_fs;
	return;
    } 
    /* broadcast status and (if successful) valid pinode refn */
    MPI_Bcast(MPI_BOTTOM, 1, open_status_type, 0, fd->comm);
    if (o_status.error != 0 ) {
	goto error_status;
    } else {
	pvfs2_fs->pinode_refn = o_status.pinode_refn;
	*error_code = MPI_SUCCESS;
    }
    MPI_Type_free(&open_status_type);
    fd->fs_ptr = pvfs2_fs;
    return;

error_status:
error_modes:
error_create:
error_getparent:
    ADIOI_PVFS2_pvfs_error_convert(o_status.error, error_code);
    return;
}
