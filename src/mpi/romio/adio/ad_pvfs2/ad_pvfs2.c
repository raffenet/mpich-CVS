/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$
 *
 *   Copyright (C) 2003 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_pvfs2.h"

/* adioi.h has the ADIOI_Fns_struct define */
#include "adioi.h"

struct ADIOI_Fns_struct ADIO_PVFS2_operations = {
    ADIOI_PVFS2_Open, /* Open */
    ADIOI_PVFS2_ReadContig, /* ReadContig */
    ADIOI_PVFS2_WriteContig, /* WriteContig */
    ADIOI_GEN_ReadStridedColl, /* ReadStridedColl */
    ADIOI_GEN_WriteStridedColl, /* WriteStridedColl */
    ADIOI_GEN_SeekIndividual, /* SeekIndividual */
    ADIOI_PVFS2_Fcntl, /* Fcntl */
    ADIOI_PVFS2_SetInfo, /* SetInfo */
    ADIOI_PVFS2_ReadStrided, /* ReadStrided */
    ADIOI_PVFS2_WriteStrided, /* WriteStrided */
    ADIOI_PVFS2_Close, /* Close */
    ADIOI_GEN_IreadContig, /* IreadContig */
    ADIOI_GEN_IwriteContig, /* IwriteContig */
    ADIOI_PVFS2_ReadDone, /* ReadDone */
    ADIOI_PVFS2_WriteDone, /* WriteDone */
    ADIOI_PVFS2_ReadComplete, /* ReadComplete */
    ADIOI_PVFS2_WriteComplete, /* WriteComplete */
    ADIOI_GEN_IreadStrided, /* IreadStrided */
    ADIOI_GEN_IwriteStrided, /* IwriteStrided */
    ADIOI_PVFS2_Flush, /* Flush */
    ADIOI_PVFS2_Resize, /* Resize */
    ADIOI_PVFS2_Delete, /* Delete */
};

/* 
 * vim: ts=8 sts=4 sw=4 noexpandtab 
 */
