/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$
 *
 *   Copyright (C) 2001 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_piofs.h"

/* adioi.h has the ADIOI_Fns_struct define */
#include "adioi.h"

struct ADIOI_Fns_struct ADIO_PIOFS_operations = {
    ADIOI_PIOFS_Open, /* Open */
    ADIOI_PIOFS_ReadContig, /* ReadContig */
    ADIOI_PIOFS_WriteContig, /* WriteContig */
    ADIOI_GEN_ReadStridedColl, /* ReadStridedColl */
    ADIOI_GEN_WriteStridedColl, /* WriteStridedColl */
    ADIOI_PIOFS_SeekIndividual, /* SeekIndividual */
    ADIOI_PIOFS_Fcntl, /* Fcntl */
    ADIOI_PIOFS_SetInfo, /* SetInfo */
    ADIOI_GEN_ReadStrided, /* ReadStrided */
    ADIOI_PIOFS_WriteStrided, /* WriteStrided */
    ADIOI_GEN_Close, /* Close */
    ADIOI_GEN_IreadContig, /* IreadContig */
    ADIOI_GEN_IwriteContig, /* IwriteContig */
    ADIOI_PIOFS_ReadDone, /* ReadDone */
    ADIOI_PIOFS_WriteDone, /* WriteDone */
    ADIOI_PIOFS_ReadComplete, /* ReadComplete */
    ADIOI_PIOFS_WriteComplete, /* WriteComplete */
    ADIOI_GEN_IreadStrided, /* IreadStrided */
    ADIOI_GEN_IwriteStrided, /* IwriteStrided */
    ADIOI_GEN_Flush, /* Flush */
    ADIOI_GEN_Resize, /* Resize */
    ADIOI_GEN_Delete, /* Delete */
};
