/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "attr.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_delete_attr */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_delete_attr = PMPI_Win_delete_attr
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_delete_attr  MPI_Win_delete_attr
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_delete_attr as PMPI_Win_delete_attr
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Win_delete_attr PMPI_Win_delete_attr

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Win_delete_attr

/*@
   MPI_Win_delete_attr - delete window attribute

   Input Parameters:
+ win - window from which the attribute is deleted (handle) 
- win_keyval - key value (integer) 

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_WIN
.N MPI_ERR_KEYVAL
.N MPI_ERR_OTHER
@*/
int MPI_Win_delete_attr(MPI_Win win, int win_keyval)
{
    static const char FCNAME[] = "MPI_Win_delete_attr";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_Attribute *p, **old_p;
    MPID_Keyval *keyval_ptr=0;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WIN_DELETE_ATTR);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_WIN_DELETE_ATTR);

    MPID_Win_get_ptr( win, win_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INITIALIZED(mpi_errno);
            /* Validate win_ptr */
            MPID_Win_valid_ptr( win_ptr, mpi_errno );
	    /* If win_ptr is not valid, it will be reset to null */
	    /* Validate keyval */
	    if (HANDLE_GET_MPI_KIND(win_keyval) != MPID_KEYVAL) {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_KEYVAL, 
						  "**keyval", 0 );
	    } 
	    else if (((win_keyval&0x03c00000) >> 22) != MPID_WIN) {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_KEYVAL, 
						  "**keyvalnotwin", 0 );
	    }
	    else {
		MPID_Keyval_get_ptr( win_keyval, keyval_ptr );
		MPID_Keyval_valid_ptr( keyval_ptr, mpi_errno );
	    }
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_WIN_DELETE_ATTR);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   else    
    MPID_Keyval_get_ptr( win_keyval, keyval_ptr );
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    /* Look for attribute.  They are ordered by keyval handle */

    /* The thread lock prevents a valid attr delete on the same window
       but in a different thread from causing problems */
    MPID_Common_thread_lock();
    old_p = &win_ptr->attributes;
    p     = win_ptr->attributes;
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
	mpi_errno = MPIR_Call_attr_delete( win, p );

	if (!mpi_errno) {
	    int in_use;
	    /* We found the attribute.  Remove it from the list */
	    *old_p = p->next;
	    /* Decrement the use of the keyval */
	    MPIU_Object_release_ref( p->keyval, &in_use);
	    if (!in_use) {
		MPIU_Handle_obj_free( &MPID_Keyval_mem, p->keyval );
	    }
	    MPID_Attr_free(p);
	}
    }

    MPID_Common_thread_unlock( );
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_WIN_DELETE_ATTR);
    if (mpi_errno)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    }

    return MPI_SUCCESS;
}
