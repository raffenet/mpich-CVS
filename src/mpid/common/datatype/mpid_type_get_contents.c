/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpi.h>
#include <mpiimpl.h>
#include <mpid_datatype.h>
#include <mpid_dataloop.h>

/*@
  MPID_Type_get_contents - get content information from datatype

  Input Parameters:
+ datatype - MPI datatype
. max_integers - size of array_of_integers
. max_addresses - size of array_of_addresses
- max_datatypes - size of array_of_datatypes

  Output Parameters:
+ array_of_integers - integers used in creating type
. array_of_addresses - MPI_Aints used in creating type
- array_of_datatypes - MPI_Datatypes used in creating type

@*/
int MPID_Type_get_contents(MPI_Datatype datatype, 
			   int max_integers, 
			   int max_addresses, 
			   int max_datatypes, 
			   int array_of_integers[], 
			   MPI_Aint array_of_addresses[], 
			   MPI_Datatype array_of_datatypes[])
{
    int mpi_errno;
    int i;
    MPID_Datatype *dtp;
    MPID_Datatype_contents *cp;
    char *ptr;

    /* --BEGIN ERROR HANDLING-- */
    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN)
    {
	/* this is erroneous according to the standard */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, "MPID_Type_get_contents", __LINE__, MPI_ERR_OTHER, "**dtype", 0);
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    MPID_Datatype_get_ptr(datatype, dtp);
    cp = dtp->contents;
    MPIU_Assert(cp != NULL);

    /* --BEGIN ERROR HANDLING-- */
    if (max_integers < cp->nr_ints ||
	max_addresses < cp->nr_aints ||
	max_datatypes < cp->nr_types)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, "MPID_Type_get_contents", __LINE__, MPI_ERR_OTHER, "**dtype", 0);
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    /* recall that contents data is stored contiguously after the
     * contents structure, in types, ints, aints order
     *
     * TODO: PADDING!!!
     */
    ptr = ((char *) cp) + sizeof(MPID_Datatype_contents);
    memcpy(array_of_datatypes, ptr, cp->nr_types * sizeof(MPI_Datatype));
    ptr += cp->nr_types * sizeof(MPI_Datatype);

    if (cp->nr_ints > 0)
    {
	memcpy(array_of_integers, ptr, cp->nr_ints * sizeof(int));
	ptr += cp->nr_ints * sizeof(int);
    }

    if (cp->nr_aints > 0) memcpy(array_of_addresses, ptr, cp->nr_aints * sizeof(MPI_Aint));

    for (i=0; i < cp->nr_types; i++)
    {
    	if (HANDLE_GET_KIND(array_of_datatypes[i]) != HANDLE_KIND_BUILTIN)
	{
	    MPID_Datatype_get_ptr(array_of_datatypes[i], dtp);
	    MPID_Datatype_add_ref(dtp);
	}
    }

    return MPI_SUCCESS;
}
