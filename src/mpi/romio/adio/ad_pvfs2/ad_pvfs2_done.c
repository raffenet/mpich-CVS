/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_pvfs2.h"

int ADIOI_PVFS2_ReadDone(ADIO_Request *request, ADIO_Status *status, int *error_code)  
{
    return 0;

}


int ADIOI_PVFS2_WriteDone(ADIO_Request *request, ADIO_Status *status, int *error_code)  
{
    return ADIOI_PVFS2_ReadDone(request, status, error_code);
} 

/* 
 * vim: ts=8 sts=4 sw=4 noexpandtab 
 */
