/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_get_attr */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_get_attr = PMPI_Comm_get_attr
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_get_attr  MPI_Comm_get_attr
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_get_attr as PMPI_Comm_get_attr
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Comm_get_attr PMPI_Comm_get_attr

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_get_attr

/*@
   MPI_Comm_get_attr - get communicator attribute

   Arguments:
+  MPI_Comm comm - communicator
.  int comm_keyval - keyval
.  void *attribute_val - value
-  int *flag - flag

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Comm_get_attr(MPI_Comm comm, int comm_keyval, void *attribute_val, int *flag)
{
    static const char FCNAME[] = "MPI_Comm_get_attr";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    static PreDefined_attrs attr_copy;    /* Used to provide a copy of the
					     predefined attributes */
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_GET_ATTR);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_GET_ATTR);
    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( comm, comm_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not value, it will be reset to null */
	    /* Validate keyval */
	    if (HANDLE_GET_MPI_KIND(comm_keyval) != MPID_KEYVAL) {
		mpi_errno = MPIR_Err_create_code( MPI_ERR_KEYVAL, 
						  "**keyval", 0 );
	    } 
	    else if (((comm_keyval&0x03c00000) >> 22) != MPID_COMM) {
		mpi_errno = MPIR_Err_create_code( MPI_ERR_KEYVAL, 
						  "**keyvalnotcomm", 0 );
	    }
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_GET_ATTR);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    /* Check for builtin attribute */
    /* This code is ok for correct programs, but it would be better
       to copy the values from the per-process block and pass the user
       a pointer to a copy */
    /* FIXME: If we are called from Fortran, we must return the values,
       not the addresses, of these attributes */
    if (HANDLE_GET_KIND(comm_keyval) == HANDLE_KIND_BUILTIN) {
	int attr_idx = comm_keyval & 0x0000000f;
	void **attr_val_p = (void **)attribute_val;
#ifdef HAVE_FORTRAN_BINDING
	MPI_Fint  *attr_int = (MPI_Fint *)attribute_val;
#endif
	*flag = 1;

	/* FIXME: We could initialize some of these here; only tag_ub is 
	 used in the error checking. */
	/* 
	 * The C versions of the attributes return the address of a 
	 * *COPY* of the value (to prevent the user from changing it)
	 * and the Fortran versions provide the actual value (as an Fint)
	 */
	attr_copy = MPIR_Process.attrs;
	switch (attr_idx) {
	case 1: /* TAG_UB */
	    *attr_val_p = &attr_copy.tag_ub;
	    break;
	case 3: /* HOST */
	    *attr_val_p = &attr_copy.host;
	    break;
	case 5: /* IO */
	    *attr_val_p = &attr_copy.io;
	    break;
	case 7: /* WTIME */
	    *attr_val_p = &attr_copy.wtime_is_global;
	    break;
	case 9: /* UNIVERSE */
	    *attr_val_p = &attr_copy.universe;
	    break;
	case 11: /* LASTUSEDCODE */
	    *attr_val_p = &attr_copy.lastusedcode;
	    break;
	case 13: /* APPNUM */
	    *attr_val_p = &attr_copy.appnum;
	    break;
#ifdef HAVE_FORTRAN_BINDING
	case 2: /* Fortran TAG_UB */
	    *attr_int = attr_copy.tag_ub;
	    break;
	case 4: /* Fortran HOST */
	    *attr_int = attr_copy.host;
	    break;
	case 6: /* Fortran IO */
	    *attr_int = attr_copy.io;
	    break;
	case 8: /* Fortran WTIME */
	    *attr_int = attr_copy.wtime_is_global;
	    break;
	case 10: /* UNIVERSE */
	    *attr_int = attr_copy.universe;
	    break;
	case 12: /* LASTUSEDCODE */
	    *attr_int = attr_copy.lastusedcode;
	    break;
	case 14: /* APPNUM */
	    *attr_int = attr_copy.appnum;
	    break;
	}
#endif
    }
    else {
	MPID_Attribute *p = comm_ptr->attributes;

	*flag = 0;
	while (p) {
	    if (p->keyval->handle == comm_keyval) {
		*flag = 1;
		(*(void **)attribute_val) = p->value;
		break;
	    }
	    p = p->next;
	}
    }
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_GET_ATTR);
    return MPI_SUCCESS;
}
