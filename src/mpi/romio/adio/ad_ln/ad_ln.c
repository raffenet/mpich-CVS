/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$
 *
 *   Copyright (C) 2001 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_ln.h"

/* adioi.h has the ADIOI_Fns_struct define */
#include "adioi.h"

struct ADIOI_Fns_struct ADIO_LN_operations = {
    ADIOI_LN_Open, /* Open */
    ADIOI_LN_ReadContig, /* ReadContig */
    ADIOI_LN_WriteContig, /* WriteContig */
    ADIOI_LN_ReadStridedColl, /* ReadStridedColl */
    ADIOI_LN_WriteStridedColl, /* WriteStridedColl */
    ADIOI_GEN_SeekIndividual, /* SeekIndividual */
    ADIOI_LN_Fcntl, /* Fcntl */
    ADIOI_LN_SetInfo, /* SetInfo */
    ADIOI_GEN_ReadStrided, /* ReadStrided */
    ADIOI_LN_WriteStrided, /* WriteStrided */
    ADIOI_LN_Close, /* Close */
    ADIOI_LN_IreadContig, /* IreadContig */
    ADIOI_LN_IwriteContig, /* IwriteContig */
    ADIOI_LN_ReadDone, /* ReadDone */
    ADIOI_LN_WriteDone, /* WriteDone */
    ADIOI_LN_ReadComplete, /* ReadComplete */
    ADIOI_LN_WriteComplete, /* WriteComplete */
    ADIOI_GEN_IreadStrided, /* IreadStrided */
    ADIOI_GEN_IwriteStrided, /* IwriteStrided */
    ADIOI_LN_Flush, /* Flush */
    ADIOI_LN_Resize, /* Resize */
    ADIOI_LN_Delete, /* Delete */
};
