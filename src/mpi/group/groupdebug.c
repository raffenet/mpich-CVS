/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* style: allow:fprintf:2 sig:0 */

/*
 * This file contains routines that are used only to perform testing
 * and debugging of the group routines
 */

void MPITEST_Group_create( int nproc, int myrank, MPI_Group *new_group )
{
    MPID_Group *new_group_ptr;
    int i;

    new_group_ptr = (MPID_Group *)MPIU_Handle_obj_alloc( &MPID_Group_mem );
    if (!new_group_ptr) {
	fprintf( stderr, "Could not create a new group\n" );
	MPI_Abort( MPI_COMM_WORLD, 1 );
    }
    new_group_ptr->lrank_to_lpid = (MPID_Group_pmap_t *)MPIU_Malloc( nproc * sizeof(MPID_Group_pmap_t) );
    if (!new_group_ptr) {
	fprintf( stderr, "Could not create lrank map for new group\n" );
	MPI_Abort( MPI_COMM_WORLD, 1 );
    }

    new_group_ptr->rank = MPI_UNDEFINED;
    for (i=0; i<nproc; i++) {
	new_group_ptr->lrank_to_lpid[i].lrank = i;
	new_group_ptr->lrank_to_lpid[i].lpid  = i;
    }
    new_group_ptr->size = nproc;
    new_group_ptr->rank = myrank;
    new_group_ptr->idx_of_first_lpid = -1;

    *new_group = new_group_ptr->handle;
}
