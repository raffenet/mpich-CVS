/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_xfs.h"

void ADIOI_XFS_ReadContig(ADIO_File fd, void *buf, int len, int file_ptr_type,
		     ADIO_Offset offset, ADIO_Status *status, int *error_code)
{
    int err=-1;
#ifdef __PFS_ON_ADIO
    int myrank, nprocs, i, *len_vec;
    ADIO_Offset off;
#endif

    fd->fp_sys_posn = -1; /* set it to null, since we are using pread */
    if ((fd->iomode == M_ASYNC) || (fd->iomode == M_UNIX)) {
        if (file_ptr_type == ADIO_EXPLICIT_OFFSET) 
	    err = pread(fd->fd_sys, buf, len, offset);
        else {    /* read from curr. location of ind. file pointer */
	    err = pread(fd->fd_sys, buf, len, fd->fp_ind);
	    fd->fp_ind += len;
	}
    }


#ifdef __PFS_ON_ADIO
/* The remaining file pointer modes are relevant only for implementing 
   the Intel PFS interface on top of ADIO. They should never appear,
   for example, in the MPI-IO implementation. */

    if (fd->iomode == M_RECORD) {
/* this occurs only in the Intel PFS interface where there is no
   explicit offset */

        MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
        MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
	lseek64(fd->fd_sys, fd->fp_ind + myrank*len, SEEK_SET);
        err = read(fd->fd_sys, buf, len);
        fd->fp_ind = lseek64(fd->fd_sys, (nprocs-myrank-1)*len, SEEK_CUR);
    }

    if (fd->iomode == M_GLOBAL) {
/* this occurs only in the Intel PFS interface where there is no
   explicit offset */
        MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	if (myrank == 0) {
	    lseek64(fd->fd_sys, fd->fp_ind, SEEK_SET);
	    err = read(fd->fd_sys, buf, len);
	}
	MPI_Bcast(buf, len, MPI_BYTE, 0, MPI_COMM_WORLD);
	fd->fp_ind += len;
    }

    if (fd->iomode == M_SYNC) {
        MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
        MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	len_vec = (int *) ADIOI_Malloc(nprocs*sizeof(int));
	MPI_Allgather(&len, 1, MPI_INT, len_vec, 1, MPI_INT,
		      MPI_COMM_WORLD); 
	off = 0;
	for (i=0; i<myrank; i++) off += len_vec[i];
        lseek64(fd->fd_sys, fd->fp_ind + off, SEEK_SET);
        err = read(fd->fd_sys, buf, len);
	off = 0;
	for (i=(myrank+1); i<nprocs; i++) off += len_vec[i];
        fd->fp_ind = lseek64(fd->fd_sys, off, SEEK_CUR);
	ADIOI_Free(len_vec);
    }
#endif

    *error_code = (err == -1) ? MPI_ERR_UNKNOWN : MPI_SUCCESS;
}




void ADIOI_XFS_ReadStrided(ADIO_File fd, void *buf, int count,
                       MPI_Datatype datatype, int file_ptr_type,
                       ADIO_Offset offset, ADIO_Status *status, int
                       *error_code)
{
    ADIOI_GEN_ReadStrided(fd, buf, count, datatype, file_ptr_type,
                        offset, status, error_code);
}
