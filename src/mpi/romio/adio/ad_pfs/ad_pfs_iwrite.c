/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_pfs.h"

void ADIOI_PFS_IwriteContig(ADIO_File fd, void *buf, int len, int file_ptr_type,
                ADIO_Offset offset, ADIO_Request *request, int *error_code)  
{
    long *id_sys;
    ADIO_Offset off;
    int err;
#ifndef __PRINT_ERR_MSG
    static char myname[] = "ADIOI_PFS_IWRITECONTIG";
#endif

    *request = ADIOI_Malloc_request();
    (*request)->optype = ADIOI_WRITE;
    (*request)->fd = fd;
    (*request)->next = ADIO_REQUEST_NULL;

    id_sys = (long *) ADIOI_Malloc(sizeof(long));
    (*request)->handle = (void *) id_sys;

    off = (file_ptr_type == ADIO_INDIVIDUAL) ? fd->fp_ind : offset;

    lseek(fd->fd_sys, off, SEEK_SET);
    *id_sys = _iwrite(fd->fd_sys, buf, len);

    if ((*id_sys == -1) && (errno == EQNOMID)) {
     /* the man pages say EMREQUEST, but in reality errno is set to EQNOMID! */

        /* exceeded the max. no. of outstanding requests. */

        /* complete all previous async. requests */
        ADIOI_Complete_async(&err);

        /* try again */
	*id_sys = _iwrite(fd->fd_sys, buf, len);

        if ((*id_sys == -1) && (errno == EQNOMID)) {
#ifdef __PRINT_ERR_MSG
            FPRINTF(stderr, "Error in asynchronous I/O\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
#else
	    *error_code = MPIR_Err_setmsg(MPI_ERR_IO, MPIR_ADIO_ERROR,
			      myname, "I/O Error", "%s", strerror(errno));
	    ADIOI_Error(fd, *error_code, myname);	    
	    return;
#endif
        }
    }
    else if (*id_sys == -1) {
#ifdef __PRINT_ERR_MSG
	FPRINTF(stderr, "Unknown errno %d in ADIOI_PFS_IwriteContig\n", errno);
	MPI_Abort(MPI_COMM_WORLD, 1);
#else
	*error_code = MPIR_Err_setmsg(MPI_ERR_IO, MPIR_ADIO_ERROR,
			         myname, "I/O Error", "%s", strerror(errno));
	ADIOI_Error(fd, *error_code, myname);	    
	return;
#endif
    }

    if (file_ptr_type == ADIO_INDIVIDUAL) fd->fp_ind += len; 

    (*request)->queued = 1;
    ADIOI_Add_req_to_list(request);
    fd->async_count++;

    fd->fp_sys_posn = -1;   /* set it to null. */

#ifdef __PRINT_ERR_MSG
    *error_code = (*id_sys == -1) ? MPI_ERR_UNKNOWN : MPI_SUCCESS;
#else
    if (*id_sys == -1) {
	*error_code = MPIR_Err_setmsg(MPI_ERR_IO, MPIR_ADIO_ERROR,
			      myname, "I/O Error", "%s", strerror(errno));
	ADIOI_Error(fd, *error_code, myname);	    
    }
    else *error_code = MPI_SUCCESS;
#endif

/* status info. must be linked to the request structure, so that it
   can be accessed later from a wait */
}



void ADIOI_PFS_IwriteStrided(ADIO_File fd, void *buf, int count, 
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
    ADIOI_PFS_WriteStrided(fd, buf, count, datatype, file_ptr_type, 
                            offset, &status, error_code);  

    fd->async_count++;

/* status info. must be linked to the request structure, so that it
   can be accessed later from a wait */

}
