/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"

/*@
    MPI_Info_get_valuelen - Retrieves the length of the value associated with a key

Input Parameters:
. info - info object (handle)
. key - key (string)

Output Parameters:
. valuelen - length of value argument (integer)
. flag - true if key defined, false if not (boolean)

.N fortran
@*/
int MPI_Info_get_valuelen(MPI_Info info, char *key, int *valuelen, int *flag)
{
    MPI_Info curr;

    if ((info <= (MPI_Info) 0) || (info->cookie != MPIR_INFO_COOKIE)) {
        printf("MPI_Info_get_valuelen: Invalid info object\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (key <= (char *) 0) {
	printf("MPI_Info_get_valuelen: key is an invalid address\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (strlen(key) > MPI_MAX_INFO_KEY) {
	printf("MPI_Info_get_valuelen: key is longer than MPI_MAX_INFO_KEY\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (!strlen(key)) {
	printf("MPI_Info_get_valuelen: key is a null string\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    curr = info->next;
    *flag = 0;

    while (curr) {
	if (!strcmp(curr->key, key)) {
	    *valuelen = strlen(curr->value);
	    *flag = 1;
	    break;
	}
	curr = curr->next;
    }

    return MPI_SUCCESS;
}
