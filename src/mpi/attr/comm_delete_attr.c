/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "attr.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_delete_attr */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_delete_attr = PMPI_Comm_delete_attr
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_delete_attr  MPI_Comm_delete_attr
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_delete_attr as PMPI_Comm_delete_attr
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Comm_delete_attr PMPI_Comm_delete_attr

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_delete_attr

/*@
   MPI_Comm_delete_attr - Deletes an attribute value associated with a key on 
   a  communicator

Input Parameters:
+ comm - communicator to which attribute is attached (handle) 
- comm_keyval - The key value of the deleted attribute (integer) 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_PERM_KEY

.seealso MPI_Comm_set_attr, MPI_Comm_create_keyval
@*/
int MPI_Comm_delete_attr(MPI_Comm comm, int comm_keyval)
{
    static const char FCNAME[] = "MPI_Comm_delete_attr";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_Attribute *p, **old_p;
    MPID_Keyval *keyval_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_DELETE_ATTR);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_DELETE_ATTR);
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
	    if (HANDLE_GET_MPI_KIND(comm_keyval) != MPID_KEYVAL) {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, 
		    MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, 
                             MPI_ERR_KEYVAL, "**keyval", 0 );
	    } 
	    else if (((comm_keyval&0x03c00000) >> 22) != MPID_COMM) {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, 
                    MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
			     MPI_ERR_KEYVAL, "**keyvalnotcomm", 0 );
	    }
	    else if (HANDLE_GET_KIND(comm_keyval) == HANDLE_KIND_BUILTIN) {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, 
                    MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, 
                             MPI_ERR_OTHER, "**permattr", 0 );
	    }
	    else {
		MPID_Keyval_get_ptr( comm_keyval, keyval_ptr );
		MPID_Keyval_valid_ptr( keyval_ptr, mpi_errno );
	    }
            if (mpi_errno) goto fn_fail;
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
    /* Look for attribute.  They are ordered by keyval handle */

    /* The thread lock prevents a valid attr delete on the same communicator
       but in a different thread from causing problems */
    MPID_Comm_thread_lock( comm_ptr );
    old_p = &comm_ptr->attributes;
    p     = comm_ptr->attributes;
    while (p) {
	if (p->keyval->handle == keyval_ptr->handle) {
	    break;
	}
	old_p = &p->next;
	p     = p->next;
    }

    /* We can't unlock yet, because we must not free the attribute until
       we know whether the delete function has returned with a 0 status
       code */

    if (p) {
	/* Run the delete function, if any, and then free the attribute 
	   storage */
	mpi_errno = MPIR_Call_attr_delete( comm, p );

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

    MPID_Comm_thread_unlock( comm_ptr );
    /* ... end of body of routine ... */

    if (mpi_errno == MPI_SUCCESS)
    {
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_DELETE_ATTR);
	return MPI_SUCCESS;
    }
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, 
				     FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_comm_delete_attr", "**mpi_comm_delete_attr %C %d", comm, comm_keyval);
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_DELETE_ATTR);
    return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    /* --END ERROR HANDLING-- */
}
