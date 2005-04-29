/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "adio.h"
#include "ad_ln.h"

void ADIOI_LN_Flush(ADIO_File fd, int *error_code)
{
    int err;
    static char myname[] = "ADIOI_LN_FLUSH";

    err = ADIOI_LNIO_Flush(fd);
    
    /* --BEGIN ERROR HANDLING-- */
    if (err == -1) {
	*error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
					   myname, __LINE__, MPI_ERR_IO,
					   "**io",
					   "**io %s", strerror(errno));
	return;
    }
    /* --END ERROR HANDLING-- */
    
    *error_code = MPI_SUCCESS;
}
