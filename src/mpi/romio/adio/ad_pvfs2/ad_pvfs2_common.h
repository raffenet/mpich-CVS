/* -*- Mode: C; c-basic-offset:4 ; -*-
 * vim: ts=8 sts=4 sw=4 noexpandtab
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_pvfs2.h"

struct ADIOI_PVFS2_fs_s {
    PVFS_pinode_reference pinode;
    PVFS_credentials credentials;
} ADIOI_PVFS2_fs_s;

typedef struct ADIOI_PVFS2_fs_s *ADIOI_PVFS2_fs;

extern PVFS_fs_id * ADIOI_PVFS2_fs_id_list;

void ADIOI_PVFS2_Init(int *error_code );
void ADIOI_PVFS2_makecredentials(PVFS_credentials * credentials);
