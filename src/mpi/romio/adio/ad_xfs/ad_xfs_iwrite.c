/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_xfs.h"

void ADIOI_XFS_IwriteContig(ADIO_File fd, void *buf, int len, int file_ptr_type,
                ADIO_Offset offset, ADIO_Request *request, int *error_code)  
{
    int err=-1;
#ifdef __PFS_ON_ADIO
    int myrank, nprocs, i, *len_vec;
    ADIO_Offset off;
#endif

    *request = ADIOI_Malloc_request();
    (*request)->optype = ADIOI_WRITE;
    (*request)->fd = fd;
    (*request)->next = ADIO_REQUEST_NULL;

    if ((fd->iomode == M_ASYNC) || (fd->iomode == M_UNIX)) {
	if (file_ptr_type == ADIO_INDIVIDUAL) offset = fd->fp_ind;

	err = ADIOI_XFS_aio(fd, buf, len, offset, 1, 
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
	err = ADIOI_XFS_aio(fd, buf, len, fd->fp_ind + myrank*len, 1, 
                           &((*request)->handle));
	fd->fp_ind += nprocs*len;
    }

    if (fd->iomode == M_GLOBAL) {
/* this occurs only in the Intel PFS interface where there is no
   explicit offset */
        MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
        if (myrank == 0) {
	    err = ADIOI_XFS_aio(fd, buf, len, fd->fp_ind, 1, 
                           &((*request)->handle));
	    fd->fp_ind += len;
	}
	else {
	    (*request)->queued = 0;
	    fd->fp_ind += len;
	    *error_code = MPI_SUCCESS;
	    return;
	}
    }

    if (fd->iomode == M_SYNC) {
        MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
        MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
        len_vec = (int *) ADIOI_Malloc(nprocs*sizeof(int));
        MPI_Allgather(&len, 1, MPI_INT, len_vec, 1, MPI_INT,
                      MPI_COMM_WORLD); 
        off = 0;
        for (i=0; i<myrank; i++) off += len_vec[i];

	err = ADIOI_XFS_aio(fd, buf, len, fd->fp_ind + off, 1, 
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




void ADIOI_XFS_IwriteStrided(ADIO_File fd, void *buf, int count, 
		       MPI_Datatype datatype, int file_ptr_type,
                       ADIO_Offset offset, ADIO_Request *request, int
                       *error_code)
{
    ADIO_Status status;

    *request = ADIOI_Malloc_request();
    (*request)->optype = ADIOI_WRITE;
    (*request)->fd = fd;
    (*request)->next = ADIO_REQUEST_NULL;
    (*request)->queued = 0;
    (*request)->handle = 0;

/* call the blocking version. It is faster because it does data sieving. */
    ADIOI_XFS_WriteStrided(fd, buf, count, datatype, file_ptr_type, 
                            offset, &status, error_code);  

    fd->async_count++;

/* status info. must be linked to the request structure, so that it
   can be accessed later from a wait */

}


/* This function is for implementation convenience. It is not user-visible.
   It takes care of the differences in the interface for nonblocking I/O
   on various Unix machines! If wr==1 write, wr==0 read. */

int ADIOI_XFS_aio(ADIO_File fd, void *buf, int len, ADIO_Offset offset,
		  int wr, void *handle)
{
    int err, error_code;
    aiocb64_t *aiocbp;

    aiocbp = (aiocb64_t *) ADIOI_Malloc(sizeof(aiocb64_t));
    aiocbp->aio_fildes = fd->fd_sys;
    aiocbp->aio_offset = offset;
    aiocbp->aio_buf = buf;
    aiocbp->aio_nbytes = len;
    aiocbp->aio_reqprio = 0;

#ifdef __AIO_SIGNOTIFY_NONE
/* SGI IRIX 6 */
    aiocbp->aio_sigevent.sigev_notify = SIGEV_NONE;
#else
    aiocbp->aio_sigevent.sigev_signo = 0;
#endif

    if (wr) err = aio_write64(aiocbp);
    else err = aio_read64(aiocbp);

    if (err == -1) {
	if (errno == EAGAIN) {
        /* exceeded the max. no. of outstanding requests.
	   complete all previous async. requests and try again. */

	    ADIOI_Complete_async(&error_code);
	    if (wr) err = aio_write64(aiocbp);
	    else err = aio_read64(aiocbp);

	    while (err == -1) {
		if (errno == EAGAIN) {
		    /* sleep and try again */
		    sleep(1);
		    if (wr) err = aio_write64(aiocbp);
		    else err = aio_read64(aiocbp);
		}
		else {
		    printf("Unknown errno %d in ADIOI_XFS_aio\n", errno);
		    MPI_Abort(MPI_COMM_WORLD, 1);
		}
	    }
        }
        else {
            printf("Unknown errno %d in ADIOI_XFS_aio\n", errno);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    *((aiocb64_t **) handle) = aiocbp;
    return err;
}
