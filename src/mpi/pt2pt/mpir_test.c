/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpir_pt2pt.h"

int MPIR_Test(MPID_Request * request)
{
    int ret_val;
    
    MPID_Progress_start();
    /*ret_val = (*request->cc_ptr == 0) ? TRUE : FALSE;*/
    ret_val = (*request->cc_ptr == 0);
    MPID_Progress_end();
    return ret_val;
}
