/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_pvfs2.h"
#include "adio_extern.h"

void ADIOI_PVFS2_Fcntl(ADIO_File fd, int flag, ADIO_Fcntl_t *fcntl_struct, int *error_code)
{

    switch(flag) {
    case ADIO_FCNTL_GET_FSIZE:
    case ADIO_FCNTL_SET_DISKSPACE:
    case ADIO_FCNTL_SET_IOMODE:
    case ADIO_FCNTL_SET_ATOMICITY:
    default:
	    *error_code = MPI_SUCCESS;
    }
}

/* 
 * vim: ts=8 sts=4 sw=4 noexpandtab 
 */
