/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_nfs.h"

void ADIOI_NFS_IwriteContig(ADIO_File fd, void *buf, int len, int file_ptr_type,
                ADIO_Offset offset, ADIO_Request *request, int *error_code)  
{
    ADIO_Status status;
    int err=-1;
#ifdef __PFS_ON_ADIO
    int myrank, nprocs, i, *len_vec;
    ADIO_Offset off;
#endif

    *request = ADIOI_Malloc_request();
    (*request)->optype = ADIOI_WRITE;
    (*request)->fd = fd;
    (*request)->next = ADIO_REQUEST_NULL;

#ifdef __NO_AIO
    /* HP, FreeBSD, Linux */
    /* no support for nonblocking I/O. Use blocking I/O. */

    ADIOI_NFS_WriteContig(fd, buf, len, file_ptr_type, offset, &status,
                    error_code);  
    (*request)->queued = 0;

#else
    if ((fd->iomode == M_ASYNC) || (fd->iomode == M_UNIX)) {
	if (file_ptr_type == ADIO_INDIVIDUAL) offset = fd->fp_ind;

	err = ADIOI_NFS_aio(fd, buf, len, offset, 1, 
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
	err = ADIOI_NFS_aio(fd, buf, len, fd->fp_ind + myrank*len, 1, 
                           &((*request)->handle));
	fd->fp_ind += nprocs*len;
    }

    if (fd->iomode == M_GLOBAL) {
/* this occurs only in the Intel PFS interface where there is no
   explicit offset */
        MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
        if (myrank == 0) {
	    err = ADIOI_NFS_aio(fd, buf, len, fd->fp_ind, 1, 
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

	err = ADIOI_NFS_aio(fd, buf, len, fd->fp_ind + off, 1, 
                           &((*request)->handle));

        for (i=myrank; i<nprocs; i++) off += len_vec[i];
        fd->fp_ind += off;
        ADIOI_Free(len_vec);
    }
#endif

    (*request)->queued = 1;
    ADIOI_Add_req_to_list(request);

    *error_code = (err == -1) ? MPI_ERR_UNKNOWN : MPI_SUCCESS;
#endif

    fd->fp_sys_posn = -1;   /* set it to null. */

    fd->async_count++;

/* status info. must be linked to the request structure, so that it
   can be accessed later from a wait */
}



void ADIOI_NFS_IwriteStrided(ADIO_File fd, void *buf, int count, 
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
    ADIOI_NFS_WriteStrided(fd, buf, count, datatype, file_ptr_type, 
                            offset, &status, error_code);  

    fd->async_count++;

/* status info. must be linked to the request structure, so that it
   can be accessed later from a wait */

}


/* This function is for implementation convenience. It is not user-visible.
   It takes care of the differences in the interface for nonblocking I/O
   on various Unix machines! If wr==1 write, wr==0 read. */

int ADIOI_NFS_aio(ADIO_File fd, void *buf, int len, ADIO_Offset offset,
		  int wr, void *handle)
{
    int err=-1, error_code, this_errno, fd_sys;

#ifndef __NO_AIO
#ifdef __AIO_SUN 
    aio_result_t *result;
#else
    struct aiocb *aiocbp;
#endif
#endif
    
    fd_sys = fd->fd_sys;

#ifdef __AIO_SUN
    result = (aio_result_t *) ADIOI_Malloc(sizeof(aio_result_t));
    result->aio_return = AIO_INPROGRESS;
    if (wr) {
	ADIOI_WRITE_LOCK(fd, offset, SEEK_SET, len);
	err = aiowrite(fd_sys, buf, len, offset, SEEK_SET, result); 
        this_errno = errno;
	ADIOI_UNLOCK(fd, offset, SEEK_SET, len);
    }
    else {
	ADIOI_READ_LOCK(fd, offset, SEEK_SET, len);
	err = aioread(fd_sys, buf, len, offset, SEEK_SET, result);
        this_errno = errno;
	ADIOI_UNLOCK(fd, offset, SEEK_SET, len);
    }

    if (err == -1) {
	if (this_errno == EAGAIN) { 
       /* the man pages say EPROCLIM, but in reality errno is set to EAGAIN! */

        /* exceeded the max. no. of outstanding requests.
           complete all previous async. requests and try again.*/

	    ADIOI_Complete_async(&error_code);
	    if (wr) {
		ADIOI_WRITE_LOCK(fd, offset, SEEK_SET, len);
		err = aiowrite(fd_sys, buf, len, offset, SEEK_SET, result); 
		this_errno = errno;
		ADIOI_UNLOCK(fd, offset, SEEK_SET, len);
	    }
	    else {
		ADIOI_READ_LOCK(fd, offset, SEEK_SET, len);
		err = aioread(fd_sys, buf, len, offset, SEEK_SET, result);
                this_errno = errno;
		ADIOI_UNLOCK(fd, offset, SEEK_SET, len);
	    }

	    while (err == -1) {
		if (this_errno == EAGAIN) {
                    /* sleep and try again */
                    sleep(1);
		    if (wr) {
			ADIOI_WRITE_LOCK(fd, offset, SEEK_SET, len);
			err = aiowrite(fd_sys, buf, len, offset, SEEK_SET, result); 
			this_errno = errno;
			ADIOI_UNLOCK(fd, offset, SEEK_SET, len);
		    }
		    else {
			ADIOI_READ_LOCK(fd, offset, SEEK_SET, len);
			err = aioread(fd_sys, buf, len, offset, SEEK_SET, result);
			this_errno = errno;
			ADIOI_UNLOCK(fd, offset, SEEK_SET, len);
		    }
		}
                else {
                    printf("Unknown errno %d in ADIOI_NFS_aio\n", this_errno);
                    MPI_Abort(MPI_COMM_WORLD, 1);
                }
	    }
	}
        else {
            printf("Unknown errno %d in ADIOI_NFS_aio\n", this_errno);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    *((aio_result_t **) handle) = result;
#endif

#ifdef __NO_FD_IN_AIOCB
/* IBM */
    aiocbp = (struct aiocb *) ADIOI_Malloc(sizeof(struct aiocb));
    aiocbp->aio_whence = SEEK_SET;
    aiocbp->aio_offset = offset;
    aiocbp->aio_buf = buf;
    aiocbp->aio_nbytes = len;
    if (wr) {
	ADIOI_WRITE_LOCK(fd, offset, SEEK_SET, len);
	err = aio_write(fd_sys, aiocbp);
        this_errno = errno;
	ADIOI_UNLOCK(fd, offset, SEEK_SET, len);
    }
    else {
	ADIOI_READ_LOCK(fd, offset, SEEK_SET, len);
	err = aio_read(fd_sys, aiocbp);
        this_errno = errno;
	ADIOI_UNLOCK(fd, offset, SEEK_SET, len);
    }

    if (err == -1) {
	if (this_errno == EAGAIN) {
        /* exceeded the max. no. of outstanding requests.
          complete all previous async. requests and try again. */

	    ADIOI_Complete_async(&error_code);
	    if (wr) {
		ADIOI_WRITE_LOCK(fd, offset, SEEK_SET, len);
		err = aio_write(fd_sys, aiocbp);
		this_errno = errno;
		ADIOI_UNLOCK(fd, offset, SEEK_SET, len);
	    }
	    else {
		ADIOI_READ_LOCK(fd, offset, SEEK_SET, len);
		err = aio_read(fd_sys, aiocbp);
		this_errno = errno;
		ADIOI_UNLOCK(fd, offset, SEEK_SET, len);
	    }

            while (err == -1) {
                if (this_errno == EAGAIN) {
                    /* sleep and try again */
                    sleep(1);
		    if (wr) {
			ADIOI_WRITE_LOCK(fd, offset, SEEK_SET, len);
			err = aio_write(fd_sys, aiocbp);
			this_errno = errno;
			ADIOI_UNLOCK(fd, offset, SEEK_SET, len);
		    }
		    else {
			ADIOI_READ_LOCK(fd, offset, SEEK_SET, len);
			err = aio_read(fd_sys, aiocbp);
			this_errno = errno;
			ADIOI_UNLOCK(fd, offset, SEEK_SET, len);
		    }
		}
                else {
                    printf("Unknown errno %d in ADIOI_NFS_aio\n", this_errno);
                    MPI_Abort(MPI_COMM_WORLD, 1);
                }
            }
	}
        else {
            printf("Unknown errno %d in ADIOI_NFS_aio\n", this_errno);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    *((struct aiocb **) handle) = aiocbp;

#elif (!defined(__NO_AIO) && !defined(__AIO_SUN))
/* DEC, SGI IRIX 5 and 6 */

    aiocbp = (struct aiocb *) ADIOI_Calloc(sizeof(struct aiocb), 1);
    aiocbp->aio_fildes = fd_sys;
    aiocbp->aio_offset = offset;
    aiocbp->aio_buf = buf;
    aiocbp->aio_nbytes = len;

#ifdef __AIO_PRIORITY_DEFAULT
/* DEC */
    aiocbp->aio_reqprio = AIO_PRIO_DFL;   /* not needed DEC Unix 4.0 */
    aiocbp->aio_sigevent.sigev_signo = 0;
#else
    aiocbp->aio_reqprio = 0;
#endif

#ifdef __AIO_SIGNOTIFY_NONE
/* SGI IRIX 6 */
    aiocbp->aio_sigevent.sigev_notify = SIGEV_NONE;
#else
    aiocbp->aio_sigevent.sigev_signo = 0;
#endif

    if (wr) {
	ADIOI_WRITE_LOCK(fd, offset, SEEK_SET, len);
	err = aio_write(aiocbp);
	this_errno = errno;
	ADIOI_UNLOCK(fd, offset, SEEK_SET, len);
    }
    else {
	ADIOI_READ_LOCK(fd, offset, SEEK_SET, len);
	err = aio_read(aiocbp);
	this_errno = errno;
	ADIOI_UNLOCK(fd, offset, SEEK_SET, len);
    }

    if (err == -1) {
	if (this_errno == EAGAIN) {
        /* exceeded the max. no. of outstanding requests.
           complete all previous async. requests and try again. */

	    ADIOI_Complete_async(&error_code);
	    if (wr) {
		ADIOI_WRITE_LOCK(fd, offset, SEEK_SET, len);
		err = aio_write(aiocbp);
		this_errno = errno;
		ADIOI_UNLOCK(fd, offset, SEEK_SET, len);
	    }
	    else {
		ADIOI_READ_LOCK(fd, offset, SEEK_SET, len);
		err = aio_read(aiocbp);
		this_errno = errno;
		ADIOI_UNLOCK(fd, offset, SEEK_SET, len);
	    }

	    while (err == -1) {
		if (this_errno == EAGAIN) {
		    /* sleep and try again */
		    sleep(1);
		    if (wr) {
			ADIOI_WRITE_LOCK(fd, offset, SEEK_SET, len);
			err = aio_write(aiocbp);
			this_errno = errno;
			ADIOI_UNLOCK(fd, offset, SEEK_SET, len);
		    }
		    else {
			ADIOI_READ_LOCK(fd, offset, SEEK_SET, len);
			err = aio_read(aiocbp);
			this_errno = errno;
			ADIOI_UNLOCK(fd, offset, SEEK_SET, len);
		    }
		}
		else {
		    printf("Unknown errno %d in ADIOI_NFS_aio\n", this_errno);
		    MPI_Abort(MPI_COMM_WORLD, 1);
		}
	    }
        }
	else {
	    printf("Unknown errno %d in ADIOI_NFS_aio\n", this_errno);
	    MPI_Abort(MPI_COMM_WORLD, 1);
	}
    }

    *((struct aiocb **) handle) = aiocbp;
#endif

    return err;
}
