/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"

/*@
    MPI_Info_delete - Deletes a (key,value) pair from info

Input Parameters:
. info - info object (handle)
. key - key (string)

.N fortran
@*/
int MPI_Info_delete(MPI_Info info, char *key)
{
    MPI_Info prev, curr;
    int done;

    if ((info <= (MPI_Info) 0) || (info->cookie != MPIR_INFO_COOKIE)) {
        printf("MPI_Info_delete: Invalid info object\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (key <= (char *) 0) {
	printf("MPI_Info_delete: key is an invalid address\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (strlen(key) > MPI_MAX_INFO_KEY) {
	printf("MPI_Info_delete: key is longer than MPI_MAX_INFO_KEY\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (!strlen(key)) {
	printf("MPI_Info_delete: key is a null string\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    prev = info;
    curr = info->next;
    done = 0;

    while (curr) {
	if (!strcmp(curr->key, key)) {
	    free(curr->key);   /* not ADIOI_Free, because it was strdup'ed */
	    free(curr->value);
	    prev->next = curr->next;
	    ADIOI_Free(curr);
	    done = 1;
	    break;
	}
	prev = curr;
	curr = curr->next;
    }

    if (!done) {
	printf("MPI_Info_delete: key not defined in info\n");
        MPI_Abort(MPI_COMM_WORLD, 1);	
    }

    return MPI_SUCCESS;
}
