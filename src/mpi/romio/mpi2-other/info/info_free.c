/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"

/*@
    MPI_Info_free - Frees an info object

Input Parameters:
. info - info object (handle)

.N fortran
@*/
int MPI_Info_free(MPI_Info *info)
{
    MPI_Info curr, next;

    if ((*info <= (MPI_Info) 0) || ((*info)->cookie != MPIR_INFO_COOKIE)) {
        printf("MPI_Info_free: Invalid info object\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    curr = (*info)->next;
    ADIOI_Free(*info);
    *info = MPI_INFO_NULL;

    while (curr) {
	next = curr->next;
	free(curr->key);
	free(curr->value);
	ADIOI_Free(curr);
	curr = next;
    }

    return MPI_SUCCESS;
}
