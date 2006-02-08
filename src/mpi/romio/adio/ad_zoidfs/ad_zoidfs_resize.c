#include "ad_zoidfs.h"

/* as with ADIOI_PVFS2_Flush, implement the resize operation in a scalable
 * manner. one process does the work, then broadcasts the result to everyone
 * else.  fortunately, this operation is defined to be collective */
void ADIOI_ZOIDFS_Resize(ADIO_File fd, ADIO_Offset size, int *error_code)
{
    int ret, rank;
    ADIOI_ZOIDFS_fs *zoidfs_fs;
    static char myname[] = "ADIOI_ZOIDFS_RESIZE";

    *error_code = MPI_SUCCESS;

    zoidfs_fs = (ADIOI_ZOIDFS_fs*)fd->fs_ptr;

    MPI_Comm_rank(fd->comm, &rank);

    /* We desginate one node in the communicator to be an 'io_worker' in 
     * ADIO_Open.  This node can perform operations on files and then 
     * inform the other nodes of the result */

    /* MPI-IO semantics treat conflicting MPI_File_set_size requests the
     * same as conflicting write requests. Thus, a resize from one
     * process does not have to be visible to the other processes until a
     * syncronization point is reached */

    if (rank == fd->hints->ranklist[0]) {
	zoidfs_sattr_t sattr;

	sattr.mask = ZOIDFS_ATTR_SIZE;
	sattr.size = size;
	
	ret = zoidfs_setattr(&(zoidfs_fs->fhandle),
		&sattr,
		NULL);
	
	MPI_Bcast(&ret, 1, MPI_INT, 0, fd->comm);
    } else  {
	MPI_Bcast(&ret, 1, MPI_INT, 0, fd->comm);
    }
    /* --BEGIN ERROR HANDLING-- */
    if (ret != ZFS_OK) {
	*error_code = MPIO_Err_create_code(MPI_SUCCESS,
					   MPIR_ERR_RECOVERABLE,
					   myname, __LINE__,
					   MPI_ERR_IO,
					   "Error in zoidfs_setattr", 0);
	return;
    }
    /* --END ERROR HANDLING-- */
}
