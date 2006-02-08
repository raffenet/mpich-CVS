#include "ad_zoidfs.h"

/* adioi.h has the ADIOI_Fns_struct define */
#include "adioi.h"

struct ADIOI_Fns_struct ADIO_ZOIDFS_operations = {
    ADIOI_ZOIDFS_Open, /* Open */
    ADIOI_ZOIDFS_ReadContig, /* ReadContig */
    ADIOI_ZOIDFS_WriteContig, /* WriteContig */
    ADIOI_GEN_ReadStridedColl, /* ReadStridedColl */
    ADIOI_GEN_WriteStridedColl, /* WriteStridedColl */
    ADIOI_GEN_SeekIndividual, /* SeekIndividual */
    ADIOI_ZOIDFS_Fcntl, /* Fcntl */
    ADIOI_GEN_SetInfo, /* SetInfo */
    ADIOI_ZOIDFS_ReadStrided, /* ReadStrided */
    ADIOI_ZODIFS_WriteStrided, /* WriteStrided */
    ADIOI_ZOIDFS_Close, /* Close */
    ADIOI_GEN_IreadContig, /* IreadContig */
    ADIOI_GEN_IwriteContig, /* IwriteContig */
    ADIOI_GEN_IODone, /* ReadDone */
    ADIOI_GEN_IODone, /* WriteDone */
    ADIOI_GEN_IOComplete, /* ReadComplete */
    ADIOI_GEN_IOComplete, /* WriteComplete */
    ADIOI_GEN_IreadStrided, /* IreadStrided */
    ADIOI_GEN_IwriteStrided, /* IwriteStrided */
    ADIOI_ZOIDFS_Flush, /* Flush */
    ADIOI_ZOIDFS_Resize, /* Resize */
    ADIOI_ZOIDFS_Delete, /* Delete */
};

/* 
 * vim: ts=8 sts=4 sw=4 noexpandtab 
 */
