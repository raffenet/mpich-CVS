/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_xfs.h"

static void ADIOI_XFS_Aligned_Mem_File_Read(ADIO_File fd, void *buf, int len, 
					     ADIO_Offset offset, int *err);

void ADIOI_XFS_ReadContig(ADIO_File fd, void *buf, int count, 
                     MPI_Datatype datatype, int file_ptr_type,
		     ADIO_Offset offset, ADIO_Status *status, int *error_code)
{
    int err=-1, datatype_size, len, diff, size;
    void *newbuf;
#ifndef PRINT_ERR_MSG
    static char myname[] = "ADIOI_XFS_READCONTIG";
#endif

    MPI_Type_size(datatype, &datatype_size);
    len = datatype_size * count;

    fd->fp_sys_posn = -1; /* set it to null, since we are using pread */

    if (file_ptr_type == ADIO_INDIVIDUAL) offset = fd->fp_ind;

    if (!(fd->direct_read))     /* direct I/O not enabled */
	err = pread(fd->fd_sys, buf, len, offset);
    else {       /* direct I/O enabled */

	/* (1) if mem_aligned && file_aligned 
                    use direct I/O to read up to correct io_size
                    use buffered I/O for remaining  */

	if (!(((long) buf) % fd->d_mem) && !(offset % fd->d_miniosz)) 
	    ADIOI_XFS_Aligned_Mem_File_Read(fd, buf, len, offset, &err);

        /* (2) if !file_aligned
                    use buffered I/O to read up to file_aligned
                    At that point, if still mem_aligned, use (1)
   		        else copy into aligned buf and then use (1) */
	else if (offset % fd->d_miniosz) {
	    diff = fd->d_miniosz - (offset % fd->d_miniosz);
	    diff = ADIOI_MIN(diff, len);
	    err = pread(fd->fd_sys, buf, diff, offset);
	    if (err == diff) {
		buf = ((char *) buf) + diff;
		offset += diff;
		size = len - diff;
		if (!(((long) buf) % fd->d_mem))
		    ADIOI_XFS_Aligned_Mem_File_Read(fd, buf, size, offset, &err);
		else {
		    newbuf = (void *) memalign(XFS_MEMALIGN, size);
		    if (newbuf) {
			ADIOI_XFS_Aligned_Mem_File_Read(fd, newbuf, size, offset, &err);
			memcpy(buf, newbuf, size);
			free(newbuf);
		    }
		    else err = pread(fd->fd_sys, buf, size, offset);
		    if (err != size) err = -1;
		}
		if (err != -1) err = len;
	    }
	    else err = -1;
	}

        /* (3) if !mem_aligned && file_aligned
    	            copy into aligned buf, then use (1)  */
	else {
	    newbuf = (void *) memalign(XFS_MEMALIGN, len);
	    if (newbuf) {
		ADIOI_XFS_Aligned_Mem_File_Read(fd, newbuf, len, offset, &err);
		memcpy(buf, newbuf, len);
		free(newbuf);
	    }
	    else err = pread(fd->fd_sys, buf, len, offset);
	}
    }

    if (file_ptr_type == ADIO_INDIVIDUAL) fd->fp_ind += err;

#ifdef HAVE_STATUS_SET_BYTES
    if (err != -1) MPIR_Status_set_bytes(status, datatype, err);
#endif

#ifdef PRINT_ERR_MSG
    *error_code = (err == -1) ? MPI_ERR_UNKNOWN : MPI_SUCCESS;
#else
    if (err == -1) {
	*error_code = MPIR_Err_setmsg(MPI_ERR_IO, MPIR_ADIO_ERROR,
			      myname, "I/O Error", "%s", strerror(errno));
	ADIOI_Error(fd, *error_code, myname);
    }
    else *error_code = MPI_SUCCESS;
#endif
}


void ADIOI_XFS_Aligned_Mem_File_Read(ADIO_File fd, void *buf, int len, 
              ADIO_Offset offset, int *err)
{
    int ntimes, rem, newrem, i, size;

    /* memory buffer is aligned, offset in file is aligned,
       io_size may or may not be of the right size.
       use direct I/O to read up to correct io_size,
       use buffered I/O for remaining. */

    if (!(len % fd->d_miniosz) && 
	(len >= fd->d_miniosz) && (len <= fd->d_maxiosz))
	*err = pread(fd->fd_direct, buf, len, offset);
    else if (len < fd->d_miniosz)
	*err = pread(fd->fd_sys, buf, len, offset);
    else if (len > fd->d_maxiosz) {
	ntimes = len/(fd->d_maxiosz);
	rem = len - ntimes * fd->d_maxiosz;
	for (i=0; i<ntimes; i++) {
	    *err = pread(fd->fd_direct, ((char *)buf) + i * fd->d_maxiosz,
			 fd->d_maxiosz, offset);
	    if (*err != fd->d_maxiosz) {
		*err = -1;
		return;
	    }
	    offset += fd->d_maxiosz;
	}
	if (rem) {
	    if (!(rem % fd->d_miniosz)) {
		*err = pread(fd->fd_direct, 
		     ((char *)buf) + ntimes * fd->d_maxiosz, rem, offset);
		if (*err != rem) {
		    *err = -1;
		    return;
		}
	    }
	    else {
		newrem = rem % fd->d_miniosz;
		size = rem - newrem;
		if (size) {
		    *err = pread(fd->fd_direct, 
		      ((char *)buf) + ntimes * fd->d_maxiosz, size, offset);
		    if (*err != size) {
			*err = -1;
			return;
		    }
		    offset += size;
		}
		*err = pread(fd->fd_sys, 
	          ((char *)buf) + ntimes*fd->d_maxiosz + size, newrem, offset);
		if (*err != newrem) {
		    *err = -1;
		    return;
		}
	    }
	}
	*err = len;
    }
    else {
	rem = len % fd->d_miniosz;
	size = len - rem;
	*err = pread(fd->fd_direct, buf, size, offset);
	if (*err != size) {
	    *err = -1;
	    return;
	}
	*err = pread(fd->fd_sys, (char *)buf + size, rem, offset+size);
	if (*err != rem) {
	    *err = -1;
	    return;
	}
	*err = len;
    }
}


void ADIOI_XFS_ReadStrided(ADIO_File fd, void *buf, int count,
                       MPI_Datatype datatype, int file_ptr_type,
                       ADIO_Offset offset, ADIO_Status *status, int
                       *error_code)
{
    ADIOI_GEN_ReadStrided(fd, buf, count, datatype, file_ptr_type,
                        offset, status, error_code);
}
