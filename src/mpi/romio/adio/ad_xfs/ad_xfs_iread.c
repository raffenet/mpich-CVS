/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_xfs.h"

void ADIOI_XFS_IreadContig(ADIO_File fd, void *buf, int len, int file_ptr_type,
                ADIO_Offset offset, ADIO_Request *request, int *error_code)  
{
    int err=-1;
#ifdef __PFS_ON_ADIO
    int myrank, nprocs, i, *len_vec;
    ADIO_Offset off;
#endif

    (*request) = ADIOI_Malloc_request();
    (*request)->optype = ADIOI_READ;
    (*request)->fd = fd;
    (*request)->next = ADIO_REQUEST_NULL;

    if ((fd->iomode == M_ASYNC) || (fd->iomode == M_UNIX)) {
        if (file_ptr_type == ADIO_INDIVIDUAL) offset = fd->fp_ind;

        err = ADIOI_XFS_aio(fd, buf, len, offset, 0, 
                           &((*request)->handle));

        if (file_ptr_type == ADIO_INDIVIDUAL) fd->fp_ind += len;
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
        err = ADIOI_XFS_aio(fd, buf, len, fd->fp_ind + myrank*len, 0, 
                           &((*request)->handle));
        fd->fp_ind += nprocs*len;
    }

    if (fd->iomode == M_GLOBAL) {
/* this occurs only in the Intel PFS interface where there is no
   explicit offset */
        MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
/* currently using blocking I/O for this, because of the read and 
   broadcast nature of the access*/
        if (myrank == 0) {
            lseek(fd->fd_sys, fd->fp_ind, SEEK_SET);
            err = read(fd->fd_sys, buf, len);
        }
        MPI_Bcast(buf, len, MPI_BYTE, 0, MPI_COMM_WORLD);
        fd->fp_ind += len;

	(*request)->queued = 0;
	*error_code = (err == -1) ? MPI_ERR_UNKNOWN : MPI_SUCCESS;
	return;
    }

    if (fd->iomode == M_SYNC) {
        MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
        MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
        len_vec = (int *) ADIOI_Malloc(nprocs*sizeof(int));
        MPI_Allgather(&len, 1, MPI_INT, len_vec, 1, MPI_INT,
                      MPI_COMM_WORLD); 
        off = 0;
        for (i=0; i<myrank; i++) off += len_vec[i];

        err = ADIOI_XFS_aio(fd, buf, len, fd->fp_ind + off, 0, 
                           &((*request)->handle));

        for (i=myrank; i<nprocs; i++) off += len_vec[i];
        fd->fp_ind += off;
        ADIOI_Free(len_vec);
    }
#endif

    (*request)->queued = 1;
    ADIOI_Add_req_to_list(request);

    *error_code = (err == -1) ? MPI_ERR_UNKNOWN : MPI_SUCCESS;

    fd->fp_sys_posn = -1;   /* set it to null. */

    fd->async_count++;

/* status info. must be linked to the request structure, so that it
   can be accessed later from a wait */
}



void ADIOI_XFS_IreadStrided(ADIO_File fd, void *buf, int count, 
		       MPI_Datatype datatype, int file_ptr_type,
                       ADIO_Offset offset, ADIO_Request *request, int
                       *error_code)
{
    ADIO_Status status;

    *request = ADIOI_Malloc_request();
    (*request)->optype = ADIOI_READ;
    (*request)->fd = fd;
    (*request)->next = ADIO_REQUEST_NULL;
    (*request)->queued = 0;
    (*request)->handle = 0;

/* call the blocking version. It is faster because it does data sieving. */
    ADIOI_XFS_ReadStrided(fd, buf, count, datatype, file_ptr_type, 
                            offset, &status, error_code);  

    fd->async_count++;

/* status info. must be linked to the request structure, so that it
   can be accessed later from a wait */

}
