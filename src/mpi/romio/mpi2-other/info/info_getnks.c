/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"

/*@
    MPI_Info_get_nkeys - Returns the number of currently defined keys in info

Input Parameters:
. info - info object (handle)

Output Parameters:
. nkeys - number of defined keys (integer)

.N fortran
@*/
int MPI_Info_get_nkeys(MPI_Info info, int *nkeys)
{
    MPI_Info curr;

    if ((info <= (MPI_Info) 0) || (info->cookie != MPIR_INFO_COOKIE)) {
        printf("MPI_Info_get_nkeys: Invalid info object\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    curr = info->next;
    *nkeys = 0;

    while (curr) {
	curr = curr->next;
	(*nkeys)++;
    }

    return MPI_SUCCESS;
}
