/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "adio.h"
#include "ad_ln.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

void ADIOI_LN_Delete(char *filename, int *error_code)
{
    int err;
    static char myname[] = "ADIOI_LN_DELETE";

    err = unlink(filename); 
    /* for now, just delete the exNode file and let its IBP allocations 
       expire */
    
    if (err == -1) {
	*error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
					   myname, __LINE__, MPI_ERR_IO, "**io",
					   "**io %s", strerror(errno));
	return;
    }
    else *error_code = MPI_SUCCESS;
}
