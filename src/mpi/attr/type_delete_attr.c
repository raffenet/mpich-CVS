/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "attr.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_delete_attr */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_delete_attr = PMPI_Type_delete_attr
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_delete_attr  MPI_Type_delete_attr
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_delete_attr as PMPI_Type_delete_attr
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Type_delete_attr PMPI_Type_delete_attr

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_delete_attr

/*@
   MPI_Type_delete_attr - delete type attribute

   Arguments:
+  MPI_Datatype type - type
-  int type_keyval - value

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Type_delete_attr(MPI_Datatype type, int type_keyval)
{
    static const char FCNAME[] = "MPI_Type_delete_attr";
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *type_ptr = NULL;
    MPID_Attribute *p, **old_p;
    MPID_Keyval *keyval_ptr = 0;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_DELETE_ATTR);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_DELETE_ATTR);
    /* Get handles to MPI objects. */
    MPID_Datatype_get_ptr( type, type_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            /* Validate type_ptr */
            MPID_Datatype_valid_ptr( type_ptr, mpi_errno );
	    /* If type_ptr is not valid, it will be reset to null */
	    /* Validate keyval */
	    if (HANDLE_GET_MPI_KIND(type_keyval) != MPID_KEYVAL) {
		mpi_errno = MPIR_Err_create_code( MPI_ERR_KEYVAL, 
						  "**keyval", 0 );
	    } 
	    else if (((type_keyval&0x03c00000) >> 22) != MPID_DATATYPE) {
		mpi_errno = MPIR_Err_create_code( MPI_ERR_KEYVAL, 
						  "**keyvalnotdatatype", 0 );
	    }
	    else {
		MPID_Keyval_get_ptr( type_keyval, keyval_ptr );
		MPID_Keyval_valid_ptr( keyval_ptr, mpi_errno );
	    }
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_DELETE_ATTR);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   else    
    MPID_Keyval_get_ptr( type_keyval, keyval_ptr );
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    /* Look for attribute.  They are ordered by keyval handle */

    /* The thread lock prevents a valid attr delete on the same datatype
       but in a different thread from causing problems */
    MPID_Common_thread_lock();
    old_p = &type_ptr->attributes;
    p     = type_ptr->attributes;
    while (p) {
	if (p->keyval->handle == keyval_ptr->handle) {
	    break;
	}
	old_p = &p->next;
	p = p->next;
    }

    /* We can't unlock yet, because we must not free the attribute until
       we know whether the delete function has returned with a 0 status
       code */

    if (p) {
	/* Run the delete function, if any, and then free the attribute 
	   storage */
	mpi_errno = MPIR_Call_attr_delete( type, p );

	if (!mpi_errno) {
	    /* We found the attribute.  Remove it from the list */
	    *old_p = p->next;
	    MPID_Attr_free(p);
	}
    }

    MPID_Common_thread_unlock( );
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_DELETE_ATTR);
    if (mpi_errno) 
	return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );

    return MPI_SUCCESS;
}
