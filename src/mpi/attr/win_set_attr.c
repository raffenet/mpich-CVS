/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_set_attr */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_set_attr = PMPI_Win_set_attr
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_set_attr  MPI_Win_set_attr
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_set_attr as PMPI_Win_set_attr
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Win_set_attr PMPI_Win_set_attr

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Win_set_attr

/*@
   MPI_Win_set_attr - set window attribute

   Arguments:
+  MPI_Win win - window
.  int win_keyval - keyval
-  void *attribute_val - value

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Win_set_attr(MPI_Win win, int win_keyval, void *attribute_val)
{
    static const char FCNAME[] = "MPI_Win_set_attr";
    int mpi_errno = MPI_SUCCESS;

    MPID_Comm *comm_ptr = NULL;
    MPID_Keyval *keyval_ptr = NULL;
    MPID_Attribute *p, **old_p;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_SET_ATTR);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_SET_ATTR);
    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( comm, comm_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INITIALIZED(mpi_errno);

            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not valid, it will be reset to null */
	    /* Validate keyval */
	    if (comm_keyval == MPI_KEYVAL_INVALID) {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
						  MPI_ERR_KEYVAL, "**keyvalinvalid", 0 );
	    }
	    else if (HANDLE_GET_MPI_KIND(comm_keyval) != MPID_KEYVAL) {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_KEYVAL, "**keyval", 0 );
	    } 
	    else if (((comm_keyval&0x03c00000) >> 22) != MPID_COMM) {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
						  MPI_ERR_KEYVAL, "**keyvalnotcomm", 0 );
	    }
	    else if (HANDLE_GET_KIND(comm_keyval) == HANDLE_KIND_BUILTIN) {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**permattr", 0 );
	    }
	    else {
		MPID_Keyval_get_ptr( comm_keyval, keyval_ptr );
		MPID_Keyval_valid_ptr( keyval_ptr, mpi_errno );
		}
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_SET_ATTR);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
	}
	MPID_ELSE_ERROR_CHECKS;
	{
	    MPID_Keyval_get_ptr( comm_keyval, keyval_ptr );
	}
        MPID_END_ERROR_CHECKS;
    }
#   else    
    MPID_Keyval_get_ptr( comm_keyval, keyval_ptr );
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    /* Look for attribute.  They are ordered by keyval handle.  This uses 
       a simple linear list algorithm because few applications use more than a 
       handful of attributes */
    
    /* The thread lock prevents a valid attr delete on the same communicator
       but in a different thread from causing problems */
    MPID_Comm_thread_lock( comm_ptr );
    old_p = &comm_ptr->attributes;
    p = comm_ptr->attributes;
    while (p) {
	if (p->keyval->handle == keyval_ptr->handle) {
	    /* If found, call the delete function before replacing the 
	       attribute */
	    mpi_errno = MPIR_Call_attr_delete( comm, p );
	    if (mpi_errno) {
		MPID_Comm_thread_unlock( comm_ptr );
		MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_SET_ATTR);
		return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
	    }
	    p->value = attribute_val;
	    /* Does not change the reference count on the keyval */
	    break;
	}
	else if (p->keyval->handle > keyval_ptr->handle) {
	    MPID_Attribute *new_p = (MPID_Attribute *)MPIU_Handle_obj_alloc( &MPID_Attr_mem );
	    if (!new_p) {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
		MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_SET_ATTR);
		return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
	    }
	    new_p->keyval	 = keyval_ptr;
	    new_p->pre_sentinal	 = 0;
	    new_p->value	 = attribute_val;
	    new_p->post_sentinal = 0;
	    new_p->next		 = p->next;
	    MPIU_Object_add_ref( keyval_ptr );
	    p->next		 = new_p;
	    break;
	}
	old_p = &p->next;
	p = p->next;
    }
    if (!p) {
	MPID_Attribute *new_p = (MPID_Attribute *)MPIU_Handle_obj_alloc( &MPID_Attr_mem );
	if (!new_p) {
	    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
	    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_SET_ATTR);
	    return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
	}
	/* Did not find in list.  Add at end */
	new_p->keyval	     = keyval_ptr;
	new_p->pre_sentinal  = 0;
	new_p->value	     = attribute_val;
	new_p->post_sentinal = 0;
	new_p->next	     = 0;
	MPIU_Object_add_ref( keyval_ptr );
	*old_p		     = new_p;
    }
    
    /* Here is where we could add a hook for the device to detect attribute
       value changes, using something like
       MPID_Dev_comm_attr_hook( comm_ptr, keyval, attribute_val );
    */
    MPID_Comm_thread_unlock( comm_ptr );
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_SET_ATTR);
    return MPI_SUCCESS;
}
