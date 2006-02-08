#include "ad_zoidfs.h"

/* we want to be a bit clever here:  at scale, if every client sends a
 * flush request, it will stress the PVFS2 servers with redundant
 * PVFS_sys_flush requests.  Instead, one process should wait for
 * everyone to catch up, do the sync, then broadcast the result.  We can
 * get away with this thanks to PVFS2's stateless design 
 */

void ADIOI_ZOIDFS_Flush(ADIO_File fd, int *error_code) 
{ 
    int ret, rank, dummy=0, dummy_in=0; 
    ADIOI_ZOIDFS_fs *zoidfs_fs;
    static char myname[] = "ADIOI_ZOIDFS_FLUSH";

    *error_code = MPI_SUCCESS;

    zoidfs_fs = (ADIOI_ZOIDFS_fs*)fd->fs_ptr;

    MPI_Comm_rank(fd->comm, &rank);


    /* unlike ADIOI_PVFS2_Resize, MPI_File_sync() does not perform any
     * syncronization */
    MPI_Reduce(&dummy_in, &dummy, 1, MPI_INT, MPI_SUM, 
	    fd->hints->ranklist[0], fd->comm);

    /* io_worker computed in ADIO_Open */
    if (rank == fd->hints->ranklist[0]) {
	ret = zoidfs_commit(&(zoidfs_fs->fhandle));

	MPI_Bcast(&ret, 1, MPI_INT, 0, fd->comm);
    } else {
	MPI_Bcast(&ret, 1, MPI_INT, 0, fd->comm);
    }
    /* --BEGIN ERROR HANDLING-- */
    if (ret != ZFS_OK) {
	*error_code = MPIO_Err_create_code(MPI_SUCCESS,
					   MPIR_ERR_RECOVERABLE,
					   myname, __LINE__,
					   MPI_ERR_IO,
					   "Error in zoidfs_commit", 0);
    }
    /* --END ERROR HANDLING-- */
}
