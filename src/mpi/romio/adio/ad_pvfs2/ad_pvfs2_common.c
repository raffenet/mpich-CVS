/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$
 *
 *   Copyright (C) 2003 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_pvfs2.h"
#include <unistd.h>
#include <sys/types.h>

/* maybe give romio access to the globalconfig struct */
int ADIOI_PVFS2_Initialized = MPI_KEYVAL_INVALID;
PVFS_fs_id * ADIOI_PVFS2_fs_id_list;

void ADIOI_PVFS2_Init(int *error_code )
{
	pvfs_mntlist mnt = {0,NULL};
	PVFS_sysresp_init resp_init;
	int ret;

	/* do nothing if we've already fired up the pvfs2 interface */
	if (ADIOI_PVFS2_Initialized != MPI_KEYVAL_INVALID) {
		*error_code = MPI_SUCCESS;
		return;
	}

	ret = parse_pvfstab(NULL, &mnt);
	if (ret < 0) {
	    /* XXX: better error handling */
	    fprintf(stderr, "error parsing pvfstab\n");
	    *error_code = MPI_UNDEFINED;
	    return;
	}
	ret = PVFS_sys_initialize(mnt, &resp_init);
	if (ret < 0 ) {
	    /* XXX: better error handling */
	    fprintf(stderr, "error initializing pvfs\n");
	    *error_code = MPI_UNDEFINED;
	    return;
	}
	ADIOI_PVFS2_fs_id_list = resp_init.fsid_list;
	MPI_Keyval_create(MPI_NULL_COPY_FN, ADIOI_End_call,
		&ADIOI_PVFS2_Initialized, (void *)0); 
}

void ADIOI_PVFS2_makeattribs(PVFS_object_attr * attribs)
{
    attribs->owner = geteuid();
    attribs->group = getegid();
    attribs->perms = 1877;
    attribs->mask =  (PVFS_ATTR_SYS_UID | PVFS_ATTR_SYS_GID | 
	    PVFS_ATTR_SYS_PERM);
}


void ADIOI_PVFS2_makecredentials(PVFS_credentials * credentials)
{
    credentials->uid = geteuid();
    credentials->gid = getegid();
    /* XXX: are there any good default credentials? */
    credentials->perms = PVFS_U_WRITE|PVFS_U_READ;
}

/* 
 * vim: ts=8 sts=4 sw=4 noexpandtab 
 */
