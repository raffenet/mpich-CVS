/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_pvfs2.h"

void ADIOI_PVFS2_Close(ADIO_File fd, int *error_code)
{
    /* pvfs2 doesn't have a 'close' */
    *error_code = MPI_SUCCESS;
}

/* 
 * vim: ts=8 sts=4 sw=4 noexpandtab 
 */
