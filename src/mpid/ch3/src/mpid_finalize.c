/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int MPID_Finalize()
{
    int mpi_errno;

    mpi_errno = MPID_CH3_Finalize();
    
    return mpi_errno;
}
