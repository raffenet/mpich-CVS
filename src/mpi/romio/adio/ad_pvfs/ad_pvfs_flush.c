/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_pvfs.h"

void ADIOI_PVFS_Flush(ADIO_File fd, int *error_code)
{
    int err, rank, dummy=0, dummy_in=0;
#ifndef PRINT_ERR_MSG
    static char myname[] = "ADIOI_PVFS_FLUSH";
#endif

    /* a collective routine: because we do not cache data in PVFS1, one process
     * can initiate the fsync operation and broadcast the result to the others.
     * One catch: MPI_File_sync has special meaning with respect to file system
     * consistency.  Ensure no clients have outstanding write operations.
     */

    MPI_Comm_rank(fd->comm, &rank);
    MPI_Reduce(&dummy_in, &dummy, 1, MPI_INT, MPI_SUM, 
		    fd->hints->ranklist[0], fd->comm);
    if (rank == fd->hints->ranklist[0]) {
	    err = pvfs_fsync(fd->fd_sys);
    }
    MPI_Bcast(&err, 1, MPI_INT, 0, fd->comm);

    if (err == -1) {
#ifdef MPICH2
	*error_code = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, myname, __LINE__, MPI_ERR_IO, "**io",
	    "**io %s", strerror(errno));
#elif defined(PRINT_ERR_MSG)
	*error_code = MPI_ERR_UNKNOWN;
#else /* MPICH-1 */
	*error_code = MPIR_Err_setmsg(MPI_ERR_IO, MPIR_ADIO_ERROR,
			      myname, "I/O Error", "%s", strerror(errno));
	ADIOI_Error(fd, *error_code, myname);	    
#endif
    }
    else *error_code = MPI_SUCCESS;
}
