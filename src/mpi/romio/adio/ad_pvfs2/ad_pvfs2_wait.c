/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_pvfs2.h"

void ADIOI_PVFS2_ReadComplete(ADIO_Request *request, ADIO_Status *status, int *error_code)  
{

}

void ADIOI_PVFS2_WriteComplete(ADIO_Request *request, ADIO_Status *status, int *error_code)  
{
    ADIOI_PVFS2_ReadComplete(request, status, error_code);
}
