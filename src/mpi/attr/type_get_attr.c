/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "attr.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_get_attr */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_get_attr = PMPI_Type_get_attr
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_get_attr  MPI_Type_get_attr
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_get_attr as PMPI_Type_get_attr
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Type_get_attr PMPI_Type_get_attr

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_get_attr

/*@
   MPI_Type_get_attr - get type attribute

   Arguments:
+  MPI_Datatype type - type
.  int type_keyval - keyval
.  void *attribute_val - value
-  int *flag - flag

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Type_get_attr(MPI_Datatype type, int type_keyval, void *attribute_val, int *flag)
{
    static const char FCNAME[] = "MPI_Type_get_attr";
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *type_ptr = NULL;
    MPID_Attribute *p;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_GET_ATTR);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_GET_ATTR);
    /* Get handles to MPI objects. */
    MPID_Datatype_get_ptr( type, type_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    /* Validate datatype pointer */
	    MPID_Datatype_valid_ptr( type_ptr, mpi_errno );
	    /* If type_ptr is not valid, it will be reset to null */
	    /* Validate keyval */
	    if (HANDLE_GET_MPI_KIND(type_keyval) != MPID_KEYVAL) {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_KEYVAL, 
						  "**keyval", 0 );
	    } 
	    else if (((type_keyval&0x03c00000) >> 22) != MPID_DATATYPE) {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_KEYVAL, 
						  "**keyvalnotdatatype", 0 );
	    }

            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_GET_ATTR);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    *flag = 0;
    p = type_ptr->attributes;
    while (p) {
	if (p->keyval->handle == type_keyval) {
	    *flag = 1;
	    (*(void **)attribute_val) = p->value;
	    break;
	}
	p = p->next;
    }
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_GET_ATTR);
    return MPI_SUCCESS;
}
