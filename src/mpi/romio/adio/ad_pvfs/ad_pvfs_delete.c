/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_pvfs.h"
#include "adio.h"

void ADIOI_PVFS_Delete(char *filename, int *error_code)
{
    int err;
#ifndef PRINT_ERR_MSG
    static char myname[] = "ADIOI_PVFS_DELETE";
#endif

    err = pvfs_unlink(filename);
    if (err == -1) {
#ifdef MPICH2
			*error_code = MPIR_Err_create_code(MPI_ERR_IO, "**io",
							"**io %s", strerror(errno));
			MPIR_Err_return_file(MPI_FILE_NULL, myname, *error_code);
#elif defined(PRINT_ERR_MSG)
			*error_code = MPI_ERR_UNKNOWN; 
#else /* MPICH-1 */
	*error_code = MPIR_Err_setmsg(MPI_ERR_IO, MPIR_ADIO_ERROR,
			      myname, "I/O Error", "%s", strerror(errno));
	ADIOI_Error(MPI_FILE_NULL, *error_code, myname);	    
#endif
    }
    else *error_code = MPI_SUCCESS;
}
