/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpicomm.h"

/* This is the utility file for comm that contains the basic comm items
   and storage management */
#ifndef MPID_COMM_PREALLOC 
#define MPID_COMM_PREALLOC 8
#endif


/* Preallocated comm objects */
MPID_Comm MPID_Comm_builtin[MPID_COMM_N_BUILTIN];
MPID_Comm MPID_Comm_direct[MPID_COMM_PREALLOC];
MPIU_Object_alloc_t MPID_Comm_mem = { 0, 0, 0, 0, MPID_COMM, 
				      sizeof(MPID_Comm), MPID_Comm_direct,
                                      MPID_COMM_PREALLOC};

/* Create a new communicator with a context */
int MPIR_Comm_create( MPID_Comm *oldcomm_ptr, MPID_Comm **newcomm_ptr )
{   
    int mpi_errno;

    *newcomm_ptr = (MPID_Comm *)MPIU_Handle_obj_alloc( &MPID_Comm_mem );
    if (!*newcomm_ptr) {
	mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
	return mpi_errno;
    }
    /* If there is a context id cache in oldcomm, use it here.  Otherwise,
       use the appropriate algorithm to get a new context id */
    (*newcomm_ptr)->context_id = 0;
    return 0;
}
