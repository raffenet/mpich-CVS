/* -*- Mode: C; c-basic-offset:4 ; -*- 
 *  vim: ts=8 sts=4 sw=4 noexpandtab
 *
 *   $Id$
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_pvfs2.h"
#include "adio_extern.h"

#include "ad_pvfs2_common.h"

void ADIOI_PVFS2_WriteContig(ADIO_File fd, void *buf, int count, 
                     MPI_Datatype datatype, int file_ptr_type,
		     ADIO_Offset offset, ADIO_Status *status, int *error_code)
{
    int ret, datatype_size, len;
    PVFS_Request io_req;
    PVFS_sysresp_io resp_io;
    ADIOI_PVFS2_fs *pvfs_fs;

    pvfs_fs = (ADIOI_PVFS2_fs*)fd->fs_ptr;

    MPI_Type_size(datatype, &datatype_size);
    len = datatype_size * count;

    if (file_ptr_type == ADIO_EXPLICIT_OFFSET) {
	ret = PVFS_Request_hindexed(1, &len, &offset, PVFS_BYTE, &io_req);
	if (ret < 0 ) {
	    fprintf(stderr, "pvfs_request_hindexed returns with %d\n", ret);
	    goto error_request;
	}
	ret = PVFS_sys_write(pvfs_fs->pinode, io_req, buf, len, 
		pvfs_fs->credentials, &resp_io);
	if (ret < 0 ) {
	    fprintf(stderr, "pvfs_sys_write returns with %d\n", ret);
	    goto error_write;
	}
	fd->fp_sys_posn = offset + (int) resp_io.total_completed;
    } else {
	ret = PVFS_Request_hindexed(1, &len, &(fd->fp_ind), PVFS_BYTE, &io_req);
	if (ret < 0 ) {
	    fprintf(stderr, "pvfs_request_hindexed returns with %d\n", ret);
	    goto error_request;
	}
	ret = PVFS_sys_write( pvfs_fs->pinode, io_req, buf, len, 
	    pvfs_fs->credentials, &resp_io);
	if (ret < 0) {
	    fprintf(stderr, "pvfs_sys_write returns with %d\n", ret);
	    goto error_write;
	}
	fd->fp_ind += (int)resp_io.total_completed;
	fd->fp_sys_posn = fd->fp_ind;
    }
#ifdef HAVE_STATUS_SET_BYTES
    MPIR_Status_set_bytes(status, datatype, (int)resp_io.total_completed);
#endif
    *error_code = MPI_SUCCESS;
    return;

error_request:
error_write:
    *error_code = MPI_UNDEFINED;
}

void ADIOI_PVFS2_WriteStrided(ADIO_File fd, void *buf, int count,
                       MPI_Datatype datatype, int file_ptr_type,
                       ADIO_Offset offset, ADIO_Status *status, int
                       *error_code)
{
    /* for now, defer to the generic operations. pvfs2 should have a lot of
     * ogood ways to express strided io, however... */
    ADIOI_GEN_ReadStrided(fd, buf, count, datatype, 
	    file_ptr_type, offset, status, error_code);
}
