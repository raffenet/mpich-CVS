/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "attr.h"

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

Input Parameters:
+ win - MPI window object to which attribute will be attached (handle) 
. keyval - key value, as returned by  'MPI_Win_create_keyval' (integer) 
- attribute_val - attribute value 

Notes:

The type of the attribute value depends on whether C or Fortran is being used.
In C, an attribute value is a pointer ('void *'); in Fortran, it is an 
address-sized integer.

If an attribute is already present, the delete function (specified when the
corresponding keyval was created) will be called.
.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_WIN
.N MPI_ERR_KEYVAL
@*/
int MPI_Win_set_attr(MPI_Win win, int win_keyval, void *attribute_val)
{
    static const char FCNAME[] = "MPI_Win_set_attr";
    int mpi_errno = MPI_SUCCESS;

    MPID_Win *win_ptr = NULL;
    MPID_Keyval *keyval_ptr = NULL;
    MPID_Attribute *p, **old_p;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WIN_SET_ATTR);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_WIN_SET_ATTR);
    /* Get handles to MPI objects. */
    MPID_Win_get_ptr( win, win_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    if (mpi_errno) goto fn_fail;
            /* Validate win_ptr */
            MPID_Win_valid_ptr( win_ptr, mpi_errno );
	    /* If win_ptr is not valid, it will be reset to null */
	    /* Validate keyval */
	    if (win_keyval == MPI_KEYVAL_INVALID) {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
						  MPI_ERR_KEYVAL, "**keyvalinvalid", 0 );
	    }
	    else if (HANDLE_GET_MPI_KIND(win_keyval) != MPID_KEYVAL) {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_KEYVAL, "**keyval", 0 );
	    } 
	    else if (((win_keyval&0x03c00000) >> 22) != MPID_WIN) {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
						  MPI_ERR_KEYVAL, "**keyvalnotwin", 0 );
	    }
	    else if (HANDLE_GET_KIND(win_keyval) == HANDLE_KIND_BUILTIN) {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**permattr", 0 );
	    }
	    else {
		MPID_Keyval_get_ptr( win_keyval, keyval_ptr );
		MPID_Keyval_valid_ptr( keyval_ptr, mpi_errno );
		}
            if (mpi_errno) goto fn_fail;
	}
	MPID_ELSE_ERROR_CHECKS;
	{
	    MPID_Keyval_get_ptr( win_keyval, keyval_ptr );
	}
        MPID_END_ERROR_CHECKS;
    }
#   else    
    MPID_Keyval_get_ptr( win_keyval, keyval_ptr );
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    /* Look for attribute.  They are ordered by keyval handle.  This uses 
       a simple linear list algorithm because few applications use more than a 
       handful of attributes */
    
    /* The thread lock prevents a valid attr delete on the same window
       but in a different thread from causing problems */
    MPID_Common_thread_lock( );
    old_p = &win_ptr->attributes;
    p = win_ptr->attributes;
    while (p)
    {
	if (p->keyval->handle == keyval_ptr->handle)
	{
	    /* If found, call the delete function before replacing the 
	       attribute */
	    mpi_errno = MPIR_Call_attr_delete( win, p );
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno)
	    {
		MPID_Common_thread_unlock( );
		/* FIXME (gropp): communicator of window? */
		goto fn_fail;
	    }
	    /* --END ERROR HANDLING-- */
	    p->value = attribute_val;
	    /* Does not change the reference count on the keyval */
	    break;
	}
	else if (p->keyval->handle > keyval_ptr->handle) {
	    MPID_Attribute *new_p = (MPID_Attribute *)MPIU_Handle_obj_alloc( &MPID_Attr_mem );
	    /* --BEGIN ERROR HANDLING-- */
	    if (!new_p)
	    {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Attribute" );
		goto fn_fail;
	    }
	    /* --END ERROR HANDLING-- */
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
    if (!p)
    {
	MPID_Attribute *new_p = (MPID_Attribute *)MPIU_Handle_obj_alloc( &MPID_Attr_mem );
	/* --BEGIN ERROR HANDLING-- */
	if (!new_p)
	{
	    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Attribute" );
	    goto fn_fail;
	}
	/* --END ERROR HANDLING-- */
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
       MPID_Dev_win_attr_hook( win_ptr, keyval, attribute_val );
    */
    MPID_Common_thread_unlock(  );
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_WIN_SET_ATTR);
    return MPI_SUCCESS;
fn_fail:
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_win_set_attr", "**mpi_win_set_attr %W %d %p", win, win_keyval, attribute_val);
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_WIN_SET_ATTR);
    return MPIR_Err_return_win(win_ptr, FCNAME, mpi_errno);
}
