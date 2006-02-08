#include "ad_zoidfs.h"
#include "adio.h"

void ADIOI_ZOIDFS_Delete(char *filename, int *error_code)
{
    zoidfs_cache_hint_t parent_hint;
    static char myname[] = "ADIOI_ZOIDFS_DELETE";

    ret = zoidfs_remove(NULL,
			NULL, 
			filename, 
			&parent_hint);
    /* --BEGIN ERROR HANDLING-- */
    if (ret != ZFS_OK) {
	*error_code = MPIO_Err_create_code(MPI_SUCCESS,
					   MPIR_ERR_RECOVERABLE,
					   myname, __LINE__,
					   MPI_ERR_IO,
					   "Error in zoidfs_remove", 0);
	return;
    }
    /* --END ERROR HANDLING-- */

    *error_code = MPI_SUCCESS;
    return;
}

