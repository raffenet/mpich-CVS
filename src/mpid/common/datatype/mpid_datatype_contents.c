/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpiimpl.h>
#include <mpid_dataloop.h>
#include <stdlib.h>
#include <limits.h>

int MPID_Datatype_set_contents(MPID_Datatype *new_dtp,
			       int combiner,
			       int nr_ints,
			       int nr_aints,
			       int nr_types,
			       int array_of_ints[],
			       MPI_Aint array_of_aints[],
			       MPI_Datatype array_of_types[])
{
    int i, contents_size;
    MPID_Datatype_contents *cp;
    MPID_Datatype *old_dtp;
    char *ptr;

    /* TODO: PADDING */
    contents_size = (4 + nr_ints) * sizeof(int) +
	nr_aints * sizeof(MPI_Aint) + nr_types * sizeof(MPI_Datatype);

    cp = (MPID_Datatype_contents *) MPIU_Malloc(contents_size);
    if (cp == NULL) MPIU_Assert(0);

    cp->combiner = combiner;
    cp->nr_ints  = nr_ints;
    cp->nr_aints = nr_aints;
    cp->nr_types = nr_types;

    /* arrays are stored in the following order: types, ints, aints */
    ptr = ((char *) cp) + sizeof(MPID_Datatype_contents);
    memcpy(ptr, array_of_types, nr_types * sizeof(MPI_Datatype));
    ptr += nr_types * sizeof(MPI_Datatype);
    
    if (nr_ints > 0) {
	memcpy(ptr, array_of_ints, nr_ints * sizeof(int));
	ptr += nr_ints * sizeof(int);
    }

    if (nr_aints > 0) {
	memcpy(ptr, array_of_aints, nr_aints * sizeof(MPI_Aint));
    }
    new_dtp->contents = cp;

    /* increment reference counts on all the derived types used here */
    for (i=0; i < nr_types; i++) {
	if (HANDLE_GET_KIND(array_of_types[i]) != HANDLE_KIND_BUILTIN) {
	    MPID_Datatype_get_ptr(array_of_types[i], old_dtp);
	    MPID_Datatype_add_ref(old_dtp);
	}
    }

    return 0;
}

void MPID_Datatype_free_contents(MPID_Datatype *dtp)
{
    int i, cnt;
    MPID_Datatype *old_dtp;
    MPI_Datatype *array_of_types;

    array_of_types = (MPI_Datatype *) ((char *)dtp->contents + sizeof(MPID_Datatype_contents));

    for (i=0; i < dtp->contents->nr_types; i++) {
	if (HANDLE_GET_KIND(array_of_types[i]) != HANDLE_KIND_BUILTIN) {
	    MPID_Datatype_get_ptr(array_of_types[i], old_dtp);
	    MPIU_Object_release_ref(old_dtp, &cnt);
	    if (cnt == 0) {
		/* last reference to this type */
		MPID_Datatype_free(old_dtp);
	    }
	}
    }

    MPIU_Free(dtp->contents);
    dtp->contents = NULL;
}

