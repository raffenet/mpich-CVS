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
    MPID_Attribute *p, *new_p, *last_new_p = 0, *new_head = 0;
    int mpi_errno;

    p = comm_ptr->attributes;
    while (p) {
	/* duplicate the attribute by creating new storage, copying the
	   attribute value, and invoking the copy function */
	new_p = (MPID_Attribute *)MPIU_Handle_obj_alloc( &MPID_Attr_mem );
	if (!new_p) {
	    mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
	    return mpi_errno;
	}
	if (!new_head) { 
	    new_head = new_p;
	}
	else {
	    last_new_p->next = new_p;
	}
	last_new_p       = new_p;
	new_p->keyval = p->keyval;
	new_p->pre_sentinal = 0;
	/* Call the copy function here */
	new_p->value = p->value;
	new_p->post_sentinal = 0;
	new_p->next = p->next;
	
	p = p->next;
    }
    return MPI_SUCCESS;
}

