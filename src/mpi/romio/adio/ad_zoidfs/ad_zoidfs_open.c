#include "ad_zoidfs.h"

/* open_status is helpful for bcasting values around */
struct open_status_s {
    int error;
    zoidfs_handle_t fhandle;
};

typedef struct open_status_s open_status;

void ADIOI_ZOIDFS_Open(ADIO_File fd, int *error_code)
{
    int rank;
    static char myname[] = "ADIOI_ZOIDFS_OPEN";

    ADIOI_ZOIDFS_fs zoidfs_fs;
    
    /* since one process is doing the open, that means one process is also
     * doing the error checking.  define a struct for both the object reference
     * and the error code to broadcast to all the processors */
    
    open_status o_status;
    MPI_Datatype open_status_type;
    MPI_Datatype types[2] = {MPI_INT, MPI_BYTE};
    int lens[2] = {1, sizeof(zoidfs_handle_t)};
    MPI_Aint offsets[2];
    
    /* create zoidfs_fs object */

    zoidfs_fs = (ADIOI_ZOIDFS_fs *) ADIOI_Malloc(sizeof(ADIOI_ZOIDFS_fs));
    /* --BEGIN ERROR HANDLING-- */
    if (zoidfs_fs == NULL) {
      *error_code = MPIO_Err_create_code(MPI_SUCCESS,
					 MPIR_ERR_RECOVERABLE,
					 myname, __LINE__,
					 MPI_ERR_IO,
					 "Error allocating memory", 0);
      return;
    }
    /* --END ERROR HANDLING-- */
    
    MPI_Comm_rank(fd->comm, &rank);
    
    /* we only have to do this on one node. we'll broadcast the handle to
     * everyone else in the communicator */
    
    if (rank == fd->hints->ranklist[0]) {
 
      if (access_mode & MPI_MODE_CREATE) {
	zoidfs_sattr_t sattr; /* settable attributes */
	int created;
	
	/* Here, we don't set any attributes for create*/
	sattr.mask = 0;

	o_status.error = zoidfs_create(NULL,
				       NULL,
				       fd->filename, 
				       &sattr,
				       &(zoidfs_fs->fhandle), 
				       &created);

	if (o_status.error == ZFS_OK && !created && 
			access_mode & MPI_MODE_EXCL) {
	  /* ERROR - file already exists 
	   * (but zoidfs_create will retrun ZFS_OK) */
	  o_status.error = ZFSERR_EXIST;
	}
      } else {
	o_status.error = zoidfs_lookup(NULL, 
				       NULL, 
				       fd->filename, 
				       &(zoidfs_fs->fhandle));
      }
    }
    
    
    /* NOTE: if MPI_MODE_EXCL was set, ADIO_Open will call
     * ADIOI_ZOIDFS_Open from just one processor.  This really confuses MPI 
     * when one procesor on a communicator broadcasts to no listners.  
     * Since ADIO_Open will close the file and call ADIOI_ZOIDFS_Open again 
     * (but w/o EXCL), we can bail out right here and return early */
    if ((fd->access_mode & MPI_MODE_EXCL)) {
      if (o_status.error == ZFS_OK)
	{
	  *error_code = MPI_SUCCESS;
	  fd->fs_ptr = (void *)zoidfs_fs;
	}
      else
	{
	  /* --BEGIN ERROR HANDLING-- */
	  ADIOI_Free(zoidfs_fs);
	  *error_code = MPIO_Err_create_code(MPI_SUCCESS,
					     MPIR_ERR_RECOVERABLE,
					     myname, __LINE__,
					     MPI_ERR_IO,
					     "Unknown error", 0);
	  /* --END ERROR HANDLING-- */
	} 
      return;
    } 

    o_status.fhandle = zoidfs_fs->fhandle;

    /* broadcast status and (possibly valid) object reference */
    MPI_Address(&o_status.error, &offsets[0]);
    MPI_Address(&o_status.fhandle, &offsets[1]);
    
    MPI_Type_struct(2, lens, offsets, types, &open_status_type);
    MPI_Type_commit(&open_status_type);
    
    MPI_Bcast(MPI_BOTTOM, 1, open_status_type, 0, fd->comm);
    MPI_Type_free(&open_status_type);
    
    /* --BEGIN ERROR HANDLING-- */
    if (o_status.error != ZFS_OK)
      { 
	ADIOI_Free(zoidfs_fs);
	*error_code = MPIO_Err_create_code(MPI_SUCCESS,
					   MPIR_ERR_RECOVERABLE,
					   myname, __LINE__,
					   MPI_ERR_IO),
					   "Unknown error", 0);
	/* TODO: FIX STRING */
	return;
    }
    /* --END ERROR HANDLING-- */

    memcpy(&(zoidfs_fs->fhandle), &(o_status.fhandle), sizeof(zoidfs_handle_t));
    fd->fs_ptr = (void *)zoidfs_fs;

    *error_code = MPI_SUCCESS;

    return;
}
