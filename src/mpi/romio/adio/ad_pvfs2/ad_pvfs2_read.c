/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "adio.h"
#include "adio_extern.h"
#include "ad_pvfs2.h"

#include "ad_pvfs2_common.h"

void ADIOI_PVFS2_ReadContig(ADIO_File fd, void *buf, int count, 
                     MPI_Datatype datatype, int file_ptr_type,
		     ADIO_Offset offset, ADIO_Status *status, int *error_code)
{
    int ret, datatype_size, len;
    PVFS_Request file_req, mem_req;
    PVFS_sysresp_io resp_io;
    ADIOI_PVFS2_fs *pvfs_fs;

    pvfs_fs = (ADIOI_PVFS2_fs*)fd->fs_ptr;

    MPI_Type_size(datatype, &datatype_size);
    len = datatype_size * count;

    ret = PVFS_Request_contiguous(len, PVFS_BYTE, &mem_req);
    if (ret < 0) {
	fprintf(stderr, "pvfs_request_contig returns with %d\n", ret);
	goto error_request;
    }

    ret = PVFS_Request_contiguous(len, PVFS_BYTE, &file_req);
    if (ret < 0) {
	fprintf(stderr, "pvfs_request_contig returns with %d\n", ret);
	goto error_request;
    }

    if (file_ptr_type == ADIO_EXPLICIT_OFFSET) {
	ret = PVFS_sys_read(pvfs_fs->object_ref, file_req, offset, buf, 
		mem_req, &(pvfs_fs->credentials), &resp_io);
	if (ret < 0 ) {
	    fprintf(stderr, "pvfs_sys_read returns with %d\n", ret);
	    goto error_read;
	}
	fd->fp_sys_posn = offset + (int)resp_io.total_completed;
    } else { 
	ret = PVFS_sys_read(pvfs_fs->object_ref, file_req, fd->fp_ind, buf, 
		mem_req, &(pvfs_fs->credentials), &resp_io);
	if (ret < 0) {
	    fprintf(stderr, "pvfs_sys_read returns with %d\n", ret);
	    goto error_read;
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
error_read:
    ADIOI_PVFS2_pvfs_error_convert(ret, error_code);
}


void ADIOI_PVFS2_ReadStrided(ADIO_File fd, void *buf, int count,
                       MPI_Datatype datatype, int file_ptr_type,
                       ADIO_Offset offset, ADIO_Status *status, int
                       *error_code)
{
    /* for now, defer to the generic operations. pvfs2 should have a lot of
     * ogood ways to express strided io, however... */
    ADIOI_GEN_ReadStrided(fd, buf, count, datatype, 
	    file_ptr_type, offset, status, error_code);
}

/*
 * vim: ts=8 sts=4 sw=4 noexpandtab 
 */
