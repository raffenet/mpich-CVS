/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_pvfs.h"

/* ADIOI_PVFS_Flush()
 *
 * Assumptions:
 * - called collectively (as defined in MPI-2 standard)
 * - pvfs_fsync() flushes all changes to file even if one node makes the
 *   call (which it does)
 *
 * Approach:
 * - Process 0 does the pvfs_fsync(), so only one message goes out to
 *   the file system
 * - Process 0 Bcasts its return value to the rest of the processes
 * - When other processes receive value, they know the pvfs_fsync() has
 *   completed.
 */
void ADIOI_PVFS_Flush(ADIO_File fd, int *error_code)
{
    int err, rank, procs;
#ifndef PRINT_ERR_MSG
    static char myname[] = "ADIOI_PVFS_FLUSH";
#endif

    MPI_Comm_rank(fd->comm, &rank);
    if (rank == 0) {
    	err = pvfs_fsync(fd->fd_sys);
    }

    MPI_Comm_size(fd->comm, &procs);
    if (procs > 1) {
    	MPI_Bcast(&err, 1, MPI_INT, 0, fd->comm);
    }

#ifdef PRINT_ERR_MSG
    *error_code = (err == 0) ? MPI_SUCCESS : MPI_ERR_UNKNOWN;
#else
    if (err == -1) {
	*error_code = MPIR_Err_setmsg(MPI_ERR_IO, MPIR_ADIO_ERROR,
			      myname, "I/O Error", "%s", strerror(errno));
	ADIOI_Error(fd, *error_code, myname);	    
    }
    else *error_code = MPI_SUCCESS;
#endif
}
