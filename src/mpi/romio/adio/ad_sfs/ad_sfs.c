/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$
 *
 *   Copyright (C) 2001 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_sfs.h"

/* adioi.h has the ADIOI_Fns_struct define */
#include "adioi.h"

struct ADIOI_Fns_struct ADIO_SFS_operations = {
    ADIOI_SFS_Open, /* Open */
    ADIOI_GEN_ReadContig, /* ReadContig */
    ADIOI_GEN_WriteContig, /* WriteContig */
    ADIOI_GEN_ReadStridedColl, /* ReadStridedColl */
    ADIOI_GEN_WriteStridedColl, /* WriteStridedColl */
    ADIOI_GEN_SeekIndividual, /* SeekIndividual */
    ADIOI_SFS_Fcntl, /* Fcntl */
    ADIOI_GEN_SetInfo, /* SetInfo */
    ADIOI_GEN_ReadStrided, /* ReadStrided */
    ADIOI_GEN_WriteStrided, /* WriteStrided */
    ADIOI_GEN_Close, /* Close */
    ADIOI_SFS_IreadContig, /* IreadContig */
    ADIOI_SFS_IwriteContig, /* IwriteContig */
    ADIOI_SFS_ReadDone, /* ReadDone */
    ADIOI_SFS_WriteDone, /* WriteDone */
    ADIOI_SFS_ReadComplete, /* ReadComplete */
    ADIOI_SFS_WriteComplete, /* WriteComplete */
    ADIOI_GEN_IreadStrided, /* IreadStrided */
    ADIOI_GEN_IwriteStrided, /* IwriteStrided */
    ADIOI_SFS_Flush, /* Flush */
    ADIOI_GEN_Resize, /* Resize */
    ADIOI_GEN_Delete, /* Delete */
};
