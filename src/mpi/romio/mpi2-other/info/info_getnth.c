/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"

/*@
    MPI_Info_get_nthkey - Returns the nth defined key in info

Input Parameters:
. info - info object (handle)
. n - key number (integer)

Output Parameters:
. keys - key (string)

.N fortran
@*/
int MPI_Info_get_nthkey(MPI_Info info, int n, char *key)
{
    MPI_Info curr;
    int nkeys, i;

    if ((info <= (MPI_Info) 0) || (info->cookie != MPIR_INFO_COOKIE)) {
        printf("MPI_Info_get_nthkey: Invalid info object\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (key <= (char *) 0) {
	printf("MPI_Info_get: key is an invalid address\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    curr = info->next;
    nkeys = 0;
    while (curr) {
	curr = curr->next;
	nkeys++;
    }

    if ((n < 0) || (n >= nkeys)) {
        printf("MPI_Info_get_nthkey: n is an invalid number\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    curr = info->next;
    i = 0;
    while (i < n) {
	curr = curr->next;
	i++;
    }
    strcpy(key, curr->key);

    return MPI_SUCCESS;
}
