/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "attr.h"
/*
 * Keyvals.  These are handled just like the other opaque objects in MPICH
 * The predefined keyvals (and their associated attributes) are handled 
 * separately, without using the keyval 
 * storage
 */

#ifndef MPID_KEYVAL_PREALLOC 
#define MPID_KEYVAL_PREALLOC 16
#endif

/* Preallocated keyval objects */
MPID_Keyval MPID_Keyval_direct[MPID_KEYVAL_PREALLOC];
MPIU_Object_alloc_t MPID_Keyval_mem = { 0, 0, 0, 0, MPID_KEYVAL, 
					    sizeof(MPID_Keyval), 
					    MPID_Keyval_direct,
					    MPID_KEYVAL_PREALLOC, };

void MPID_Keyval_free(MPID_Keyval *keyval_ptr)
{
    MPIU_Handle_obj_free(&MPID_Keyval_mem, keyval_ptr);
}

#ifndef MPID_ATTR_PREALLOC 
#define MPID_ATTR_PREALLOC 32
#endif

/* Preallocated keyval objects */
MPID_Attribute MPID_Attr_direct[MPID_ATTR_PREALLOC];
MPIU_Object_alloc_t MPID_Attr_mem = { 0, 0, 0, 0, MPID_ATTR, 
					    sizeof(MPID_Attribute), 
					    MPID_Attr_direct,
					    MPID_ATTR_PREALLOC, };

void MPID_Attr_free(MPID_Attribute *attr_ptr)
{
    MPIU_Handle_obj_free(&MPID_Attr_mem, attr_ptr);
}

/* Routine to duplicate an attribute list */
int MPIR_Comm_attr_dup( MPID_Comm *comm_ptr, MPID_Attribute **new_attr )
{
    MPID_Attribute     *p, *new_p, **next_new_attr_ptr = new_attr;
    MPID_Copy_function copyfn;
    MPID_Lang_t        language;
    void               *new_value;
    int                flag;
    int                mpi_errno = 0;

    p = comm_ptr->attributes;
    while (p) {
	/* Call the copy function here */
	copyfn   = p->keyval->copyfn;
	language = p->keyval->language;
	/* Run the copy function if present */
	flag = 0;
	switch (language) {
	case MPID_LANG_C: 
#ifdef HAVE_CXX_BINDING
	case MPID_LANG_CXX: 
#endif
	    if (copyfn.C_CommCopyFunction) {
		mpi_errno = copyfn.C_CommCopyFunction( comm_ptr->handle, 
						p->keyval->handle, 
						p->keyval->extra_state, 
						p->value, &new_value, &flag );
	    }
	    break;
#ifdef HAVE_FORTRAN_BINDING
	case MPID_LANG_FORTRAN: 
	    {
		MPI_Fint fcomm, fkeyval, fvalue, fextra, fflag, fnew, ierr;
		if (copyfn.F77_CopyFunction) {
		    fcomm   = (MPI_Fint) (comm_ptr->handle);
		    fkeyval = (MPI_Fint) (p->keyval->handle);
		    fvalue  = (MPI_Fint) (p->value);
		    fextra  = (MPI_Fint) (p->keyval->extra_state );
		    delfn.F77_DeleteFunction( &fcomm, &fkeyval, &fextra,
					      &fvalue, &fnew, &flag, &ierr );
		    if (ierr) mpi_errno = (int)ierr;
		    flag = fflag;
		}
	    }
	    break;
	case MPID_LANG_FORTRAN90: 
	    {
		MPI_Fint fcomm, fkeyval, fflag, ierr;
		MPI_Aint fvalue, fnew, fextra;
		if (copyfn.F90_CopyFunction) {
		    fcomm   = (MPI_Fint) (comm_ptr->handle);
		    fkeyval = (MPI_Fint) (p->keyval->handle);
		    fvalue  = (MPI_Aint) (p->value);
		    fextra  = (MPI_Aint) (p->keyval->extra_state );
		    delfn.F90_DeleteFunction( &fcomm, &fkeyval, &fextra,
					      &fvalue, &fnew, &fflag, &ierr );
		    if (ierr) mpi_errno = (int)ierr;
		    flag = fflag;
		}
	    }
	    break;
#endif
	}
	
	/* If flag was returned as true and there was no error, then
	   insert this attribute into the new list (new_attr) */
	if (flag && !mpi_errno) {
	    /* duplicate the attribute by creating new storage, copying the
	       attribute value, and invoking the copy function */
	    new_p = (MPID_Attribute *)MPIU_Handle_obj_alloc( &MPID_Attr_mem );
	    if (!new_p) {
		mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
		return mpi_errno;
	    }
	    if (!*new_attr) { 
		*new_attr = new_p;
	    }
	    new_p->keyval        = p->keyval;
	    *(next_new_attr_ptr) = new_p;
	    
	    new_p->pre_sentinal  = 0;
	    new_p->value 	 = new_value;
	    new_p->post_sentinal = 0;
	    new_p->next	         = 0;
	    next_new_attr_ptr    = &(new_p->next);
	}
	else if (mpi_errno) 
	    return mpi_errno;

	p = p->next;
    }
    return MPI_SUCCESS;
}

/* Routine to delete an attribute list */
int MPIR_Comm_attr_delete( MPID_Comm *comm_ptr, MPID_Attribute *attr )
{
    MPID_Attribute *p, *new_p;
    MPID_Delete_function delfn;
    MPID_Lang_t          language;
    int mpi_errno = MPI_SUCCESS;

    p = attr;
    while (p) {
	/* delete the attribute by first executing the delete routine, if any,
	   determine the the next attribute, and recover the attributes 
	   storage */
	new_p = p->next;
	
	/* Check the sentinals first */
	if (p->pre_sentinal != 0 || p->post_sentinal != 0) {
	    mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, 
					      "**attrsentinal", 0 );
	    /* We could keep trying to free the attributes, but for now
	       we'll just bag it */
	    return mpi_errno;
	}
	/* For this attribute, find the delete function for the 
	   corresponding keyval */
	/* Still to do: capture any error returns but continue to 
	   process attributes */
	delfn    = p->keyval->delfn;
	language = p->keyval->language;
	switch (language) {
	case MPID_LANG_C: 
#ifdef HAVE_CXX_BINDING
	case MPID_LANG_CXX: 
#endif
	    if (delfn.C_CommDeleteFunction) {
		mpi_errno = delfn.C_CommDeleteFunction( comm_ptr->handle, 
						p->keyval->handle, 
						p->value, 
						p->keyval->extra_state );
	    }
	    break;
#ifdef HAVE_FORTRAN_BINDING
	case MPID_LANG_FORTRAN: 
	    {
		MPI_Fint fcomm, fkeyval, fvalue, fextra, ierr;
		if (delfn.F77_DeleteFunction) {
		    fcomm   = (MPI_Fint) (comm_ptr->handle);
		    fkeyval = (MPI_Fint) (p->keyval->handle);
		    fvalue  = (MPI_Fint) (p->value);
		    fextra  = (MPI_Fint) (p->keyval->extra_state );
		    delfn.F77_DeleteFunction( &fcomm, &fkeyval, &fvalue, 
					      &fextra, &ierr );
		    if (ierr) mpi_errno = (int)ierr;
		}
	    }
	    break;
	case MPID_LANG_FORTRAN90: 
	    {
		MPI_Fint fcomm, fkeyval, ierr;
		MPI_Aint fvalue, fextra;
		if (delfn.F90_DeleteFunction) {
		    fcomm   = (MPI_Fint) (comm_ptr->handle);
		    fkeyval = (MPI_Fint) (p->keyval->handle);
		    fvalue  = (MPI_Aint) (p->value);
		    fextra  = (MPI_Aint) (p->keyval->extra_state );
		    delfn.F90_DeleteFunction( &fcomm, &fkeyval, &fvalue, 
					      &fextra, &ierr );
		    if (ierr) mpi_errno = (int)ierr;
		}
	    }
	    break;
#endif
	}

	MPIU_Handle_obj_free( &MPID_Attr_mem, p );
	
	p = new_p;
    }
    return mpi_errno;
}

