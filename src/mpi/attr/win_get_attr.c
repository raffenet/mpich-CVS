/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_get_attr */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_get_attr = PMPI_Win_get_attr
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_get_attr  MPI_Win_get_attr
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_get_attr as PMPI_Win_get_attr
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Win_get_attr PMPI_Win_get_attr

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Win_get_attr

/*@
   MPI_Win_get_attr - Get attribute cached on an MPI window object

   Input Parameters:
+ win - window to which the attribute is attached (handle) 
- win_keyval - key value (integer) 

   Output Parameters:
+ attribute_val - attribute value, unless flag = false 
- flag - false if no attribute is associated with the key (logical) 

   Notes:
   The following attributes are predefined for all MPI Window objects\:

+ MPI_WIN_BASE - window base address. 
. MPI_WIN_SIZE - window size, in bytes. 
- MPI_WIN_DISP_UNIT - displacement unit associated with the window. 

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_WIN
.N MPI_ERR_KEYVAL
.N MPI_ERR_OTHER
@*/
int MPI_Win_get_attr(MPI_Win win, int win_keyval, void *attribute_val, int *flag)
{
    static const char FCNAME[] = "MPI_Win_get_attr";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WIN_GET_ATTR);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_WIN_GET_ATTR);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_WIN_GET_ATTR);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    MPID_Win_get_ptr( win, win_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
	MPID_BEGIN_ERROR_CHECKS;
            /* Validate win_ptr */
            MPID_Win_valid_ptr( win_ptr, mpi_errno );
	    /* If win_ptr is not valid, it will be reset to null */
	    /* Validate keyval */
	    if (HANDLE_GET_MPI_KIND(win_keyval) != MPID_KEYVAL) {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_KEYVAL, "**keyval", 0 );
	    } 
	    else if (((win_keyval&0x03c00000) >> 22) != MPID_WIN) {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
						  MPI_ERR_KEYVAL, "**keyvalnotwin", 0 );
	    }
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_WIN_GET_ATTR);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
	
	MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */    

    /* Check for builtin attribute */
    /* This code is ok for correct programs, but it would be better
       to copy the values from the per-process block and pass the user
       a pointer to a copy */
    /* Note that if we are called from Fortran, we must return the values,
       not the addresses, of these attributes */
    if (HANDLE_GET_KIND(win_keyval) == HANDLE_KIND_BUILTIN) {
	int attr_idx = win_keyval & 0x0000000f;
	void **attr_val_p = (void **)attribute_val;
#ifdef HAVE_FORTRAN_BINDING
	MPI_Fint  *attr_int = (MPI_Fint *)attribute_val;
#endif
	*flag = 1;

	/* 
	 * The C versions of the attributes return the address of a 
	 * *COPY* of the value (to prevent the user from changing it)
	 * and the Fortran versions provide the actual value (as an Fint)
	 */
	switch (attr_idx) {
	case 1: /* WIN_BASE */
	    *attr_val_p = win_ptr->base;
	    break;
	case 3: /* SIZE */
	    win_ptr->copySize = win_ptr->size;
	    *attr_val_p = &win_ptr->copySize;
	    break;
	case 5: /* DISP_UNIT */
	    win_ptr->copyDispUnit = win_ptr->disp_unit;
	    *attr_val_p = &win_ptr->copyDispUnit;
	    break;
#ifdef HAVE_FORTRAN_BINDING
	case 2: /* Fortran BASE */
	    *attr_int = (MPI_Fint)(win_ptr->base);
	    break;
	case 4: /* Fortran SIZE */
	    /* We do not need to copy because we return the value,
	       not a pointer to the value */
	    *attr_int = win_ptr->size;
	    break;
	case 6: /* Fortran DISP_UNIT */
	    /* We do not need to copy because we return the value,
	       not a pointer to the value */
	    *attr_int = win_ptr->disp_unit;
	    break;
#endif
	}
    }
    else {
	MPID_Attribute *p = win_ptr->attributes;

	*flag = 0;
	while (p) {
	    if (p->keyval->handle == win_keyval) {
		*flag = 1;
		(*(void **)attribute_val) = p->value;
		break;
	    }
	    p = p->next;
	}
    }

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_WIN_GET_ATTR);
    return MPI_SUCCESS;
}
