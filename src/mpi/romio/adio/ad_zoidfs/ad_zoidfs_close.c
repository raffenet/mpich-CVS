#include "ad_zoidfs.h"

void ADIOI_ZOIDFS_Close(ADIO_File fd, int *error_code)
{
    ADIOI_Free(fd->fs_ptr);

    *error_code = MPI_SUCCESS;
}

