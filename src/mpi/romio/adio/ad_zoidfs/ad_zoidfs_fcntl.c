#include "ad_zoidfs.h"
#include "adio_extern.h"

void ADIOI_ZOIDFS_Fcntl(ADIO_File fd, int flag, ADIO_Fcntl_t *fcntl_struct,
		       int *error_code)
{
    int ret;
    ADIOI_ZOIDFS_fs *zoidfs_fs;
    static char myname[] = "ADIOI_ZOIDFS_FCNTL";

    zoidfs_fs = (ADIOI_ZOIDFS_fs*)fd->fs_ptr;

    switch(flag) {
    case ADIO_FCNTL_GET_FSIZE:
	zoidfs_attr_t attr;

	attr.mask = ZOIDFS_ATTR_SIZE;

	ret = zoidfs_getattr(&(zoidfs_fs->fhandle),
		&attr);
	if (ret != ZFS_OK ) {
	    /* --BEGIN ERROR HANDLING-- */
	    *error_code = MPIO_Err_create_code(MPI_SUCCESS,
					       MPIR_ERR_RECOVERABLE,
					       myname, __LINE__,
					       MPI_ERR_IO,
					       "Error in zoidfs_getattr", 0);
	    /* --END ERROR HANDLING-- */
	}
	else {
	    *error_code = MPI_SUCCESS;
	}
	fcntl_struct->fsize = attr.size;
	return;

    case ADIO_FCNTL_SET_DISKSPACE:
	ADIOI_GEN_Prealloc(fd, fcntl_struct->diskspace, error_code);
	break;

    /* --BEGIN ERROR HANDLING-- */
    case ADIO_FCNTL_SET_ATOMICITY:
    default:
	*error_code = MPIO_Err_create_code(MPI_SUCCESS,
					   MPIR_ERR_RECOVERABLE,
					   myname, __LINE__,
					   MPI_ERR_ARG,
					   "**flag", "**flag %d", flag);
    /* --END ERROR HANDLING-- */
    }
}

