/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_nfs.h"

void ADIOI_NFS_IwriteContig(ADIO_File fd, void *buf, int count, 
                MPI_Datatype datatype, int file_ptr_type,
                ADIO_Offset offset, ADIO_Request *request, int *error_code)  
{
    int len, typesize;
#ifdef NO_AIO
    ADIO_Status status;
#else
    int aio_errno = 0;
#endif
    static char myname[] = "ADIOI_NFS_IWRITECONTIG";

    *request = ADIOI_Malloc_request();
    (*request)->optype = ADIOI_WRITE;
    (*request)->fd = fd;
    (*request)->datatype = datatype;

    MPI_Type_size(datatype, &typesize);
    len = count * typesize;

#ifdef NO_AIO
    /* HP, FreeBSD, Linux */
    /* no support for nonblocking I/O. Use blocking I/O. */

    ADIOI_NFS_WriteContig(fd, buf, len, MPI_BYTE, file_ptr_type, offset,
			  &status,
			  error_code);  
    (*request)->queued = 0;
#ifdef HAVE_STATUS_SET_BYTES
    if (*error_code == MPI_SUCCESS) {
	MPI_Get_elements(&status, MPI_BYTE, &len);
	(*request)->nbytes = len;
    }
#endif

    fd->fp_sys_posn = -1;

#else
    if (file_ptr_type == ADIO_INDIVIDUAL) offset = fd->fp_ind;
    aio_errno = ADIOI_NFS_aio(fd, buf, len, offset, 1, &((*request)->handle));
    if (file_ptr_type == ADIO_INDIVIDUAL) fd->fp_ind += len;

    (*request)->queued = 1;
    ADIOI_Add_req_to_list(request);

    fd->fp_sys_posn = -1;

    if (aio_errno != 0) {
	/* --BEGIN ERROR HANDLING-- */
	MPIO_ERR_CREATE_CODE_ERRNO(myname, aio_errno, error_code);
	return;
	/* --END ERROR HANDLING-- */
    }
    else *error_code = MPI_SUCCESS;
#endif

    fd->async_count++;
}


/* This function is for implementation convenience. It is not user-visible.
 * It takes care of the differences in the interface for nonblocking I/O
 * on various Unix machines! If wr==1 write, wr==0 read.
 *
 * Returns 0 on success, -errno on failure.
 */

int ADIOI_NFS_aio(ADIO_File fd, void *buf, int len, ADIO_Offset offset,
		  int wr, void *handle)
{
    int err=-1, fd_sys;
    int error_code, this_errno;

#ifndef NO_AIO
#ifdef AIO_SUN 
    aio_result_t *result;
#else
    struct aiocb *aiocbp;
#endif
#endif
    
    fd_sys = fd->fd_sys;

#ifdef AIO_SUN
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
	    if (error_code != MPI_SUCCESS) return -EIO;

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
		    return -this_errno;
                }
	    }
	}
        else {
	    return -this_errno;
        }
    }

    *((aio_result_t **) handle) = result;
#endif

#ifdef NO_FD_IN_AIOCB
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
	    if (error_code != MPI_SUCCESS) return -EIO;

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
		    return -this_errno;
                }
            }
	}
        else {
	    return -this_errno;
        }
    }

    *((struct aiocb **) handle) = aiocbp;

#elif (!defined(NO_AIO) && !defined(AIO_SUN))
/* DEC, SGI IRIX 5 and 6 */

    aiocbp = (struct aiocb *) ADIOI_Calloc(sizeof(struct aiocb), 1);
    aiocbp->aio_fildes = fd_sys;
    aiocbp->aio_offset = offset;
    aiocbp->aio_buf = buf;
    aiocbp->aio_nbytes = len;

#ifdef AIO_PRIORITY_DEFAULT
/* DEC */
    aiocbp->aio_reqprio = AIO_PRIO_DFL;   /* not needed DEC Unix 4.0 */
    aiocbp->aio_sigevent.sigev_signo = 0;
#else
    aiocbp->aio_reqprio = 0;
#endif

#ifdef AIO_SIGNOTIFY_NONE
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
	    if (error_code != MPI_SUCCESS) return -EIO;

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
		    return -this_errno;
		}
	    }
        }
	else {
	    return -this_errno;
	}
    }

    *((struct aiocb **) handle) = aiocbp;
#endif

    return ((err == 0) ? 0 : -this_errno);
}
