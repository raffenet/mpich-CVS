/* -*- Mode: C; c-basic-offset:4 ; -*-
 * vim: ts=8 sts=4 sw=4 noexpandtab
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#ifndef _AD_PVFS2_h
#define _AD_PVFS2_H
#include "ad_pvfs2.h"

#define ADIOI_PVFS2_DEBUG_MASK CLIENT_DEBUG


struct ADIOI_PVFS2_fs_s {
    PVFS_pinode_reference pinode_refn;
    PVFS_credentials credentials;
} ADIOI_PVFS2_fs_s;

typedef struct ADIOI_PVFS2_fs_s ADIOI_PVFS2_fs;

extern PVFS_fs_id * ADIOI_PVFS2_fs_id_list;

void ADIOI_PVFS2_Init(int *error_code );
void ADIOI_PVFS2_makeattribs(PVFS_sys_attr * attribs);
void ADIOI_PVFS2_makecredentials(PVFS_credentials * credentials);
void ADIOI_PVFS2_End(int *error_code);
int ADIOI_PVFS2_End_call(MPI_Comm comm, int keyval, 
	void *attribute_val, void *extra_state);
int ADIOI_PVFS2_pvfs_error_convert(int pvfs_error, int *mpi_error);

#endif
